/*
 * JWT Validation Helper Implementation
 */

#include <src/hydrogen.h>
#include <string.h>
#include <jansson.h>
#include <microhttpd.h>
#include "auth_jwt_helper.h"

const char* get_jwt_error_message(jwt_error_t error) {
    switch (error) {
        case JWT_ERROR_EXPIRED:
            return "JWT token has expired";
        case JWT_ERROR_REVOKED:
            return "JWT token has been revoked";
        case JWT_ERROR_INVALID_SIGNATURE:
            return "Invalid JWT signature";
        case JWT_ERROR_INVALID_FORMAT:
            return "Invalid JWT format";
        case JWT_ERROR_NOT_YET_VALID:
            return "JWT token not yet valid";
        case JWT_ERROR_UNSUPPORTED_ALGORITHM:
            return "Unsupported JWT algorithm";
        case JWT_ERROR_NONE:
        default:
            return "Invalid or expired JWT token";
    }
}

bool extract_and_validate_jwt(const char* auth_header, jwt_validation_result_t* jwt_result) {
    if (!jwt_result) {
        return false;
    }

    // Initialize result
    memset(jwt_result, 0, sizeof(jwt_validation_result_t));

    // Check for Authorization header
    if (!auth_header) {
        jwt_result->valid = false;
        jwt_result->error = JWT_ERROR_INVALID_FORMAT;
        return false;
    }

    // Check for "Bearer " prefix
    if (strncmp(auth_header, "Bearer ", 7) != 0) {
        jwt_result->valid = false;
        jwt_result->error = JWT_ERROR_INVALID_FORMAT;
        return false;
    }

    // Extract token (skip "Bearer " prefix)
    const char *token = auth_header + 7;

    // Validate JWT token
    jwt_validation_result_t result = validate_jwt(token, NULL);
    
    if (!result.valid) {
        jwt_result->valid = false;
        jwt_result->error = result.error;
        if (result.claims) {
            free_jwt_claims(result.claims);
        }
        return false;
    }

    // Extract database from JWT claims
    if (!result.claims) {
        jwt_result->valid = false;
        jwt_result->error = JWT_ERROR_INVALID_FORMAT;
        return false;
    }

    if (!result.claims->database || strlen(result.claims->database) == 0) {
        jwt_result->valid = false;
        jwt_result->error = JWT_ERROR_INVALID_FORMAT;
        free_jwt_claims(result.claims);
        return false;
    }

    // Success - copy the result
    *jwt_result = result;
    return true;
}

enum MHD_Result send_jwt_error_response(struct MHD_Connection *connection, const char* error_msg, unsigned int http_status) {
    json_t *error_response = json_object();
    if (!error_response) {
        return MHD_NO;
    }
    json_object_set_new(error_response, "success", json_false());
    json_object_set_new(error_response, "error", json_string(error_msg));
    char *response_str = json_dumps(error_response, JSON_COMPACT);
    json_decref(error_response);

    if (!response_str) {
        return MHD_NO;
    }

    struct MHD_Response *response = MHD_create_response_from_buffer(
        strlen(response_str), response_str, MHD_RESPMEM_MUST_FREE);
    if (!response) {
        free(response_str);
        return MHD_NO;
    }
    MHD_add_response_header(response, "Content-Type", "application/json");
    MHD_queue_response(connection, http_status, response);
    MHD_destroy_response(response);
    return MHD_NO;
}

enum MHD_Result send_missing_authorization_response(struct MHD_Connection *connection) {
    return send_jwt_error_response(connection, "Authentication required - include Authorization: Bearer <token> header", MHD_HTTP_UNAUTHORIZED);
}

enum MHD_Result send_invalid_authorization_format_response(struct MHD_Connection *connection) {
    return send_jwt_error_response(connection, "Invalid Authorization header - expected 'Bearer <token>' format", MHD_HTTP_UNAUTHORIZED);
}

enum MHD_Result send_internal_server_error_response(struct MHD_Connection *connection) {
    return send_jwt_error_response(connection, "Internal server error", MHD_HTTP_INTERNAL_SERVER_ERROR);
}

bool validate_jwt_claims(jwt_validation_result_t* jwt_result, struct MHD_Connection *connection) {
    if (!jwt_result || !jwt_result->valid) {
        return false;
    }
    
    // Check for claims
    if (!jwt_result->claims) {
        send_jwt_error_response(connection, "JWT token missing claims", MHD_HTTP_UNAUTHORIZED);
        return false;
    }
    
    // Check for database claim
    if (!jwt_result->claims->database) {
        free_jwt_claims(jwt_result->claims);
        jwt_result->claims = NULL;
        send_jwt_error_response(connection, "JWT token missing database claim", MHD_HTTP_UNAUTHORIZED);
        return false;
    }
    
    // Check for empty database
    if (strlen(jwt_result->claims->database) == 0) {
        free_jwt_claims(jwt_result->claims);
        jwt_result->claims = NULL;
        send_jwt_error_response(connection, "JWT token has empty database", MHD_HTTP_UNAUTHORIZED);
        return false;
    }
    
    return true;
}
