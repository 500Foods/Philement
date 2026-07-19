/*
 * Mock Auth Service JWT functions implementation
 */

#include <src/globals.h>
#include "mock_auth_service_jwt.h"
#include <string.h>
#include <stdlib.h>

// Static state for mock control
static jwt_validation_result_t mock_validation_result = {false, NULL, JWT_ERROR_NONE};

/*
 * Mock implementation of validate_jwt
 */
static jwt_claims_t* mock_copy_claims(const jwt_claims_t* mock_claims) {
    if (!mock_claims) {
        return NULL;
    }

    jwt_claims_t* claims = calloc(1, sizeof(jwt_claims_t));
    if (!claims) {
        return NULL;
    }

    claims->iss = mock_claims->iss ? strdup(mock_claims->iss) : NULL;
    claims->sub = mock_claims->sub ? strdup(mock_claims->sub) : NULL;
    claims->aud = mock_claims->aud ? strdup(mock_claims->aud) : NULL;
    claims->exp = mock_claims->exp;
    claims->iat = mock_claims->iat;
    claims->nbf = mock_claims->nbf;
    claims->jti = mock_claims->jti ? strdup(mock_claims->jti) : NULL;
    claims->user_id = mock_claims->user_id;
    claims->system_id = mock_claims->system_id;
    claims->app_id = mock_claims->app_id;
    claims->username = mock_claims->username ? strdup(mock_claims->username) : NULL;
    claims->email = mock_claims->email ? strdup(mock_claims->email) : NULL;
    claims->roles = mock_claims->roles ? strdup(mock_claims->roles) : NULL;
    claims->ip = mock_claims->ip ? strdup(mock_claims->ip) : NULL;
    claims->tz = mock_claims->tz ? strdup(mock_claims->tz) : NULL;
    claims->tzoffset = mock_claims->tzoffset;
    claims->database = mock_claims->database ? strdup(mock_claims->database) : NULL;
    claims->id_token = mock_claims->id_token ? strdup(mock_claims->id_token) : NULL;
    claims->idp_provider = mock_claims->idp_provider ? strdup(mock_claims->idp_provider) : NULL;
    return claims;
}

jwt_validation_result_t mock_validate_jwt(const char* token, const char* database) {
    (void)token;  // Suppress unused parameter warning
    (void)database;  // Suppress unused parameter warning

    jwt_validation_result_t result = mock_validation_result;

    // Always deep-copy claims so callers can free_jwt_validation_result safely
    // (covers both valid and invalid-with-claims cases).
    if (result.claims) {
        result.claims = mock_copy_claims(mock_validation_result.claims);
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
        free(result->claims->id_token);
        free(result->claims->idp_provider);
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

    // Set new result and deep-copy claims (valid or invalid-with-claims)
    mock_validation_result = result;
    mock_validation_result.claims = NULL;
    if (result.claims) {
        mock_validation_result.claims = mock_copy_claims(result.claims);
    }
}