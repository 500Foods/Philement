/*
 * OIDC RP /start endpoint — Phase 10 implementation
 *
 * Initiates an Authorization Code + PKCE flow with the configured OIDC
 * provider:
 *
 *   1. Look up the provider config (`?provider=` name, else Providers[0]).
 *   2. Generate state, nonce, and PKCE verifier+challenge.
 *   3. Validate the optional `return_to` query parameter against the
 *      open-redirect allow-list.
 *   4. Resolve the discovery doc (cache hit or fresh fetch) so we know
 *      the IdP's authorization_endpoint.
 *   5. Persist {state, nonce, code_verifier, database, return_to,
 *      client_ip, provider_name} in the state store with the configured TTL.
 *   6. Build the authorization URL and 302-redirect the browser to it.
 *
 * Disabled-feature, method-mismatch, and runtime-init failures all use
 * the existing service helpers (oidc_rp_service.h) so the response
 * envelopes stay consistent across the OIDC RP surface.
 *
 * Sensitive values (nonce, code_verifier) are NEVER logged. The state
 * value is logged at DEBUG level only, truncated to a short prefix for
 * grep-ability.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/api/api_utils.h>

// Local includes
#include "oidc_rp_start.h"
#include "oidc_rp_service.h"
#include "oidc_rp_state.h"
#include "oidc_rp_discovery.h"
#include "oidc_rp_pkce.h"

#include <stdlib.h>
#include <string.h>

// Send an envelope-style 500 response and log the cause. Used for
// internal failures (allocation, RNG, etc.) where 503 would be misleading.
enum MHD_Result send_internal_error(struct MHD_Connection *connection,
                                           const char *cause) {
    log_this(SR_AUTH,
             "OIDC RP /start internal error: %s",
             LOG_LEVEL_ERROR, 1,
             cause ? cause : "(unknown)");
    json_t *response = json_object();
    if (!response) {
        return MHD_NO;
    }
    json_object_set_new(response, "error", json_string("oidc_internal_error"));
    return api_send_json_response(connection, response,
                                  MHD_HTTP_INTERNAL_SERVER_ERROR);
}

// Shared failure teardown for handle_get_auth_oidc_start: scrub and free
// the sensitive buffers (state, nonce, code_verifier) before emitting the
// internal-error envelope. Any pointer may be NULL (e.g. when an earlier
// allocation already failed); NULL pointers are simply skipped. The
// non-sensitive `auth_endpoint_copy` and `client_ip` are freed if present.
enum MHD_Result oidc_rp_start_fail(struct MHD_Connection *connection,
                                   const char *cause,
                                   char *state,
                                   char *nonce,
                                   char *code_verifier,
                                   char *code_challenge,
                                   char *client_ip,
                                   char *auth_endpoint_copy) {
    free(state);
    if (nonce) {
        volatile char *p = nonce;
        while (*p) { *p++ = 0; }
    }
    free(nonce);
    if (code_verifier) {
        volatile char *p = code_verifier;
        while (*p) { *p++ = 0; }
    }
    free(code_verifier);
    free(code_challenge);
    free(client_ip);
    free(auth_endpoint_copy);
    return send_internal_error(connection, cause);
}

// Truncated state for logging — first 8 chars only.
void start_truncated_state(const char *state, char out[16]) {
    if (!state) {
        snprintf(out, 16, "(null)");
        return;
    }
    size_t n = strlen(state);
    if (n > 8) {
        n = 8;
    }
    memcpy(out, state, n);
    out[n] = '\0';
}

enum MHD_Result handle_get_auth_oidc_start(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *version,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls
) {
    (void)url;              // Unused parameter
    (void)version;          // Unused parameter
    (void)upload_data;      // Unused parameter
    (void)upload_data_size; // Unused parameter
    (void)con_cls;          // Unused parameter

    // Method discrimination — /start is GET-only
    if (!method || strcmp(method, "GET") != 0) {
        return oidc_rp_send_method_not_allowed(connection, method, "start", "GET");
    }

    // Feature gate — disabled is the production default
    if (!oidc_rp_is_enabled()) {
        return oidc_rp_send_disabled_response(connection, method, "start");
    }

    // Lazy-initialize state store + discovery cache on first real call.
    // Phase 14 will replace this with a proper startup hook.
    if (!oidc_rp_runtime_lazy_init()) {
        return send_internal_error(connection, "runtime init failed");
    }

    // Pull the optional query parameters. Validation comes after we
    // have all of them so the failure messages can be precise.
    const char *return_to = MHD_lookup_connection_value(
        connection, MHD_GET_ARGUMENT_KIND, "return_to");
    const char *database  = MHD_lookup_connection_value(
        connection, MHD_GET_ARGUMENT_KIND, "database");
    const char *provider_q = MHD_lookup_connection_value(
        connection, MHD_GET_ARGUMENT_KIND, "provider");

    // Resolve provider: named ?provider= or default Providers[0].
    // An explicit unknown name is 400 unknown_provider (stable contract).
    // Missing config entirely is 503 oidc_no_provider.
    const OIDCRPProviderConfig *provider = oidc_rp_find_provider(provider_q);
    if (!provider) {
        if (provider_q && *provider_q) {
            log_this(SR_AUTH,
                     "OIDC RP /start: unknown provider name",
                     LOG_LEVEL_ALERT, 0);
            json_t *response = json_object();
            if (!response) {
                return MHD_NO;
            }
            json_object_set_new(response, "error",
                                json_string("unknown_provider"));
            return api_send_json_response(connection, response,
                                          MHD_HTTP_BAD_REQUEST);
        }
        log_this(SR_AUTH,
                 "OIDC RP /start: enabled but no providers configured",
                 LOG_LEVEL_ERROR, 0);
        json_t *response = json_object();
        if (!response) {
            return MHD_NO;
        }
        json_object_set_new(response, "error",
                            json_string("oidc_no_provider"));
        return api_send_json_response(connection, response,
                                      MHD_HTTP_SERVICE_UNAVAILABLE);
    }

    if (!oidc_rp_safe_return_to(return_to)) {
        log_this(SR_AUTH,
                 "OIDC RP /start: rejected unsafe return_to",
                 LOG_LEVEL_ALERT, 0);
        json_t *response = json_object();
        if (!response) {
            return MHD_NO;
        }
        json_object_set_new(response, "error",
                            json_string("invalid_return_to"));
        return api_send_json_response(connection, response,
                                      MHD_HTTP_BAD_REQUEST);
    }

    // Generate PKCE + state + nonce. Each is a fresh allocation; we own
    // them until they are either stored (on success) or freed (on error).
    char *state         = oidc_rp_make_random_hex(32);
    char *nonce         = oidc_rp_make_random_hex(32);
    char *code_verifier = oidc_rp_make_code_verifier();
    char *code_challenge = (code_verifier
                             ? oidc_rp_make_code_challenge(code_verifier)
                             : NULL);

    if (!state || !nonce || !code_verifier || !code_challenge) {
        oidc_rp_start_fail(connection, "PKCE/state generation failed",
                           state, nonce, code_verifier, code_challenge,
                           NULL, NULL);
        return MHD_NO;
    }

    // Resolve the discovery doc. This may make an outbound HTTPS call on
    // the first hit; subsequent hits within DiscoveryCacheSeconds reuse
    // the cached doc. The const pointer remains valid until the next
    // mutating call on this provider's cache slot, so we copy the auth
    // URL out immediately.
    const OidcRpDiscoveryDoc *doc = oidc_rp_discovery_get(
        provider->name,
        provider->issuer,
        provider->verify_ssl,
        provider->discovery_cache_seconds);
    if (!doc || !doc->authorization_endpoint) {
        log_this(SR_AUTH,
                 "OIDC RP /start: discovery resolution failed for provider %s",
                 LOG_LEVEL_ERROR, 1,
                 provider->name ? provider->name : "(unnamed)");
        oidc_rp_start_fail(connection, "oidc_discovery_failed",
                           state, nonce, code_verifier, code_challenge,
                           NULL, NULL);
        return MHD_NO;
    }

    // Copy fields we need before any further calls that might mutate
    // the discovery cache. Per `oidc_rp_discovery.h`, the const pointer
    // is valid until the next mutating call on this provider.
    char *auth_endpoint_copy = strdup(doc->authorization_endpoint);
    if (!auth_endpoint_copy) {
        oidc_rp_start_fail(connection, "auth endpoint copy failed",
                           state, nonce, code_verifier, code_challenge,
                           NULL, NULL);
        return MHD_NO;
    }

    // Capture the client IP for state-bound auditing. NULL is acceptable
    // here — the state-store record is structured to handle it.
    char *client_ip = api_get_client_ip(connection);

    // Insert the state record. The state store copies every input into
    // its own heap buffers, so we still own ours after this call.
    bool put_ok = oidc_rp_state_put(
        state, nonce, code_verifier,
        database, return_to, client_ip,
        provider->name,
        provider->state_ttl_seconds);

    if (!put_ok) {
        free(auth_endpoint_copy);
        oidc_rp_start_fail(connection, "state store put failed",
                           state, nonce, code_verifier, code_challenge,
                           client_ip, NULL);
        return MHD_NO;
    }

    // Choose the scope string: provider config first, fallback to the
    // OIDC required scope set.
    const char *scope = (provider->scopes && *provider->scopes)
                          ? provider->scopes
                          : "openid profile email";

    char *redirect_url = oidc_rp_build_authorize_url(
        auth_endpoint_copy,
        provider->client_id,
        provider->redirect_uri,
        scope,
        state,
        nonce,
        code_challenge);
    free(auth_endpoint_copy);

    if (!redirect_url) {
        // The state record we just inserted will age out via the TTL
        // sweeper; not worth taking the lock again to remove it.
        oidc_rp_start_fail(connection, "authorize URL build failed",
                           state, nonce, code_verifier, code_challenge,
                           client_ip, NULL);
        return MHD_NO;
    }

    // Log at STATE level — operator-visible but no secrets. State is
    // truncated, nonce / code_verifier / code_challenge are never logged.
    char state_short[16];
    start_truncated_state(state, state_short);
    log_this(SR_AUTH,
             "OIDC RP /start: provider=%s state=%s... ip=%s -> 302",
             LOG_LEVEL_STATE, 3,
             provider->name      ? provider->name      : "(unnamed)",
             state_short,
             client_ip           ? client_ip           : "(unknown)");

    // Free our copies — the state store has its own duplicates.
    free(state);
    {
        volatile char *p = nonce;
        while (*p) { *p++ = 0; }
    }
    free(nonce);
    {
        volatile char *p = code_verifier;
        while (*p) { *p++ = 0; }
    }
    free(code_verifier);
    free(code_challenge);
    free(client_ip);

    enum MHD_Result ret = oidc_rp_send_redirect(connection, redirect_url);
    free(redirect_url);
    return ret;
}
