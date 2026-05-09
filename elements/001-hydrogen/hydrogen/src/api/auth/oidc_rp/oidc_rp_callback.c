/*
 * OIDC RP /callback endpoint — Phase 6 stub
 *
 * Returns 503 {"error":"oidc_disabled"} when the OIDC RP feature is
 * gated off (the default per Phase 5). Non-GET methods get 405. The
 * real callback chain (state lookup, code exchange, ID-token
 * validation, linker, JWT mint, handoff store, SPA redirect) lands
 * across Phases 7–22 and is finalised in Phase 14.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/api/api_utils.h>

// Local includes
#include "oidc_rp_callback.h"
#include "oidc_rp_service.h"

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

    // Method discrimination — /callback is GET-only
    if (!method || strcmp(method, "GET") != 0) {
        return oidc_rp_send_method_not_allowed(connection, method, "callback", "GET");
    }

    // Feature gate — Phase 6 only handles the disabled case
    if (!oidc_rp_is_enabled()) {
        return oidc_rp_send_disabled_response(connection, method, "callback");
    }

    // Real flow lands in Phase 14. Until then, an "enabled" config is
    // a misconfiguration we should surface explicitly rather than a
    // silent 200.
    log_this(SR_AUTH,
             "OIDC RP /callback invoked while enabled, but Phase 14 not yet implemented",
             LOG_LEVEL_ERROR, 0);
    json_t *response = json_object();
    if (!response) {
        return MHD_NO;
    }
    json_object_set_new(response, "error", json_string("oidc_not_implemented"));
    return api_send_json_response(connection, response, MHD_HTTP_NOT_IMPLEMENTED);
}
