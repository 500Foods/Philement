/*
 * NATS Configuration
 *
 * JSON section "NATS" (AppConfig letter V). Disabled by default.
 * Subject values are suffixes. Code builds cluster.<ClusterId>.<suffix>.
 */

#ifndef HYDROGEN_CONFIG_NATS_H
#define HYDROGEN_CONFIG_NATS_H

#include <src/globals.h>

#include <stddef.h>
#include <stdbool.h>

#include <jansson.h>

#include "config_forward.h"

#define NATS_MAX_SERVERS 8
#define NATS_MAX_DELAYS 8
#define NATS_MAX_SUBSCRIPTIONS 16
#define NATS_MAX_RELAY_EVENTS 64

typedef struct NATSSubscriptionConfig {
    char *Subject;
    char *Type;
    char *QueueGroup;
} NATSSubscriptionConfig;

typedef struct NATSReconnectConfig {
    int MaxRetries;
    int Delays[NATS_MAX_DELAYS];
    size_t DelayCount;
    int SteadyDelaySeconds;
} NATSReconnectConfig;

typedef struct NATSWebSocketRelayConfig {
    bool Enabled;
    char *Events[NATS_MAX_RELAY_EVENTS];
    size_t EventCount;
} NATSWebSocketRelayConfig;

typedef struct NATSPresenceConfig {
    bool Enabled;
    int HeartbeatIntervalSeconds;
    int StaleAfterSeconds;
    bool ReportConnections;
} NATSPresenceConfig;

typedef struct NATSTestConfig {
    bool FailNextPublishOnLaunch;
    bool MockConnection;
} NATSTestConfig;

typedef struct NATSConfig {
    bool Enabled;
    bool LoadOk;
    char *Servers[NATS_MAX_SERVERS];
    size_t ServerCount;
    char *ClusterId;
    char *InstanceId;
    char *Group;
    char *Username;
    char *Password;
    bool TlsEnabled;
    char *TlsCaCert;
    int ConnectionTimeoutSeconds;
    NATSReconnectConfig Reconnect;
    NATSSubscriptionConfig Subscriptions[NATS_MAX_SUBSCRIPTIONS];
    size_t SubscriptionCount;
    NATSWebSocketRelayConfig WebSocketRelay;
    NATSPresenceConfig Presence;
    NATSTestConfig Test;
} NATSConfig;

bool load_nats_config(json_t *root, AppConfig *config);
void dump_nats_config(const NATSConfig *config);
void cleanup_nats_config(NATSConfig *config);
void nats_config_apply_defaults(NATSConfig *config);

/* Structural loaders. A false return is allocation failure. */
bool nats_load_delays(json_t *root, NATSConfig *config);
bool nats_load_subscriptions(json_t *root, NATSConfig *config);

/* Readiness predicates. Disabled config is usable. */
bool nats_server_url_ok(const char *url);
bool nats_server_endpoint(const char *url, char *host, size_t host_cap, int *port);
bool nats_reconnect_delays_ok(const NATSConfig *config);
bool nats_subscription_subjects_ok(const NATSConfig *config);
bool nats_config_is_usable(const NATSConfig *config);

#endif /* HYDROGEN_CONFIG_NATS_H */
