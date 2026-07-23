/*
 * OIDC Relying Party shared service helpers
 *
 * Provides the `oidc_rp_is_enabled()` gate, response helpers shared by
 * all three OIDC RP endpoint handlers, and Phase 10 helpers:
 *
 *   - `oidc_rp_send_disabled_response()`     — 503 + {"error":"oidc_disabled"}
 *   - `oidc_rp_send_method_not_allowed()`    — 405 + {"error":"method_not_allowed"}
 *   - `oidc_rp_safe_return_to()`             — open-redirect allow-list
 *   - `oidc_rp_build_authorize_url()`        — IdP redirect URL builder
 *   - `oidc_rp_send_redirect()`              — 302 + Cache-Control: no-store
 *   - `oidc_rp_runtime_lazy_init()`          — first-use init of state +
 *                                              discovery (Phase 14 will
 *                                              replace with proper hook)
 *   - `oidc_rp_get_active_provider()`        — currently always Providers[0]
 *
 * Logic beyond Phase 10 (token exchange, ID-token validation, account
 * linking, JWT minting) lands in later phases (11–22). See
 * `docs/H/plans/OIDC-PLAN.md` for context.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/api/api_utils.h>

// Local includes
#include "oidc_rp_service.h"
#include "oidc_rp_state.h"
#include "oidc_rp_handoff_store.h"
#include "oidc_rp_discovery.h"
#include "oidc_rp_jwks.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

bool oidc_rp_is_enabled(void) {
    if (!app_config) {
        return false;
    }
    return app_config->oidc_rp.enabled;
}

enum MHD_Result oidc_rp_send_disabled_response(struct MHD_Connection *connection,
                                               const char *method,
                                               const char *endpoint) {
    log_this(SR_AUTH,
             "OIDC RP endpoint /api/auth/oidc/%s called via %s but oidc_rp is disabled",
             LOG_LEVEL_DEBUG, 2,
             endpoint ? endpoint : "(unknown)",
             method   ? method   : "(unknown)");

    json_t *response = json_object();
    if (!response) {
        log_this(SR_AUTH, "Failed to allocate JSON response for oidc_disabled",
                 LOG_LEVEL_ERROR, 0);
        return MHD_NO;
    }
    json_object_set_new(response, "error", json_string("oidc_disabled"));
    return api_send_json_response(connection, response, MHD_HTTP_SERVICE_UNAVAILABLE);
}

enum MHD_Result oidc_rp_send_method_not_allowed(struct MHD_Connection *connection,
                                                const char *method,
                                                const char *endpoint,
                                                const char *expected_method) {
    log_this(SR_AUTH,
             "OIDC RP endpoint /api/auth/oidc/%s rejected method %s (expected %s)",
             LOG_LEVEL_ALERT, 3,
             endpoint        ? endpoint        : "(unknown)",
             method          ? method          : "(unknown)",
             expected_method ? expected_method : "(unknown)");

    json_t *response = json_object();
    if (!response) {
        log_this(SR_AUTH, "Failed to allocate JSON response for method_not_allowed",
                 LOG_LEVEL_ERROR, 0);
        return MHD_NO;
    }
    json_object_set_new(response, "error", json_string("method_not_allowed"));
    return api_send_json_response(connection, response, MHD_HTTP_METHOD_NOT_ALLOWED);
}

bool oidc_rp_safe_return_to(const char *return_to) {
    // NULL is "not provided" — the caller checks for NULL before using
    // the value; validation only fails on present-but-unsafe inputs.
    if (!return_to) {
        return true;
    }
    if (return_to[0] == '\0') {
        return false;                          // Empty string is unsafe
    }
    if (return_to[0] != '/') {
        return false;                          // Must be a relative path
    }
    if (return_to[1] == '/') {
        return false;                          // Protocol-relative // form
    }
    if (return_to[1] == '\\') {
        return false;                          // /\evil.com (some parsers fold to /)
    }
    // Reject any backslash, scheme-marker, or CR/LF anywhere in the
    // string. CR/LF would let an attacker inject extra HTTP headers if
    // we ever echoed return_to into one.
    for (const char *p = return_to; *p; p++) {
        if (*p == '\\' || *p == '\r' || *p == '\n') {
            return false;
        }
        if (p[0] == ':' && p[1] == '/' && p[2] == '/') {
            return false;                      // Embedded scheme like "/foo://bar"
        }
    }
    return true;
}

// Helper: append "&key=encoded(value)" to a string buffer that we
// realloc as needed. Returns true on success; on failure the buffer
// may be partially written and is released by the caller.
bool append_param(char **buf, size_t *len, size_t *cap,
                         const char *prefix, const char *key,
                         const char *value) {
    if (!buf || !*buf || !key || !value) {
        return false;
    }
    char *encoded = api_url_encode(value);
    if (!encoded) {
        return false;
    }
    size_t add = strlen(prefix) + strlen(key) + 1 /* = */ + strlen(encoded) + 1;
    if (*len + add > *cap) {
        size_t new_cap = (*cap) ? (*cap) * 2 : 256;
        while (*len + add > new_cap) {
            new_cap *= 2;
        }
        char *grown = realloc(*buf, new_cap);
        if (!grown) {
            free(encoded);
            return false;
        }
        *buf = grown;
        *cap = new_cap;
    }
    int n = snprintf(*buf + *len, *cap - *len, "%s%s=%s", prefix, key, encoded);
    free(encoded);
    if (n < 0 || (size_t)n >= *cap - *len) {
        return false;
    }
    *len += (size_t)n;
    return true;
}

char *oidc_rp_build_authorize_url(const char *authorization_endpoint,
                                  const char *client_id,
                                  const char *redirect_uri,
                                  const char *scope,
                                  const char *state,
                                  const char *nonce,
                                  const char *code_challenge) {
    if (!authorization_endpoint || !client_id || !redirect_uri || !scope ||
        !state || !nonce || !code_challenge) {
        return NULL;
    }
    if (!*authorization_endpoint || !*client_id || !*redirect_uri ||
        !*scope || !*state || !*nonce || !*code_challenge) {
        return NULL;
    }

    size_t cap = 512;
    size_t len = 0;
    char *buf = malloc(cap);
    if (!buf) {
        return NULL;
    }
    buf[0] = '\0';

    // Authorization endpoint is appended verbatim — it is configured by
    // the operator and not subject to URL-encoding (it IS the URL).
    int n = snprintf(buf, cap, "%s", authorization_endpoint);
    if (n < 0 || (size_t)n >= cap) {
        free(buf);
        return NULL;
    }
    len = (size_t)n;

    // First parameter uses '?'; subsequent use '&'. response_type=code is
    // the only static parameter and goes first to make eyeballing easy.
    if (!append_param(&buf, &len, &cap, "?",  "response_type",         "code") ||
        !append_param(&buf, &len, &cap, "&",  "client_id",             client_id) ||
        !append_param(&buf, &len, &cap, "&",  "redirect_uri",          redirect_uri) ||
        !append_param(&buf, &len, &cap, "&",  "scope",                 scope) ||
        !append_param(&buf, &len, &cap, "&",  "state",                 state) ||
        !append_param(&buf, &len, &cap, "&",  "nonce",                 nonce) ||
        !append_param(&buf, &len, &cap, "&",  "code_challenge",        code_challenge) ||
        !append_param(&buf, &len, &cap, "&",  "code_challenge_method", "S256")) {
        free(buf);
        return NULL;
    }
    return buf;
}

enum MHD_Result oidc_rp_send_redirect(struct MHD_Connection *connection,
                                      const char *location) {
    if (!connection || !location || !*location) {
        return MHD_NO;
    }
    struct MHD_Response *response =
        MHD_create_response_from_buffer(0, (void *)"", MHD_RESPMEM_PERSISTENT);
    if (!response) {
        log_this(SR_AUTH,
                 "OIDC RP: failed to allocate redirect response",
                 LOG_LEVEL_ERROR, 0);
        return MHD_NO;
    }
    MHD_add_response_header(response, "Location",      location);
    MHD_add_response_header(response, "Cache-Control", "no-store");
    MHD_add_response_header(response, "Pragma",        "no-cache");
    enum MHD_Result ret =
        MHD_queue_response(connection, MHD_HTTP_FOUND, response);
    MHD_destroy_response(response);
    return ret;
}

// ---------------------------------------------------------------------------
// Lazy runtime init (Phase 10 only; Phase 14 replaces this)
// ---------------------------------------------------------------------------
//
// The state store and discovery cache need to be initialized exactly once
// before any /oidc/start request is served. Phase 14 will move this into
// Hydrogen's startup sequence proper. Until then, the first request that
// finds the feature gate enabled does the init.
//
// pthread_once gives us correctness without a recursive-mutex worry; the
// init flag is observed under acquire ordering by every caller.

static pthread_once_t   oidc_rp_init_once  = PTHREAD_ONCE_INIT;
static bool             oidc_rp_init_ok    = false;

void oidc_rp_runtime_init_impl(void) {
    int state_ttl = 600;                       // Plan default — 10 minutes
    int handoff_ttl = 60;                      // Plan default — 60 seconds
    if (app_config && app_config->oidc_rp.provider_count > 0) {
        int provider_state_ttl =
            app_config->oidc_rp.providers[0].state_ttl_seconds;
        if (provider_state_ttl > 0) {
            state_ttl = provider_state_ttl;
        }
        int provider_handoff_ttl =
            app_config->oidc_rp.providers[0].handoff_ttl_seconds;
        if (provider_handoff_ttl > 0) {
            handoff_ttl = provider_handoff_ttl;
        }
    }
    if (!oidc_rp_state_init(state_ttl)) {
        log_this(SR_AUTH,
                 "OIDC RP: failed to initialize state store",
                 LOG_LEVEL_ERROR, 0);
        return;
    }
    if (!oidc_rp_handoff_store_init(handoff_ttl)) {
        log_this(SR_AUTH,
                 "OIDC RP: failed to initialize handoff store",
                 LOG_LEVEL_ERROR, 0);
        oidc_rp_state_shutdown();
        return;
    }
    if (!oidc_rp_discovery_init()) {
        log_this(SR_AUTH,
                 "OIDC RP: failed to initialize discovery cache",
                 LOG_LEVEL_ERROR, 0);
        oidc_rp_handoff_store_shutdown();
        oidc_rp_state_shutdown();
        return;
    }
    if (!oidc_rp_jwks_init()) {
        log_this(SR_AUTH,
                 "OIDC RP: failed to initialize JWKS cache",
                 LOG_LEVEL_ERROR, 0);
        oidc_rp_discovery_shutdown();
        oidc_rp_handoff_store_shutdown();
        oidc_rp_state_shutdown();
        return;
    }
    oidc_rp_init_ok = true;
    log_this(SR_AUTH,
             "OIDC RP: runtime initialized (state TTL=%ds, handoff TTL=%ds)",
             LOG_LEVEL_STATE, 2, state_ttl, handoff_ttl);
}

bool oidc_rp_runtime_lazy_init(void) {
    pthread_once(&oidc_rp_init_once, oidc_rp_runtime_init_impl);
    return oidc_rp_init_ok;
}

void oidc_rp_runtime_shutdown(void) {
    if (!oidc_rp_init_ok) {
        // Either lazy_init was never called or it failed. Either way,
        // there's nothing to tear down.
        return;
    }
    oidc_rp_jwks_shutdown();
    oidc_rp_discovery_shutdown();
    oidc_rp_handoff_store_shutdown();
    oidc_rp_state_shutdown();
    oidc_rp_init_ok = false;
    // Reset the pthread_once gate so a subsequent lazy_init starts
    // afresh (only relevant for tests / re-init-after-config-reload).
    static const pthread_once_t fresh_once = PTHREAD_ONCE_INIT;
    oidc_rp_init_once = fresh_once;
    log_this(SR_AUTH,
             "OIDC RP: runtime shut down",
             LOG_LEVEL_STATE, 0);
}

const OIDCRPProviderConfig *oidc_rp_get_active_provider(void) {
    if (!app_config || !app_config->oidc_rp.enabled) {
        return NULL;
    }
    if (app_config->oidc_rp.provider_count == 0) {
        return NULL;
    }
    return &app_config->oidc_rp.providers[0];
}
