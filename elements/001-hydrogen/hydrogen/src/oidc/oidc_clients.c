/*
 * OpenID Connect (OIDC) Client Registry Implementation
 */

#include <src/hydrogen.h>

#include "oidc_clients.h"

#include <src/utils/utils_crypto.h>
#include <src/logging/logging.h>

#include <string.h>
#include <openssl/crypto.h>
#include <jansson.h>

OIDCClientContext* init_oidc_client_registry(void);
void cleanup_oidc_client_registry(OIDCClientContext *client_context);
void oidc_client_free(OIDCClient *client);
OIDCClient* oidc_client_create(const char *client_id,
                               const char *client_secret_hash_or_null,
                               const char *client_name,
                               bool confidential,
                               bool require_pkce,
                               const char *redirect_uris_json,
                               const char *grant_types,
                               const char *response_types);
bool oidc_client_registry_add(OIDCClientContext *ctx, OIDCClient *client);
OIDCClient* oidc_client_registry_find(const OIDCClientContext *ctx, const char *client_id);
bool oidc_redirect_uri_allowed(const char *redirect_uris_json, const char *redirect_uri);
char* oidc_hash_client_secret(const char *client_secret);
bool oidc_client_secrets_equal(const char *stored_hash, const char *presented_secret);
bool oidc_validate_client(const OIDCClientContext *client_context,
                          const char *client_id,
                          const char *redirect_uri);
bool oidc_authenticate_client(const OIDCClientContext *client_context,
                              const char *client_id,
                              const char *client_secret);
bool oidc_register_client(OIDCClientContext *client_context,
                          const char *client_name,
                          const char *redirect_uri,
                          bool confidential,
                          char **client_id_out,
                          char **client_secret_out);

void oidc_client_free(OIDCClient *client) {
    free(client);
}

OIDCClient* oidc_client_create(const char *client_id,
                               const char *client_secret_hash_or_null,
                               const char *client_name,
                               bool confidential,
                               bool require_pkce,
                               const char *redirect_uris_json,
                               const char *grant_types,
                               const char *response_types) {
    if (!client_id || client_id[0] == '\0' || !client_name || !redirect_uris_json ||
        !grant_types || !response_types) {
        return NULL;
    }

    OIDCClient *c = (OIDCClient*)calloc(1, sizeof(OIDCClient));
    if (!c) {
        return NULL;
    }

    snprintf(c->client_id, sizeof(c->client_id), "%s", client_id);
    snprintf(c->client_name, sizeof(c->client_name), "%s", client_name);
    snprintf(c->redirect_uris, sizeof(c->redirect_uris), "%s", redirect_uris_json);
    snprintf(c->grant_types, sizeof(c->grant_types), "%s", grant_types);
    snprintf(c->response_types, sizeof(c->response_types), "%s", response_types);
    c->is_confidential = confidential;
    c->require_pkce = require_pkce || !confidential;
    c->active = true;

    if (client_secret_hash_or_null && client_secret_hash_or_null[0] != '\0') {
        snprintf(c->client_secret_hash, sizeof(c->client_secret_hash), "%s",
                 client_secret_hash_or_null);
    }

    return c;
}

bool oidc_client_registry_add(OIDCClientContext *ctx, OIDCClient *client) {
    if (!ctx || !client) {
        return false;
    }
    if (oidc_client_registry_find(ctx, client->client_id) != NULL) {
        return false;
    }

    if (ctx->client_count >= ctx->client_capacity) {
        size_t new_cap = ctx->client_capacity == 0U ? 4U : ctx->client_capacity * 2U;
        OIDCClient **grown = (OIDCClient**)realloc(ctx->clients, new_cap * sizeof(OIDCClient*));
        if (!grown) {
            return false;
        }
        ctx->clients = grown;
        ctx->client_capacity = new_cap;
    }

    ctx->clients[ctx->client_count] = client;
    ctx->client_count++;
    return true;
}

OIDCClient* oidc_client_registry_find(const OIDCClientContext *ctx, const char *client_id) {
    if (!ctx || !client_id || !ctx->clients) {
        return NULL;
    }
    for (size_t i = 0; i < ctx->client_count; i++) {
        OIDCClient *c = ctx->clients[i];
        if (c && c->active && strcmp(c->client_id, client_id) == 0) {
            return c;
        }
    }
    return NULL;
}

bool oidc_redirect_uri_allowed(const char *redirect_uris_json, const char *redirect_uri) {
    if (!redirect_uris_json || !redirect_uri || redirect_uri[0] == '\0') {
        return false;
    }

    json_error_t err;
    json_t *arr = json_loads(redirect_uris_json, 0, &err);
    if (!arr || !json_is_array(arr)) {
        if (arr) {
            json_decref(arr);
        }
        return false;
    }

    bool allowed = false;
    size_t n = json_array_size(arr);
    for (size_t i = 0; i < n; i++) {
        json_t *el = json_array_get(arr, i);
        if (json_is_string(el) && strcmp(json_string_value(el), redirect_uri) == 0) {
            allowed = true;
            break;
        }
    }
    json_decref(arr);
    return allowed;
}

char* oidc_hash_client_secret(const char *client_secret) {
    if (!client_secret || client_secret[0] == '\0') {
        return NULL;
    }
    return utils_sha256_hash((const unsigned char*)client_secret, strlen(client_secret));
}

bool oidc_client_secrets_equal(const char *stored_hash, const char *presented_secret) {
    if (!stored_hash || stored_hash[0] == '\0' || !presented_secret) {
        return false;
    }
    char *presented_hash = oidc_hash_client_secret(presented_secret);
    if (!presented_hash) {
        return false;
    }

    size_t a_len = strlen(stored_hash);
    size_t b_len = strlen(presented_hash);
    bool match = false;
    if (a_len == b_len && a_len > 0U) {
        match = (CRYPTO_memcmp(stored_hash, presented_hash, a_len) == 0);
    }
    free(presented_hash);
    return match;
}

OIDCClientContext* init_oidc_client_registry(void) {
    log_this(SR_OIDC, "Initializing client registry", LOG_LEVEL_STATE, 0);

    OIDCClientContext *context = (OIDCClientContext *)calloc(1, sizeof(OIDCClientContext));
    if (!context) {
        log_this(SR_OIDC, "Failed to allocate client registry context", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    context->initialized = true;
    context->client_count = 0;
    context->client_capacity = 0;
    context->clients = NULL;
    context->database_name = NULL;

    log_this(SR_OIDC, "Client registry initialized successfully", LOG_LEVEL_STATE, 0);
    return context;
}

void cleanup_oidc_client_registry(OIDCClientContext *client_context) {
    if (!client_context) {
        return;
    }

    log_this(SR_OIDC, "Cleaning up client registry", LOG_LEVEL_STATE, 0);

    if (client_context->clients) {
        for (size_t i = 0; i < client_context->client_count; i++) {
            oidc_client_free(client_context->clients[i]);
            client_context->clients[i] = NULL;
        }
        free(client_context->clients);
        client_context->clients = NULL;
    }
    free(client_context->database_name);
    client_context->database_name = NULL;
    free(client_context);

    log_this(SR_OIDC, "Client registry cleanup complete", LOG_LEVEL_STATE, 0);
}

bool oidc_validate_client(const OIDCClientContext *client_context,
                          const char *client_id,
                          const char *redirect_uri) {
    if (!client_context || !client_id || !redirect_uri) {
        return false;
    }

    log_this(SR_OIDC, "Validating client", LOG_LEVEL_DEBUG, 0);

    OIDCClient *client = oidc_client_registry_find(client_context, client_id);
    if (!client) {
        return false;
    }
    return oidc_redirect_uri_allowed(client->redirect_uris, redirect_uri);
}

bool oidc_authenticate_client(const OIDCClientContext *client_context,
                              const char *client_id,
                              const char *client_secret) {
    if (!client_context || !client_id) {
        return false;
    }

    log_this(SR_OIDC, "Authenticating client", LOG_LEVEL_DEBUG, 0);

    OIDCClient *client = oidc_client_registry_find(client_context, client_id);
    if (!client) {
        return false;
    }

    if (!client->is_confidential) {
        /* Public clients do not use a secret. */
        return client_secret == NULL || client_secret[0] == '\0';
    }

    if (!client_secret || client_secret[0] == '\0') {
        return false;
    }
    return oidc_client_secrets_equal(client->client_secret_hash, client_secret);
}

bool oidc_seed_client_from_config(OIDCClientContext *client_context,
                                  const char *client_id,
                                  const char *client_secret_or_null,
                                  const char *redirect_uri) {
    if (!client_context || !client_id || client_id[0] == '\0') {
        return true;
    }
    if (!redirect_uri || redirect_uri[0] == '\0') {
        log_this(SR_OIDC, "OIDC config ClientId set without RedirectUri — skip seed",
                 LOG_LEVEL_ALERT, 0);
        return true;
    }
    if (oidc_client_registry_find(client_context, client_id) != NULL) {
        return true;
    }

    char *uris_json = NULL;
    if (asprintf(&uris_json, "[\"%s\"]", redirect_uri) < 0 || !uris_json) {
        return false;
    }

    bool confidential = (client_secret_or_null && client_secret_or_null[0] != '\0');
    char *secret_hash = NULL;
    if (confidential) {
        secret_hash = oidc_hash_client_secret(client_secret_or_null);
        if (!secret_hash) {
            free(uris_json);
            return false;
        }
    }

    OIDCClient *client = oidc_client_create(
        client_id, secret_hash, "config-seed",
        confidential, true, uris_json,
        "authorization_code refresh_token", "code");
    free(secret_hash);
    free(uris_json);
    if (!client) {
        return false;
    }
    if (!oidc_client_registry_add(client_context, client)) {
        oidc_client_free(client);
        return false;
    }
    log_this(SR_OIDC, "Seeded OIDC client from config", LOG_LEVEL_STATE, 0);
    return true;
}

bool oidc_register_client(OIDCClientContext *client_context,
                          const char *client_name,
                          const char *redirect_uri,
                          bool confidential,
                          char **client_id_out,
                          char **client_secret_out) {
    if (!client_context || !client_name || !redirect_uri || !client_id_out || !client_secret_out) {
        return false;
    }

    log_this(SR_OIDC, "Registering new client", LOG_LEVEL_STATE, 0);

    unsigned char id_raw[16];
    if (!utils_random_bytes(id_raw, sizeof(id_raw))) {
        return false;
    }
    char *client_id = utils_base64url_encode(id_raw, sizeof(id_raw));
    if (!client_id) {
        return false;
    }

    char *secret = NULL;
    char *secret_hash = NULL;
    if (confidential) {
        unsigned char sec_raw[32];
        if (!utils_random_bytes(sec_raw, sizeof(sec_raw))) {
            free(client_id);
            return false;
        }
        secret = utils_base64url_encode(sec_raw, sizeof(sec_raw));
        if (!secret) {
            free(client_id);
            return false;
        }
        secret_hash = oidc_hash_client_secret(secret);
        if (!secret_hash) {
            free(client_id);
            free(secret);
            return false;
        }
    }

    char redirects[OIDC_CLIENT_REDIRECTS_MAX];
    json_t *arr = json_array();
    if (!arr) {
        free(client_id);
        free(secret);
        free(secret_hash);
        return false;
    }
    json_array_append_new(arr, json_string(redirect_uri));
    char *redirects_json = json_dumps(arr, JSON_COMPACT);
    json_decref(arr);
    if (!redirects_json) {
        free(client_id);
        free(secret);
        free(secret_hash);
        return false;
    }
    snprintf(redirects, sizeof(redirects), "%s", redirects_json);
    free(redirects_json);

    OIDCClient *client = oidc_client_create(client_id, secret_hash, client_name,
                                            confidential, !confidential,
                                            redirects,
                                            "authorization_code refresh_token",
                                            "code");
    free(secret_hash);
    if (!client || !oidc_client_registry_add(client_context, client)) {
        oidc_client_free(client);
        free(client_id);
        free(secret);
        return false;
    }

    *client_id_out = client_id;
    *client_secret_out = secret;
    return true;
}
