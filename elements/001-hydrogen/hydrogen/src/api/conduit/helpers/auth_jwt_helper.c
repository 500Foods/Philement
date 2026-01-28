/*
 * JWT Validation Helper Implementation
 */

#include <src/hydrogen.h>
#include <string.h>
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
            free(result.claims);
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
        if (result.claims) {
            free_jwt_claims(result.claims);
            free(result.claims);
        }
        return false;
    }

    // Success - copy the result
    *jwt_result = result;
    return true;
}
