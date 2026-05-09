/*
 * OIDC RP /callback endpoint — Phase 14 implementation
 *
 * Composes Phases 7, 9, 10, 11, 12, 13 into the working chain that
 * turns an IdP authorization code into a Hydrogen JWT and a one-time
 * handoff record. See oidc_rp_callback.h for the step-by-step flow
 * and the typed-error vocabulary.
 *
 * Account linking is deliberately stubbed via
 * `oidc_rp_link_stub_resolve` (Phase 14 returns the fixed test
 * account); Phase 21 swaps in the four-strategy real linker. The
 * stub keeps Phase 14 free of schema work (Phases 15-17) so the
 * end-to-end happy path can be black-box-tested today.
 *
 * Logging policy: NEVER logs `code`, `state`, `nonce`, `id_token`,
 * `access_token`, the minted Hydrogen JWT, or the handoff code.
 * Logs typed error names, truncated state prefixes, account_id,
 * and client_ip. This matches the discipline established in
 * oidc_rp_start.c (Phase 10 lesson #10) and the security plan
 * deny-list (`docs/OIDC-PLAN.md` line 765).
 */

// Project includes
#include <src/hydrogen.h>
#include <src/api/api_utils.h>
#include <src/api/auth/auth_service.h>

// Local includes
#include "oidc_rp_callback.h"
#include "oidc_rp_service.h"
#include "oidc_rp_state.h"
#include "oidc_rp_discovery.h"
#include "oidc_rp_token.h"
#include "oidc_rp_idtoken.h"
#include "oidc_rp_handoff_store.h"
#include "oidc_rp_pkce.h"
#include "oidc_rp_link_stub.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

// Hydrogen JWT lifetime — matches login.c:356 and renew.c:19. A future
// refactor could lift this into auth_service_jwt.h, but for now mirror
// the existing constant verbatim so the contract is identical to
// password login.
#define OIDC_RP_CALLBACK_JWT_LIFETIME 3600

// Truncated state for logging — first 8 chars only. Same shape as the
// helper in oidc_rp_start.c.
static void truncated_state(const char *state, char out[16]) {
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

// Build the SPA error redirect URL: <return_origin>/login?oidc_error=<code>
// (plus &return_to=<sanitized> when supplied). The return-origin is taken
// from the provider's redirect_uri minus the path, so the SPA's origin is
// reachable without additional config. NULL inputs degrade to a minimal URL.
static char *build_spa_error_url(const char *redirect_uri,
                                 const char *error_code,
                                 const char *return_to) {
    if (!error_code) {
        error_code = "server_error";
    }

    // Derive the SPA origin from the redirect_uri scheme://authority.
    // E.g. "http://localhost:5243/api/auth/oidc/callback" → "http://localhost:5243".
    char origin[512];
    origin[0] = '\0';
    if (redirect_uri && *redirect_uri) {
        const char *scheme_end = strstr(redirect_uri, "://");
        if (scheme_end) {
            const char *path_start = strchr(scheme_end + 3, '/');
            size_t origin_len = path_start
                ? (size_t)(path_start - redirect_uri)
                : strlen(redirect_uri);
            if (origin_len < sizeof(origin)) {
                memcpy(origin, redirect_uri, origin_len);
                origin[origin_len] = '\0';
            }
        }
    }

    char *encoded_err = api_url_encode(error_code);
    if (!encoded_err) {
        return NULL;
    }

    char *encoded_return = NULL;
    if (return_to && *return_to) {
        encoded_return = api_url_encode(return_to);
        if (!encoded_return) {
            free(encoded_err);
            return NULL;
        }
    }

    size_t cap = 1024;
    char *url = malloc(cap);
    if (!url) {
        free(encoded_err);
        free(encoded_return);
        return NULL;
    }

    int n;
    if (encoded_return) {
        n = snprintf(url, cap, "%s/login?oidc=1&oidc_error=%s&return_to=%s",
                     origin, encoded_err, encoded_return);
    } else {
        n = snprintf(url, cap, "%s/login?oidc=1&oidc_error=%s",
                     origin, encoded_err);
    }
    free(encoded_err);
    free(encoded_return);

    if (n < 0 || (size_t)n >= cap) {
        free(url);
        return NULL;
    }
    return url;
}

// Build the SPA success redirect URL with the one-time handoff code:
// <origin>/login?oidc=1&handoff=<code>[&return_to=<sanitized>]
static char *build_spa_success_url(const char *redirect_uri,
                                   const char *handoff_code,
                                   const char *return_to) {
    if (!handoff_code || !*handoff_code) {
        return NULL;
    }

    char origin[512];
    origin[0] = '\0';
    if (redirect_uri && *redirect_uri) {
        const char *scheme_end = strstr(redirect_uri, "://");
        if (scheme_end) {
            const char *path_start = strchr(scheme_end + 3, '/');
            size_t origin_len = path_start
                ? (size_t)(path_start - redirect_uri)
                : strlen(redirect_uri);
            if (origin_len < sizeof(origin)) {
                memcpy(origin, redirect_uri, origin_len);
                origin[origin_len] = '\0';
            }
        }
    }

    char *encoded_handoff = api_url_encode(handoff_code);
    if (!encoded_handoff) {
        return NULL;
    }

    char *encoded_return = NULL;
    if (return_to && *return_to) {
        encoded_return = api_url_encode(return_to);
        if (!encoded_return) {
            free(encoded_handoff);
            return NULL;
        }
    }

    size_t cap = 1024;
    char *url = malloc(cap);
    if (!url) {
        free(encoded_handoff);
        free(encoded_return);
        return NULL;
    }

    int n;
    if (encoded_return) {
        n = snprintf(url, cap, "%s/login?oidc=1&handoff=%s&return_to=%s",
                     origin, encoded_handoff, encoded_return);
    } else {
        n = snprintf(url, cap, "%s/login?oidc=1&handoff=%s",
                     origin, encoded_handoff);
    }
    free(encoded_handoff);
    free(encoded_return);

    if (n < 0 || (size_t)n >= cap) {
        free(url);
        return NULL;
    }
    return url;
}

// Redirect to the SPA's login page with `?oidc_error=<code>`. Used by
// every failure branch below. Returns MHD_NO on allocation failure
// (the connection is then closed by MHD).
static enum MHD_Result redirect_with_error(struct MHD_Connection *connection,
                                           const OIDCRPProviderConfig *provider,
                                           const char *error_code,
                                           const char *return_to) {
    const char *redirect_uri = provider ? provider->redirect_uri : NULL;
    char *url = build_spa_error_url(redirect_uri, error_code, return_to);
    if (!url) {
        log_this(SR_AUTH,
                 "OIDC RP /callback: failed to build SPA error URL (%s)",
                 LOG_LEVEL_ERROR, 1,
                 error_code ? error_code : "(unknown)");
        return MHD_NO;
    }
    enum MHD_Result ret = oidc_rp_send_redirect(connection, url);
    free(url);
    return ret;
}

// Scrub a sensitive heap string (volatile-ptr walk to defeat -O2
// dead-store elimination) and free it. Mirrors the discipline used
// in oidc_rp_state.c, oidc_rp_start.c, and oidc_rp_token.c.
static void scrub_free(char *s) {
    if (!s) {
        return;
    }
    volatile char *p = s;
    while (*p) {
        *p++ = 0;
    }
    free(s);
}

enum MHD_Result handle_get_auth_oidc_callback(
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

    // ---- Method discrimination — /callback is GET-only ----
    if (!method || strcmp(method, "GET") != 0) {
        return oidc_rp_send_method_not_allowed(connection, method,
                                               "callback", "GET");
    }

    // ---- Feature gate — disabled is the production default ----
    if (!oidc_rp_is_enabled()) {
        return oidc_rp_send_disabled_response(connection, method, "callback");
    }

    // ---- Resolve the active provider ----
    const OIDCRPProviderConfig *provider = oidc_rp_get_active_provider();
    if (!provider) {
        log_this(SR_AUTH,
                 "OIDC RP /callback: enabled but no providers configured",
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

    // ---- Lazy runtime init (state + handoff + discovery) ----
    if (!oidc_rp_runtime_lazy_init()) {
        return redirect_with_error(connection, provider, "server_error", NULL);
    }

    // ---- Pull the IdP-supplied query parameters ----
    const char *idp_error = MHD_lookup_connection_value(
        connection, MHD_GET_ARGUMENT_KIND, "error");
    const char *idp_error_desc = MHD_lookup_connection_value(
        connection, MHD_GET_ARGUMENT_KIND, "error_description");
    const char *code = MHD_lookup_connection_value(
        connection, MHD_GET_ARGUMENT_KIND, "code");
    const char *state = MHD_lookup_connection_value(
        connection, MHD_GET_ARGUMENT_KIND, "state");

    // The IdP can short-circuit the flow with ?error=...; respect it
    // as a pass-through to the SPA. We never echo the IdP's
    // error_description into the URL — that would be an injection
    // surface — but we DO log it for operator diagnostics.
    if (idp_error && *idp_error) {
        log_this(SR_AUTH,
                 "OIDC RP /callback: IdP returned error=%s desc=%s",
                 LOG_LEVEL_ALERT, 2,
                 idp_error,
                 idp_error_desc ? idp_error_desc : "(none)");
        return redirect_with_error(connection, provider, "idp_error", NULL);
    }

    if (!code || !*code || !state || !*state) {
        log_this(SR_AUTH,
                 "OIDC RP /callback: missing required code/state",
                 LOG_LEVEL_ALERT, 0);
        return redirect_with_error(connection, provider,
                                   "state_invalid", NULL);
    }

    // ---- Atomic state take ----
    OidcRpStateRecord *state_record = oidc_rp_state_take(state);
    if (!state_record) {
        // Unknown / expired / replay all collapse to one error so the
        // network-side observer cannot tell modes apart.
        char state_short[16];
        truncated_state(state, state_short);
        log_this(SR_AUTH,
                 "OIDC RP /callback: state_invalid (state=%s...)",
                 LOG_LEVEL_ALERT, 1, state_short);
        return redirect_with_error(connection, provider,
                                   "state_invalid", NULL);
    }

    // Pull the return_to out of the state record now so we can use it
    // on every error path below. The state record owns the string;
    // we read it without dup'ing because we control its lifetime.
    const char *return_to = state_record->return_to;

    // ---- Resolve the discovery doc for the token endpoint ----
    const OidcRpDiscoveryDoc *doc = oidc_rp_discovery_get(
        provider->name,
        provider->issuer,
        provider->verify_ssl,
        provider->discovery_cache_seconds);
    if (!doc || !doc->token_endpoint || !doc->jwks_uri) {
        log_this(SR_AUTH,
                 "OIDC RP /callback: discovery resolution failed for provider %s",
                 LOG_LEVEL_ERROR, 1,
                 provider->name ? provider->name : "(unnamed)");
        enum MHD_Result ret = redirect_with_error(connection, provider,
                                                  "server_error", return_to);
        oidc_rp_state_record_free(state_record);
        return ret;
    }

    // Copy the URLs we need before any further mutating call on the
    // discovery cache (per discovery.h: const pointer valid only
    // until the next mutating call on this provider's slot).
    char *token_endpoint = strdup(doc->token_endpoint);
    char *jwks_uri       = strdup(doc->jwks_uri);
    if (!token_endpoint || !jwks_uri) {
        free(token_endpoint);
        free(jwks_uri);
        oidc_rp_state_record_free(state_record);
        return redirect_with_error(connection, provider, "server_error",
                                   return_to);
    }

    // ---- Token exchange (POST to IdP /token) ----
    OidcRpTokenResponse *token_response = NULL;
    OidcRpTokenError token_err = oidc_rp_exchange_code(
        provider,
        token_endpoint,
        code,
        provider->redirect_uri,
        state_record->code_verifier,
        &token_response);
    free(token_endpoint);

    if (token_err != OIDC_RP_TOKEN_OK || !token_response) {
        log_this(SR_AUTH,
                 "OIDC RP /callback: token exchange failed (%s)",
                 LOG_LEVEL_ALERT, 1,
                 oidc_rp_token_error_name(token_err));
        // Build a typed error string: token_<name> stays under the
        // length budget for typical error names (longest is
        // "invalid_grant" at 13).
        char err_code[48];
        snprintf(err_code, sizeof(err_code), "token_%s",
                 oidc_rp_token_error_name(token_err));
        enum MHD_Result ret = redirect_with_error(connection, provider,
                                                  err_code, return_to);
        free(jwks_uri);
        oidc_rp_state_record_free(state_record);
        oidc_rp_token_response_free(token_response);
        return ret;
    }

    // ---- ID token validation ----
    OidcRpIdTokenClaims *claims = NULL;
    OidcRpIdTokenError id_err = oidc_rp_validate_id_token(
        provider,
        jwks_uri,
        token_response->id_token,
        state_record->nonce,
        time(NULL),
        &claims);
    free(jwks_uri);

    if (id_err != OIDC_RP_IDTOKEN_OK || !claims) {
        log_this(SR_AUTH,
                 "OIDC RP /callback: id_token validation failed (%s)",
                 LOG_LEVEL_ALERT, 1,
                 oidc_rp_idtoken_error_name(id_err));
        char err_code[48];
        snprintf(err_code, sizeof(err_code), "id_token_%s",
                 oidc_rp_idtoken_error_name(id_err));
        enum MHD_Result ret = redirect_with_error(connection, provider,
                                                  err_code, return_to);
        oidc_rp_token_response_free(token_response);
        oidc_rp_state_record_free(state_record);
        oidc_rp_idtoken_claims_free(claims);
        return ret;
    }

    // The token bundle is no longer needed once claims are extracted.
    // Free aggressively so the access/id token strings are scrubbed
    // from memory at the earliest opportunity.
    oidc_rp_token_response_free(token_response);
    token_response = NULL;

    // ---- Resolve the OIDC identity to a Hydrogen account (Phase 14 stub) ----
    // Database resolution precedence: state record → top-level OIDC_RP.Database
    // → "Lithium" fallback. Matches the plan's contract for Phase 10/14.
    const char *database = state_record->database;
    if (!database || !*database) {
        database = (app_config && app_config->oidc_rp.database)
                       ? app_config->oidc_rp.database
                       : "Lithium";
    }

    account_info_t *account = oidc_rp_link_stub_resolve(claims, database);
    if (!account) {
        oidc_rp_idtoken_claims_free(claims);
        oidc_rp_state_record_free(state_record);
        return redirect_with_error(connection, provider, "no_account",
                                   return_to);
    }

    // ---- API key + system info (server-side, never sent to browser) ----
    const char *system_api_key = provider->system_api_key;
    if (!system_api_key || !*system_api_key) {
        log_this(SR_AUTH,
                 "OIDC RP /callback: provider %s has no SystemApiKey configured",
                 LOG_LEVEL_ERROR, 1,
                 provider->name ? provider->name : "(unnamed)");
        free_account_info(account);
        oidc_rp_idtoken_claims_free(claims);
        oidc_rp_state_record_free(state_record);
        return redirect_with_error(connection, provider, "no_api_key",
                                   return_to);
    }

    system_info_t sys_info = {0};
    if (!verify_api_key(system_api_key, database, &sys_info)) {
        log_this(SR_AUTH,
                 "OIDC RP /callback: SystemApiKey rejected by verify_api_key",
                 LOG_LEVEL_ALERT, 0);
        free_account_info(account);
        oidc_rp_idtoken_claims_free(claims);
        oidc_rp_state_record_free(state_record);
        return redirect_with_error(connection, provider, "no_api_key",
                                   return_to);
    }

    // ---- Mint the Hydrogen JWT — same code path as password login ----
    // tz: Phase 14 doesn't surface a per-request timezone for OIDC
    // (the SPA's `/start` query param plumbing for tz is a future
    // refinement). Pass "UTC" so the JWT has a sane non-NULL tz claim.
    // Phase 26+ may extend `/start` to accept ?tz=... and propagate
    // it through the state record.
    const char *tz = "UTC";
    time_t issued_at = time(NULL);

    char *client_ip = state_record->client_ip
        ? strdup(state_record->client_ip)
        : api_get_client_ip(connection);

    char *jwt_token = generate_jwt(account, &sys_info, client_ip, tz,
                                   database, issued_at);
    if (!jwt_token) {
        log_this(SR_AUTH,
                 "OIDC RP /callback: generate_jwt failed for account_id=%d",
                 LOG_LEVEL_ERROR, 1, account->id);
        free(client_ip);
        free_account_info(account);
        oidc_rp_idtoken_claims_free(claims);
        oidc_rp_state_record_free(state_record);
        return redirect_with_error(connection, provider, "server_error",
                                   return_to);
    }

    char *jwt_hash = compute_token_hash(jwt_token);
    if (!jwt_hash) {
        scrub_free(jwt_token);
        free(client_ip);
        free_account_info(account);
        oidc_rp_idtoken_claims_free(claims);
        oidc_rp_state_record_free(state_record);
        return redirect_with_error(connection, provider, "server_error",
                                   return_to);
    }

    time_t expires_at = issued_at + OIDC_RP_CALLBACK_JWT_LIFETIME;
    store_jwt(account->id, jwt_hash, expires_at,
              sys_info.system_id, sys_info.app_id, database, client_ip);
    free(jwt_hash);

    // ---- Generate the handoff code and put the record ----
    char *handoff_code = oidc_rp_make_random_hex(32);
    if (!handoff_code) {
        scrub_free(jwt_token);
        free(client_ip);
        free_account_info(account);
        oidc_rp_idtoken_claims_free(claims);
        oidc_rp_state_record_free(state_record);
        return redirect_with_error(connection, provider, "server_error",
                                   return_to);
    }

    bool put_ok = oidc_rp_handoff_store_put(
        handoff_code,
        jwt_token,
        account->id,
        account->username,
        account->roles,
        client_ip,
        expires_at,
        provider->handoff_ttl_seconds);

    // The handoff store has copied everything it needs; we can free
    // our local references now. The JWT is scrubbed (sensitive); the
    // others are routine.
    scrub_free(jwt_token);
    jwt_token = NULL;

    if (!put_ok) {
        log_this(SR_AUTH,
                 "OIDC RP /callback: handoff_store_put failed",
                 LOG_LEVEL_ERROR, 0);
        scrub_free(handoff_code);
        free(client_ip);
        free_account_info(account);
        oidc_rp_idtoken_claims_free(claims);
        oidc_rp_state_record_free(state_record);
        return redirect_with_error(connection, provider, "server_error",
                                   return_to);
    }

    log_this(SR_AUTH,
             "OIDC RP /callback: success — account_id=%d, ip=%s -> 302 handoff",
             LOG_LEVEL_DEBUG, 2,
             account->id,
             client_ip ? client_ip : "(unknown)");

    // ---- Build the SPA success redirect URL and 302 ----
    char *spa_url = build_spa_success_url(provider->redirect_uri,
                                          handoff_code, return_to);
    // Scrub the local handoff-code string after building the URL
    // (the URL itself contains the encoded copy; that's fine — the
    // browser is the next hop and the handoff is single-use anyway).
    scrub_free(handoff_code);
    free(client_ip);
    free_account_info(account);
    oidc_rp_idtoken_claims_free(claims);
    oidc_rp_state_record_free(state_record);

    if (!spa_url) {
        log_this(SR_AUTH,
                 "OIDC RP /callback: failed to build SPA success URL",
                 LOG_LEVEL_ERROR, 0);
        return MHD_NO;
    }

    enum MHD_Result ret = oidc_rp_send_redirect(connection, spa_url);
    free(spa_url);
    return ret;
}
