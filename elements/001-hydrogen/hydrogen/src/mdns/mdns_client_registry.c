#include <src/hydrogen.h>

#include <arpa/inet.h>

#include <src/mdns/mdns_server.h>
#include <src/state/state.h>
#include "mdns_client.h"

int mdns_client_add_browse_type(mdns_client_t *client, const char *type)
{
    char **next;
    size_t i;

    if (!client || !type || type[0] == '\0') {
        return -1;
    }
    for (i = 0; i < client->ntypes; i++) {
        if (client->browse_types[i] && mdns_name_equal(client->browse_types[i], type)) {
            return 0;
        }
    }
    next = realloc(client->browse_types, (client->ntypes + 1) * sizeof(char *));
    if (!next) {
        return -1;
    }
    client->browse_types = next;
    client->browse_types[client->ntypes] = strdup(type);
    if (!client->browse_types[client->ntypes]) {
        return -1;
    }
    client->ntypes++;
    return 0;
}

int mdns_client_is_own_instance(const char *instance)
{
    size_t i;
    char name[MDNS_NAME_MAX];

    if (!instance || !mdns_server) {
        return 0;
    }
    for (i = 0; i < mdns_server->num_services; i++) {
        if (!mdns_server->services || !mdns_server->services[i].claimed) {
            continue;
        }
        mdns_server_format_instance_name(&mdns_server->services[i], name, sizeof name);
        if (mdns_name_equal(name, instance)) {
            return 1;
        }
    }
    return 0;
}

int mdns_client_service_visible(const mdns_client_t *client, const mdns_client_service_t *svc)
{
    if (!client || !svc) {
        return 0;
    }
    if (!client->own_services && mdns_client_is_own_instance(svc->instance)) {
        return 0;
    }
    return 1;
}

size_t mdns_client_count(void)
{
    size_t n = 0;
    size_t i;
    mdns_client_t *client = mdns_client_instance;

    if (!client) {
        return 0;
    }
    pthread_mutex_lock(&client->lock);
    for (i = 0; i < client->nservices; i++) {
        if (mdns_client_service_visible(client, &client->services[i])) {
            n++;
        }
    }
    pthread_mutex_unlock(&client->lock);
    return n;
}

mdns_client_service_t *mdns_client_snapshot(size_t *count)
{
    mdns_client_t *client = mdns_client_instance;
    mdns_client_service_t *copy;
    size_t n = 0;
    size_t i;

    if (count) {
        *count = 0;
    }
    if (!client) {
        return NULL;
    }
    pthread_mutex_lock(&client->lock);
    for (i = 0; i < client->nservices; i++) {
        if (mdns_client_service_visible(client, &client->services[i])) {
            n++;
        }
    }
    if (n == 0) {
        pthread_mutex_unlock(&client->lock);
        return NULL;
    }
    copy = calloc(n, sizeof(*copy));
    if (!copy) {
        pthread_mutex_unlock(&client->lock);
        return NULL;
    }
    n = 0;
    for (i = 0; i < client->nservices; i++) {
        if (mdns_client_service_visible(client, &client->services[i])) {
            copy[n++] = client->services[i];
        }
    }
    pthread_mutex_unlock(&client->lock);
    if (count) {
        *count = n;
    }
    return copy;
}

void mdns_client_snapshot_free(mdns_client_service_t *copy)
{
    free(copy);
}

size_t mdns_client_lookup_by_type(const char *type, mdns_client_service_t **out)
{
    mdns_client_service_t *snap;
    mdns_client_service_t *filtered;
    size_t n = 0;
    size_t m = 0;
    size_t i;

    if (out) {
        *out = NULL;
    }
    snap = mdns_client_snapshot(&n);
    if (!snap || !type) {
        mdns_client_snapshot_free(snap);
        return 0;
    }
    filtered = calloc(n, sizeof(*filtered));
    if (!filtered) {
        mdns_client_snapshot_free(snap);
        return 0;
    }
    for (i = 0; i < n; i++) {
        if (mdns_name_equal(snap[i].type, type)) {
            filtered[m++] = snap[i];
        }
    }
    mdns_client_snapshot_free(snap);
    if (m == 0) {
        free(filtered);
        return 0;
    }
    if (out) {
        *out = filtered;
    } else {
        free(filtered);
    }
    return m;
}

json_t *mdns_client_info_json(void)
{
    json_t *mdns;
    json_t *instances;
    json_t *claimed;
    mdns_client_service_t *snap;
    size_t n = 0;
    size_t i;
    size_t limit;
    char hostname[MDNS_NAME_MAX];

    mdns = json_object();
    if (!mdns) {
        return NULL;
    }
    claimed = json_array();
    hostname[0] = '\0';
    if (mdns_server) {
        if (mdns_server->hostname) {
            strncpy(hostname, mdns_server->hostname, sizeof hostname - 1);
        }
        for (i = 0; i < mdns_server->num_services; i++) {
            char inst[MDNS_NAME_MAX];
            if (!mdns_server->services || !mdns_server->services[i].claimed) {
                continue;
            }
            mdns_server_format_instance_name(&mdns_server->services[i], inst, sizeof inst);
            json_array_append_new(claimed, json_string(inst));
        }
    }
    json_object_set_new(mdns, "hostname", json_string(hostname));
    json_object_set_new(mdns, "claimed", claimed);

    snap = mdns_client_snapshot(&n);
    json_object_set_new(mdns, "cache_count", json_integer((json_int_t)mdns_client_count()));
    instances = json_array();
    limit = n > MDNS_CLIENT_INFO_MAX ? MDNS_CLIENT_INFO_MAX : n;
    for (i = 0; i < limit; i++) {
        json_t *svc = json_object();
        json_t *addrs = json_array();
        size_t e;
        char buf[INET6_ADDRSTRLEN];

        json_object_set_new(svc, "name", json_string(snap[i].instance));
        json_object_set_new(svc, "type", json_string(snap[i].type));
        json_object_set_new(svc, "port", json_integer(snap[i].port));
        json_object_set_new(svc, "healthy", json_boolean(snap[i].healthy != 0));
        for (e = 0; e < snap[i].nendpoints; e++) {
            const mdns_client_endpoint_t *ep = &snap[i].endpoints[e];
            if (ep->family == AF_INET) {
                inet_ntop(AF_INET, ep->addr, buf, sizeof buf);
            } else {
                inet_ntop(AF_INET6, ep->addr, buf, sizeof buf);
            }
            json_array_append_new(addrs, json_string(buf));
        }
        json_object_set_new(svc, "addrs", addrs);
        json_array_append_new(instances, svc);
    }
    mdns_client_snapshot_free(snap);
    json_object_set_new(mdns, "instances", instances);
    return mdns;
}
