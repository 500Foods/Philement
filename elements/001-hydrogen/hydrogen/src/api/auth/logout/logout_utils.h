/*
 * Auth Logout Utility Functions Header
 *
 * Declarations for helper functions used in logout endpoint
 */

#ifndef LOGOUT_UTILS_H
#define LOGOUT_UTILS_H

#include <jansson.h>
#include <src/api/api_utils.h>
#include <src/api/auth/auth_service.h>

// Create error response for logout failures
json_t* create_logout_error_response(const char* error_msg);

// Create success response for logout
json_t* create_logout_success_response(void);

// Extract database from request body or JWT claims
const char* extract_database_from_request_or_claims(
    ApiPostBuffer* buffer,
    const jwt_claims_t* claims,
    json_t** request_out
);

// Get error message for JWT validation error
const char* get_jwt_validation_error_message(jwt_error_t error);

#endif // LOGOUT_UTILS_H