/*
 * OIDC RP /start endpoint — Phase 6 stub
 *
 * Returns 503 {"error":"oidc_disabled"} when the OIDC RP feature is
 * gated off (the default per Phase 5). Non-GET methods get 405. The
 * real redirect builder lands in Phase 10.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/api/api_utils.h>

// Local includes
#include "oidc_rp_start.h"
#include "oidc_rp_service.h"

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

    // Feature gate — Phase 6 only handles the disabled case
    if (!oidc_rp_is_enabled()) {
        return oidc_rp_send_disabled_response(connection, method, "start");
    }

    // Real flow lands in Phase 10. Until then, an "enabled" config is
    // a misconfiguration we should surface explicitly rather than a
    // silent 200.
    log_this(SR_AUTH,
             "OIDC RP /start invoked while enabled, but Phase 10 not yet implemented",
             LOG_LEVEL_ERROR, 0);
    json_t *response = json_object();
    if (!response) {
        return MHD_NO;
    }
    json_object_set_new(response, "error", json_string("oidc_not_implemented"));
    return api_send_json_response(connection, response, MHD_HTTP_NOT_IMPLEMENTED);
}
