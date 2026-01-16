/*
 * renew_utils.h - Helper functions for JWT token renewal
 * 
 * This header declares utility functions for the renew endpoint that can be
 * tested independently to improve code coverage and maintainability.
 */

#ifndef RENEW_UTILS_H
#define RENEW_UTILS_H

#include <src/hydrogen.h>
#include <src/api/auth/auth_service.h>
#include <microhttpd.h>

/*
 * Extracts the JWT token from the Authorization header
 * 
 * @param connection The HTTP connection
 * @return Extracted token string (must be freed by caller), or NULL if invalid/missing
 */
char* extract_token_from_authorization_header(struct MHD_Connection *connection);

/*
 * Extracts the database name from the request body or JWT claims
 * 
 * @param request JSON request body (may be NULL)
 * @param claims JWT claims (may be NULL)
 * @return Database name string (must be freed by caller), or NULL if not found
 */
char* extract_database_from_request_or_claims_renew(json_t *request, const jwt_claims_t *claims);

/*
 * Creates an error response for JWT validation failures
 * 
 * @param error_msg Error message to include in response
 * @return JSON error response object (must be freed by caller)
 */
json_t* create_renew_error_response(const char* error_msg);

/*
 * Creates a success response with the new JWT token
 * 
 * @param new_token The new JWT token
 * @param expires_at Expiration timestamp
 * @return JSON success response object (must be freed by caller)
 */
json_t* create_renew_success_response(const char* new_token, time_t expires_at);

/*
 * Maps JWT error codes to user-friendly error messages
 * 
 * @param error JWT error code
 * @return User-friendly error message string
 */
const char* get_jwt_validation_error_message_renew(jwt_error_t error);

/*
 * Validates the JWT token and extracts claims
 * 
 * @param token JWT token to validate
 * @param database Database name for validation
 * @param validation_result Pointer to store validation result
 * @return true if validation succeeded, false otherwise
 */
bool validate_token_and_extract_claims(const char* token, const char* database, jwt_validation_result_t* validation_result);

#endif /* RENEW_UTILS_H */