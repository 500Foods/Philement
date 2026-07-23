/*
 * OIDC Discovery Endpoint Implementation
 *
 * Implements the OpenID Connect discovery document endpoint
 * (.well-known/openid-configuration)
 */

#include <src/hydrogen.h>
#include <src/api/oidc/oidc_service.h>
#include <src/oidc/oidc_service.h>

#include "discovery.h"

enum MHD_Result handle_oidc_discovery_endpoint(struct MHD_Connection *connection) {
    log_this(SR_OIDC, "Handling discovery endpoint request", LOG_LEVEL_DEBUG, 0);

    char *discovery_json = oidc_generate_discovery_document();
    if (!discovery_json) {
        return send_oidc_json_response(connection,
            "{\"error\":\"server_error\",\"error_description\":\"Failed to generate discovery document\"}",
            MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    enum MHD_Result ret = send_oidc_json_response(connection, discovery_json, MHD_HTTP_OK);
    free(discovery_json);
    return ret;
}
