/*
 * mDNS probe / claim / conflict rename (RFC 6762 s8.1).
 */

#include <src/hydrogen.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "mdns_server.h"
#include <src/mdns/mdns_wire.h>

void mdns_server_next_instance_name(const char *base, unsigned attempt, char *out, size_t cap)
{
    if (!out || cap == 0) {
        return;
    }
    out[0] = '\0';
    if (!base || attempt < 2u) {
        if (base) {
            snprintf(out, cap, "%s", base);
        }
        return;
    }
    snprintf(out, cap, "%s (%u)", base, attempt);
}

void mdns_server_next_hostname(const char *base, unsigned attempt, char *out, size_t cap)
{
    if (!out || cap == 0) {
        return;
    }
    out[0] = '\0';
    if (!base) {
        return;
    }
    if (attempt < 2u) {
        snprintf(out, cap, "%s.local", base);
        return;
    }
    snprintf(out, cap, "%s-%u.local", base, attempt);
}

int mdns_server_all_claimed(const mdns_server_t *server)
{
    size_t i;

    if (!server) {
        return 0;
    }
    if (!server->hostname_claimed) {
        return 0;
    }
    for (i = 0; i < server->num_services; i++) {
        if (!server->services || !server->services[i].claimed) {
            return 0;
        }
    }
    return 1;
}

int mdns_server_any_claimed(const mdns_server_t *server)
{
    size_t i;

    if (!server) {
        return 0;
    }
    if (server->hostname_claimed) {
        return 1;
    }
    for (i = 0; i < server->num_services; i++) {
        if (server->services && server->services[i].claimed) {
            return 1;
        }
    }
    return 0;
}

int mdns_server_any_probe_conflict(const mdns_server_t *server)
{
    size_t i;

    if (!server) {
        return 0;
    }
    if (!server->hostname_claimed && server->hostname_conflict) {
        return 1;
    }
    for (i = 0; i < server->num_services; i++) {
        if (server->services && !server->services[i].claimed && server->services[i].probe_conflict) {
            return 1;
        }
    }
    return 0;
}

void mdns_server_clear_probe_conflicts(mdns_server_t *server)
{
    size_t i;

    if (!server) {
        return;
    }
    server->hostname_conflict = 0;
    for (i = 0; i < server->num_services; i++) {
        if (server->services) {
            server->services[i].probe_conflict = 0;
        }
    }
}

void mdns_server_claim_unclaimed(mdns_server_t *server)
{
    size_t i;
    char instance[256];

    if (!server || server->probe_failed) {
        return;
    }
    if (!server->hostname_claimed) {
        server->hostname_claimed = 1;
        log_this(SR_MDNS_SERVER, "MDNS_SERVER CLAIMED %s", LOG_LEVEL_DEBUG, 1,
                 server->hostname ? server->hostname : "");
    }
    for (i = 0; i < server->num_services; i++) {
        if (!server->services || server->services[i].claimed) {
            continue;
        }
        server->services[i].claimed = 1;
        mdns_server_format_instance_name(&server->services[i], instance, sizeof instance);
        log_this(SR_MDNS_SERVER, "MDNS_SERVER CLAIMED %s", LOG_LEVEL_DEBUG, 1, instance);
    }
}

void mdns_server_probe_fail(mdns_server_t *server)
{
    size_t i;

    if (!server) {
        return;
    }
    server->probe_failed = 1;
    server->hostname_claimed = 0;
    for (i = 0; i < server->num_services; i++) {
        if (server->services) {
            server->services[i].claimed = 0;
        }
    }
    log_this(SR_MDNS_SERVER, "MDNS_SERVER CONFLICT probe failed after %d attempts", LOG_LEVEL_ALERT, 1,
             MDNS_MAX_NAME_ATTEMPTS);
}

int mdns_server_apply_probe_renames(mdns_server_t *server)
{
    size_t i;
    char next[256];

    if (!server) {
        return -1;
    }
    if (!server->hostname_claimed && server->hostname_conflict) {
        if (server->hostname_attempts >= MDNS_MAX_NAME_ATTEMPTS) {
            mdns_server_probe_fail(server);
            return -1;
        }
        server->hostname_attempts++;
        log_this(SR_MDNS_SERVER, "MDNS_SERVER CONFLICT %s", LOG_LEVEL_DEBUG, 1,
                 server->hostname ? server->hostname : "");
        mdns_server_next_hostname(server->hostname_base, (unsigned)server->hostname_attempts, next, sizeof next);
        free(server->hostname);
        server->hostname = strdup(next);
        if (!server->hostname) {
            mdns_server_probe_fail(server);
            return -1;
        }
    }
    for (i = 0; i < server->num_services; i++) {
        mdns_server_service_t *svc;

        if (!server->services) {
            break;
        }
        svc = &server->services[i];
        if (svc->claimed || !svc->probe_conflict) {
            continue;
        }
        if (svc->name_attempts >= MDNS_MAX_NAME_ATTEMPTS) {
            mdns_server_probe_fail(server);
            return -1;
        }
        svc->name_attempts++;
        mdns_server_format_instance_name(svc, next, sizeof next);
        log_this(SR_MDNS_SERVER, "MDNS_SERVER CONFLICT %s", LOG_LEVEL_DEBUG, 1, next);
        mdns_server_next_instance_name(svc->name_base, (unsigned)svc->name_attempts, next, sizeof next);
        free(svc->name);
        svc->name = strdup(next);
        if (!svc->name) {
            mdns_server_probe_fail(server);
            return -1;
        }
    }
    return 0;
}

int mdns_server_rr_conflicts_probe(const mdns_server_t *server, const mdns_rr *rr)
{
    size_t k;
    char instance[256];

    if (!server || !rr || rr->ttl == 0) {
        return 0;
    }
    if (!server->hostname_claimed && server->hostname && mdns_name_equal(rr->name, server->hostname)) {
        return 1;
    }
    for (k = 0; k < server->num_services; k++) {
        if (!server->services || server->services[k].claimed) {
            continue;
        }
        mdns_server_format_instance_name(&server->services[k], instance, sizeof instance);
        if (instance[0] != '\0' && mdns_name_equal(rr->name, instance)) {
            return 1;
        }
    }
    return 0;
}

void mdns_server_note_probe_conflicts(mdns_server_t *server, const mdns_msg *msg)
{
    size_t i;
    size_t k;
    char instance[256];

    if (!server || !msg) {
        return;
    }
    if ((msg->flags & DNS_QR_BIT) == 0) {
        return;
    }
    if (server->probe_failed || mdns_server_all_claimed(server)) {
        return;
    }
    for (i = 0; i < msg->nrr; i++) {
        const mdns_rr *rr = &msg->rr[i];

        if (!mdns_server_rr_conflicts_probe(server, rr)) {
            continue;
        }
        if (!server->hostname_claimed && server->hostname && mdns_name_equal(rr->name, server->hostname)) {
            server->hostname_conflict = 1;
        }
        for (k = 0; k < server->num_services; k++) {
            if (!server->services || server->services[k].claimed) {
                continue;
            }
            mdns_server_format_instance_name(&server->services[k], instance, sizeof instance);
            if (instance[0] != '\0' && mdns_name_equal(rr->name, instance)) {
                server->services[k].probe_conflict = 1;
            }
        }
    }
}

void mdns_server_want_mask_unclaimed(mdns_server_want_t *w, const mdns_server_t *server)
{
    size_t k;

    if (!w || !server) {
        return;
    }
    if (server->probe_failed) {
        mdns_server_want_clear(w, w->nsvc);
        return;
    }
    if (!server->hostname_claimed) {
        w->host_answer = 0;
        w->host_additional = 0;
    }
    for (k = 0; k < w->nsvc && k < server->num_services; k++) {
        if (!server->services || !server->services[k].claimed) {
            w->svc_answer[k] = 0;
            w->svc_additional[k] = 0;
        }
    }
}

void mdns_server_build_probe(uint8_t *packet, size_t *packet_len, const mdns_server_t *server,
                             const mdns_server_interface_t *iface)
{
    mdns_buf b;
    size_t qd_pos;
    size_t ns_pos;
    uint16_t qdcount = 0;
    uint16_t nscount = 0;
    size_t k;
    char instance[256];
    const char *hostname;
    uint16_t qu_class;

    if (!packet_len) {
        return;
    }
    *packet_len = 0;
    if (!packet || !server) {
        return;
    }
    hostname = server->hostname;
    qu_class = (uint16_t)(DNS_CLASS_IN | DNS_QU_BIT);

    mdns_buf_init(&b, packet, MDNS_MAX_PACKET_SIZE);
    if (mdns_put_u16(&b, 0) < 0) {
        return;
    }
    if (mdns_put_u16(&b, DNS_FLAG_QUERY) < 0) {
        return;
    }
    qd_pos = b.len;
    if (mdns_put_u16(&b, 0) < 0) {
        return;
    }
    if (mdns_put_u16(&b, 0) < 0) {
        return;
    }
    ns_pos = b.len;
    if (mdns_put_u16(&b, 0) < 0) {
        return;
    }
    if (mdns_put_u16(&b, 0) < 0) {
        return;
    }

    if (!server->hostname_claimed && hostname) {
        if (mdns_put_name(&b, hostname) < 0) {
            return;
        }
        if (mdns_put_u16(&b, MDNS_TYPE_ANY) < 0) {
            return;
        }
        if (mdns_put_u16(&b, qu_class) < 0) {
            return;
        }
        qdcount++;
    }
    for (k = 0; k < server->num_services; k++) {
        if (!server->services || server->services[k].claimed) {
            continue;
        }
        mdns_server_format_instance_name(&server->services[k], instance, sizeof instance);
        if (instance[0] == '\0') {
            continue;
        }
        if (mdns_put_name(&b, instance) < 0) {
            return;
        }
        if (mdns_put_u16(&b, MDNS_TYPE_ANY) < 0) {
            return;
        }
        if (mdns_put_u16(&b, qu_class) < 0) {
            return;
        }
        qdcount++;
    }

    if (!server->hostname_claimed && hostname && iface) {
        uint16_t addrs = 0;

        if (mdns_server_put_host_addrs(&b, hostname, iface, MDNS_TTL_HOST, 1,
                                       MDNS_W_A | MDNS_W_AAAA, &addrs) < 0) {
            *packet_len = 0;
            return;
        }
        nscount = (uint16_t)(nscount + addrs);
    }
    for (k = 0; k < server->num_services; k++) {
        uint16_t added = 0;

        if (!server->services || server->services[k].claimed) {
            continue;
        }
        if (mdns_server_put_service_bits(&b, server, k, hostname,
                                         MDNS_W_SRV | MDNS_W_TXT, MDNS_TTL_SHARED, MDNS_TTL_HOST, 1, &added) < 0) {
            *packet_len = 0;
            return;
        }
        nscount = (uint16_t)(nscount + added);
    }

    if (b.overflow) {
        log_this(SR_MDNS_SERVER, "Probe packet overflow, skipping send", LOG_LEVEL_ALERT, 0);
        return;
    }
    if (qdcount == 0) {
        return;
    }
    b.buf[qd_pos] = (uint8_t)(qdcount >> 8);
    b.buf[qd_pos + 1] = (uint8_t)(qdcount & 0xffu);
    b.buf[ns_pos] = (uint8_t)(nscount >> 8);
    b.buf[ns_pos + 1] = (uint8_t)(nscount & 0xffu);
    *packet_len = b.len;
}

void mdns_server_send_probe(mdns_server_t *server)
{
    struct sockaddr_in addr_v4;
    struct sockaddr_in6 addr_v6;
    size_t i;
    size_t k;
    char instance[256];

    if (!server || server->probe_failed || mdns_server_all_claimed(server)) {
        return;
    }

    memset(&addr_v4, 0, sizeof addr_v4);
    addr_v4.sin_family = AF_INET;
    addr_v4.sin_port = htons(MDNS_PORT);
    inet_pton(AF_INET, MDNS_GROUP_V4, &addr_v4.sin_addr);

    memset(&addr_v6, 0, sizeof addr_v6);
    addr_v6.sin6_family = AF_INET6;
    addr_v6.sin6_port = htons(MDNS_PORT);
    inet_pton(AF_INET6, MDNS_GROUP_V6, &addr_v6.sin6_addr);

    if (!server->hostname_claimed && server->hostname) {
        log_this(SR_MDNS_SERVER, "MDNS_SERVER PROBE %s", LOG_LEVEL_DEBUG, 1, server->hostname);
    }
    for (k = 0; k < server->num_services; k++) {
        if (!server->services || server->services[k].claimed) {
            continue;
        }
        mdns_server_format_instance_name(&server->services[k], instance, sizeof instance);
        log_this(SR_MDNS_SERVER, "MDNS_SERVER PROBE %s", LOG_LEVEL_DEBUG, 1, instance);
    }

    for (i = 0; i < server->num_interfaces; i++) {
        mdns_server_interface_t *iface;
        uint8_t packet[MDNS_MAX_PACKET_SIZE];
        size_t packet_len = 0;

        if (!server->interfaces) {
            break;
        }
        iface = &server->interfaces[i];
        mdns_server_build_probe(packet, &packet_len, server, iface);
        if (packet_len == 0) {
            continue;
        }
        if (iface->sockfd_v4 >= 0) {
            if (sendto(iface->sockfd_v4, packet, packet_len, 0, (struct sockaddr *)&addr_v4, sizeof addr_v4) < 0) {
                log_this(SR_MDNS_SERVER, "Failed to send IPv4 probe on %s: %s", LOG_LEVEL_DEBUG, 2,
                         iface->if_name, strerror(errno));
            }
        }
        if (iface->sockfd_v6 >= 0) {
            if (sendto(iface->sockfd_v6, packet, packet_len, 0, (struct sockaddr *)&addr_v6, sizeof addr_v6) < 0) {
                log_this(SR_MDNS_SERVER, "Failed to send IPv6 probe on %s: %s", LOG_LEVEL_DEBUG, 2,
                         iface->if_name, strerror(errno));
            }
        }
    }
}
