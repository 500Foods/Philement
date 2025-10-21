/*
 * OIDC Discovery Endpoint Implementation
 * 
 * Implements the OpenID Connect discovery document endpoint 
 * (.well-known/openid-configuration)
 */

// Project includes 
#include <src/hydrogen.h>
#include <src/api/oidc/oidc_service.h>

// Local includes
#include "discovery.h"

/**
 * Discovery document endpoint
 * 
 * @param connection The MHD connection
 * @return MHD_Result indicating success or failure
 */
enum MHD_Result handle_oidc_discovery_endpoint(struct MHD_Connection *connection) {
    log_this(SR_OIDC, "Handling discovery endpoint request", LOG_LEVEL_STATE, 0);
    
    // Create a minimal discovery document
    const char *discovery_json = "{"
        "\"issuer\":\"https://example.com\","
        "\"authorization_endpoint\":\"https://example.com/oauth/authorize\","
        "\"token_endpoint\":\"https://example.com/oauth/token\","
        "\"userinfo_endpoint\":\"https://example.com/oauth/userinfo\","
        "\"jwks_uri\":\"https://example.com/oauth/jwks\","
        "\"response_types_supported\":[\"code\",\"token\",\"id_token\"],"
        "\"subject_types_supported\":[\"public\"],"
        "\"id_token_signing_alg_values_supported\":[\"RS256\"]"
        "}";
    
    return send_oidc_json_response(connection, discovery_json, MHD_HTTP_OK);
}
