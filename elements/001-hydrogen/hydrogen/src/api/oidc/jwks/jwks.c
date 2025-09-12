/*
 * OIDC JWKS Endpoint Implementation
 * 
 * Implements the JSON Web Key Set endpoint (/oauth/jwks)
 */

  // Global includes 
#include "../../../hydrogen.h"

// Local includes
#include "jwks.h"
#include "../oidc_service.h"
#include "../../../oidc/oidc_service.h"

/**
 * JWKS endpoint
 * 
 * @param connection The MHD connection
 * @return MHD_Result indicating success or failure
 */
enum MHD_Result handle_oidc_jwks_endpoint(struct MHD_Connection *connection) {
    log_this(SR_OIDC, "Handling JWKS endpoint request", LOG_LEVEL_STATE, 0);
    
    // Generate JWKS document
    char *jwks = oidc_generate_jwks_document();
    
    if (!jwks) {
        return send_oidc_json_response(connection, 
                                   "{\"error\":\"server_error\",\"error_description\":\"Failed to generate JWKS\"}",
                                   MHD_HTTP_INTERNAL_SERVER_ERROR);
    }
    
    enum MHD_Result ret = send_oidc_json_response(connection, jwks, MHD_HTTP_OK);
    free(jwks);
    
    return ret;
}
