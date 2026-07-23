/*
 * OIDC RP /handoff endpoint — Phase 13
 *
 * Atomically exchanges a single-use handoff code (minted by
 * /api/auth/oidc/callback in Phase 14) for the Hydrogen JWT that
 * was stashed in the handoff store. The success envelope deliberately
 * mirrors POST /api/auth/login so the Lithium SPA can treat both
 * auth paths uniformly.
 *
 * Threat model (per the Phase 13 plan and the Security Plan section
 * of `docs/H/plans/OIDC-PLAN.md`):
 *
 *   - Replay: every successful take is atomic. A second take with
 *     the same code returns NULL, which becomes 401 handoff_invalid.
 *   - Stolen handoff code (browser history / referer): when
 *     `BindHandoffToIp = true`, the requester IP must match the IP
 *     that completed /callback. Mismatch becomes 401 handoff_invalid.
 *   - Brute-force guessing: codes are 32 random bytes (256 bits) of
 *     entropy from CSPRNG; the search space is too large for online
 *     guessing within the 60-second TTL.
 *   - Timing-side channel for "code exists vs. doesn't exist": a
 *     single 401 envelope is returned for every failure mode, with
 *     no observable timing asymmetry beyond the cost of the
 *     unconditional hash-table lookup.
 *
 * The body is buffered + JSON-validated by the api_service middleware
 * because `auth/oidc/handoff` is in the `endpoint_expects_json`
 * list. By the time this handler runs, `con_cls` already holds a
 * complete `ApiPostBuffer` with valid JSON.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/api/api_utils.h>

// Local includes
#include "oidc_rp_handoff.h"
#include "oidc_rp_handoff_store.h"
#include "oidc_rp_service.h"

// Single failure envelope. The plan deliberately collapses every
// failure mode (missing/expired/replay/IP-mismatch/malformed) onto
// the same 401 envelope so the SPA shows one error message and a
// network observer cannot tell the failure modes apart.
enum MHD_Result send_handoff_invalid(struct MHD_Connection *connection,
                                            void **con_cls,
                                            const char *reason_for_log) {
    log_this(SR_AUTH,
             "OIDC RP /handoff: returning 401 handoff_invalid (%s)",
             LOG_LEVEL_ALERT, 1,
             reason_for_log ? reason_for_log : "(unspecified)");
    api_free_post_buffer(con_cls);
    json_t *response = json_object();
    if (!response) {
        return MHD_NO;
    }
    json_object_set_new(response, "error", json_string("handoff_invalid"));
    return api_send_json_response(connection, response, MHD_HTTP_UNAUTHORIZED);
}

enum MHD_Result handle_post_auth_oidc_handoff(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *version,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls
) {
    (void)url;     // Unused parameter
    (void)version; // Unused parameter

    // --- Method discrimination — /handoff is POST-only ---
    if (!method || strcmp(method, "POST") != 0) {
        return oidc_rp_send_method_not_allowed(connection, method,
                                               "handoff", "POST");
    }

    // --- Feature gate ---
    // The middleware in api_service.c has already buffered the body
    // by the time we get here (auth/oidc/handoff is in
    // endpoint_expects_json). For the disabled case we must:
    //   - cleanup the buffer so MHD doesn't double-deliver,
    //   - return the canonical 503 envelope.
    if (!oidc_rp_is_enabled()) {
        api_free_post_buffer(con_cls);
        return oidc_rp_send_disabled_response(connection, method, "handoff");
    }

    // --- Lazy-init the runtime (handoff store + state + discovery) ---
    // First call after server startup with feature enabled does the
    // init. Phase 14 will replace this with a proper startup hook.
    if (!oidc_rp_runtime_lazy_init()) {
        api_free_post_buffer(con_cls);
        log_this(SR_AUTH,
                 "OIDC RP /handoff: runtime init failed",
                 LOG_LEVEL_ERROR, 0);
        json_t *response = json_object();
        if (!response) return MHD_NO;
        json_object_set_new(response, "error",
                            json_string("oidc_runtime_unavailable"));
        return api_send_json_response(connection, response,
                                      MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    // --- Retrieve the buffered POST body ---
    // The middleware already validated this is well-formed JSON.
    // Calling api_buffer_post_data again with non-NULL *con_cls
    // returns API_BUFFER_COMPLETE with the same buffer.
    ApiPostBuffer *buffer = NULL;
    ApiBufferResult buf_result = api_buffer_post_data(method, upload_data,
                                                      upload_data_size,
                                                      con_cls, &buffer);
    switch (buf_result) {
        case API_BUFFER_CONTINUE:
            return MHD_YES;
        case API_BUFFER_COMPLETE:
            break;
        case API_BUFFER_ERROR:
            return api_send_error_and_cleanup(connection, con_cls,
                "Request processing error",
                MHD_HTTP_INTERNAL_SERVER_ERROR);
        case API_BUFFER_METHOD_ERROR:
            return api_send_error_and_cleanup(connection, con_cls,
                "Method not allowed - use POST",
                MHD_HTTP_METHOD_NOT_ALLOWED);
    }

    if (!buffer || !buffer->data || buffer->size == 0) {
        return send_handoff_invalid(connection, con_cls, "empty body");
    }

    // --- Parse the body and pull the `handoff` field ---
    json_t *request = api_parse_json_body(buffer);
    if (!request) {
        return send_handoff_invalid(connection, con_cls, "invalid json");
    }

    const char *handoff_code =
        json_string_value(json_object_get(request, "handoff"));
    if (!handoff_code || !*handoff_code) {
        json_decref(request);
        return send_handoff_invalid(connection, con_cls,
                                    "missing or empty handoff field");
    }

    // --- Atomic take from the handoff store ---
    OidcRpHandoffRecord *record = oidc_rp_handoff_store_take(handoff_code);
    json_decref(request);

    if (!record) {
        // Either unknown, expired, or already-taken (replay). Single
        // envelope on purpose.
        return send_handoff_invalid(connection, con_cls,
                                    "unknown/expired/replay");
    }

    // --- IP-bind enforcement (defensive) ---
    // Only enforced when the active provider's BindHandoffToIp is true
    // AND the record actually carries a stored IP. If either side is
    // missing the binding (e.g. callback ran behind a NAT that didn't
    // forward the client IP), we fall through rather than block the
    // user — same posture as the plan's "off by default if behind
    // aggressive load balancers" caveat.
    const OIDCRPProviderConfig *provider = oidc_rp_get_active_provider();
    bool bind_to_ip = provider ? provider->bind_handoff_to_ip : true;
    if (bind_to_ip && record->client_ip) {
        char *requester_ip = api_get_client_ip(connection);
        if (!requester_ip || strcmp(requester_ip, record->client_ip) != 0) {
            log_this(SR_AUTH,
                     "OIDC RP /handoff: IP-bind mismatch "
                     "(stored=%s, requester=%s)",
                     LOG_LEVEL_ALERT, 2,
                     record->client_ip,
                     requester_ip ? requester_ip : "(unknown)");
            free(requester_ip);
            oidc_rp_handoff_record_free(record);
            return send_handoff_invalid(connection, con_cls, "ip mismatch");
        }
        free(requester_ip);
    }

    // --- Build the success envelope ---
    // Shape mirrors POST /api/auth/login (see login.c:378-389) so the
    // Lithium SPA can treat both responses uniformly.
    json_t *response = json_object();
    if (!response) {
        oidc_rp_handoff_record_free(record);
        api_free_post_buffer(con_cls);
        return MHD_NO;
    }
    json_object_set_new(response, "success", json_true());
    json_object_set_new(response, "token", json_string(record->jwt));
    json_object_set_new(response, "expires_at",
                        json_integer((json_int_t)record->expires_at));
    json_object_set_new(response, "user_id",
                        json_integer((json_int_t)record->account_id));
    json_object_set_new(response, "username",
                        json_string(record->username ? record->username : ""));
    json_object_set_new(response, "roles",
                        json_string(record->roles ? record->roles : ""));

    log_this(SR_AUTH,
             "OIDC RP /handoff: success — user_id=%d, ip=%s",
             LOG_LEVEL_DEBUG, 2,
             record->account_id,
             record->client_ip ? record->client_ip : "(unknown)");

    oidc_rp_handoff_record_free(record);
    api_free_post_buffer(con_cls);
    return api_send_json_response(connection, response, MHD_HTTP_OK);
}
