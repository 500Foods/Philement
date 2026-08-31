/*
 * mDNS selective query responder (RFC 6762 questions, known-answer, QU, legacy).
 */

#include <src/hydrogen.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "mdns_keys.h"
#include "mdns_server.h"
#include <src/mdns/mdns_wire.h>

void mdns_server_format_instance_name(const mdns_server_service_t *svc, char *out, size_t cap)
{
    if (!out || cap == 0) {
        return;
    }
    if (!svc || !svc->name || !svc->type) {
        out[0] = '\0';
        return;
    }
    snprintf(out, cap, "%s.%s", svc->name, svc->type);
}

void mdns_server_want_clear(mdns_server_want_t *w, size_t nsvc)
{
    if (!w) {
        return;
    }
    memset(w, 0, sizeof(*w));
    w->nsvc = nsvc;
    if (w->nsvc > MDNS_SERVER_WANT_MAX_SVC) {
        w->nsvc = MDNS_SERVER_WANT_MAX_SVC;
    }
}

int mdns_server_want_empty(const mdns_server_want_t *w)
{
    size_t i;

    if (!w) {
        return 1;
    }
    if (w->host_answer != 0 || w->host_additional != 0) {
        return 0;
    }
    for (i = 0; i < w->nsvc; i++) {
        if (w->svc_answer[i] != 0 || w->svc_additional[i] != 0) {
            return 0;
        }
    }
    return 1;
}

void mdns_server_want_add_question(mdns_server_want_t *w, const mdns_server_t *server, const mdns_rr *q)
{
    uint16_t qtype;
    uint16_t qclass;
    size_t k;
    int is_any;

    if (!w || !server || !q) {
        return;
    }
    qclass = (uint16_t)(q->cls & DNS_CLASS_MASK);
    if (qclass != DNS_CLASS_IN && qclass != DNS_CLASS_ANY) {
        return;
    }
    if (q->cls & DNS_QU_BIT) {
        w->qu = 1;
    }
    qtype = q->type;
    is_any = (qtype == MDNS_TYPE_ANY);

    if (mdns_name_equal(q->name, MDNS_DNS_SD_NAME) && (qtype == MDNS_TYPE_PTR || is_any)) {
        for (k = 0; k < w->nsvc && k < server->num_services; k++) {
            w->svc_answer[k] |= MDNS_W_SD;
        }
    }

    if (server->hostname && (mdns_name_equal(q->name, server->hostname))) {
        if (qtype == MDNS_TYPE_A || is_any) {
            w->host_answer |= MDNS_W_A | MDNS_W_NSEC;
        }
        if (qtype == MDNS_TYPE_AAAA || is_any) {
            w->host_answer |= MDNS_W_AAAA | MDNS_W_NSEC;
        }
        if (qtype == (uint16_t)RR_NSEC) {
            w->host_answer |= MDNS_W_NSEC;
        }
    }

    for (k = 0; k < w->nsvc && k < server->num_services; k++) {
        char instance[256];

        if (!server->services) {
            break;
        }
        mdns_server_format_instance_name(&server->services[k], instance, sizeof instance);
        if (server->services[k].type && mdns_name_equal(q->name, server->services[k].type) &&
            (qtype == MDNS_TYPE_PTR || is_any)) {
            w->svc_answer[k] |= MDNS_W_PTR;
            w->svc_additional[k] |= MDNS_W_SRV | MDNS_W_TXT;
            w->host_additional |= MDNS_W_A | MDNS_W_AAAA | MDNS_W_NSEC;
        }
        if (instance[0] != '\0' && mdns_name_equal(q->name, instance)) {
            if (qtype == MDNS_TYPE_SRV || is_any) {
                w->svc_answer[k] |= MDNS_W_SRV;
                w->host_additional |= MDNS_W_A | MDNS_W_AAAA | MDNS_W_NSEC;
            }
            if (qtype == MDNS_TYPE_TXT || is_any) {
                w->svc_answer[k] |= MDNS_W_TXT;
            }
        }
    }
}

void mdns_server_strip_known_answers(mdns_server_want_t *w, const mdns_server_t *server,
                                     const uint8_t *raw, size_t rawlen, const mdns_msg *msg)
{
    size_t i;
    size_t k;
    uint32_t shared_half = MDNS_TTL_SHARED / 2u;
    uint32_t host_half = MDNS_TTL_HOST / 2u;

    if (!w || !server || !msg) {
        return;
    }
    if (msg->flags & DNS_QR_BIT) {
        return;
    }

    for (i = 0; i < msg->nrr; i++) {
        const mdns_rr *rr = &msg->rr[i];

        if (rr->section != MDNS_SEC_ANSWER) {
            continue;
        }
        if (rr->type == MDNS_TYPE_PTR) {
            char target[MDNS_NAME_MAX];

            if (!raw || mdns_rdata_name(raw, rawlen, rr, target, sizeof target) < 0) {
                continue;
            }
            if (rr->ttl <= shared_half) {
                continue;
            }
            if (mdns_name_equal(rr->name, MDNS_DNS_SD_NAME)) {
                for (k = 0; k < w->nsvc && k < server->num_services; k++) {
                    if (server->services && server->services[k].type &&
                        mdns_name_equal(target, server->services[k].type)) {
                        w->svc_answer[k] &= ~MDNS_W_SD;
                    }
                }
            }
            for (k = 0; k < w->nsvc && k < server->num_services; k++) {
                char instance[256];

                if (!server->services) {
                    break;
                }
                mdns_server_format_instance_name(&server->services[k], instance, sizeof instance);
                if (server->services[k].type && mdns_name_equal(rr->name, server->services[k].type) &&
                    mdns_name_equal(target, instance)) {
                    w->svc_answer[k] &= ~MDNS_W_PTR;
                }
            }
        } else if (rr->type == MDNS_TYPE_SRV || rr->type == MDNS_TYPE_TXT) {
            uint32_t half = (rr->type == MDNS_TYPE_TXT) ? shared_half : host_half;
            uint32_t bit = (rr->type == MDNS_TYPE_TXT) ? MDNS_W_TXT : MDNS_W_SRV;

            if (rr->ttl <= half) {
                continue;
            }
            for (k = 0; k < w->nsvc && k < server->num_services; k++) {
                char instance[256];

                if (!server->services) {
                    break;
                }
                mdns_server_format_instance_name(&server->services[k], instance, sizeof instance);
                if (mdns_name_equal(rr->name, instance)) {
                    w->svc_answer[k] &= ~bit;
                    w->svc_additional[k] &= ~bit;
                }
            }
        } else if (rr->type == MDNS_TYPE_A || rr->type == MDNS_TYPE_AAAA) {
            uint32_t bit = (rr->type == MDNS_TYPE_A) ? MDNS_W_A : MDNS_W_AAAA;

            if (rr->ttl <= host_half) {
                continue;
            }
            if (server->hostname && mdns_name_equal(rr->name, server->hostname)) {
                w->host_answer &= ~bit;
                w->host_additional &= ~bit;
            }
        }
    }
}

int mdns_server_should_unicast(int legacy, int qu)
{
    return (legacy || qu) ? 1 : 0;
}

int mdns_server_should_multicast(int legacy, int qu)
{
    (void)qu;
    return legacy ? 0 : 1;
}

uint16_t mdns_server_sockaddr_port(const void *src_addr, uint32_t src_len)
{
    const struct sockaddr *sa = (const struct sockaddr *)src_addr;

    if (!src_addr || src_len < sizeof(sa_family_t)) {
        return (uint16_t)MDNS_PORT;
    }
    if (sa->sa_family == AF_INET && src_len >= (uint32_t)sizeof(struct sockaddr_in)) {
        return ntohs(((const struct sockaddr_in *)sa)->sin_port);
    }
    if (sa->sa_family == AF_INET6 && src_len >= (uint32_t)sizeof(struct sockaddr_in6)) {
        return ntohs(((const struct sockaddr_in6 *)sa)->sin6_port);
    }
    return (uint16_t)MDNS_PORT;
}

const mdns_server_interface_t *mdns_server_iface_for_sock(const mdns_server_t *server, int sockfd)
{
    size_t i;

    if (!server || !server->interfaces || sockfd < 0) {
        return NULL;
    }
    for (i = 0; i < server->num_interfaces; i++) {
        if (server->interfaces[i].sockfd_v4 == sockfd || server->interfaces[i].sockfd_v6 == sockfd) {
            return &server->interfaces[i];
        }
    }
    return NULL;
}

uint32_t mdns_server_response_ttl(uint32_t base, int legacy)
{
    if (legacy && base > MDNS_LEGACY_TTL_CAP) {
        return MDNS_LEGACY_TTL_CAP;
    }
    return base;
}

int mdns_server_iface_has_af(const mdns_server_interface_t *iface, int family)
{
    size_t i;

    if (!iface || !iface->ip_addresses) {
        return 0;
    }
    for (i = 0; i < iface->num_addresses; i++) {
        struct in_addr addr;
        struct in6_addr addr6;

        if (!iface->ip_addresses[i]) {
            continue;
        }
        if (family == AF_INET && inet_pton(AF_INET, iface->ip_addresses[i], &addr) == 1) {
            return 1;
        }
        if (family == AF_INET6 && inet_pton(AF_INET6, iface->ip_addresses[i], &addr6) == 1) {
            return 1;
        }
    }
    return 0;
}

void mdns_server_want_apply_missing_family(mdns_server_want_t *w, const mdns_server_interface_t *iface)
{
    int has_v4;
    int has_v6;

    if (!w || !iface) {
        return;
    }
    has_v4 = mdns_server_iface_has_af(iface, AF_INET);
    has_v6 = mdns_server_iface_has_af(iface, AF_INET6);
    if ((w->host_answer & MDNS_W_A) && !has_v4) {
        w->host_answer &= ~MDNS_W_A;
        w->host_answer |= MDNS_W_NSEC;
    }
    if ((w->host_additional & MDNS_W_A) && !has_v4) {
        w->host_additional &= ~MDNS_W_A;
        w->host_additional |= MDNS_W_NSEC;
    }
    if ((w->host_answer & MDNS_W_AAAA) && !has_v6) {
        w->host_answer &= ~MDNS_W_AAAA;
        w->host_answer |= MDNS_W_NSEC;
    }
    if ((w->host_additional & MDNS_W_AAAA) && !has_v6) {
        w->host_additional &= ~MDNS_W_AAAA;
        w->host_additional |= MDNS_W_NSEC;
    }
}

int mdns_server_put_host_addrs(mdns_buf *b, const char *hostname, const mdns_server_interface_t *iface,
                               uint32_t ttl, int flush, uint32_t bits, uint16_t *count)
{
    size_t i;

    if (!b || !hostname || !iface || !count) {
        return 0;
    }
    for (i = 0; i < iface->num_addresses; i++) {
        struct in_addr addr;
        struct in6_addr addr6;
        size_t pos;

        if (!iface->ip_addresses || !iface->ip_addresses[i]) {
            continue;
        }
        if ((bits & MDNS_W_A) && inet_pton(AF_INET, iface->ip_addresses[i], &addr) == 1) {
            if (mdns_rr_head(b, hostname, MDNS_TYPE_A, ttl, flush, &pos) < 0) {
                return -1;
            }
            if (mdns_put_bytes(b, &addr.s_addr, 4) < 0) {
                return -1;
            }
            if (mdns_rr_tail(b, pos) < 0) {
                return -1;
            }
            (*count)++;
        } else if ((bits & MDNS_W_AAAA) && inet_pton(AF_INET6, iface->ip_addresses[i], &addr6) == 1) {
            if (mdns_rr_head(b, hostname, MDNS_TYPE_AAAA, ttl, flush, &pos) < 0) {
                return -1;
            }
            if (mdns_put_bytes(b, &addr6.s6_addr, 16) < 0) {
                return -1;
            }
            if (mdns_rr_tail(b, pos) < 0) {
                return -1;
            }
            (*count)++;
        }
    }
    return 0;
}

int mdns_server_put_host_nsec(mdns_buf *b, const char *hostname, const mdns_server_interface_t *iface,
                              uint32_t ttl, int flush, uint16_t *count)
{
    uint16_t types[3];
    size_t ntypes = 0;

    if (!b || !hostname || !iface || !count) {
        return 0;
    }
    if (mdns_server_iface_has_af(iface, AF_INET)) {
        types[ntypes] = MDNS_TYPE_A;
        ntypes++;
    }
    if (mdns_server_iface_has_af(iface, AF_INET6)) {
        types[ntypes] = MDNS_TYPE_AAAA;
        ntypes++;
    }
    types[ntypes] = (uint16_t)RR_NSEC;
    ntypes++;
    if (mdns_put_rr_nsec(b, hostname, types, ntypes, ttl, flush) < 0) {
        return -1;
    }
    (*count)++;
    return 0;
}

int mdns_server_put_service_bits(mdns_buf *b, const mdns_server_t *server, size_t si, const char *hostname,
                                 uint32_t bits, uint32_t shared_ttl, uint32_t host_ttl, int flush, uint16_t *count)
{
    const mdns_server_service_t *svc;
    char instance[256];
    size_t pos;

    if (!b || !server || !server->services || si >= server->num_services || !count) {
        return 0;
    }
    svc = &server->services[si];
    mdns_server_format_instance_name(svc, instance, sizeof instance);

    if ((bits & MDNS_W_SD) && svc->type) {
        if (mdns_rr_head(b, MDNS_DNS_SD_NAME, MDNS_TYPE_PTR, shared_ttl, 0, &pos) < 0) {
            return -1;
        }
        if (mdns_put_name(b, svc->type) < 0) {
            return -1;
        }
        if (mdns_rr_tail(b, pos) < 0) {
            return -1;
        }
        (*count)++;
    }
    if ((bits & MDNS_W_PTR) && svc->type && instance[0] != '\0') {
        if (mdns_rr_head(b, svc->type, MDNS_TYPE_PTR, shared_ttl, 0, &pos) < 0) {
            return -1;
        }
        if (mdns_put_name(b, instance) < 0) {
            return -1;
        }
        if (mdns_rr_tail(b, pos) < 0) {
            return -1;
        }
        (*count)++;
    }
    if ((bits & MDNS_W_SRV) && instance[0] != '\0' && hostname) {
        if (mdns_rr_head(b, instance, MDNS_TYPE_SRV, host_ttl, flush, &pos) < 0) {
            return -1;
        }
        if (mdns_put_u16(b, 0) < 0 || mdns_put_u16(b, 0) < 0) {
            return -1;
        }
        if (mdns_put_u16(b, (uint16_t)svc->port) < 0) {
            return -1;
        }
        if (mdns_put_name(b, hostname) < 0) {
            return -1;
        }
        if (mdns_rr_tail(b, pos) < 0) {
            return -1;
        }
        (*count)++;
    }
    if ((bits & MDNS_W_TXT) && instance[0] != '\0') {
        if (mdns_rr_head(b, instance, MDNS_TYPE_TXT, shared_ttl, flush, &pos) < 0) {
            return -1;
        }
        for (size_t t = 0; t < svc->num_txt_records; t++) {
            const char *txt = svc->txt_records ? svc->txt_records[t] : NULL;
            size_t txt_len;

            if (!txt) {
                continue;
            }
            txt_len = strlen(txt);
            if (txt_len > 255) {
                txt_len = 255;
            }
            if (mdns_put_u8(b, (uint8_t)txt_len) < 0) {
                return -1;
            }
            if (mdns_put_bytes(b, txt, txt_len) < 0) {
                return -1;
            }
        }
        if (mdns_rr_tail(b, pos) < 0) {
            return -1;
        }
        (*count)++;
    }
    return 0;
}

void mdns_server_build_query_response(uint8_t *packet, size_t *packet_len,
                                      const mdns_server_t *server,
                                      const mdns_server_interface_t *iface,
                                      const mdns_msg *query,
                                      const mdns_server_want_t *want,
                                      int legacy)
{
    mdns_buf b;
    mdns_server_want_t local_want;
    size_t an_pos;
    size_t ns_pos;
    size_t ar_pos;
    uint16_t ancount = 0;
    uint16_t arcount = 0;
    uint32_t shared_ttl;
    uint32_t host_ttl;
    int flush;
    size_t k;
    const char *hostname;

    if (!packet || !packet_len) {
        return;
    }
    *packet_len = 0;
    if (!server || !want) {
        return;
    }

    local_want = *want;
    mdns_server_want_apply_missing_family(&local_want, iface);
    want = &local_want;

    hostname = server->hostname;
    shared_ttl = mdns_server_response_ttl(MDNS_TTL_SHARED, legacy);
    host_ttl = mdns_server_response_ttl(MDNS_TTL_HOST, legacy);
    flush = legacy ? 0 : 1;

    mdns_buf_init(&b, packet, MDNS_MAX_PACKET_SIZE);
    if (mdns_put_u16(&b, (uint16_t)(legacy && query ? query->id : 0)) < 0) {
        return;
    }
    if (mdns_put_u16(&b, DNS_FLAG_RESPONSE) < 0) {
        return;
    }
    if (legacy && query) {
        size_t qi;

        if (mdns_put_u16(&b, query->qdcount) < 0) {
            return;
        }
        an_pos = b.len;
        if (mdns_put_u16(&b, 0) < 0) {
            return;
        }
        ns_pos = b.len;
        if (mdns_put_u16(&b, 0) < 0) {
            return;
        }
        ar_pos = b.len;
        if (mdns_put_u16(&b, 0) < 0) {
            return;
        }
        for (qi = 0; qi < query->nquestions; qi++) {
            const mdns_rr *q = &query->questions[qi];

            if (mdns_put_name(&b, q->name) < 0) {
                *packet_len = 0;
                return;
            }
            if (mdns_put_u16(&b, q->type) < 0) {
                *packet_len = 0;
                return;
            }
            if (mdns_put_u16(&b, (uint16_t)(q->cls & DNS_CLASS_MASK)) < 0) {
                *packet_len = 0;
                return;
            }
        }
    } else {
        if (mdns_put_u16(&b, 0) < 0) {
            return;
        }
        an_pos = b.len;
        if (mdns_put_u16(&b, 0) < 0) {
            return;
        }
        ns_pos = b.len;
        if (mdns_put_u16(&b, 0) < 0) {
            return;
        }
        ar_pos = b.len;
        if (mdns_put_u16(&b, 0) < 0) {
            return;
        }
    }
    (void)ns_pos;

    for (k = 0; k < want->nsvc; k++) {
        if (mdns_server_put_service_bits(&b, server, k, hostname, want->svc_answer[k],
                                         shared_ttl, host_ttl, flush, &ancount) < 0) {
            *packet_len = 0;
            return;
        }
    }
    if (hostname && iface) {
        if (mdns_server_put_host_addrs(&b, hostname, iface, host_ttl, flush,
                                       want->host_answer, &ancount) < 0) {
            *packet_len = 0;
            return;
        }
        if ((want->host_answer & MDNS_W_NSEC) &&
            mdns_server_put_host_nsec(&b, hostname, iface, host_ttl, flush, &ancount) < 0) {
            *packet_len = 0;
            return;
        }
    }

    for (k = 0; k < want->nsvc; k++) {
        uint32_t extra = want->svc_additional[k] & ~want->svc_answer[k];

        if (mdns_server_put_service_bits(&b, server, k, hostname, extra,
                                         shared_ttl, host_ttl, flush, &arcount) < 0) {
            *packet_len = 0;
            return;
        }
    }
    if (hostname && iface) {
        uint32_t extra = want->host_additional & ~want->host_answer;

        if (mdns_server_put_host_addrs(&b, hostname, iface, host_ttl, flush, extra, &arcount) < 0) {
            *packet_len = 0;
            return;
        }
        if ((extra & MDNS_W_NSEC) &&
            mdns_server_put_host_nsec(&b, hostname, iface, host_ttl, flush, &arcount) < 0) {
            *packet_len = 0;
            return;
        }
    }

    if (b.overflow) {
        log_this(SR_MDNS_SERVER, "Query response overflow, skipping send", LOG_LEVEL_ALERT, 0);
        *packet_len = 0;
        return;
    }
    b.buf[an_pos] = (uint8_t)(ancount >> 8);
    b.buf[an_pos + 1] = (uint8_t)(ancount & 0xffu);
    b.buf[ar_pos] = (uint8_t)(arcount >> 8);
    b.buf[ar_pos + 1] = (uint8_t)(arcount & 0xffu);
    if (ancount == 0 && arcount == 0) {
        *packet_len = 0;
        return;
    }
    *packet_len = b.len;
}

void mdns_server_send_query_response(int sockfd, const mdns_server_t *server, const void *src_addr,
                                     uint32_t src_len, int legacy, int qu, const uint8_t *packet, size_t packet_len)
{
    const mdns_server_interface_t *iface;
    int is_v6 = 0;
    const struct sockaddr *sa = (const struct sockaddr *)src_addr;

    if (sockfd < 0 || !packet || packet_len == 0) {
        return;
    }
    iface = mdns_server_iface_for_sock(server, sockfd);
    if (iface && sockfd == iface->sockfd_v6) {
        is_v6 = 1;
    } else if (iface && sockfd == iface->sockfd_v4) {
        is_v6 = 0;
    } else if (sa && src_len >= sizeof(sa_family_t)) {
        is_v6 = (sa->sa_family == AF_INET6) ? 1 : 0;
    }

    if (mdns_server_should_multicast(legacy, qu)) {
        if (is_v6) {
            struct sockaddr_in6 dest;

            memset(&dest, 0, sizeof dest);
            dest.sin6_family = AF_INET6;
            dest.sin6_port = htons(MDNS_PORT);
            inet_pton(AF_INET6, MDNS_GROUP_V6, &dest.sin6_addr);
            if (sendto(sockfd, packet, packet_len, 0, (struct sockaddr *)&dest, sizeof dest) < 0) {
                log_this(SR_MDNS_SERVER, "Failed to send mDNS multicast reply: %s", LOG_LEVEL_DEBUG, 1, strerror(errno));
            }
        } else {
            struct sockaddr_in dest;

            memset(&dest, 0, sizeof dest);
            dest.sin_family = AF_INET;
            dest.sin_port = htons(MDNS_PORT);
            inet_pton(AF_INET, MDNS_GROUP_V4, &dest.sin_addr);
            if (sendto(sockfd, packet, packet_len, 0, (struct sockaddr *)&dest, sizeof dest) < 0) {
                log_this(SR_MDNS_SERVER, "Failed to send mDNS multicast reply: %s", LOG_LEVEL_DEBUG, 1, strerror(errno));
            }
        }
    }
    if (mdns_server_should_unicast(legacy, qu) && src_addr && src_len > 0) {
        if (sendto(sockfd, packet, packet_len, 0, sa, (socklen_t)src_len) < 0) {
            log_this(SR_MDNS_SERVER, "Failed to send mDNS unicast reply: %s", LOG_LEVEL_DEBUG, 1, strerror(errno));
        }
    }
}

bool mdns_server_process_query_packet(mdns_server_t *mdns_server_instance,
                                       const network_info_t *net_info_instance,
                                       const uint8_t *buffer,
                                       ssize_t len,
                                       int sockfd,
                                       const void *src_addr,
                                       uint32_t src_len)
{
    mdns_msg msg;
    mdns_server_want_t want;
    const mdns_server_interface_t *iface;
    uint8_t packet[MDNS_MAX_PACKET_SIZE];
    size_t packet_len = 0;
    size_t j;
    int legacy;
    uint16_t src_port;

    (void)net_info_instance;
    mdns_wire_keep_linked();

    if (!mdns_server_instance || !buffer || len < (ssize_t)sizeof(dns_header_t)) {
        return false;
    }
    if (mdns_parse(buffer, (size_t)len, &msg) < 0) {
        return false;
    }
    if (msg.flags & DNS_QR_BIT) {
        mdns_server_note_probe_conflicts(mdns_server_instance, &msg);
        return true;
    }
    if (mdns_server_instance->probe_failed) {
        return true;
    }

    mdns_server_want_clear(&want, mdns_server_instance->num_services);
    for (j = 0; j < msg.nquestions; j++) {
        mdns_server_want_add_question(&want, mdns_server_instance, &msg.questions[j]);
    }
    mdns_server_want_mask_unclaimed(&want, mdns_server_instance);
    mdns_server_strip_known_answers(&want, mdns_server_instance, buffer, (size_t)len, &msg);

    src_port = mdns_server_sockaddr_port(src_addr, src_len);
    legacy = (src_port != (uint16_t)MDNS_PORT) ? 1 : 0;
    if (sockfd < 0) {
        return true;
    }

    iface = mdns_server_iface_for_sock(mdns_server_instance, sockfd);
    mdns_server_want_apply_missing_family(&want, iface);
    if (mdns_server_want_empty(&want)) {
        return true;
    }
    mdns_server_build_query_response(packet, &packet_len, mdns_server_instance, iface, &msg, &want, legacy);
    if (packet_len == 0) {
        return true;
    }
    mdns_server_send_query_response(sockfd, mdns_server_instance, src_addr, src_len, legacy, want.qu, packet, packet_len);
    return true;
}
