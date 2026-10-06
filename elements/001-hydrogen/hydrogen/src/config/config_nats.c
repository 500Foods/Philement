/*
 * NATS Configuration Implementation
 *
 * Semantic rejects (TLS, empty servers, a bad URL, a bad delay list)
 * stay in the loaded struct. Launch readiness turns those into
 * ready = false. A missing section keeps the defaults and stays disabled.
 */

#include <src/hydrogen.h>

#include <string.h>
#include <unistd.h>

#include "config_nats.h"
#include "config_utils.h"

#include <src/nats/nats_subject.h>

void nats_config_apply_defaults(NATSConfig *config) {
    char host[256];

    if (!config) {
        return;
    }

    config->Enabled = false;
    config->LoadOk = true;
    config->ServerCount = 0;
    config->ClusterId = strdup("philement");
    host[0] = '\0';
    if (gethostname(host, sizeof(host)) != 0) {
        host[0] = '\0';
    }
    host[sizeof(host) - 1] = '\0';
    if (host[0] == '\0') {
        config->InstanceId = strdup("hydrogen");
    } else {
        config->InstanceId = strdup(host);
    }
    config->Group = NULL;
    config->Username = NULL;
    config->Password = NULL;
    config->TlsEnabled = false;
    config->TlsCaCert = NULL;
    config->ConnectionTimeoutSeconds = 10;
    config->Reconnect.MaxRetries = -1;
    config->Reconnect.Delays[0] = 30;
    config->Reconnect.Delays[1] = 60;
    config->Reconnect.Delays[2] = 120;
    config->Reconnect.Delays[3] = 240;
    config->Reconnect.Delays[4] = 480;
    config->Reconnect.DelayCount = 5;
    config->Reconnect.SteadyDelaySeconds = 480;
    config->SubscriptionCount = 0;
    config->WebSocketRelay.Enabled = false;
    config->WebSocketRelay.EventCount = 0;
    config->Presence.Enabled = false;
    config->Presence.HeartbeatIntervalSeconds = 60;
    config->Presence.StaleAfterSeconds = 0;
    config->Presence.ReportConnections = true;
    config->Test.FailNextPublishOnLaunch = false;
    config->Test.MockConnection = false;
}

bool nats_server_url_ok(const char *url) {
    const char *rest;
    const char *colon;
    char *end;
    long port;

    if (!url || strncmp(url, "nats://", 7) != 0) {
        return false;
    }
    rest = url + 7;
    if (rest[0] == '\0' || rest[0] == ':' || rest[0] == '/') {
        return false;
    }
    if (strchr(rest, '@') != NULL || strchr(rest, '/') != NULL) {
        return false;
    }
    colon = strrchr(rest, ':');
    if (colon == NULL) {
        return true;
    }
    if (colon == rest || colon[1] == '\0') {
        return false;
    }
    port = strtol(colon + 1, &end, 10);
    if (end == colon + 1 || *end != '\0') {
        return false;
    }
    if (port < 1 || port > 65535 || port == 6222) {
        return false;
    }
    return true;
}

bool nats_server_endpoint(const char *url, char *host, size_t host_cap, int *port) {
    const char *rest;
    const char *colon;
    size_t host_len;

    if (!host || host_cap < 2 || !port || !nats_server_url_ok(url)) {
        return false;
    }
    rest = url + 7;
    colon = strrchr(rest, ':');
    if (colon == NULL) {
        host_len = strlen(rest);
        *port = 4222;
    } else {
        char *end = NULL;
        long parsed;

        host_len = (size_t)(colon - rest);
        parsed = strtol(colon + 1, &end, 10);
        if (end == colon + 1 || parsed < 1 || parsed > 65535) {
            return false;
        }
        *port = (int)parsed;
    }
    if (host_len == 0 || host_len >= host_cap || *port == 6222) {
        return false;
    }
    memcpy(host, rest, host_len);
    host[host_len] = '\0';
    if (host[0] == '[') {
        size_t n = strlen(host);

        if (n < 3 || host[n - 1] != ']') {
            return false;
        }
        memmove(host, host + 1, n - 2);
        host[n - 2] = '\0';
    }
    return host[0] != '\0';
}

bool nats_reconnect_delays_ok(const NATSConfig *config) {
    size_t i;

    if (!config) {
        return false;
    }
    if (config->Reconnect.DelayCount == 0 ||
        config->Reconnect.DelayCount > NATS_MAX_DELAYS) {
        return false;
    }
    for (i = 0; i < config->Reconnect.DelayCount; i++) {
        if (config->Reconnect.Delays[i] <= 0) {
            return false;
        }
    }
    if (config->Reconnect.SteadyDelaySeconds <= 0) {
        return false;
    }
    return true;
}

bool nats_subscription_subjects_ok(const NATSConfig *config) {
    size_t i;

    if (!config || !config->ClusterId || config->ClusterId[0] == '\0') {
        return false;
    }
    if (config->SubscriptionCount > NATS_MAX_SUBSCRIPTIONS) {
        return false;
    }
    for (i = 0; i < config->SubscriptionCount; i++) {
        const NATSSubscriptionConfig *sub = &config->Subscriptions[i];
        char *built;

        if (!sub->Type) {
            return false;
        }
        if (strcmp(sub->Type, "cluster-wide") != 0 &&
            strcmp(sub->Type, "queue-group") != 0) {
            return false;
        }
        if (strcmp(sub->Type, "queue-group") == 0 &&
            (!sub->QueueGroup || sub->QueueGroup[0] == '\0')) {
            return false;
        }
        built = nats_subject_build(config->ClusterId, sub->Subject);
        if (!built) {
            return false;
        }
        free(built);
    }
    return true;
}

bool nats_config_is_usable(const NATSConfig *config) {
    size_t i;

    if (!config) {
        return false;
    }
    if (!config->Enabled) {
        return true;
    }
    if (!config->LoadOk || config->TlsEnabled) {
        return false;
    }
    if (config->ServerCount == 0 || config->ServerCount > NATS_MAX_SERVERS) {
        return false;
    }
    for (i = 0; i < config->ServerCount; i++) {
        if (!nats_server_url_ok(config->Servers[i])) {
            return false;
        }
    }
    if (!nats_reconnect_delays_ok(config)) {
        return false;
    }
    return nats_subscription_subjects_ok(config);
}

bool nats_load_delays(json_t *root, NATSConfig *config) {
    json_t *section;
    json_t *reconnect;
    json_t *delays;
    size_t i;
    size_t n;

    if (!root || !config) {
        return true;
    }
    section = json_object_get(root, "NATS");
    if (!json_is_object(section)) {
        return true;
    }
    reconnect = json_object_get(section, "Reconnect");
    if (!reconnect) {
        return true;
    }
    if (!json_is_object(reconnect)) {
        config->Reconnect.DelayCount = 0;
        return true;
    }
    delays = json_object_get(reconnect, "Delays");
    if (!delays) {
        return true;
    }
    if (!json_is_array(delays)) {
        config->Reconnect.DelayCount = 0;
        return true;
    }
    n = json_array_size(delays);
    if (n == 0 || n > NATS_MAX_DELAYS) {
        config->Reconnect.DelayCount = 0;
        return true;
    }
    for (i = 0; i < n; i++) {
        json_t *element = json_array_get(delays, i);
        if (!json_is_integer(element)) {
            config->Reconnect.DelayCount = 0;
            return true;
        }
        config->Reconnect.Delays[i] = (int)json_integer_value(element);
    }
    config->Reconnect.DelayCount = n;
    return true;
}

bool nats_load_subscriptions(json_t *root, NATSConfig *config) {
    json_t *section;
    json_t *subs;
    size_t i;
    size_t n;

    if (!root || !config) {
        return true;
    }
    section = json_object_get(root, "NATS");
    if (!json_is_object(section)) {
        return true;
    }
    subs = json_object_get(section, "Subscriptions");
    if (!subs) {
        return true;
    }
    if (!json_is_array(subs)) {
        config->LoadOk = false;
        return true;
    }
    n = json_array_size(subs);
    if (n > NATS_MAX_SUBSCRIPTIONS) {
        config->LoadOk = false;
        return true;
    }
    for (i = 0; i < n; i++) {
        json_t *obj = json_array_get(subs, i);
        json_t *subject;
        json_t *type;
        json_t *queue;
        const char *type_text;

        if (!json_is_object(obj)) {
            config->LoadOk = false;
            return true;
        }
        subject = json_object_get(obj, "Subject");
        type = json_object_get(obj, "Type");
        queue = json_object_get(obj, "QueueGroup");
        if ((subject && !json_is_string(subject)) ||
            (type && !json_is_string(type)) ||
            (queue && !json_is_string(queue))) {
            config->LoadOk = false;
            return true;
        }
        if (json_is_string(subject)) {
            config->Subscriptions[i].Subject = strdup(json_string_value(subject));
            if (!config->Subscriptions[i].Subject) {
                return false;
            }
        }
        type_text = json_is_string(type) ? json_string_value(type) : "cluster-wide";
        if (!type_text) {
            type_text = "cluster-wide";
        }
        config->Subscriptions[i].Type = strdup(type_text);
        if (!config->Subscriptions[i].Type) {
            return false;
        }
        if (json_is_string(queue) && json_string_value(queue) &&
            json_string_value(queue)[0] != '\0') {
            config->Subscriptions[i].QueueGroup = strdup(json_string_value(queue));
            if (!config->Subscriptions[i].QueueGroup) {
                return false;
            }
        }
    }
    config->SubscriptionCount = n;
    return true;
}

bool load_nats_config(json_t *root, AppConfig *config) {
    bool success = true;
    NATSConfig *nats;
    json_t *section;

    if (!config) {
        return false;
    }

    nats = &config->nats;
    cleanup_nats_config(nats);
    nats_config_apply_defaults(nats);

    success = PROCESS_SECTION(root, "NATS");
    success = success && PROCESS_BOOL(root, nats, Enabled, "NATS.Enabled", "NATS");
    success = success && process_string_array_config(root,
        (ConfigStringArray){
            .array = nats->Servers,
            .count = &nats->ServerCount,
            .capacity = NATS_MAX_SERVERS
        }, "NATS.Servers", "NATS");
    success = success && PROCESS_STRING(root, nats, ClusterId, "NATS.ClusterId", "NATS");
    success = success && PROCESS_STRING(root, nats, InstanceId, "NATS.InstanceId", "NATS");
    success = success && PROCESS_STRING(root, nats, Group, "NATS.Group", "NATS");
    success = success && PROCESS_STRING(root, nats, Username, "NATS.Username", "NATS");
    success = success && PROCESS_SENSITIVE(root, nats, Password, "NATS.Password", "NATS");
    success = success && PROCESS_BOOL(root, nats, TlsEnabled, "NATS.TlsEnabled", "NATS");
    success = success && PROCESS_STRING(root, nats, TlsCaCert, "NATS.TlsCaCert", "NATS");
    success = success && PROCESS_INT(root, nats, ConnectionTimeoutSeconds,
                                      "NATS.ConnectionTimeoutSeconds", "NATS");
    success = success && PROCESS_INT(root, &nats->Reconnect, MaxRetries,
                                      "NATS.Reconnect.MaxRetries", "NATS");
    success = success && PROCESS_INT(root, &nats->Reconnect, SteadyDelaySeconds,
                                      "NATS.Reconnect.SteadyDelaySeconds", "NATS");
    success = success && nats_load_delays(root, nats);
    success = success && nats_load_subscriptions(root, nats);
    success = success && PROCESS_BOOL(root, &nats->WebSocketRelay, Enabled,
                                       "NATS.WebSocketRelay.Enabled", "NATS");
    success = success && process_string_array_config(root,
        (ConfigStringArray){
            .array = nats->WebSocketRelay.Events,
            .count = &nats->WebSocketRelay.EventCount,
            .capacity = NATS_MAX_RELAY_EVENTS
        }, "NATS.WebSocketRelay.Events", "NATS");
    success = success && PROCESS_BOOL(root, &nats->Presence, Enabled,
                                       "NATS.Presence.Enabled", "NATS");
    success = success && PROCESS_INT(root, &nats->Presence, HeartbeatIntervalSeconds,
                                      "NATS.Presence.HeartbeatIntervalSeconds", "NATS");
    success = success && PROCESS_INT(root, &nats->Presence, StaleAfterSeconds,
                                      "NATS.Presence.StaleAfterSeconds", "NATS");
    success = success && PROCESS_BOOL(root, &nats->Presence, ReportConnections,
                                       "NATS.Presence.ReportConnections", "NATS");
    success = success && PROCESS_BOOL(root, &nats->Test, FailNextPublishOnLaunch,
                                       "NATS.Test.FailNextPublishOnLaunch", "NATS");
    success = success && PROCESS_BOOL(root, &nats->Test, MockConnection,
                                       "NATS.Test.MockConnection", "NATS");

    section = root ? json_object_get(root, "NATS") : NULL;
    if (json_is_object(section)) {
        json_t *servers = json_object_get(section, "Servers");
        json_t *events = json_object_get(section, "WebSocketRelay");
        if (json_is_array(servers) && json_array_size(servers) > NATS_MAX_SERVERS) {
            nats->LoadOk = false;
        }
        if (json_is_object(events)) {
            json_t *event_list = json_object_get(events, "Events");
            if (json_is_array(event_list) &&
                json_array_size(event_list) > NATS_MAX_RELAY_EVENTS) {
                nats->LoadOk = false;
            }
        }
    }

    if (success) {
        log_this(SR_CONFIG, "― NATS configuration loaded successfully", LOG_LEVEL_DEBUG, 0);
    }
    return success;
}

void dump_nats_config(const NATSConfig *config) {
    size_t i;

    if (!config) {
        return;
    }

    log_this(SR_CONFIG_CURRENT, "NATS Configuration:", LOG_LEVEL_DEBUG, 0);
    log_this(SR_CONFIG_CURRENT, "  Enabled: %s", LOG_LEVEL_DEBUG, 1,
             config->Enabled ? "true" : "false");
    log_this(SR_CONFIG_CURRENT, "  ServerCount: %zu", LOG_LEVEL_DEBUG, 1, config->ServerCount);
    for (i = 0; i < config->ServerCount && i < NATS_MAX_SERVERS; i++) {
        log_this(SR_CONFIG_CURRENT, "  Server: %s", LOG_LEVEL_DEBUG, 1,
                 config->Servers[i] ? config->Servers[i] : "(null)");
    }
    log_this(SR_CONFIG_CURRENT, "  ClusterId: %s", LOG_LEVEL_DEBUG, 1,
             config->ClusterId ? config->ClusterId : "(null)");
    log_this(SR_CONFIG_CURRENT, "  InstanceId: %s", LOG_LEVEL_DEBUG, 1,
             config->InstanceId ? config->InstanceId : "(null)");
    log_this(SR_CONFIG_CURRENT, "  Group: %s", LOG_LEVEL_DEBUG, 1,
             config->Group ? config->Group : "(null)");
    log_this(SR_CONFIG_CURRENT, "  Username: %s", LOG_LEVEL_DEBUG, 1,
             config->Username ? config->Username : "(null)");
    log_this(SR_CONFIG_CURRENT, "  Password: %s", LOG_LEVEL_DEBUG, 1,
             config->Password ? "*****" : "(not set)");
    log_this(SR_CONFIG_CURRENT, "  TlsEnabled: %s", LOG_LEVEL_DEBUG, 1,
             config->TlsEnabled ? "true" : "false");
    log_this(SR_CONFIG_CURRENT, "  ConnectionTimeoutSeconds: %d", LOG_LEVEL_DEBUG, 1,
             config->ConnectionTimeoutSeconds);
    log_this(SR_CONFIG_CURRENT, "  Reconnect.MaxRetries: %d", LOG_LEVEL_DEBUG, 1,
             config->Reconnect.MaxRetries);
    log_this(SR_CONFIG_CURRENT, "  Reconnect.DelayCount: %zu", LOG_LEVEL_DEBUG, 1,
             config->Reconnect.DelayCount);
    log_this(SR_CONFIG_CURRENT, "  Reconnect.SteadyDelaySeconds: %d", LOG_LEVEL_DEBUG, 1,
             config->Reconnect.SteadyDelaySeconds);
    log_this(SR_CONFIG_CURRENT, "  SubscriptionCount: %zu", LOG_LEVEL_DEBUG, 1,
             config->SubscriptionCount);
    for (i = 0; i < config->SubscriptionCount && i < NATS_MAX_SUBSCRIPTIONS; i++) {
        log_this(SR_CONFIG_CURRENT, "  Subscription: %s", LOG_LEVEL_DEBUG, 1,
                 config->Subscriptions[i].Subject ? config->Subscriptions[i].Subject : "(null)");
    }
    log_this(SR_CONFIG_CURRENT, "  WebSocketRelay.Enabled: %s", LOG_LEVEL_DEBUG, 1,
             config->WebSocketRelay.Enabled ? "true" : "false");
    log_this(SR_CONFIG_CURRENT, "  Presence.Enabled: %s", LOG_LEVEL_DEBUG, 1,
             config->Presence.Enabled ? "true" : "false");
    log_this(SR_CONFIG_CURRENT, "  Presence.StaleAfterSeconds: %d", LOG_LEVEL_DEBUG, 1,
             config->Presence.StaleAfterSeconds);
}

void cleanup_nats_config(NATSConfig *config) {
    size_t i;

    if (!config) {
        return;
    }

    for (i = 0; i < NATS_MAX_SERVERS; i++) {
        free(config->Servers[i]);
        config->Servers[i] = NULL;
    }
    free(config->ClusterId);
    free(config->InstanceId);
    free(config->Group);
    free(config->Username);
    free(config->Password);
    free(config->TlsCaCert);
    for (i = 0; i < NATS_MAX_SUBSCRIPTIONS; i++) {
        free(config->Subscriptions[i].Subject);
        free(config->Subscriptions[i].Type);
        free(config->Subscriptions[i].QueueGroup);
        config->Subscriptions[i].Subject = NULL;
        config->Subscriptions[i].Type = NULL;
        config->Subscriptions[i].QueueGroup = NULL;
    }
    for (i = 0; i < NATS_MAX_RELAY_EVENTS; i++) {
        free(config->WebSocketRelay.Events[i]);
        config->WebSocketRelay.Events[i] = NULL;
    }
    memset(config, 0, sizeof(NATSConfig));
}
