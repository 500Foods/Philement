/**
 * @file queries_response_helpers.h
 * @brief Helper functions for multi-query response building
 *
 * Common response building helpers shared by queries, auth_queries,
 * and alt_queries endpoints. Extracted from handler functions to improve
 * testability and reduce duplication.
 *
 * @author Hydrogen Framework
 * @date 2026-02-19
 */

#ifndef QUERIES_RESPONSE_HELPERS_H
#define QUERIES_RESPONSE_HELPERS_H

// Standard includes
#include <stdbool.h>
#include <stdio.h>

// Third-party libraries
#include <microhttpd.h>
#include <jansson.h>

// Include DeduplicationResult enum
#include <src/api/conduit/queries/queries.h>

/**
 * @brief Build a JSON error response for deduplication validation failures
 *
 * Creates a standardized JSON error response based on the deduplication
 * result code. Used by queries, auth_queries, and alt_queries endpoints.
 *
 * @param dedup_code The deduplication result code
 * @param database The database name (for rate limit messages)
 * @param max_queries The max queries limit (for rate limit messages)
 * @return JSON object with error details (caller must decref), or NULL on error
 */
json_t *build_dedup_error_json(DeduplicationResult dedup_code, const char *database, int max_queries);

/**
 * @brief Get the HTTP status code for a deduplication error
 *
 * @param dedup_code The deduplication result code
 * @return The appropriate HTTP status code
 */
unsigned int get_dedup_http_status(DeduplicationResult dedup_code);

/**
 * @brief Determine HTTP status code from results array error analysis
 *
 * Analyzes the results array to determine the appropriate HTTP status code
 * based on the types of errors encountered across all query results.
 *
 * Priority order:
 * 1. Rate limit errors → 429
 * 2. Parameter/validation errors → 400
 * 3. Auth/permission errors → 401
 * 4. Not found errors → 404
 * 5. Database execution errors → 422
 * 6. Duplicate-only errors → 200
 *
 * @param results_array The JSON array of query results
 * @param result_count Number of results in the array
 * @return The appropriate HTTP status code
 */
unsigned int determine_queries_http_status(json_t *results_array, size_t result_count);

/**
 * @brief Build a single result entry for a rate-limited query
 *
 * Creates a JSON error object for queries that exceeded the rate limit.
 *
 * @param max_queries The max queries limit
 * @return JSON object with rate limit error (caller must decref)
 */
json_t *build_rate_limit_result_entry(int max_queries);

/**
 * @brief Build a single result entry for a duplicate query
 *
 * Creates a JSON error object for duplicate queries.
 *
 * @return JSON object with duplicate error (caller must decref)
 */
json_t *build_duplicate_result_entry(void);

/**
 * @brief Build a single result entry for an invalid query mapping
 *
 * Creates a JSON error object for queries with invalid mapping indices.
 *
 * @return JSON object with error (caller must decref)
 */
json_t *build_invalid_mapping_result_entry(void);

/**
 * @brief Send a standardized JSON error response
 *
 * Creates and sends a JSON response with {success: false, error: error_msg}
 * at the specified HTTP status. Consolidates the repeated error pattern
 * used across conduit endpoints.
 *
 * @param connection The MHD connection
 * @param error_msg The error message string
 * @param http_status The HTTP status code to send
 * @return Result of api_send_json_response
 */
enum MHD_Result send_conduit_error_response(
    struct MHD_Connection *connection, const char *error_msg, unsigned int http_status);

#endif /* QUERIES_RESPONSE_HELPERS_H */
