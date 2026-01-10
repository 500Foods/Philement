/**
 * @file renew.h
 * @brief JWT Token Renewal Endpoint
 *
 * This module provides the token renewal endpoint for the Hydrogen authentication system.
 * It allows users to refresh their JWT tokens before expiration without re-authenticating.
 *
 * @author Hydrogen Framework
 * @date 2026-01-10
 */

#ifndef RENEW_H
#define RENEW_H

#include <src/hydrogen.h>
#include <microhttpd.h>

/**
 * @brief Handle POST /api/auth/renew requests
 *
 * Endpoint for renewing JWT tokens. Accepts a valid JWT token and issues a new token
 * with updated expiration timestamp. The old token is invalidated upon successful renewal.
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
//@ swagger:path /api/auth/renew
//@ swagger:method POST
//@ swagger:operationId postAuthRenew
//@ swagger:tags "Auth Service"
//@ swagger:summary Renew JWT token
//@ swagger:description Renews a valid JWT token with updated expiration timestamp
//@ swagger:security bearerAuth
//@ swagger:request body application/json {"type":"object","required":["token"],"properties":{"token":{"type":"string","description":"JWT token to renew","example":"eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."}}}
//@ swagger:response 200 application/json {"type":"object","properties":{"success":{"type":"boolean","example":true},"token":{"type":"string","description":"New JWT token","example":"eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."},"expires_at":{"type":"integer","description":"Unix timestamp of expiration","example":1704830000}}}
//@ swagger:response 400 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"Missing or invalid token"}}}
//@ swagger:response 401 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"Invalid or expired token"}}}
//@ swagger:response 500 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"Internal server error"}}}
enum MHD_Result handle_post_auth_renew(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *version,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls
);

#endif // RENEW_H
