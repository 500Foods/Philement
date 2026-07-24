/*
 * OIDC UserInfo endpoint
 *
 * Validates the access token and returns claims filtered by scope
 * (sub always; email/profile from optional JWT user_data JSON).
 */

// Global includes
#include <src/hydrogen.h>

// Local includes
#include "oidc_service.h"
#include "oidc_tokens.h"

#include <jansson.h>

/*
 * True if space-delimited scope list contains needle as a whole token.
 * Avoids prefix false-positives (e.g. "open" must not match "openid").
 */
bool oidc_scope_has(const char *scope, const char *needle) {
    if (!scope || !needle || needle[0] == '\0') {
        return false;
    }
    size_t nlen = strlen(needle);
    const char *p = scope;
    while (p && *p) {
        while (*p == ' ') {
            p++;
        }
        if (strncmp(p, needle, nlen) == 0 &&
            (p[nlen] == '\0' || p[nlen] == ' ')) {
            return true;
        }
        p = strchr(p, ' ');
    }
    return false;
}

/*
 * Copy scoped profile/email claims from optional user_data JSON into out.
 * out must already be a JSON object; extra is loaded from user_data string.
 */
void oidc_userinfo_apply_scoped_claims(json_t *out, const char *scope, const char *user_data) {
    if (!out || !user_data || user_data[0] == '\0') {
        return;
    }

    const char *use_scope = (scope && scope[0] != '\0') ? scope : "openid";
    json_error_t err;
    json_t *extra = json_loads(user_data, 0, &err);
    if (!extra || !json_is_object(extra)) {
        if (extra) {
            json_decref(extra);
        }
        return;
    }

    if (oidc_scope_has(use_scope, "email")) {
        json_t *email = json_object_get(extra, "email");
        json_t *ev = json_object_get(extra, "email_verified");
        if (email) {
            json_object_set(out, "email", email);
        }
        if (ev) {
            json_object_set(out, "email_verified", ev);
        }
    }
    if (oidc_scope_has(use_scope, "profile")) {
        json_t *name = json_object_get(extra, "name");
        json_t *pref = json_object_get(extra, "preferred_username");
        if (name) {
            json_object_set(out, "name", name);
        }
        if (pref) {
            json_object_set(out, "preferred_username", pref);
        }
    }

    json_decref(extra);
}

/*
 * UserInfo: validate Bearer access token and return claims JSON (caller frees).
 * Returns NULL for invalid/missing token or uninitialized service.
 */
char* oidc_process_userinfo_request(const char *access_token) {
    OIDCContext *ctx = get_oidc_context();
    if (!ctx || !ctx->initialized) {
        log_this(SR_OIDC, "OIDC service not initialized", LOG_LEVEL_ERROR, 0);
        return NULL;
    }
    if (!access_token || access_token[0] == '\0') {
        return NULL;
    }

    OIDCTokenContext *tok = (OIDCTokenContext*)ctx->token_context;
    if (!tok) {
        return NULL;
    }

    OIDCTokenClaims *claims = NULL;
    if (!oidc_validate_access_token(tok, access_token, &claims) || !claims) {
        log_this(SR_OIDC, "Userinfo rejected invalid access token", LOG_LEVEL_DEBUG, 0);
        return NULL;
    }

    const char *issuer = ctx->config.issuer ? ctx->config.issuer : "";
    if (claims->iss && issuer[0] != '\0' && strcmp(claims->iss, issuer) != 0) {
        oidc_free_token_claims(claims);
        return NULL;
    }

    json_t *out = json_object();
    if (!out) {
        oidc_free_token_claims(claims);
        return NULL;
    }
    json_object_set_new(out, "sub", json_string(claims->sub ? claims->sub : ""));

    /* openid required for userinfo; always return sub. Optional email/profile from JWT. */
    const char *scope = claims->scope ? claims->scope : "openid";
    oidc_userinfo_apply_scoped_claims(out, scope, claims->user_data);

    /* Accounts DB profile fill deferred until lookup-by-id exists. */

    char *json = json_dumps(out, JSON_COMPACT);
    json_decref(out);
    oidc_free_token_claims(claims);
    return json;
}
