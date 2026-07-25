/**
 * @file register.h
 * @brief User Registration Endpoint
 *
 * This module provides the registration endpoint for the Hydrogen authentication system.
 * It allows new users to create accounts with username, password, email, and optional full name.
 *
 * @author Hydrogen Framework
 * @date 2026-01-10
 */

#ifndef REGISTER_H
#define REGISTER_H

#include <src/hydrogen.h>
#include <microhttpd.h>

/**
 * @brief Handle POST /api/auth/register requests
 *
 * Endpoint for user registration. Creates a new account after validating input parameters,
 * checking username/email availability, verifying API key, and hashing the password securely.
 *
 * @param connection The HTTP connection
 * @param url The request URL
 * @param method The HTTP method
 * @param version The HTTP version
 * @param upload_data Request body data
 * @param upload_data_size Size of request body
 * @param con_cls Connection-specific data
 * @return HTTP response code
 */
//@ swagger:path /api/auth/register
//@ swagger:method POST
//@ swagger:operationId postAuthRegister
//@ swagger:tags "Auth Service"
//@ swagger:summary User registration endpoint
//@ swagger:description Registers a new user account with comprehensive validation
//@ swagger:request body application/json {"type":"object","required":["username","password","email","api_key"],"properties":{"username":{"type":"string","minLength":3,"maxLength":50,"description":"Username (3-50 chars, alphanumeric with underscore/hyphen)","example":"john_doe"},"password":{"type":"string","minLength":8,"maxLength":128,"description":"Password (8-128 chars)","example":"SecurePass123!"},"email":{"type":"string","maxLength":255,"format":"email","description":"Email address","example":"john@example.com"},"full_name":{"type":"string","maxLength":255,"description":"Full name (optional)","example":"John Doe"},"api_key":{"type":"string","description":"API key for license validation","example":"abc123"}}}
//@ swagger:response 201 application/json {"type":"object","properties":{"success":{"type":"boolean","example":true},"message":{"type":"string","example":"Account created successfully"},"account_id":{"type":"integer","example":123},"username":{"type":"string","example":"john_doe"},"email":{"type":"string","example":"john@example.com"}}}
//@ swagger:response 400 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"Invalid input parameters"}}}
//@ swagger:response 401 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"Invalid API key"}}}
//@ swagger:response 409 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"Username or email already exists"}}}
//@ swagger:response 500 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"Internal server error"}}}
enum MHD_Result handle_post_auth_register(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *version,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls
);

/**
 * @brief Handle error response for registration failures
 *
 * Builds a JSON error response, optionally frees the request object,
 * and sends the response with the given HTTP status code.
 *
 * @param connection The HTTP connection
 * @param con_cls Connection-specific data (unused)
 * @param error_message Error message for the JSON response
 * @param http_status HTTP status code
 * @param request JSON request object to free (may be NULL)
 * @return MHD_Result indicating success or failure
 */
enum MHD_Result handle_register_error(
    struct MHD_Connection *connection,
    void **con_cls,
    const char *error_message,
    unsigned int http_status,
    json_t *request
);

/**
 * @brief Extract and validate registration parameters from JSON request
 *
 * Extracts username, password, email, full_name, api_key, and database
 * from the JSON request object and validates that all required fields
 * are present.
 *
 * @param request JSON request object
 * @param username Output: username string (or NULL if missing)
 * @param password Output: password string (or NULL if missing)
 * @param email Output: email string (or NULL if missing)
 * @param full_name Output: full name string (or NULL if missing/optional)
 * @param api_key Output: API key string (or NULL if missing)
 * @param database Output: database string (or NULL if missing)
 * @return true if all required parameters are present, false otherwise
 */
bool extract_and_validate_parameters(
    json_t *request,
    const char **username,
    const char **password,
    const char **email,
    const char **full_name,
    const char **api_key,
    const char **database
);

#endif // REGISTER_H
