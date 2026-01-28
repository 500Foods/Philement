/*
 * JWT Validation Helper for Conduit Auth Endpoints
 *
 * This module provides JWT validation helper functions that can be tested
 * independently of the HTTP request handling.
 */

#ifndef CONDUIT_AUTH_JWT_HELPER_H
#define CONDUIT_AUTH_JWT_HELPER_H

#include <stdbool.h>
#include <microhttpd.h>
#include <src/api/auth/auth_service.h>

/**
 * @brief Extract and validate JWT from Authorization header
 *
 * This helper function performs the JWT extraction and validation
 * without directly handling HTTP responses. It returns validation
 * results that the caller can use to determine appropriate responses.
 *
 * @param auth_header The Authorization header value (may be NULL)
 * @param jwt_result Output parameter for JWT validation result
 * @return true on successful validation, false on error
 */
bool extract_and_validate_jwt(const char* auth_header, jwt_validation_result_t* jwt_result);

/**
 * @brief Get the error message for a JWT error code
 *
 * @param error The JWT error code
 * @return Human-readable error message
 */
const char* get_jwt_error_message(jwt_error_t error);

#endif /* CONDUIT_AUTH_JWT_HELPER_H */
