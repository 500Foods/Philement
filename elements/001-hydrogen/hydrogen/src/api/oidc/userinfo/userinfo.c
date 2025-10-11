/*
 * OIDC UserInfo Endpoint Implementation
 * 
 * Implements the OpenID Connect UserInfo endpoint (/oauth/userinfo)
 */

  // Global includes 
#include <src/hydrogen.h>

// Local includes
#include "userinfo.h"
#include "../oidc_service.h"
#include "../../../oidc/oidc_service.h"

/**
 * UserInfo endpoint
 * 
 * @param connection The MHD connection
 * @param method The HTTP method
 * @return MHD_Result indicating success or failure
 */
enum MHD_Result handle_oidc_userinfo_endpoint(struct MHD_Connection *connection,
                                       const char *method) {
    /* Mark unused parameters */
    (void)method;
    
    log_this(SR_OIDC, "Handling userinfo endpoint request", LOG_LEVEL_STATE, 0);
    
    // Extract access token from Authorization header
    const char *auth_header = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Authorization");
    if (!auth_header || strncmp(auth_header, "Bearer ", 7) != 0) {
        return send_oidc_json_response(connection, 
                                   "{\"error\":\"invalid_token\",\"error_description\":\"Missing or invalid access token\"}",
                                   MHD_HTTP_UNAUTHORIZED);
    }
    
    const char *access_token = auth_header + 7;  // Skip "Bearer "
    
    // Process the userinfo request
    char *userinfo = oidc_process_userinfo_request(access_token);
    
    if (!userinfo) {
        return send_oidc_json_response(connection, 
                                   "{\"error\":\"invalid_token\",\"error_description\":\"Invalid access token\"}",
                                   MHD_HTTP_UNAUTHORIZED);
    }
    
    enum MHD_Result ret = send_oidc_json_response(connection, userinfo, MHD_HTTP_OK);
    free(userinfo);
    
    return ret;
}
