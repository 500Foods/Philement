/*
 * mDNS client browse worker: sockets, queries, thread.
 */

#include <src/hydrogen.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <src/mdns/mdns_server.h>
#include <src/network/network.h>
#include "mdns_client.h"

extern AppConfig *app_config;

mdns_client_t *mdns_client_instance = NULL;
mdns_client_on_change_fn mdns_client_on_change = NULL;

int mdns_client_build_query(uint8_t *out, size_t cap, size_t *outlen,
                            const char *qname, uint16_t qtype)
{
    mdns_buf b;

    if (!out || !outlen || !qname) {
        return -1;
    }
    mdns_buf_init(&b, out, cap);
    if (mdns_put_u16(&b, 0) < 0 ||
        mdns_put_u16(&b, DNS_FLAG_QUERY) < 0 ||
        mdns_put_u16(&b, 1) < 0 ||
        mdns_put_u16(&b, 0) < 0 ||
        mdns_put_u16(&b, 0) < 0 ||
        mdns_put_u16(&b, 0) < 0 ||
        mdns_put_name(&b, qname) < 0 ||
        mdns_put_u16(&b, qtype) < 0 ||
        mdns_put_u16(&b, DNS_CLASS_IN) < 0) {
        return -1;
    }
    if (b.overflow) {
        return -1;
    }
    *outlen = b.len;
    return 0;
}

int mdns_client_send_on_ifaces(mdns_client_t *client, const uint8_t *packet, size_t packet_len)
{
    struct sockaddr_in addr_v4;
    struct sockaddr_in6 addr_v6;
    size_t i;

    memset(&addr_v4, 0, sizeof addr_v4);
    addr_v4.sin_family = AF_INET;
    addr_v4.sin_port = htons(MDNS_PORT);
    addr_v4.sin_addr.s_addr = inet_addr(MDNS_GROUP_V4);

    memset(&addr_v6, 0, sizeof addr_v6);
    addr_v6.sin6_family = AF_INET6;
    addr_v6.sin6_port = htons(MDNS_PORT);
    inet_pton(AF_INET6, MDNS_GROUP_V6, &addr_v6.sin6_addr);

    for (i = 0; i < client->nifaces; i++) {
        mdns_client_iface_t *iface = &client->ifaces[i];
        if (iface->sockfd_v4 >= 0) {
            if (sendto(iface->sockfd_v4, packet, packet_len, 0,
                       (struct sockaddr *)&addr_v4, sizeof addr_v4) < 0) {
                log_this(SR_MDNS_CLIENT, "send IPv4 query failed: %s", LOG_LEVEL_DEBUG, 1, strerror(errno));
            }
        }
        if (iface->sockfd_v6 >= 0) {
            if (sendto(iface->sockfd_v6, packet, packet_len, 0,
                       (struct sockaddr *)&addr_v6, sizeof addr_v6) < 0) {
                log_this(SR_MDNS_CLIENT, "send IPv6 query failed: %s", LOG_LEVEL_DEBUG, 1, strerror(errno));
            }
        }
    }
    return 0;
}

int mdns_client_send_query(mdns_client_t *client, const char *qname, uint16_t qtype)
{
    uint8_t packet[512];
    size_t packet_len = 0;

    if (!client || !qname) {
        return -1;
    }
    if (mdns_client_build_query(packet, sizeof packet, &packet_len, qname, qtype) < 0) {
        return -1;
    }
    log_this(SR_MDNS_CLIENT, "MDNS_CLIENT QUERY %s", LOG_LEVEL_DEBUG, 1, qname);
    return mdns_client_send_on_ifaces(client, packet, packet_len);
}

void mdns_client_browse_all(mdns_client_t *client)
{
    size_t i;

    for (i = 0; i < client->ntypes; i++) {
        if (client->browse_types[i]) {
            (void)mdns_client_send_query(client, client->browse_types[i], MDNS_TYPE_PTR);
        }
    }
}

void mdns_client_resolve_pending(mdns_client_t *client)
{
    size_t i;
    uint64_t now = mdns_client_now_ms();

    pthread_mutex_lock(&client->lock);
    for (i = 0; i < client->nservices; i++) {
        mdns_client_service_t *svc = &client->services[i];
        char host[MDNS_NAME_MAX];
        char instance[MDNS_NAME_MAX];
        int need_srv;
        int need_txt;
        int need_addr;

        strncpy(instance, svc->instance, sizeof instance - 1);
        instance[sizeof instance - 1] = '\0';
        strncpy(host, svc->host, sizeof host - 1);
        host[sizeof host - 1] = '\0';
        need_srv = !svc->have_srv;
        need_txt = !svc->have_txt &&
                   (now < svc->found_ms + (uint64_t)MDNS_CLIENT_TXT_GRACE_MS);
        need_addr = svc->have_srv && svc->nendpoints == 0 && host[0] != '\0';
        pthread_mutex_unlock(&client->lock);

        if (need_srv) {
            (void)mdns_client_send_query(client, instance, MDNS_TYPE_SRV);
        }
        if (need_txt) {
            (void)mdns_client_send_query(client, instance, MDNS_TYPE_TXT);
        }
        if (need_addr) {
            (void)mdns_client_send_query(client, host, MDNS_TYPE_A);
            (void)mdns_client_send_query(client, host, MDNS_TYPE_AAAA);
        }

        pthread_mutex_lock(&client->lock);
    }
    pthread_mutex_unlock(&client->lock);
}

void mdns_client_close_ifaces(mdns_client_t *client)
{
    size_t i;

    if (!client || !client->ifaces) {
        return;
    }
    for (i = 0; i < client->nifaces; i++) {
        if (client->ifaces[i].sockfd_v4 >= 0) {
            close(client->ifaces[i].sockfd_v4);
            client->ifaces[i].sockfd_v4 = -1;
        }
        if (client->ifaces[i].sockfd_v6 >= 0) {
            close(client->ifaces[i].sockfd_v6);
            client->ifaces[i].sockfd_v6 = -1;
        }
    }
}

int mdns_client_open_ifaces(mdns_client_t *client)
{
    network_info_t *raw;
    network_info_t *net;
    int i;
    size_t n = 0;

    raw = get_network_info();
    if (!raw) {
        return -1;
    }
    net = filter_enabled_interfaces(raw, app_config);
    free_network_info(raw);
    if (!net || net->count == 0) {
        if (net) {
            free_network_info(net);
        }
        return -1;
    }

    client->ifaces = calloc((size_t)net->count, sizeof(mdns_client_iface_t));
    if (!client->ifaces) {
        free_network_info(net);
        return -1;
    }

    for (i = 0; i < net->count; i++) {
        const interface_t *iface = &net->interfaces[i];
        mdns_client_iface_t *out;

        if (strcmp(iface->name, "lo") == 0 || iface->ip_count == 0) {
            continue;
        }
        out = &client->ifaces[n];
        strncpy(out->if_name, iface->name, sizeof(out->if_name) - 1);
        out->ifindex = if_nametoindex(iface->name);
        out->sockfd_v4 = -1;
        out->sockfd_v6 = -1;
        if (client->enable_ipv4) {
            out->sockfd_v4 = create_multicast_socket(AF_INET, MDNS_GROUP_V4, out->if_name);
        }
        if (client->enable_ipv6) {
            out->sockfd_v6 = create_multicast_socket(AF_INET6, MDNS_GROUP_V6, out->if_name);
        }
        if (out->sockfd_v4 >= 0 || out->sockfd_v6 >= 0) {
            n++;
        }
    }
    client->nifaces = n;
    free_network_info(net);
    return (n > 0) ? 0 : -1;
}

void mdns_client_recv_iface(mdns_client_t *client, unsigned ifindex, int fd)
{
    uint8_t buf[MDNS_MSG_MAX];
    ssize_t n;

    n = recv(fd, buf, sizeof buf, MSG_DONTWAIT);
    if (n <= 0) {
        return;
    }
    (void)mdns_client_handle_response(client, buf, (size_t)n, ifindex);
}

void *mdns_client_thread(void *arg)
{
    mdns_client_t *client = (mdns_client_t *)arg;
    uint64_t last_browse;
    uint64_t last_health;
    int scan_ms;

    if (!client) {
        return NULL;
    }
    if (mdns_client_open_ifaces(client) < 0) {
        log_this(SR_MDNS_CLIENT, "No multicast sockets for client", LOG_LEVEL_ALERT, 0);
        return NULL;
    }

    mdns_client_browse_all(client);
    last_browse = mdns_client_now_ms();
    last_health = last_browse;
    scan_ms = client->scan_interval_ms > 0 ? client->scan_interval_ms : 1000;

    while (!mdns_client_system_shutdown) {
        struct pollfd pfds[64];
        nfds_t np = 0;
        size_t i;
        int timeout;
        uint64_t now;

        for (i = 0; i < client->nifaces && np < 64; i++) {
            if (client->ifaces[i].sockfd_v4 >= 0) {
                pfds[np].fd = client->ifaces[i].sockfd_v4;
                pfds[np].events = POLLIN;
                pfds[np].revents = 0;
                np++;
            }
            if (client->ifaces[i].sockfd_v6 >= 0 && np < 64) {
                pfds[np].fd = client->ifaces[i].sockfd_v6;
                pfds[np].events = POLLIN;
                pfds[np].revents = 0;
                np++;
            }
        }

        timeout = scan_ms > 250 ? 250 : scan_ms;
        (void)poll(pfds, np, timeout);

        for (i = 0; i < client->nifaces; i++) {
            if (client->ifaces[i].sockfd_v4 >= 0) {
                mdns_client_recv_iface(client, client->ifaces[i].ifindex, client->ifaces[i].sockfd_v4);
            }
            if (client->ifaces[i].sockfd_v6 >= 0) {
                mdns_client_recv_iface(client, client->ifaces[i].ifindex, client->ifaces[i].sockfd_v6);
            }
        }

        mdns_client_resolve_pending(client);

        now = mdns_client_now_ms();
        if (now - last_browse >= (uint64_t)scan_ms) {
            mdns_client_browse_all(client);
            last_browse = now;
        }
        if (client->health_check_enabled && client->health_check_interval_ms > 0 &&
            now - last_health >= (uint64_t)client->health_check_interval_ms) {
            mdns_client_health_scan(client);
            last_health = now;
        }
    }

    mdns_client_close_ifaces(client);
    return NULL;
}

mdns_client_t *mdns_client_create(const MDNSClientConfig *config)
{
    mdns_client_t *client;

    if (!config) {
        return NULL;
    }
    client = calloc(1, sizeof(*client));
    if (!client) {
        return NULL;
    }
    pthread_mutex_init(&client->lock, NULL);
    client->max_services = config->max_services > 0 ? config->max_services : 100;
    client->services = calloc(client->max_services, sizeof(mdns_client_service_t));
    if (!client->services) {
        pthread_mutex_destroy(&client->lock);
        free(client);
        return NULL;
    }
    client->enable_ipv4 = config->enable_ipv4 ? 1 : 0;
    client->enable_ipv6 = config->enable_ipv6 ? 1 : 0;
    client->scan_interval_ms = config->scan_interval;
    client->health_check_enabled = config->health_check_enabled ? 1 : 0;
    client->health_check_interval_ms = config->health_check_interval;
    client->health_check_timeout_ms = config->health_check_timeout > 0 ? config->health_check_timeout : 1000;
    client->health_check_retries = config->health_check_retries > 0 ? config->health_check_retries : 3;
    client->own_services = config->own_services ? 1 : 0;
    for (size_t i = 0; i < config->num_service_types; i++) {
        if (config->service_types && config->service_types[i].type) {
            if (mdns_client_add_browse_type(client, config->service_types[i].type) < 0) {
                mdns_client_destroy(client);
                return NULL;
            }
        }
    }
    if (config->printer_services) {
        static const char *printer_types[] = {
            "_http._tcp.local",
            "_octoprint._tcp.local",
            "_hydrogen._tcp.local",
            "_ipp._tcp.local",
            "_printer._tcp.local"
        };
        size_t p;
        for (p = 0; p < sizeof printer_types / sizeof printer_types[0]; p++) {
            if (mdns_client_add_browse_type(client, printer_types[p]) < 0) {
                mdns_client_destroy(client);
                return NULL;
            }
        }
    }
    for (size_t i = 0; i < config->num_custom_services; i++) {
        if (config->custom_services && config->custom_services[i]) {
            if (mdns_client_add_browse_type(client, config->custom_services[i]) < 0) {
                mdns_client_destroy(client);
                return NULL;
            }
        }
    }
    return client;
}

void mdns_client_destroy(mdns_client_t *client)
{
    if (!client) {
        return;
    }
    mdns_client_close_ifaces(client);
    free(client->ifaces);
    if (client->browse_types) {
        for (size_t i = 0; i < client->ntypes; i++) {
            free(client->browse_types[i]);
        }
        free(client->browse_types);
    }
    free(client->services);
    pthread_mutex_destroy(&client->lock);
    free(client);
}

int mdns_client_start(const MDNSClientConfig *config)
{
    pthread_attr_t attr;

    if (mdns_client_instance) {
        return 1;
    }
    mdns_client_instance = mdns_client_create(config);
    if (!mdns_client_instance) {
        return 0;
    }
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    if (pthread_create(&mdns_client_instance->thread, &attr, mdns_client_thread,
                       mdns_client_instance) != 0) {
        pthread_attr_destroy(&attr);
        mdns_client_destroy(mdns_client_instance);
        mdns_client_instance = NULL;
        return 0;
    }
    pthread_attr_destroy(&attr);
    mdns_client_instance->thread_started = 1;
    return 1;
}

void mdns_client_stop(void)
{
    if (!mdns_client_instance) {
        return;
    }
    mdns_client_system_shutdown = 1;
    if (mdns_client_instance->thread_started) {
        pthread_join(mdns_client_instance->thread, NULL);
        mdns_client_instance->thread_started = 0;
    }
    mdns_client_destroy(mdns_client_instance);
    mdns_client_instance = NULL;
}
