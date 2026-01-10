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

#endif // REGISTER_H
