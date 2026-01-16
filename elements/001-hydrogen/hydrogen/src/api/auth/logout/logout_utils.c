/*
 * Auth Logout Utility Functions
 *
 * Helper functions for logout endpoint to improve testability and reduce code duplication
 */

// Project includes
#include <src/hydrogen.h>
#include <src/api/api_utils.h>
#include <src/api/auth/auth_service.h>

// Local includes
#include "logout.h"
#include "logout_utils.h"

// Create error response for logout failures
json_t* create_logout_error_response(const char* error_msg) {
    json_t* response = json_object();
    if (!response) return NULL;

    json_object_set_new(response, "success", json_false());
    json_object_set_new(response, "error", json_string(error_msg));
    return response;
}

// Create success response for logout
json_t* create_logout_success_response(void) {
    json_t* response = json_object();
    if (!response) return NULL;

    json_object_set_new(response, "success", json_true());
    json_object_set_new(response, "message", json_string("Logout successful"));
    return response;
}

// Extract database from request body or JWT claims
const char* extract_database_from_request_or_claims(
    ApiPostBuffer* buffer,
    const jwt_claims_t* claims,
    json_t** request_out
) {
    const char* database = NULL;
    json_t* request = NULL;

    // Try to parse database from request body first
    if (buffer && buffer->http_method == 'P' && buffer->data && buffer->size > 0) {
        request = api_parse_json_body(buffer);
        if (request) {
            database = json_string_value(json_object_get(request, "database"));
        }
    }

    // If no database in request, try JWT claims
    if (!database && claims && claims->database) {
        database = claims->database;
        log_this(SR_AUTH, "Using database from JWT claims: %s", LOG_LEVEL_DEBUG, 1, database);
    }

    if (request_out) {
        *request_out = request;
    } else if (request) {
        json_decref(request);
    }

    return database;
}

// Get error message for JWT validation error
const char* get_jwt_validation_error_message(jwt_error_t error) {
    switch (error) {
        case JWT_ERROR_NONE:
            return "Unknown error";
        case JWT_ERROR_EXPIRED:
            return "Token has expired";
        case JWT_ERROR_NOT_YET_VALID:
            return "Token not yet valid";
        case JWT_ERROR_INVALID_SIGNATURE:
            return "Invalid token signature";
        case JWT_ERROR_UNSUPPORTED_ALGORITHM:
            return "Unsupported token algorithm";
        case JWT_ERROR_INVALID_FORMAT:
            return "Invalid token format";
        case JWT_ERROR_REVOKED:
            return "Token already revoked";
        default:
            return "Invalid token";
    }
}