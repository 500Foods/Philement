/**
 * @file logout.h
 * @brief JWT Token Logout Endpoint
 *
 * This module provides the logout endpoint for the Hydrogen authentication system.
 * It invalidates JWT tokens by removing them from the active token storage.
 *
 * @author Hydrogen Framework
 * @date 2026-01-10
 */

#ifndef LOGOUT_H
#define LOGOUT_H

#include <src/hydrogen.h>
#include <microhttpd.h>

/**
 * @brief Handle POST /api/auth/logout requests
 *
 * Endpoint for logging out and invalidating JWT tokens. Accepts a JWT token
 * (even if expired) and removes it from active storage to prevent further use.
 * This ensures tokens cannot be reused after logout, even if not yet expired.
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
//@ swagger:path /api/auth/logout
//@ swagger:method POST
//@ swagger:operationId postAuthLogout
//@ swagger:tags "Auth Service"
//@ swagger:summary Logout and invalidate JWT token
//@ swagger:description Invalidates a JWT token by removing it from storage
//@ swagger:security bearerAuth
//@ swagger:request body application/json {"type":"object","required":["token"],"properties":{"token":{"type":"string","description":"JWT token to invalidate","example":"eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."}}}
//@ swagger:response 200 application/json {"type":"object","properties":{"success":{"type":"boolean","example":true},"message":{"type":"string","example":"Logout successful"}}}
//@ swagger:response 400 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"Missing or invalid token"}}}
//@ swagger:response 401 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"Invalid token"}}}
//@ swagger:response 500 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"Internal server error"}}}
enum MHD_Result handle_post_auth_logout(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *version,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls
);

#endif // LOGOUT_H
