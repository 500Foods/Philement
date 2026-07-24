/*
 * OIDC token introspection (RFC 7662)
 *
 * Authenticated clients may inspect access JWTs or refresh store records.
 * Unknown / wrong-client tokens return {"active":false} without leaking detail.
 */

// Global includes
#include <src/hydrogen.h>

// Local includes
#include "oidc_service.h"
#include "oidc_tokens.h"
#include "oidc_refresh_tokens.h"

#include <jansson.h>

/*
 * Inactive introspection body (RFC 7662). Caller frees.
 */
char* oidc_inactive_json(void) {
    return strdup("{\"active\":false}");
}

/*
 * Build active access_token introspection JSON from validated claims.
 * Enforces client binding via client_id claim or aud[0].
 */
char* oidc_introspect_access_json(const OIDCTokenClaims *claims, const char *client_id) {
    if (!claims || !claims->sub) {
        return oidc_inactive_json();
    }
    /* Token must be bound to this client when client_id claim present. */
    if (claims->client_id && claims->client_id[0] != '\0' && client_id &&
        strcmp(claims->client_id, client_id) != 0) {
        return oidc_inactive_json();
    }
    if (claims->aud_count > 0U && claims->aud && claims->aud[0] && client_id &&
        strcmp(claims->aud[0], client_id) != 0 &&
        (!claims->client_id || claims->client_id[0] == '\0')) {
        return oidc_inactive_json();
    }

    json_t *out = json_object();
    if (!out) {
        return NULL;
    }
    json_object_set_new(out, "active", json_true());
    json_object_set_new(out, "token_type", json_string("access_token"));
    json_object_set_new(out, "sub", json_string(claims->sub));
    if (claims->client_id && claims->client_id[0] != '\0') {
        json_object_set_new(out, "client_id", json_string(claims->client_id));
    } else if (client_id) {
        json_object_set_new(out, "client_id", json_string(client_id));
    }
    if (claims->scope && claims->scope[0] != '\0') {
        json_object_set_new(out, "scope", json_string(claims->scope));
    }
    if (claims->exp > 0) {
        json_object_set_new(out, "exp", json_integer((json_int_t)claims->exp));
    }
    if (claims->iat > 0) {
        json_object_set_new(out, "iat", json_integer((json_int_t)claims->iat));
    }
    if (claims->iss && claims->iss[0] != '\0') {
        json_object_set_new(out, "iss", json_string(claims->iss));
    }
    char *json = json_dumps(out, JSON_COMPACT);
    json_decref(out);
    return json;
}

/*
 * Build active refresh_token introspection JSON from a store record.
 */
char* oidc_introspect_refresh_json(const OIDCRefreshRecord *rec, const char *client_id) {
    if (!rec || !client_id) {
        return oidc_inactive_json();
    }
    if (strcmp(rec->client_id, client_id) != 0) {
        return oidc_inactive_json();
    }
    time_t now = time(NULL);
    if (rec->revoked_at != 0 || rec->expires_at < now) {
        return oidc_inactive_json();
    }

    json_t *out = json_object();
    if (!out) {
        return NULL;
    }
    char sub_buf[32];
    snprintf(sub_buf, sizeof(sub_buf), "%d", rec->account_id);
    json_object_set_new(out, "active", json_true());
    json_object_set_new(out, "token_type", json_string("refresh_token"));
    json_object_set_new(out, "client_id", json_string(rec->client_id));
    json_object_set_new(out, "sub", json_string(sub_buf));
    if (rec->scope[0] != '\0') {
        json_object_set_new(out, "scope", json_string(rec->scope));
    }
    json_object_set_new(out, "exp", json_integer((json_int_t)rec->expires_at));
    char *json = json_dumps(out, JSON_COMPACT);
    json_decref(out);
    return json;
}

/*
 * Try access JWT validation → active JSON, else NULL (not found / invalid).
 */
char* oidc_introspect_try_access(const OIDCContext *ctx, const char *token, const char *client_id) {
    if (!ctx || !ctx->token_context || !token) {
        return NULL;
    }
    OIDCTokenClaims *claims = NULL;
    if (!oidc_validate_access_token((OIDCTokenContext*)ctx->token_context,
                                    token, &claims) || !claims) {
        return NULL;
    }
    char *json = oidc_introspect_access_json(claims, client_id);
    oidc_free_token_claims(claims);
    return json;
}

/*
 * Try refresh store lookup → introspection JSON, else NULL.
 */
char* oidc_introspect_try_refresh(OIDCContext *ctx, const char *token, const char *client_id) {
    if (!ctx || !ctx->refresh_store || !token) {
        return NULL;
    }
    OIDCRefreshRecord rec;
    memset(&rec, 0, sizeof(rec));
    if (!oidc_refresh_lookup(ctx->refresh_store, token, &rec)) {
        return NULL;
    }
    return oidc_introspect_refresh_json(&rec, client_id);
}

/*
 * Process a token introspection request (RFC 7662).
 * token_type_hint is advisory: preferred type is tried first, then the other.
 */
char* oidc_process_introspection_request(const char *token, const char *token_type_hint,
                                        const char *client_id, const char *client_secret) {
    OIDCContext *ctx = get_oidc_context();
    if (!ctx || !ctx->initialized) {
        log_this(SR_OIDC, "OIDC service not initialized", LOG_LEVEL_ERROR, 0);
        return NULL;
    }
    if (!client_id || client_id[0] == '\0') {
        return NULL;
    }
    if (!token || token[0] == '\0') {
        return oidc_inactive_json();
    }

    OIDCClientContext *clients = (OIDCClientContext*)ctx->client_context;
    if (!oidc_authenticate_client(clients, client_id, client_secret)) {
        log_this(SR_OIDC, "Introspection client authentication failed", LOG_LEVEL_DEBUG, 0);
        return NULL;
    }

    log_this(SR_OIDC, "Processing introspection request", LOG_LEVEL_STATE, 0);

    bool prefer_access = true;
    bool prefer_refresh = true;
    if (token_type_hint && token_type_hint[0] != '\0') {
        if (strcmp(token_type_hint, "access_token") == 0) {
            prefer_refresh = false;
        } else if (strcmp(token_type_hint, "refresh_token") == 0) {
            prefer_access = false;
        }
    }

    if (prefer_access) {
        char *json = oidc_introspect_try_access(ctx, token, client_id);
        if (json) {
            return json;
        }
    }
    if (prefer_refresh) {
        char *json = oidc_introspect_try_refresh(ctx, token, client_id);
        if (json) {
            return json;
        }
    }
    if (!prefer_access) {
        char *json = oidc_introspect_try_access(ctx, token, client_id);
        if (json) {
            return json;
        }
    }
    if (!prefer_refresh) {
        char *json = oidc_introspect_try_refresh(ctx, token, client_id);
        if (json) {
            return json;
        }
    }

    return oidc_inactive_json();
}
