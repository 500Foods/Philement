/*
 * Auth Login API endpoint for the Hydrogen Project.
 * Provides user authentication with username/password and returns JWT.
 */

#ifndef HYDROGEN_AUTH_LOGIN_H
#define HYDROGEN_AUTH_LOGIN_H

// Network headers
#include <microhttpd.h>

// Standard C headers
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Third-party libraries
#include <jansson.h>

/**
 * Handles the /api/auth/login endpoint request.
 * Authenticates users via username/email and password, validates API key,
 * performs security checks (IP whitelist/blacklist, rate limiting),
 * and returns a JWT token upon successful authentication.
 *
 * @param connection The MHD_Connection to send the response through
 * @param upload_data Pointer to upload data for POST requests
 * @param upload_data_size Size of the upload data
 * @return MHD_Result indicating success or failure
 */
//@ swagger:path /api/auth/login
//@ swagger:method POST
//@ swagger:operationId authLogin
//@ swagger:tags "Auth Service"
//@ swagger:summary User login with username/password
//@ swagger:description Authenticates users via username/email and password, validates API key, performs security checks (IP whitelist/blacklist, rate limiting), and returns a JWT token upon successful authentication.
//@ swagger:request body application/json {"type":"object","required":["login_id","password","api_key","tz","database"],"properties":{"login_id":{"type":"string","minLength":1,"maxLength":255,"description":"Username or email address for authentication","example":"testuser"},"password":{"type":"string","minLength":8,"maxLength":128,"description":"User password","example":"usertest"},"api_key":{"type":"string","minLength":1,"maxLength":255,"description":"API key for application identification"},"tz":{"type":"string","minLength":1,"maxLength":50,"description":"Client timezone","example":"America/Vancouver"},"database":{"type":"string","minLength":1,"maxLength":100,"description":"Database name for routing queries","example":"Demo_PG"}}}
//@ swagger:response 200 application/json {"type":"object","required":["token","expires_at","user_id"],"properties":{"token":{"type":"string","description":"JWT token with Bearer prefix"},"expires_at":{"type":"integer","description":"Token expiration timestamp (Unix epoch)"},"user_id":{"type":"integer","description":"Authenticated user ID"},"roles":{"type":"array","items":{"type":"string"},"description":"User role permissions"}}}
//@ swagger:response 400 application/json {"type":"object","properties":{"error":{"type":"string","example":"Invalid request parameters"}}}
//@ swagger:response 401 application/json {"type":"object","properties":{"error":{"type":"string","example":"Invalid credentials"}}}
//@ swagger:response 429 application/json {"type":"object","properties":{"error":{"type":"string","example":"Too many failed attempts"},"retry_after":{"type":"integer","description":"Seconds until retry allowed"}}}
//@ swagger:response 500 application/json {"type":"object","properties":{"error":{"type":"string","example":"Internal server error"}}}
enum MHD_Result handle_auth_login_request(
    void *cls,
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *version,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls
);

// Helper functions for error response building (exported for testing)
enum MHD_Result login_send_missing_params_error(struct MHD_Connection *connection);
enum MHD_Result login_send_validation_error(struct MHD_Connection *connection, const char* login_id);
enum MHD_Result login_send_license_expired_error(struct MHD_Connection *connection, int system_id);
enum MHD_Result login_send_client_ip_error(struct MHD_Connection *connection);
enum MHD_Result login_send_ip_blacklist_error(struct MHD_Connection *connection, const char* client_ip);
enum MHD_Result login_send_rate_limit_error(struct MHD_Connection *connection, const char* login_id, const char* client_ip);
enum MHD_Result login_send_account_not_found_error(struct MHD_Connection *connection, const char* login_id);
enum MHD_Result login_send_account_disabled_error(struct MHD_Connection *connection, const char* login_id, int account_id);
enum MHD_Result login_send_account_not_authorized_error(struct MHD_Connection *connection, const char* login_id, int account_id);
enum MHD_Result login_send_jwt_generation_error(struct MHD_Connection *connection, int account_id);
enum MHD_Result login_send_jwt_hash_error(struct MHD_Connection *connection, int account_id);

#endif /* HYDROGEN_AUTH_LOGIN_H */
