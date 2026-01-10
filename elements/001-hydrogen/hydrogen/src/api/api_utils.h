/*
 * API Utilities for the Hydrogen Project.
 * Provides common functions used across API endpoints.
 */

#ifndef HYDROGEN_API_UTILS_H
#define HYDROGEN_API_UTILS_H

//@ swagger:title Hydrogen REST API
//@ swagger:description A comprehensive REST API for the Hydrogen Project that provides system information, OIDC authentication, and various service endpoints
//@ swagger:version 1.0.0
//@ swagger:contact.email api@example.com
//@ swagger:license.name MIT
//@ swagger:server http://localhost:8080/api Development server
//@ swagger:server https://api.example.com/api Production server
//@ swagger:security bearerAuth JWT authentication

// Network headers
#include <microhttpd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

// Third-party libraries
#include <jansson.h>

/**
 * URL decode a string
 * Converts URL-encoded strings (e.g., %20 to space, + to space)
 * Caller must free the returned string
 *
 * @param src The URL-encoded string
 * @return A newly allocated decoded string, or NULL on error
 */
char *api_url_decode(const char *src);

/**
 * URL encode a string
 * Converts special characters to %XX format for URL transmission
 * Caller must free the returned string
 * 
 * @param src The string to encode
 * @return A newly allocated encoded string, or NULL on error
 */
char *api_url_encode(const char *src);

/**
 * Extract client IP address from a connection
 * Determines the client's IP address (IPv4 or IPv6)
 * Caller must free the returned string
 *
 * @param connection The MHD_Connection object
 * @return A newly allocated string with the IP address, or "unknown" on error
 */
char *api_get_client_ip(struct MHD_Connection *connection);

/**
 * Extract JWT claims from an Authorization header
 * Parses a Bearer token and extracts its claims
 * Looks for "Authorization: Bearer <token>" header
 * 
 * @param connection The MHD_Connection object
 * @param jwt_secret Secret key for validating the JWT signature
 * @return A newly allocated JSON object with the claims, or NULL if not found or invalid
 */
json_t *api_extract_jwt_claims(struct MHD_Connection *connection, const char *jwt_secret);

/**
 * Extract query parameters into a JSON object
 * Converts all query parameters into a JSON object for easy access
 * Automatically URL-decodes parameter values
 *
 * @param connection The MHD_Connection object
 * @return A newly allocated JSON object with parameters, or empty object if none
 */
json_t *api_extract_query_params(struct MHD_Connection *connection);

/**
 * Extract POST data into a JSON object
 * Handles application/x-www-form-urlencoded data
 * Automatically URL-decodes parameter values
 *
 * @param connection The MHD_Connection object
 * @return A newly allocated JSON object with POST data, or empty object if none
 */
json_t *api_extract_post_data(struct MHD_Connection *connection);

/**
 * Send a JSON response
 * Creates an HTTP response with the provided JSON content
 * Adds appropriate content type and CORS headers
 *
 * @param connection The MHD_Connection object
 * @param json_obj The JSON object to send
 * @param status_code The HTTP status code to use
 * @return MHD_Result indicating success or failure
 */
enum MHD_Result api_send_json_response(struct MHD_Connection *connection, 
                                    json_t *json_obj, 
                                    unsigned int status_code);

/**
 * Add standard CORS headers to a response
 *
 * @param response The MHD_Response object
 */
void api_add_cors_headers(struct MHD_Response *response);

/**
 * Validate JWT token and extract claims
 * Uses the provided secret to verify the token signature
 * Caller must free the returned JSON object
 *
 * @param token The JWT token string
 * @param secret The secret key for validation
 * @return A newly allocated JSON object with claims, or NULL if invalid
 */
json_t *api_validate_jwt(const char *token, const char *secret);

/**
 * Create a JWT token from claims
 * Generates a signed JWT using the provided claims and secret
 * Caller must free the returned string
 *
 * @param claims The JSON object containing claims
 * @param secret The secret key for signing
 * @return A newly allocated JWT string, or NULL on error
 */
char *api_create_jwt(const json_t *claims, const char *secret);

/**
 * Iterator function for processing query parameters
 * Used internally by api_extract_query_params
 *
 * @param cls User data (JSON object)
 * @param kind The type of value
 * @param key The parameter key
 * @param value The parameter value
 * @return MHD_YES to continue iteration
 */
enum MHD_Result query_param_iterator(void *cls, enum MHD_ValueKind kind, const char *key, const char *value);

/**
 * Iterator function for processing POST data
 * Used internally by api_extract_post_data
 *
 * @param cls User data (JSON object)
 * @param kind The type of value
 * @param key The parameter key
 * @param value The parameter value
 * @return MHD_YES to continue iteration
 */
enum MHD_Result post_data_iterator(void *cls, enum MHD_ValueKind kind, const char *key, const char *value);

// ============================================================================
// POST Body Buffering Utilities
// ============================================================================
// These functions handle buffering of POST body data across multiple MHD
// callback invocations. MHD delivers POST data in chunks, so endpoints must
// accumulate the data before processing.

/**
 * Maximum POST body size for API requests (64KB default, sufficient for JSON)
 */
#define API_MAX_POST_SIZE (64 * 1024)

/**
 * Initial buffer capacity for POST body accumulation
 */
#define API_INITIAL_BUFFER_CAPACITY 1024

/**
 * Structure to buffer POST body data across MHD callback invocations.
 * The http_method field determines how the endpoint should be processed.
 */
typedef struct {
    char *data;          // Buffer for accumulated POST data
    size_t size;         // Current size of accumulated data
    size_t capacity;     // Allocated capacity of buffer
    char http_method;    // 'G' for GET, 'P' for POST, 'O' for OPTIONS
} ApiPostBuffer;

/**
 * Enumeration of POST buffering states returned by api_buffer_post_data
 */
typedef enum {
    API_BUFFER_CONTINUE,     // More data expected, caller should return MHD_YES
    API_BUFFER_COMPLETE,     // All data received, caller should process request
    API_BUFFER_ERROR,        // Error occurred, response already sent or preparation failed
    API_BUFFER_METHOD_ERROR  // Unsupported HTTP method
} ApiBufferResult;

/**
 * Initialize or accumulate POST body data for an API endpoint.
 *
 * This function handles the MHD callback lifecycle:
 * 1. First call (*con_cls == NULL): Allocates buffer, returns API_BUFFER_CONTINUE
 * 2. Data calls (*upload_data_size > 0): Accumulates data, returns API_BUFFER_CONTINUE
 * 3. Final call (*upload_data_size == 0): Returns API_BUFFER_COMPLETE
 *
 * For GET requests, immediately returns API_BUFFER_COMPLETE with empty buffer.
 *
 * @param method The HTTP method string ("GET", "POST", etc.)
 * @param upload_data Pointer to current chunk of upload data
 * @param upload_data_size Pointer to size of current data chunk
 * @param con_cls Pointer to connection context (stores buffer between calls)
 * @param buffer_out On API_BUFFER_COMPLETE, set to point to the ApiPostBuffer
 * @return ApiBufferResult indicating how caller should proceed
 */
ApiBufferResult api_buffer_post_data(
    const char *method,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls,
    ApiPostBuffer **buffer_out
);

/**
 * Free an API POST buffer and its contents.
 * Sets *con_cls to NULL after freeing.
 * Safe to call with NULL or already-freed buffer.
 *
 * @param con_cls Pointer to connection context containing the buffer
 */
void api_free_post_buffer(void **con_cls);

/**
 * Parse JSON from an API POST buffer.
 * Returns NULL if buffer is empty or contains invalid JSON.
 * Error message is logged automatically.
 *
 * @param buffer The ApiPostBuffer containing JSON data
 * @return Parsed json_t object (caller must decref), or NULL on error
 */
json_t *api_parse_json_body(ApiPostBuffer *buffer);

/**
 * Send an error response and free the POST buffer.
 * Convenience function for error handling in endpoints.
 *
 * @param connection The MHD connection
 * @param con_cls Pointer to connection context (buffer will be freed)
 * @param error_message Error message for JSON response
 * @param http_status HTTP status code
 * @return MHD_Result from api_send_json_response
 */
enum MHD_Result api_send_error_and_cleanup(
    struct MHD_Connection *connection,
    void **con_cls,
    const char *error_message,
    unsigned int http_status
);

#endif /* HYDROGEN_API_UTILS_H */
