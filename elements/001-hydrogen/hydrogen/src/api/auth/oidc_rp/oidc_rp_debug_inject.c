/*
 * OIDC RP debug-inject endpoint — Phase 13
 *
 * Test-only endpoint. The entire functional body is compiled out
 * when NDEBUG is defined (the release build). See the header for
 * the full rationale.
 *
 * In Phase 14, when /callback is wired end-to-end, this endpoint
 * will still exist (because end-to-end black-box tests with a real
 * Keycloak are Phase 27, not Phase 14) — Phases 14 and beyond use
 * the same injection path to test handoff mechanics in isolation.
 *
 * Note: a tiny stub is left in place when NDEBUG is defined so the
 * translation unit is non-empty (ISO C forbids empty TUs under
 * `-Wpedantic`). The stub has no external linkage and no callers.
 */

#ifndef NDEBUG

// Project includes
#include <src/hydrogen.h>
#include <src/api/api_utils.h>

// Local includes
#include "oidc_rp_debug_inject.h"
#include "oidc_rp_handoff_store.h"
#include "oidc_rp_pkce.h"
#include "oidc_rp_service.h"

// Build a 400 envelope explaining the bad request. Free the buffer.
static enum MHD_Result send_bad_request(struct MHD_Connection *connection,
                                        void **con_cls,
                                        const char *reason) {
    log_this(SR_AUTH,
             "OIDC RP /_inject_handoff: bad request (%s)",
             LOG_LEVEL_ALERT, 1, reason ? reason : "(unspecified)");
    api_free_post_buffer(con_cls);
    json_t *response = json_object();
    if (!response) return MHD_NO;
    json_object_set_new(response, "error", json_string("bad_request"));
    json_object_set_new(response, "reason",
                        json_string(reason ? reason : "(unspecified)"));
    return api_send_json_response(connection, response,
                                  MHD_HTTP_BAD_REQUEST);
}

enum MHD_Result handle_post_auth_oidc_debug_inject(
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

    // --- Method discrimination ---
    if (!method || strcmp(method, "POST") != 0) {
        return oidc_rp_send_method_not_allowed(connection, method,
                                               "_inject_handoff", "POST");
    }

    // --- Feature gate ---
    // Same disabled-503 behaviour as the rest of the OIDC RP surface.
    // Body has been pre-buffered by the JSON middleware (the path is
    // in endpoint_expects_json) so we must free it here.
    if (!oidc_rp_is_enabled()) {
        api_free_post_buffer(con_cls);
        return oidc_rp_send_disabled_response(connection, method,
                                              "_inject_handoff");
    }

    // --- Lazy-init the runtime ---
    if (!oidc_rp_runtime_lazy_init()) {
        api_free_post_buffer(con_cls);
        log_this(SR_AUTH,
                 "OIDC RP /_inject_handoff: runtime init failed",
                 LOG_LEVEL_ERROR, 0);
        json_t *response = json_object();
        if (!response) return MHD_NO;
        json_object_set_new(response, "error",
                            json_string("oidc_runtime_unavailable"));
        return api_send_json_response(connection, response,
                                      MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    // --- Retrieve the buffered POST body ---
    ApiPostBuffer *buffer = NULL;
    ApiBufferResult buf_result = api_buffer_post_data(method, upload_data,
                                                      upload_data_size,
                                                      con_cls, &buffer);
    switch (buf_result) {
        case API_BUFFER_CONTINUE: return MHD_YES;
        case API_BUFFER_COMPLETE: break;
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
        return send_bad_request(connection, con_cls, "empty body");
    }

    json_t *request = api_parse_json_body(buffer);
    if (!request) {
        return send_bad_request(connection, con_cls, "invalid json");
    }

    // --- Pull required + optional fields ---
    const char *jwt = json_string_value(json_object_get(request, "jwt"));
    if (!jwt || !*jwt) {
        json_decref(request);
        return send_bad_request(connection, con_cls,
                                "missing or empty 'jwt' field");
    }

    json_t *account_id_j = json_object_get(request, "account_id");
    if (!account_id_j || !json_is_integer(account_id_j)) {
        json_decref(request);
        return send_bad_request(connection, con_cls,
                                "missing or non-integer 'account_id' field");
    }
    int account_id = (int)json_integer_value(account_id_j);

    const char *username =
        json_string_value(json_object_get(request, "username"));
    const char *roles =
        json_string_value(json_object_get(request, "roles"));

    json_t *expires_at_j = json_object_get(request, "expires_at");
    time_t expires_at =
        (expires_at_j && json_is_integer(expires_at_j))
            ? (time_t)json_integer_value(expires_at_j)
            : (time_t)0;

    json_t *ttl_j = json_object_get(request, "ttl_seconds");
    int ttl_seconds =
        (ttl_j && json_is_integer(ttl_j))
            ? (int)json_integer_value(ttl_j)
            : 0;

    // --- Bind to the requesting client's IP (mirrors what /callback
    //     will do in Phase 14) so /handoff IP-bind enforcement can be
    //     exercised end-to-end by the black-box test. ---
    char *client_ip = api_get_client_ip(connection);

    // --- Generate a random handoff code (32 bytes → 64 hex chars) ---
    char *handoff_code = oidc_rp_make_random_hex(32);
    if (!handoff_code) {
        free(client_ip);
        json_decref(request);
        api_free_post_buffer(con_cls);
        log_this(SR_AUTH,
                 "OIDC RP /_inject_handoff: failed to generate handoff code",
                 LOG_LEVEL_ERROR, 0);
        json_t *response = json_object();
        if (!response) return MHD_NO;
        json_object_set_new(response, "error",
                            json_string("internal_error"));
        return api_send_json_response(connection, response,
                                      MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    // --- Insert into the handoff store ---
    bool ok = oidc_rp_handoff_store_put(handoff_code,
                                        jwt,
                                        account_id,
                                        username,
                                        roles,
                                        client_ip,
                                        expires_at,
                                        ttl_seconds);
    json_decref(request);

    if (!ok) {
        free(handoff_code);
        free(client_ip);
        api_free_post_buffer(con_cls);
        log_this(SR_AUTH,
                 "OIDC RP /_inject_handoff: handoff store put failed",
                 LOG_LEVEL_ERROR, 0);
        json_t *response = json_object();
        if (!response) return MHD_NO;
        json_object_set_new(response, "error",
                            json_string("store_put_failed"));
        return api_send_json_response(connection, response,
                                      MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    // --- Build the success envelope ---
    json_t *response = json_object();
    if (!response) {
        free(handoff_code);
        free(client_ip);
        api_free_post_buffer(con_cls);
        return MHD_NO;
    }
    json_object_set_new(response, "handoff", json_string(handoff_code));
    json_object_set_new(response, "client_ip",
                        json_string(client_ip ? client_ip : ""));

    log_this(SR_AUTH,
             "OIDC RP /_inject_handoff: stored handoff for account_id=%d "
             "(client_ip=%s)",
             LOG_LEVEL_DEBUG, 2,
             account_id,
             client_ip ? client_ip : "(unknown)");

    free(handoff_code);
    free(client_ip);
    api_free_post_buffer(con_cls);
    return api_send_json_response(connection, response, MHD_HTTP_OK);
}

#else /* NDEBUG */

// In release builds, leave a small static placeholder so this
// translation unit is not empty (ISO C forbids that under
// `-Wpedantic`). No symbols are exported.
typedef int oidc_rp_debug_inject_compiled_out;

#endif // NDEBUG
