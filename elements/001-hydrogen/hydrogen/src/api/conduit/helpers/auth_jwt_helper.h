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

/**
 * @brief Validate JWT claims and send appropriate error responses
 *
 * Validates that the JWT claims are present and contain a valid database.
 * Sends HTTP error responses if validation fails.
 *
 * @param jwt_result The JWT validation result containing claims
 * @param connection The MHD connection for sending error responses
 * @return true if claims are valid, false if error response was sent
 */
bool validate_jwt_claims(jwt_validation_result_t* jwt_result, struct MHD_Connection *connection);

/**
 * @brief Send JWT error response
 *
 * Helper function to send a standardized JWT error response.
 *
 * @param connection The MHD connection
 * @param error_msg The error message to send
 * @param http_status The HTTP status code
 * @return MHD_NO on completion
 */
enum MHD_Result send_jwt_error_response(struct MHD_Connection *connection, const char* error_msg, unsigned int http_status);

/**
 * @brief Send missing Authorization header error response
 *
 * Helper function to send an error response when Authorization header is missing.
 *
 * @param connection The MHD connection
 * @return MHD_NO on completion
 */
enum MHD_Result send_missing_authorization_response(struct MHD_Connection *connection);

/**
 * @brief Send invalid Authorization header format error response
 *
 * Helper function to send an error response when Authorization header has invalid format.
 *
 * @param connection The MHD connection
 * @return MHD_NO on completion
 */
enum MHD_Result send_invalid_authorization_format_response(struct MHD_Connection *connection);

/**
 * @brief Send internal server error response
 *
 * Helper function to send an internal server error response.
 *
 * @param connection The MHD connection
 * @return MHD_NO on completion
 */
enum MHD_Result send_internal_server_error_response(struct MHD_Connection *connection);

#endif /* CONDUIT_AUTH_JWT_HELPER_H */
