/*
 * Mock Auth Service JWT functions implementation
 */

#include "mock_auth_service_jwt.h"
#include <string.h>
#include <stdlib.h>

// Static state for mock control
static jwt_validation_result_t mock_validation_result = {false, NULL, JWT_ERROR_NONE};

/*
 * Mock implementation of validate_jwt
 */
jwt_validation_result_t mock_validate_jwt(const char* token, const char* database) {
    (void)token;  // Suppress unused parameter warning
    (void)database;  // Suppress unused parameter warning

    jwt_validation_result_t result = mock_validation_result;

    // If valid result requested, create a copy of claims to avoid sharing state
    if (result.valid && result.claims) {
        result.claims = calloc(1, sizeof(jwt_claims_t));
        if (result.claims) {
            // Copy the mock claims
            jwt_claims_t* mock_claims = mock_validation_result.claims;
            result.claims->iss = mock_claims->iss ? strdup(mock_claims->iss) : NULL;
            result.claims->sub = mock_claims->sub ? strdup(mock_claims->sub) : NULL;
            result.claims->aud = mock_claims->aud ? strdup(mock_claims->aud) : NULL;
            result.claims->exp = mock_claims->exp;
            result.claims->iat = mock_claims->iat;
            result.claims->nbf = mock_claims->nbf;
            result.claims->jti = mock_claims->jti ? strdup(mock_claims->jti) : NULL;
            result.claims->user_id = mock_claims->user_id;
            result.claims->system_id = mock_claims->system_id;
            result.claims->app_id = mock_claims->app_id;
            result.claims->username = mock_claims->username ? strdup(mock_claims->username) : NULL;
            result.claims->email = mock_claims->email ? strdup(mock_claims->email) : NULL;
            result.claims->roles = mock_claims->roles ? strdup(mock_claims->roles) : NULL;
            result.claims->ip = mock_claims->ip ? strdup(mock_claims->ip) : NULL;
            result.claims->tz = mock_claims->tz ? strdup(mock_claims->tz) : NULL;
            result.claims->tzoffset = mock_claims->tzoffset;
            result.claims->database = mock_claims->database ? strdup(mock_claims->database) : NULL;
        }
    }

    return result;
}

/*
 * Mock implementation of free_jwt_validation_result
 */
void mock_free_jwt_validation_result(jwt_validation_result_t* result) {
    if (!result) return;

    if (result->claims) {
        free(result->claims->iss);
        free(result->claims->sub);
        free(result->claims->aud);
        free(result->claims->jti);
        free(result->claims->username);
        free(result->claims->email);
        free(result->claims->roles);
        free(result->claims->ip);
        free(result->claims->tz);
        free(result->claims->database);
        free(result->claims);
        result->claims = NULL;
    }
}

/*
 * Reset all mock state to defaults
 */
void mock_auth_service_jwt_reset_all(void) {
    // Free any existing mock claims
    if (mock_validation_result.claims) {
        mock_free_jwt_validation_result(&mock_validation_result);
    }

    // Reset to default invalid state
    mock_validation_result.valid = false;
    mock_validation_result.claims = NULL;
    mock_validation_result.error = JWT_ERROR_NONE;
}

/*
 * Set the mock validation result
 */
void mock_auth_service_jwt_set_validation_result(jwt_validation_result_t result) {
    // Free any existing mock claims
    if (mock_validation_result.claims) {
        mock_free_jwt_validation_result(&mock_validation_result);
    }

    // Set new result
    mock_validation_result = result;

    // If valid result with claims, we need to store a copy since the input might be freed
    if (result.valid && result.claims) {
        mock_validation_result.claims = calloc(1, sizeof(jwt_claims_t));
        if (mock_validation_result.claims) {
            jwt_claims_t* src = result.claims;
            jwt_claims_t* dst = mock_validation_result.claims;

            dst->iss = src->iss ? strdup(src->iss) : NULL;
            dst->sub = src->sub ? strdup(src->sub) : NULL;
            dst->aud = src->aud ? strdup(src->aud) : NULL;
            dst->exp = src->exp;
            dst->iat = src->iat;
            dst->nbf = src->nbf;
            dst->jti = src->jti ? strdup(src->jti) : NULL;
            dst->user_id = src->user_id;
            dst->system_id = src->system_id;
            dst->app_id = src->app_id;
            dst->username = src->username ? strdup(src->username) : NULL;
            dst->email = src->email ? strdup(src->email) : NULL;
            dst->roles = src->roles ? strdup(src->roles) : NULL;
            dst->ip = src->ip ? strdup(src->ip) : NULL;
            dst->tz = src->tz ? strdup(src->tz) : NULL;
            dst->tzoffset = src->tzoffset;
            dst->database = src->database ? strdup(src->database) : NULL;
        }
    }
}