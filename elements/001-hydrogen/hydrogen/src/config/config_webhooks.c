/*
 * Webhooks Configuration Implementation
 */

#include <src/hydrogen.h>

#include "config_webhooks.h"
#include "config_utils.h"

void cleanup_webhook_hook(WebhookHook *hook) {
    if (!hook) {
        return;
    }
    free(hook->Name);
    free(hook->SecretEnv);
    free(hook->SignatureHeader);
    free(hook->Hmac);
    free(hook->Script);
    memset(hook, 0, sizeof(*hook));
}

char *webhooks_dup_json_string(json_t *obj, const char *key) {
    json_t *val;
    const char *s;

    if (!obj || !key) {
        return NULL;
    }
    val = json_object_get(obj, key);
    if (!val || !json_is_string(val)) {
        return NULL;
    }
    s = json_string_value(val);
    if (!s) {
        return NULL;
    }
    return strdup(s);
}

bool webhooks_names_equal(const char *a, const char *b) {
    if (!a || !b) {
        return false;
    }
    while (*a && *b) {
        unsigned char ca = (unsigned char)*a;
        unsigned char cb = (unsigned char)*b;
        if (tolower(ca) != tolower(cb)) {
            return false;
        }
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
}

const WebhookHook *webhooks_find_hook(const WebhooksConfig *config,
                                      const char *name) {
    int i;

    if (!config || !name || !name[0]) {
        return NULL;
    }
    for (i = 0; i < config->HookCount; i++) {
        if (webhooks_names_equal(config->Hooks[i].Name, name)) {
            return &config->Hooks[i];
        }
    }
    return NULL;
}

bool load_webhooks_config(json_t *root, AppConfig *config) {
    WebhooksConfig *wh;
    json_t *section;
    json_t *hooks;
    size_t n;
    size_t i;

    if (!config) {
        log_this(SR_CONFIG, "Webhooks: cannot load into NULL config",
                 LOG_LEVEL_ERROR, 0);
        return false;
    }

    wh = &config->webhooks;
    cleanup_webhooks_config(wh);
    wh->Enabled = false;
    wh->HookCount = 0;

    if (!root || !json_is_object(root)) {
        return true;
    }

    section = json_object_get(root, "Webhooks");
    if (!section || !json_is_object(section)) {
        return true;
    }

    PROCESS_SECTION(root, "Webhooks");
    PROCESS_BOOL(root, wh, Enabled, "Webhooks.Enabled", "Webhooks");

    hooks = json_object_get(section, "Hooks");
    if (!hooks || !json_is_array(hooks)) {
        return true;
    }

    n = json_array_size(hooks);
    if (n > MAX_WEBHOOK_HOOKS) {
        log_this(SR_CONFIG,
                 "Webhooks: %zu hooks configured, truncating to %d",
                 LOG_LEVEL_ALERT, 2, n, MAX_WEBHOOK_HOOKS);
        n = MAX_WEBHOOK_HOOKS;
    }

    for (i = 0; i < n; i++) {
        json_t *hobj = json_array_get(hooks, i);
        WebhookHook *hook;

        if (!hobj || !json_is_object(hobj)) {
            continue;
        }
        hook = &wh->Hooks[wh->HookCount];
        memset(hook, 0, sizeof(*hook));
        hook->Name = webhooks_dup_json_string(hobj, "Name");
        hook->SecretEnv = webhooks_dup_json_string(hobj, "SecretEnv");
        hook->SignatureHeader = webhooks_dup_json_string(hobj, "SignatureHeader");
        hook->Hmac = webhooks_dup_json_string(hobj, "Hmac");
        hook->Script = webhooks_dup_json_string(hobj, "Script");
        if (!hook->Hmac) {
            hook->Hmac = strdup("sha256");
        }
        if (!hook->Name || !hook->Name[0] || !hook->Script || !hook->Script[0]) {
            cleanup_webhook_hook(hook);
            continue;
        }
        wh->HookCount++;
    }

    return true;
}

void cleanup_webhooks_config(WebhooksConfig *config) {
    int i;

    if (!config) {
        return;
    }
    for (i = 0; i < config->HookCount; i++) {
        cleanup_webhook_hook(&config->Hooks[i]);
    }
    memset(config, 0, sizeof(*config));
}

void dump_webhooks_config(const WebhooksConfig *config) {
    int i;

    if (!config) {
        DUMP_TEXT("", "Cannot dump NULL webhooks config");
        return;
    }

    DUMP_BOOL("Enabled", config->Enabled);
    DUMP_INT("Hook Count", config->HookCount);
    for (i = 0; i < config->HookCount; i++) {
        const WebhookHook *hook = &config->Hooks[i];
        DUMP_STRING("―― Name", hook->Name);
        DUMP_STRING("―― SecretEnv", hook->SecretEnv);
        DUMP_STRING("―― SignatureHeader", hook->SignatureHeader);
        DUMP_STRING("―― Hmac", hook->Hmac);
        DUMP_STRING("―― Script", hook->Script);
    }
}
