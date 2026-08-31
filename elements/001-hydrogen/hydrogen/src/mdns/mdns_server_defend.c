/*
 * mDNS Server Phase 6b: defend unique names, shared-record delay,
 * unique-record rate limit, and simultaneous probe tiebreak (RFC 6762 s6/s8.2/s9).
 */

#include <src/hydrogen.h>

#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#include "mdns_server.h"
#include <src/mdns/mdns_wire.h>

extern uint64_t mdns_client_now_ms(void);

uint32_t mdns_server_default_rand_delay(uint32_t min_ms, uint32_t max_ms)
{
    uint32_t range;
    if (max_ms <= min_ms) {
        return min_ms;
    }
    range = max_ms - min_ms + 1;
    return min_ms + (uint32_t)((uint32_t)rand() % range);
}

int mdns_server_want_is_shared_only(const mdns_server_want_t *w)
{
    size_t i;

    if (!w) {
        return 0;
    }
    if (w->host_answer & (MDNS_W_A | MDNS_W_AAAA | MDNS_W_NSEC)) {
        return 0;
    }
    if (w->host_additional & (MDNS_W_A | MDNS_W_AAAA | MDNS_W_NSEC)) {
        return 0;
    }
    for (i = 0; i < w->nsvc; i++) {
        uint32_t unique_bits = MDNS_W_SRV | MDNS_W_A | MDNS_W_AAAA | MDNS_W_NSEC;

        if (w->svc_answer[i] & unique_bits) {
            return 0;
        }
        if (w->svc_additional[i] & unique_bits) {
            return 0;
        }
    }
    return 1;
}

int mdns_server_rate_fresh(const mdns_server_t *server, const char *name, uint64_t last_ms)
{
    uint64_t now;

    if (!server) {
        return 1;
    }
    if (last_ms == 0) {
        return 1;
    }
    now = server->now_ms_fn ? server->now_ms_fn() : 0;
    if (now == 0) {
        return 1;
    }
    if (now - last_ms >= (uint64_t)MDNS_RATE_LIMIT_MS) {
        return 1;
    }
    (void)name;
    return 0;
}

int mdns_server_rr_cmp(const mdns_rr *a, const mdns_rr *b,
                       const uint8_t *msg_a, const uint8_t *msg_b,
                       size_t msglen_a, size_t msglen_b)
{
    size_t i;
    size_t minlen;

    if (!a || !b || !msg_a || !msg_b) {
        return 0;
    }
    if (a->cls != b->cls) {
        return (a->cls < b->cls) ? -1 : 1;
    }
    if (a->type != b->type) {
        return (a->type < b->type) ? -1 : 1;
    }
    if (a->rdlen == 0 && b->rdlen == 0) {
        return 0;
    }
    if (a->rdoff + a->rdlen > msglen_a || b->rdoff + b->rdlen > msglen_b) {
        return 0;
    }
    minlen = (a->rdlen < b->rdlen) ? a->rdlen : b->rdlen;
    for (i = 0; i < minlen; i++) {
        uint8_t ba = msg_a[a->rdoff + i];
        uint8_t bb = msg_b[b->rdoff + i];

        if (ba != bb) {
            return (ba < bb) ? -1 : 1;
        }
    }
    if (a->rdlen != b->rdlen) {
        return (a->rdlen < b->rdlen) ? -1 : 1;
    }
    return 0;
}

int mdns_server_addr_is_ours(const mdns_server_t *server, int family,
                             const uint8_t *addr, size_t addrlen)
{
    size_t i;
    size_t j;

    if (!server || !addr || server->num_interfaces == 0) {
        return 0;
    }
    if (!server->interfaces) {
        return 0;
    }
    for (i = 0; i < server->num_interfaces; i++) {
        const mdns_server_interface_t *iface = &server->interfaces[i];

        if (!iface->ip_addresses || iface->num_addresses == 0) {
            continue;
        }
        for (j = 0; j < iface->num_addresses; j++) {
            struct in_addr a4;
            struct in6_addr a6;

            if (!iface->ip_addresses[j]) {
                continue;
            }
            if (family == AF_INET && addrlen == 4) {
                if (inet_pton(AF_INET, iface->ip_addresses[j], &a4) == 1) {
                    if (memcmp(&a4, addr, 4) == 0) {
                        return 1;
                    }
                }
            }
            if (family == AF_INET6 && addrlen == 16) {
                if (inet_pton(AF_INET6, iface->ip_addresses[j], &a6) == 1) {
                    if (memcmp(&a6, addr, 16) == 0) {
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

int mdns_server_txt_rdata_matches(const mdns_server_service_t *svc,
                                   const uint8_t *rdata, size_t rdlen)
{
    size_t i;
    size_t off = 0;

    if (!svc || !rdata) {
        return 0;
    }
    for (i = 0; i < svc->num_txt_records; i++) {
        const char *txt = svc->txt_records ? svc->txt_records[i] : NULL;
        size_t txt_len;

        if (!txt) {
            continue;
        }
        txt_len = strlen(txt);
        if (txt_len > 255) {
            txt_len = 255;
        }
        if (off + 1 + txt_len > rdlen) {
            return 0;
        }
        if (rdata[off] != (uint8_t)txt_len) {
            return 0;
        }
        if (memcmp(rdata + off + 1, txt, txt_len) != 0) {
            return 0;
        }
        off += 1 + txt_len;
    }
    return (off == rdlen) ? 1 : 0;
}

int mdns_server_srv_rdata_matches(const mdns_server_t *server, const mdns_server_service_t *svc,
                                  const uint8_t *msg, size_t msglen, const mdns_rr *rr)
{
    uint16_t priority = 0;
    uint16_t weight = 0;
    uint16_t port = 0;
    char target[MDNS_NAME_MAX];

    if (!server || !svc || !msg || !rr) {
        return 0;
    }
    if (mdns_rdata_srv(msg, msglen, rr, &priority, &weight, &port, target, sizeof target) < 0) {
        return 0;
    }
    if (port != (uint16_t)svc->port) {
        return 0;
    }
    if (!server->hostname || !mdns_name_equal(target, server->hostname)) {
        return 0;
    }
    return 1;
}

int mdns_server_rr_conflicts_claimed(const mdns_server_t *server, const mdns_rr *rr,
                                     const uint8_t *msg, size_t msglen)
{
    char instance[256];
    size_t k;

    if (!server || !rr || !msg) {
        return 0;
    }
    if (rr->ttl == 0) {
        return 0;
    }
    if (rr->type != MDNS_TYPE_A && rr->type != MDNS_TYPE_AAAA &&
        rr->type != MDNS_TYPE_SRV && rr->type != MDNS_TYPE_TXT) {
        return 0;
    }

    if (server->hostname_claimed && server->hostname &&
        mdns_name_equal(rr->name, server->hostname)) {
        if (rr->type == MDNS_TYPE_A && rr->rdlen == 4) {
            return !mdns_server_addr_is_ours(server, AF_INET, msg + rr->rdoff, 4);
        }
        if (rr->type == MDNS_TYPE_AAAA && rr->rdlen == 16) {
            return !mdns_server_addr_is_ours(server, AF_INET6, msg + rr->rdoff, 16);
        }
        return 1;
    }

    for (k = 0; k < server->num_services; k++) {
        const mdns_server_service_t *svc = &server->services[k];

        /* cppcheck-suppress knownConditionTrueFalse -- defensive null check */
        if (!svc || !svc->claimed) {
            continue;
        }
        if (!svc->name || !svc->type) {
            continue;
        }
        mdns_server_format_instance_name(svc, instance, sizeof instance);
        if (instance[0] != '\0' && mdns_name_equal(rr->name, instance)) {
            if (rr->type == MDNS_TYPE_SRV) {
                if (mdns_server_srv_rdata_matches(server, svc, msg, msglen, rr)) {
                    return 0;
                }
                return 1;
            }
            if (rr->type == MDNS_TYPE_TXT) {
                if (mdns_server_txt_rdata_matches(svc, msg + rr->rdoff, rr->rdlen)) {
                    return 0;
                }
                return 1;
            }
            return 1;
        }
    }
    return 0;
}

void mdns_server_defend_claimed(mdns_server_t *server, const mdns_msg *msg,
                                const uint8_t *raw, size_t rawlen)
{
    size_t i;
    int conflict = 0;

    if (!server || !msg || !raw) {
        return;
    }
    if (!mdns_server_all_claimed(server)) {
        return;
    }
    if (server->probe_failed) {
        return;
    }
    if (!server->now_ms_fn) {
        return;
    }

    for (i = 0; i < msg->nrr; i++) {
        const mdns_rr *rr = &msg->rr[i];

        if (mdns_server_rr_conflicts_claimed(server, rr, raw, rawlen)) {
            conflict = 1;
            break;
        }
    }
    if (!conflict) {
        return;
    }
    if (!mdns_server_rate_fresh(server, server->hostname,
                                server->hostname_last_send_ms)) {
        return;
    }
    server->hostname_last_send_ms = server->now_ms_fn();
    log_this(SR_MDNS_SERVER, "MDNS_SERVER DEFEND", LOG_LEVEL_DEBUG, 0);
    mdns_server_send_announcement(server, NULL);
}

int mdns_server_check_tiebreak(mdns_server_t *server, int sockfd,
                               const mdns_msg *msg, const uint8_t *raw, size_t rawlen)
{
    const mdns_server_interface_t *iface;
    uint8_t our_packet[MDNS_MAX_PACKET_SIZE];
    size_t our_len = 0;
    mdns_msg our_msg;
    size_t i;
    size_t j;
    int found;

    if (!server || !msg || !raw || sockfd < 0) {
        return 0;
    }
    if (mdns_server_all_claimed(server) || server->probe_failed) {
        return 0;
    }
    iface = mdns_server_iface_for_sock(server, sockfd);
    if (!iface) {
        return 0;
    }
    mdns_server_build_probe(our_packet, &our_len, server, iface);
    if (our_len == 0) {
        return 0;
    }
    if (mdns_parse(our_packet, our_len, &our_msg) < 0) {
        return 0;
    }

    for (i = 0; i < msg->nrr; i++) {
        const mdns_rr *their_rr = &msg->rr[i];
        char instance[256];

        if (their_rr->section != MDNS_SEC_AUTHORITY) {
            continue;
        }
        if (their_rr->ttl == 0) {
            continue;
        }
        found = 0;
        for (j = 0; j < server->num_services; j++) {
            const mdns_server_service_t *svc = &server->services[j];

            /* cppcheck-suppress knownConditionTrueFalse -- defensive null check */
            if (!svc || svc->claimed || !svc->name || !svc->type) {
                continue;
            }
            mdns_server_format_instance_name(svc, instance, sizeof instance);
            if (instance[0] != '\0' && mdns_name_equal(their_rr->name, instance)) {
                size_t k2;

                for (k2 = 0; k2 < our_msg.nrr; k2++) {
                    const mdns_rr *our_rr = &our_msg.rr[k2];

                    if (our_rr->section == MDNS_SEC_AUTHORITY &&
                        our_rr->type == their_rr->type &&
                        mdns_name_equal(our_rr->name, their_rr->name)) {
                        int cmp = mdns_server_rr_cmp(our_rr, their_rr,
                                                     our_packet, raw, our_len, rawlen);
                        if (cmp < 0) {
                            server->probe_tiebreak_lose = 1;
                            log_this(SR_MDNS_SERVER, "MDNS_SERVER TIEBREAK lost, delaying probe 1s", LOG_LEVEL_DEBUG, 0);
                            return 1;
                        }
                        found = 1;
                        break;
                    }
                }
            }
            if (found) {
                break;
            }
        }
        if (server->hostname_claimed == 0 && server->hostname &&
            mdns_name_equal(their_rr->name, server->hostname)) {
            size_t k2;

            for (k2 = 0; k2 < our_msg.nrr; k2++) {
                const mdns_rr *our_rr = &our_msg.rr[k2];

                if (our_rr->section == MDNS_SEC_AUTHORITY &&
                    our_rr->type == their_rr->type &&
                    mdns_name_equal(our_rr->name, their_rr->name)) {
                    int cmp = mdns_server_rr_cmp(our_rr, their_rr,
                                                 our_packet, raw, our_len, rawlen);
                    if (cmp < 0) {
                        server->probe_tiebreak_lose = 1;
                        log_this(SR_MDNS_SERVER, "MDNS_SERVER TIEBREAK lost, delaying probe 1s", LOG_LEVEL_DEBUG, 0);
                        return 1;
                    }
                    found = 1;
                    break;
                }
            }
        }
    }
    return 0;
}
