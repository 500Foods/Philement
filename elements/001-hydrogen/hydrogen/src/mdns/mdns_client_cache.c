/*
 * mDNS client cache: parse responses, endpoints, goodbye.
 */

#include <src/hydrogen.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <time.h>

#include "mdns_client.h"

uint64_t mdns_client_now_ms(void)
{
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }
    return ((uint64_t)ts.tv_sec * 1000ull) + ((uint64_t)ts.tv_nsec / 1000000ull);
}

int mdns_client_configured_type(const mdns_client_t *client, const char *type)
{
    size_t i;

    if (!client || !type) {
        return 0;
    }
    for (i = 0; i < client->ntypes; i++) {
        if (client->browse_types[i] && mdns_name_equal(client->browse_types[i], type)) {
            return 1;
        }
    }
    return 0;
}

int mdns_client_endpoint_rank(int family, const uint8_t *addr, uint8_t addrlen)
{
    if (family == AF_INET) {
        return 0;
    }
    if (family == AF_INET6 && addr && addrlen >= 16) {
        if (addr[0] == 0xfe && (addr[1] & 0xc0) == 0x80) {
            return 2;
        }
        return 1;
    }
    return 3;
}

void mdns_client_sort_endpoints(mdns_client_service_t *svc)
{
    size_t i;
    size_t j;

    if (!svc || svc->nendpoints < 2) {
        return;
    }
    for (i = 0; i < svc->nendpoints; i++) {
        for (j = i + 1; j < svc->nendpoints; j++) {
            mdns_client_endpoint_t *a = &svc->endpoints[i];
            mdns_client_endpoint_t *b = &svc->endpoints[j];
            int ra = mdns_client_endpoint_rank(a->family, a->addr, a->addrlen);
            int rb = mdns_client_endpoint_rank(b->family, b->addr, b->addrlen);
            int cmp = 0;

            if (ra != rb) {
                cmp = ra - rb;
            } else if (a->addrlen != b->addrlen) {
                cmp = (int)a->addrlen - (int)b->addrlen;
            } else {
                cmp = memcmp(a->addr, b->addr, a->addrlen);
            }
            if (cmp > 0) {
                mdns_client_endpoint_t tmp = *a;
                *a = *b;
                *b = tmp;
            }
        }
    }
}

int mdns_client_add_endpoint(mdns_client_service_t *svc, int family,
                             const uint8_t *addr, uint8_t addrlen, unsigned ifindex)
{
    size_t i;

    if (!svc || !addr || addrlen == 0 || addrlen > 16) {
        return -1;
    }
    for (i = 0; i < svc->nendpoints; i++) {
        mdns_client_endpoint_t *ep = &svc->endpoints[i];
        if (ep->family == family && ep->addrlen == addrlen &&
            memcmp(ep->addr, addr, addrlen) == 0) {
            ep->ifindex = ifindex;
            return 0;
        }
    }
    if (svc->nendpoints >= MDNS_CLIENT_MAX_ENDPOINTS) {
        return -1;
    }
    svc->endpoints[svc->nendpoints].family = family;
    memcpy(svc->endpoints[svc->nendpoints].addr, addr, addrlen);
    svc->endpoints[svc->nendpoints].addrlen = addrlen;
    svc->endpoints[svc->nendpoints].ifindex = ifindex;
    svc->nendpoints++;
    mdns_client_sort_endpoints(svc);
    return 1;
}

void mdns_client_drop_service(mdns_client_t *client, size_t index, const char *reason)
{
    mdns_client_service_t *svc;

    if (!client || index >= client->nservices) {
        return;
    }
    svc = &client->services[index];
    log_this(SR_MDNS_CLIENT, "MDNS_CLIENT DROP %s %s", LOG_LEVEL_DEBUG, 2,
             svc->instance, reason ? reason : "");
    if (mdns_client_on_change) {
        mdns_client_on_change(MDNS_CLIENT_EVT_LOST, svc);
    }
    if (index + 1 < client->nservices) {
        memmove(&client->services[index], &client->services[index + 1],
                (client->nservices - index - 1) * sizeof(mdns_client_service_t));
    }
    client->nservices--;
}

mdns_client_service_t *mdns_client_find_instance(mdns_client_t *client, const char *instance)
{
    size_t i;

    if (!client || !instance) {
        return NULL;
    }
    for (i = 0; i < client->nservices; i++) {
        if (mdns_name_equal(client->services[i].instance, instance)) {
            return &client->services[i];
        }
    }
    return NULL;
}

mdns_client_service_t *mdns_client_insert_instance(mdns_client_t *client, const char *instance,
                                                   const char *type)
{
    mdns_client_service_t *svc;

    if (!client || !instance) {
        return NULL;
    }
    if (!client->own_services && mdns_client_is_own_instance(instance)) {
        return NULL;
    }
    if (client->nservices >= client->max_services) {
        log_this(SR_MDNS_CLIENT, "MDNS_CLIENT DROP %s max_services", LOG_LEVEL_DEBUG, 1, instance);
        return NULL;
    }
    svc = &client->services[client->nservices];
    memset(svc, 0, sizeof(*svc));
    strncpy(svc->instance, instance, sizeof(svc->instance) - 1);
    if (type) {
        strncpy(svc->type, type, sizeof(svc->type) - 1);
    }
    svc->healthy = 1;
    client->nservices++;
    return svc;
}

void mdns_client_log_addr(const char *host, int family, const uint8_t *addr)
{
    char buf[INET6_ADDRSTRLEN];

    if (family == AF_INET) {
        inet_ntop(AF_INET, addr, buf, sizeof buf);
    } else {
        inet_ntop(AF_INET6, addr, buf, sizeof buf);
    }
    log_this(SR_MDNS_CLIENT, "MDNS_CLIENT ADDR %s %s", LOG_LEVEL_DEBUG, 2, host, buf);
}

int mdns_client_handle_goodbye(mdns_client_t *client, const mdns_rr *rr, const uint8_t *msg,
                               size_t msglen)
{
    size_t i;

    if (rr->ttl != 0) {
        return 0;
    }
    if (rr->type == MDNS_TYPE_PTR) {
        char name[MDNS_NAME_MAX];
        if (mdns_rdata_name(msg, msglen, rr, name, sizeof name) < 0) {
            return 0;
        }
        for (i = 0; i < client->nservices; i++) {
            if (mdns_name_equal(client->services[i].instance, name)) {
                log_this(SR_MDNS_CLIENT, "MDNS_CLIENT GOODBYE %s", LOG_LEVEL_DEBUG, 1, name);
                mdns_client_drop_service(client, i, "goodbye");
                return 1;
            }
        }
        return 0;
    }
    if (rr->type == MDNS_TYPE_SRV || rr->type == MDNS_TYPE_TXT) {
        for (i = 0; i < client->nservices; i++) {
            if (mdns_name_equal(client->services[i].instance, rr->name)) {
                log_this(SR_MDNS_CLIENT, "MDNS_CLIENT GOODBYE %s", LOG_LEVEL_DEBUG, 1, rr->name);
                mdns_client_drop_service(client, i, "goodbye");
                return 1;
            }
        }
    }
    return 0;
}

int mdns_client_handle_response(mdns_client_t *client, const uint8_t *msg, size_t msglen,
                                unsigned ifindex)
{
    mdns_msg parsed;
    size_t i;
    uint64_t now;

    if (!client || !msg) {
        return -1;
    }
    if (mdns_parse(msg, msglen, &parsed) < 0) {
        return -1;
    }
    if ((parsed.flags & DNS_QR_BIT) == 0) {
        return 0;
    }

    pthread_mutex_lock(&client->lock);
    now = mdns_client_now_ms();

    for (i = 0; i < parsed.nrr; i++) {
        const mdns_rr *rr = &parsed.rr[i];
        char rname[MDNS_NAME_MAX];
        mdns_client_service_t *svc;
        uint16_t prio = 0;
        uint16_t weight = 0;
        uint16_t port = 0;

        if (mdns_client_handle_goodbye(client, rr, msg, msglen)) {
            continue;
        }

        if (rr->type == MDNS_TYPE_PTR && mdns_client_configured_type(client, rr->name)) {
            if (mdns_rdata_name(msg, msglen, rr, rname, sizeof rname) < 0) {
                continue;
            }
            svc = mdns_client_find_instance(client, rname);
            if (!svc) {
                svc = mdns_client_insert_instance(client, rname, rr->name);
            }
            if (svc) {
                svc->last_seen_ms = now;
                svc->ttl = rr->ttl;
                if (!svc->found_logged) {
                    log_this(SR_MDNS_CLIENT, "MDNS_CLIENT FOUND %s", LOG_LEVEL_DEBUG, 1, svc->instance);
                    svc->found_logged = 1;
                    svc->found_ms = now;
                    if (mdns_client_on_change) {
                        mdns_client_on_change(MDNS_CLIENT_EVT_FOUND, svc);
                    }
                }
            }
            continue;
        }

        if (rr->type == MDNS_TYPE_SRV) {
            char target[MDNS_NAME_MAX];
            if (mdns_rdata_srv(msg, msglen, rr, &prio, &weight, &port, target, sizeof target) < 0) {
                continue;
            }
            svc = mdns_client_find_instance(client, rr->name);
            if (!svc) {
                svc = mdns_client_insert_instance(client, rr->name, NULL);
            }
            if (svc) {
                memcpy(svc->host, target, sizeof(svc->host) - 1);
                svc->host[sizeof(svc->host) - 1] = '\0';
                svc->port = port;
                svc->have_srv = 1;
                svc->last_seen_ms = now;
                svc->ttl = rr->ttl;
                if (!svc->srv_logged) {
                    log_this(SR_MDNS_CLIENT, "MDNS_CLIENT SRV %s %s %u", LOG_LEVEL_DEBUG, 3,
                             svc->instance, svc->host, (unsigned)svc->port);
                    svc->srv_logged = 1;
                }
            }
            continue;
        }

        if (rr->type == MDNS_TYPE_TXT) {
            svc = mdns_client_find_instance(client, rr->name);
            if (!svc) {
                continue;
            }
            char path[MDNS_CLIENT_PATH_MAX];
            svc->have_txt = 1;
            svc->last_seen_ms = now;
            if (mdns_txt_get(msg + rr->rdoff, rr->rdlen, "path", path, sizeof path) == 0) {
                memcpy(svc->path, path, sizeof(svc->path) - 1);
                svc->path[sizeof(svc->path) - 1] = '\0';
            }
            if (!svc->txt_logged) {
                if (svc->path[0] != '\0') {
                    log_this(SR_MDNS_CLIENT, "MDNS_CLIENT TXT %s path=%s", LOG_LEVEL_DEBUG, 2,
                             svc->instance, svc->path);
                } else {
                    log_this(SR_MDNS_CLIENT, "MDNS_CLIENT TXT %s", LOG_LEVEL_DEBUG, 1, svc->instance);
                }
                svc->txt_logged = 1;
            }
            continue;
        }

        if (rr->type == MDNS_TYPE_A || rr->type == MDNS_TYPE_AAAA) {
            size_t s;
            int family = (rr->type == MDNS_TYPE_A) ? AF_INET : AF_INET6;
            uint8_t addrlen = (rr->type == MDNS_TYPE_A) ? 4 : 16;
            const uint8_t *addr;

            if (rr->rdlen < addrlen) {
                continue;
            }
            addr = msg + rr->rdoff;
            for (s = 0; s < client->nservices; s++) {
                int added;
                svc = &client->services[s];
                if (svc->host[0] == '\0' || !mdns_name_equal(svc->host, rr->name)) {
                    continue;
                }
                added = mdns_client_add_endpoint(svc, family, addr, addrlen, ifindex);
                if (added > 0) {
                    mdns_client_log_addr(svc->host, family, addr);
                }
                svc->last_seen_ms = now;
            }
        }
    }

    pthread_mutex_unlock(&client->lock);
    return 0;
}
