/*
 * OIDC Relying Party shared service helpers — Phase 6 stubs
 *
 * Provides the `oidc_rp_is_enabled()` gate plus two response helpers
 * used by all three Phase 6 stub handlers:
 *
 *   - `oidc_rp_send_disabled_response()` — 503 + {"error":"oidc_disabled"}
 *   - `oidc_rp_send_method_not_allowed()` — 405 + {"error":"method_not_allowed"}
 *
 * Logic beyond enable-checking and method discrimination lands in
 * later phases (7–22). See `docs/OIDC-PLAN.md` Phase 6 for context.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/api/api_utils.h>

// Local includes
#include "oidc_rp_service.h"

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
