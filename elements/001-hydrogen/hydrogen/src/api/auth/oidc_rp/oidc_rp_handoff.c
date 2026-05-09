/*
 * OIDC RP /handoff endpoint — Phase 6 stub
 *
 * Returns 503 {"error":"oidc_disabled"} when the OIDC RP feature is
 * gated off (the default per Phase 5). Non-POST methods get 405. The
 * real handoff exchange (single-use, IP-bindable, returning a
 * Hydrogen JWT) lands in Phase 13.
 *
 * Note: Phase 6 deliberately does NOT call api_buffer_post_data() or
 * inspect the request body. The feature gate runs first; the real
 * implementation in Phase 13 will buffer and parse the JSON.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/api/api_utils.h>

// Local includes
#include "oidc_rp_handoff.h"
#include "oidc_rp_service.h"

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

    // Method discrimination — /handoff is POST-only
    if (!method || strcmp(method, "POST") != 0) {
        return oidc_rp_send_method_not_allowed(connection, method, "handoff", "POST");
    }

    // Drain any buffered POST body so MHD doesn't keep re-invoking the
    // handler waiting for our verdict. We discard the body — Phase 13
    // will parse it for real.
    if (upload_data_size && *upload_data_size > 0) {
        *upload_data_size = 0;
        return MHD_YES;
    }
    (void)upload_data;
    (void)con_cls;

    // Feature gate — Phase 6 only handles the disabled case
    if (!oidc_rp_is_enabled()) {
        return oidc_rp_send_disabled_response(connection, method, "handoff");
    }

    // Real flow lands in Phase 13. Until then, an "enabled" config is
    // a misconfiguration we should surface explicitly rather than a
    // silent 200.
    log_this(SR_AUTH,
             "OIDC RP /handoff invoked while enabled, but Phase 13 not yet implemented",
             LOG_LEVEL_ERROR, 0);
    json_t *response = json_object();
    if (!response) {
        return MHD_NO;
    }
    json_object_set_new(response, "error", json_string("oidc_not_implemented"));
    return api_send_json_response(connection, response, MHD_HTTP_NOT_IMPLEMENTED);
}
