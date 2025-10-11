/*
 * OIDC Registration Endpoint Implementation
 * 
 * Implements the client registration endpoint (/oauth/register)
 */

  // Global includes 
#include <src/hydrogen.h>

// Local includes
#include "registration.h"
#include "../oidc_service.h"
#include "../../../oidc/oidc_service.h"

/**
 * Client registration endpoint
 * 
 * @param connection The MHD connection
 * @param method The HTTP method
 * @param upload_data Upload data
 * @param upload_data_size Size of upload data
 * @param con_cls Connection-specific data
 * @return MHD_Result indicating success or failure
 */
enum MHD_Result handle_oidc_registration_endpoint(struct MHD_Connection *connection,
                                           const char *method,
                                           const char *upload_data,
                                           size_t *upload_data_size,
                                           void **con_cls) {
    /* Mark unused parameters */
    (void)method;
    (void)upload_data;
    (void)upload_data_size;
    (void)con_cls;
    
    log_this(SR_OIDC, "Handling registration endpoint request", LOG_LEVEL_STATE, 0);
    
    // This is a stub implementation that always returns a "Not Implemented" response
    return send_oidc_json_response(connection, 
                               "{\"error\":\"not_implemented\",\"error_description\":\"Client registration not implemented\"}",
                               MHD_HTTP_NOT_IMPLEMENTED);
}
