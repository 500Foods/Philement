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

#endif /* HYDROGEN_API_UTILS_H */
