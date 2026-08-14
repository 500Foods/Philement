/*
 * Webhooks Configuration (LUA_CLIENT Phase 14)
 *
 * Generic signed-webhook ingress. Each hook is a config key (not a script
 * name the caller picks). C verifies HMAC and dispatches one allowlisted
 * script. Disabled by default.
 */

#ifndef HYDROGEN_CONFIG_WEBHOOKS_H
#define HYDROGEN_CONFIG_WEBHOOKS_H

#include <stddef.h>
#include <stdbool.h>

#include <jansson.h>

#include "config_forward.h"

#define MAX_WEBHOOK_HOOKS 16

typedef struct WebhookHook {
    char *Name;
    char *SecretEnv;
    char *SignatureHeader;
    char *Hmac;
    char *Script;
} WebhookHook;

typedef struct WebhooksConfig {
    bool Enabled;
    int HookCount;
    WebhookHook Hooks[MAX_WEBHOOK_HOOKS];
} WebhooksConfig;

bool load_webhooks_config(json_t *root, AppConfig *config);
void dump_webhooks_config(const WebhooksConfig *config);
void cleanup_webhooks_config(WebhooksConfig *config);
void cleanup_webhook_hook(WebhookHook *hook);

char *webhooks_dup_json_string(json_t *obj, const char *key);
bool webhooks_names_equal(const char *a, const char *b);

const WebhookHook *webhooks_find_hook(const WebhooksConfig *config,
                                      const char *name);

#endif /* HYDROGEN_CONFIG_WEBHOOKS_H */
