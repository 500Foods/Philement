/*
 * renew_utils.c - Helper functions for JWT token renewal
 * 
 * This file implements utility functions for the renew endpoint that can be
 * tested independently to improve code coverage and maintainability.
 */

#include <src/hydrogen.h>
#include <src/api/auth/renew/renew_utils.h>
#include <src/api/auth/auth_service.h>
#include <string.h>

char* extract_token_from_authorization_header(struct MHD_Connection *connection) {
    // Extract Authorization header
    const char* auth_header = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Authorization");
    if (!auth_header) {
        log_this(SR_AUTH, "Missing Authorization header in renew request", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // Validate Bearer token format
    const char* bearer_prefix = "Bearer ";
    size_t prefix_len = strlen(bearer_prefix);
    if (strncmp(auth_header, bearer_prefix, prefix_len) != 0) {
        log_this(SR_AUTH, "Invalid Authorization header format in renew request", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // Extract token (skip "Bearer " prefix)
    const char* token = auth_header + prefix_len;
    if (strlen(token) == 0) {
        log_this(SR_AUTH, "Empty token in Authorization header", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    log_this(SR_AUTH, "Authorization header present and valid format for auth/renew", LOG_LEVEL_DEBUG, 0);
    return strdup(token);
}

char* extract_database_from_request_or_claims_renew(json_t *request, const jwt_claims_t *claims) {
    const char* database = NULL;

    // First try to get database from request body
    if (request) {
        json_t* db_value = json_object_get(request, "database");
        if (db_value && json_is_string(db_value)) {
            database = json_string_value(db_value);
        }
    }

    // If not in request, try to get from JWT claims
    if (!database && claims && claims->database) {
        database = claims->database;
        log_this(SR_AUTH, "Using database from JWT claims: %s", LOG_LEVEL_DEBUG, 1, database);
    }

    if (!database) {
        log_this(SR_AUTH, "No database specified in request or JWT claims", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    return strdup(database);
}

json_t* create_renew_error_response(const char* error_msg) {
    json_t* response = json_object();
    if (!response) return NULL;
    
    json_object_set_new(response, "success", json_false());
    json_object_set_new(response, "error", json_string(error_msg ? error_msg : ""));
    return response;
}

json_t* create_renew_success_response(const char* new_token, time_t expires_at) {
    json_t* response = json_object();
    if (!response) return NULL;
    
    json_object_set_new(response, "success", json_true());
    json_object_set_new(response, "token", json_string(new_token ? new_token : ""));
    json_object_set_new(response, "expires_at", json_integer(expires_at));
    return response;
}

const char* get_jwt_validation_error_message_renew(jwt_error_t error) {
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
            return "Token has been revoked";
        default:
            return "Invalid or expired token";
    }
}

bool validate_token_and_extract_claims(const char* token, const char* database, jwt_validation_result_t* validation_result) {
    if (!validation_result) return false;
    
    jwt_validation_result_t validation = validate_jwt_token(token, database);
    
    if (!validation.valid) {
        const char* error_msg = get_jwt_validation_error_message_renew(validation.error);
        log_this(SR_AUTH, "JWT validation failed: %s", LOG_LEVEL_ALERT, 1, error_msg);
        return false;
    }
    
    // Check that claims were parsed successfully
    if (!validation.claims) {
        log_this(SR_AUTH, "JWT validation succeeded but claims are NULL", LOG_LEVEL_ERROR, 0);
        return false;
    }
    
    // Shallow copy validation result - claims pointer will be copied
    *validation_result = validation;
    
    return true;
}