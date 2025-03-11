/*
 * OIDC JWKS Endpoint Implementation
 * 
 * Implements the JSON Web Key Set endpoint (/oauth/jwks)
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "jwks.h"
#include "../oidc_service.h"
#include "../../../logging/logging.h"
#include "../../../oidc/oidc_service.h"

/**
 * JWKS endpoint
 * 
 * @param connection The MHD connection
 * @return MHD_Result indicating success or failure
 */
enum MHD_Result handle_oidc_jwks_endpoint(struct MHD_Connection *connection) {
    log_this("OIDC JWKS", "Handling JWKS endpoint request", LOG_LEVEL_STATE);
    
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