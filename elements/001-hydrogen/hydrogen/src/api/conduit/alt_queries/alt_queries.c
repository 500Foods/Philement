/**
 * @file alt_queries.c
 * @brief Alternative Authenticated Conduit Queries API endpoint implementation
 *
 * This module implements the authenticated database queries execution endpoint
 * with database override capability. It validates JWT tokens before executing
 * multiple queries in parallel and allows specifying a different database than
 * the one in the JWT claims.
 *
 * @author Hydrogen Framework
 * @date 2026-01-18
 */

#include <src/hydrogen.h>

// Network headers
#include <microhttpd.h>

// Standard C libraries
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Third-party libraries
#include <jansson.h>

// Project headers
#include <src/api/api_service.h>
#include <src/api/api_utils.h>  // For api_send_json_response
#include <src/api/conduit/alt_queries/alt_queries.h>
#include <src/api/conduit/query/query.h>  // Use existing helpers
#include <src/api/auth/auth_service.h>
#include <src/api/auth/auth_service_jwt.h>
#include <src/config/config.h>
#include <src/config/config_databases.h>
#include <src/logging/logging.h>

// External global queue manager for DQM statistics
extern DatabaseQueueManager* global_queue_manager;

// Deduplication result enum
typedef enum {
    DEDUP_OK = 0,
    DEDUP_RATE_LIMIT = 1,
    DEDUP_DATABASE_NOT_FOUND = 2,
    DEDUP_ERROR = 3
} DeduplicationResult;

/**
 * @brief Deduplicate queries and validate rate limits
 *
 * Processes the queries array to remove duplicates by query_ref and validates
 * against the MaxQueriesPerRequest limit for the specified database.
 *
 * @param connection The HTTP connection for error responses
 * @param queries_array The input queries array from the request
 * @param database The database name to check rate limits against
 * @param deduplicated_queries Output array of unique query objects (caller must decref)
 * @param mapping_array Output array mapping original indices to deduplicated indices
 * @param is_duplicate Output array tracking which original queries are duplicates
 * @param result_code Output parameter for deduplication result code
 * @return MHD_YES on success, MHD_NO on rate limit exceeded or error
 */
static enum MHD_Result alt_queries_deduplicate_and_validate(
    struct MHD_Connection *connection,
    json_t *queries_array,
    const char *database,
    json_t **deduplicated_queries,
    size_t **mapping_array,
    bool **is_duplicate,
    DeduplicationResult *result_code)
{
    (void)connection;  // Unused parameter
    
    if (!queries_array || !database || !deduplicated_queries || !mapping_array || !is_duplicate) {
        log_this(SR_AUTH, "alt_queries_deduplicate_and_validate: NULL parameters", LOG_LEVEL_ERROR, 0);
        if (result_code) {
            *result_code = DEDUP_ERROR;
        }
        return MHD_NO;
    }

    size_t original_count = json_array_size(queries_array);
    if (original_count == 0) {
        *deduplicated_queries = json_array();
        *mapping_array = NULL;
        *is_duplicate = NULL;
        if (result_code) {
            *result_code = DEDUP_OK;
        }
        return MHD_YES;
    }

    // Validate database connection first
    const DatabaseConnection *db_conn = find_database_connection(&app_config->databases, database);
    if (!db_conn) {
        // If connection not found by database name, try to find it by checking if database name matches any connection name
        for (int i = 0; i < app_config->databases.connection_count; i++) {
            const DatabaseConnection *conn = &app_config->databases.connections[i];
            if (conn->enabled && conn->connection_name && strcmp(conn->connection_name, database) == 0) {
                db_conn = conn;
                break;
            }
        }
        if (!db_conn) {
            log_this(SR_AUTH, "alt_queries_deduplicate_and_validate: Database connection not found: %s", LOG_LEVEL_ALERT, 1, database);
            if (result_code) {
                *result_code = DEDUP_DATABASE_NOT_FOUND;
            }
            return MHD_NO;
        }
    }

    // Allocate memory for duplicate tracking
    *is_duplicate = calloc(original_count, sizeof(bool));
    if (!*is_duplicate) {
        log_this(SR_AUTH, "alt_queries_deduplicate_and_validate: Failed to allocate memory for duplicate tracking", LOG_LEVEL_ERROR, 0);
        return MHD_NO;
    }

    // Create arrays to track unique queries (query_ref + params) and their mapping
    int *query_refs = calloc(original_count, sizeof(int));
    json_t **query_params = calloc(original_count, sizeof(json_t*));
    size_t *first_occurrence = calloc(original_count, sizeof(size_t));
    size_t unique_count = 0;

    if (!query_refs || !query_params || !first_occurrence) {
        log_this(SR_AUTH, "alt_queries_deduplicate_and_validate: Failed to allocate memory", LOG_LEVEL_ERROR, 0);
        free(query_refs);
        free(query_params);
        free(first_occurrence);
        free(*is_duplicate);
        return MHD_NO;
    }

    // First pass: collect unique queries (query_ref + params) and track duplicates
    for (size_t i = 0; i < original_count; i++) {
        json_t *query_obj = json_array_get(queries_array, i);
        if (!query_obj || !json_is_object(query_obj)) {
            (*is_duplicate)[i] = true;
            continue;
        }

        json_t *query_ref_json = json_object_get(query_obj, "query_ref");
        if (!query_ref_json || !json_is_integer(query_ref_json)) {
            (*is_duplicate)[i] = true;
            continue;
        }

        int query_ref = (int)json_integer_value(query_ref_json);
        json_t *params = json_object_get(query_obj, "params");
        if (!params) {
            params = json_object();  // Treat missing params as empty object
        }

        // Check if we've seen this query_ref + params combination before
        bool found = false;
        for (size_t j = 0; j < unique_count; j++) {
            if (query_refs[j] == query_ref) {
                // Compare params
                int cmp = json_equal(query_params[j], params);
                if (cmp == 1) {
                    found = true;
                    (*is_duplicate)[i] = true;
                    break;
                }
            }
        }

        if (!found) {
            query_refs[unique_count] = query_ref;
            query_params[unique_count] = params;  // Reference to original, no decref needed
            first_occurrence[unique_count] = i;
            unique_count++;
        }
    }

    // Check rate limit
    if ((int)unique_count > db_conn->max_queries_per_request) {
        log_this(SR_AUTH, "alt_queries_deduplicate_and_validate: Rate limit exceeded: %zu unique queries > %d max for database %s",
                 LOG_LEVEL_ERROR, 3, unique_count, db_conn->max_queries_per_request, database);
        free(query_refs);
        free(query_params);
        free(first_occurrence);
        free(*is_duplicate);
        if (result_code) {
            *result_code = DEDUP_RATE_LIMIT;
        }
        // Reset output parameters to NULL
        *deduplicated_queries = NULL;
        *mapping_array = NULL;
        *is_duplicate = NULL;
        return MHD_NO;
    }

    // Create deduplicated queries array
    *deduplicated_queries = json_array();
    *mapping_array = calloc(original_count, sizeof(size_t));

    if (!*deduplicated_queries || !*mapping_array) {
        log_this(SR_AUTH, "alt_queries_deduplicate_and_validate: Failed to allocate output arrays", LOG_LEVEL_ERROR, 0);
        json_decref(*deduplicated_queries);
        free(*mapping_array);
        free(query_refs);
        free(query_params);
        free(first_occurrence);
        free(*is_duplicate);
        return MHD_NO;
    }

    // Second pass: build deduplicated array and mapping
    for (size_t i = 0; i < original_count; i++) {
        json_t *query_obj = json_array_get(queries_array, i);
        if (!query_obj || !json_is_object(query_obj)) {
            continue;
        }

        json_t *query_ref_json = json_object_get(query_obj, "query_ref");
        if (!query_ref_json || !json_is_integer(query_ref_json)) {
            continue;
        }

        int query_ref = (int)json_integer_value(query_ref_json);
        json_t *params = json_object_get(query_obj, "params");
        if (!params) {
            params = json_object();
        }

        // Find the deduplicated index for this query_ref + params
        for (size_t j = 0; j < unique_count; j++) {
            if (query_refs[j] == query_ref && json_equal(query_params[j], params)) {
                (*mapping_array)[i] = j;

                // Add to deduplicated array if this is the first occurrence
                if (first_occurrence[j] == i) {
                    json_array_append(*deduplicated_queries, query_obj);
                }
                break;
            }
        }
    }

    log_this(SR_AUTH, "alt_queries_deduplicate_and_validate: Deduplicated %zu queries to %zu unique queries",
             LOG_LEVEL_DEBUG, 2, original_count, unique_count);

    free(query_refs);
    free(query_params);
    free(first_occurrence);

    if (result_code) {
        *result_code = DEDUP_OK;
    }
    
    return MHD_YES;
}

/**
 * @brief Validate JWT token for authentication (without extracting database)
 *
 * Validates the provided JWT token for authentication purposes only.
 * Unlike auth_queries, this does not extract the database from the token.
 *
 * @param connection The HTTP connection for error responses
 * @param token The JWT token string to validate
 * @return MHD_YES on success, MHD_NO on validation failure
 */
static enum MHD_Result validate_jwt_for_auth(
    struct MHD_Connection *connection,
    const char *token)
{
    if (!token) {
        log_this(SR_AUTH, "validate_jwt_for_auth: NULL token", LOG_LEVEL_ERROR, 0);
        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Missing authentication token"));
        return api_send_json_response(connection, error_response, MHD_HTTP_BAD_REQUEST);
    }

    // Validate JWT token - pass NULL since database comes from request
    jwt_validation_result_t result = validate_jwt(token, NULL);
    if (!result.valid || !result.claims) {
        log_this(SR_AUTH, "validate_jwt_for_auth: JWT validation failed", LOG_LEVEL_ALERT, 0);
        free_jwt_validation_result(&result);

        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Invalid or expired JWT token"));
        return api_send_json_response(connection, error_response, MHD_HTTP_UNAUTHORIZED);
    }

    log_this(SR_AUTH, "validate_jwt_for_auth: JWT validated successfully", LOG_LEVEL_DEBUG, 0);
    free_jwt_validation_result(&result);
    return MHD_YES;
}

/**
 * @brief Parse alternative authenticated queries request
 *
 * Parses the request JSON and extracts the JWT token, database, and queries array.
 *
 * @param connection The HTTP connection for error responses
 * @param method The HTTP method (GET or POST)
 * @param buffer The buffered request data
 * @param con_cls Connection-specific data
 * @param token Output parameter for JWT token
 * @param database Output parameter for database name
 * @param queries_array Output parameter for queries array
 * @return MHD_YES on success, MHD_NO on parsing failure
 */
static enum MHD_Result parse_alt_queries_request(
    struct MHD_Connection *connection,
    const char *method,
    ApiPostBuffer *buffer,
    void **con_cls,
    char **token,
    char **database,
    json_t **queries_array)
{
    (void)con_cls; // Unused parameter
    
    if (!method || !token || !database || !queries_array) {
        log_this(SR_AUTH, "parse_alt_queries_request: NULL parameters", LOG_LEVEL_ERROR, 0);
        return MHD_NO;
    }

    // Parse request data with proper POST buffering
    json_t *request_json = NULL;
    enum MHD_Result result = handle_request_parsing_with_buffer(connection, buffer, &request_json);
    if (result != MHD_YES) {
        log_this(SR_AUTH, "parse_alt_queries_request: Failed to parse request data", LOG_LEVEL_ERROR, 0);
        return result;
    }

    // Extract token field
    json_t *token_json = json_object_get(request_json, "token");
    if (!token_json || !json_is_string(token_json)) {
        log_this(SR_AUTH, "parse_alt_queries_request: Missing or invalid token field", LOG_LEVEL_ERROR, 0);
        json_decref(request_json);

        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Missing required parameter: token"));
        api_send_json_response(connection, error_response, MHD_HTTP_BAD_REQUEST);
        return MHD_NO;
    }

    *token = strdup(json_string_value(token_json));
    if (!*token) {
        log_this(SR_AUTH, "parse_alt_queries_request: Failed to allocate token string", LOG_LEVEL_ERROR, 0);
        json_decref(request_json);
        return MHD_NO;
    }

    // Extract database field
    json_t *database_json = json_object_get(request_json, "database");
    if (!database_json || !json_is_string(database_json)) {
        log_this(SR_AUTH, "parse_alt_queries_request: Missing or invalid database field", LOG_LEVEL_ERROR, 0);
        free(*token);
        json_decref(request_json);

        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Missing required parameter: database"));
        api_send_json_response(connection, error_response, MHD_HTTP_BAD_REQUEST);
        return MHD_NO;
    }

    *database = strdup(json_string_value(database_json));
    if (!*database) {
        log_this(SR_AUTH, "parse_alt_queries_request: Failed to allocate database string", LOG_LEVEL_ERROR, 0);
        free(*token);
        json_decref(request_json);
        return MHD_NO;
    }

    // Extract queries array
    json_t *queries = json_object_get(request_json, "queries");
    if (!queries || !json_is_array(queries)) {
        log_this(SR_AUTH, "parse_alt_queries_request: Missing or invalid queries field", LOG_LEVEL_ERROR, 0);
        free(*token);
        free(*database);
        json_decref(request_json);

        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Missing required parameter: queries (must be array)"));
        api_send_json_response(connection, error_response, MHD_HTTP_BAD_REQUEST);
        return MHD_NO;
    }

    size_t query_count = json_array_size(queries);
    if (query_count == 0) {
        log_this(SR_AUTH, "parse_alt_queries_request: Empty queries array", LOG_LEVEL_ERROR, 0);
        free(*token);
        free(*database);
        json_decref(request_json);

        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Queries array cannot be empty"));
        api_send_json_response(connection, error_response, MHD_HTTP_BAD_REQUEST);
        return MHD_NO;
    }

    // Deep copy the queries array since we need it after freeing request_json
    *queries_array = json_deep_copy(queries);
    if (!*queries_array) {
        log_this(SR_AUTH, "parse_alt_queries_request: Failed to copy queries array", LOG_LEVEL_ERROR, 0);
        free(*token);
        free(*database);
        json_decref(request_json);
        return MHD_NO;
    }

    json_decref(request_json);
    log_this(SR_AUTH, "parse_alt_queries_request: Successfully parsed, database=%s, queries=%zu", LOG_LEVEL_DEBUG, 2, *database, query_count);
    return MHD_YES;
}

/**
 * @brief Handle alternative authenticated conduit queries request
 *
 * Main handler for the /api/conduit/alt_queries endpoint. Validates JWT token,
 * extracts database from request body (overriding JWT claims), and executes
 * multiple queries in parallel.
 *
 * @param connection The HTTP connection
 * @param url The request URL
 * @param method The HTTP method (GET or POST)
 * @param upload_data Request body data
 * @param upload_data_size Size of request body
 * @param con_cls Connection-specific data
 * @return HTTP response code
 */
enum MHD_Result handle_conduit_alt_queries_request(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *upload_data,
    const size_t *upload_data_size,
    void **con_cls)
{
    (void)url;

    log_this(SR_AUTH, "handle_conduit_alt_queries_request: Processing alternative authenticated queries request", LOG_LEVEL_DEBUG, 0);

    // Step 1: Use common POST body buffering (handles both GET and POST)
    ApiPostBuffer *buffer = NULL;
    ApiBufferResult buf_result = api_buffer_post_data(method, upload_data, (size_t *)upload_data_size, con_cls, &buffer);
    
    switch (buf_result) {
        case API_BUFFER_CONTINUE:
            return MHD_YES;
        case API_BUFFER_ERROR:
            return api_send_error_and_cleanup(connection, con_cls, "Request processing error", MHD_HTTP_INTERNAL_SERVER_ERROR);
        case API_BUFFER_METHOD_ERROR:
            return api_send_error_and_cleanup(connection, con_cls, "Method not allowed - use GET or POST", MHD_HTTP_METHOD_NOT_ALLOWED);
        case API_BUFFER_COMPLETE:
            break;
        default:
            return api_send_error_and_cleanup(connection, con_cls, "Unknown buffer result", MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    // Step 2: Validate HTTP method using helper from query.h
    enum MHD_Result result = handle_method_validation(connection, method);
    if (result != MHD_YES) {
        api_free_post_buffer(con_cls);
        return result;
    }

    // Step 3: Parse request and extract token, database, queries
    char *token = NULL;
    char *database = NULL;
    json_t *queries_array = NULL;

    result = parse_alt_queries_request(connection, method, buffer, con_cls,
                                      &token, &database, &queries_array);
                                      
    api_free_post_buffer(con_cls);

    if (result != MHD_YES) {
        return result;
    }

    // Step 4: Validate JWT token for authentication
    result = validate_jwt_for_auth(connection, token);
    free(token);  // Token no longer needed after validation
    token = NULL;

    if (result != MHD_YES) {
        free(database);
        json_decref(queries_array);
        return result;
    }

    // Step 5: Deduplicate queries and validate rate limits
    json_t *deduplicated_queries = NULL;
    size_t *mapping_array = NULL;
    bool *is_duplicate = NULL;
    DeduplicationResult dedup_code = DEDUP_OK;
    enum MHD_Result dedup_result = alt_queries_deduplicate_and_validate(connection, queries_array, database,
                                                                    &deduplicated_queries, &mapping_array,
                                                                    &is_duplicate, &dedup_code);
    
    if (dedup_result != MHD_YES) {
        log_this(SR_AUTH, "alt_queries: Validation failed with code %d", LOG_LEVEL_ERROR, 1, dedup_code);
        
        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        
        if (dedup_code == DEDUP_RATE_LIMIT) {
            const DatabaseConnection *db_conn = find_database_connection(&app_config->databases, database);
            int max_queries = db_conn ? db_conn->max_queries_per_request : 10;
            char limit_msg[100];
            snprintf(limit_msg, sizeof(limit_msg), "Query limit of %d unique queries per request exceeded", max_queries);
            json_object_set_new(error_response, "error", json_string("Rate limit exceeded"));
            json_object_set_new(error_response, "message", json_string(limit_msg));
        } else if (dedup_code == DEDUP_DATABASE_NOT_FOUND) {
            json_object_set_new(error_response, "error", json_string("Invalid database"));
        } else {
            json_object_set_new(error_response, "error", json_string("Validation failed"));
        }
        
        json_decref(queries_array);
        free(database);
        
        unsigned int http_status = (dedup_code == DEDUP_RATE_LIMIT) ? MHD_HTTP_TOO_MANY_REQUESTS : MHD_HTTP_BAD_REQUEST;
        return api_send_json_response(connection, error_response, http_status);
    }

    size_t original_query_count = json_array_size(queries_array);
    size_t unique_query_count = json_array_size(deduplicated_queries);
    
    log_this(SR_AUTH, "alt_queries: Deduplicated %zu queries to %zu unique queries",
             LOG_LEVEL_DEBUG, 2, original_query_count, unique_query_count);

    // Step 6: Submit all unique queries for parallel execution
    size_t query_count = unique_query_count;
    PendingQueryResult **pending_results = calloc(query_count, sizeof(PendingQueryResult*));
    int *query_refs = calloc(query_count, sizeof(int));
    QueryCacheEntry **cache_entries = calloc(query_count, sizeof(QueryCacheEntry*));
    DatabaseQueue **selected_queues = calloc(query_count, sizeof(DatabaseQueue*));

    if (!pending_results || !query_refs || !cache_entries || !selected_queues) {
        log_this(SR_AUTH, "handle_conduit_alt_queries_request: Failed to allocate memory for parallel execution", LOG_LEVEL_ERROR, 0);
        free(database);
        json_decref(queries_array);

        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Internal server error"));
        return api_send_json_response(connection, error_response, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    log_this(SR_AUTH, "handle_conduit_alt_queries_request: Submitting %zu unique queries for parallel execution", LOG_LEVEL_DEBUG, 1, query_count);

    // Submit all unique queries first
    bool submission_failed = false;
    for (size_t i = 0; i < query_count; i++) {
        json_t *query_obj = json_array_get(deduplicated_queries, i);
        int query_ref;

        // Let's use handle_database_lookup, handle_parameter_processing, etc.
        DatabaseQueue *db_queue = NULL;
        QueryCacheEntry *cache_entry = NULL;
        bool query_not_found = false;
        
        // Extract query_ref from query object
        json_t* query_ref_json = json_object_get(query_obj, "query_ref");
        if (!query_ref_json || !json_is_integer(query_ref_json)) {
            log_this(SR_AUTH, "handle_conduit_alt_queries_request: Missing or invalid query_ref in query %zu", LOG_LEVEL_ERROR, 1, i);
            submission_failed = true;
            break;
        }
        query_ref = (int)json_integer_value(query_ref_json);

        // Lookup database and query
        enum MHD_Result lookup_result = handle_database_lookup(connection, database, query_ref, &db_queue, &cache_entry, &query_not_found, false);
        if (lookup_result != MHD_YES) {
            log_this(SR_AUTH, "handle_conduit_alt_queries_request: Database lookup failed for query %d", LOG_LEVEL_ERROR, 1, query_ref);
            submission_failed = true;
            break;
        }

        if (query_not_found) {
            log_this(SR_AUTH, "handle_conduit_alt_queries_request: Query not found for query_ref %d", LOG_LEVEL_ERROR, 1, query_ref);
            submission_failed = true;
            break;
        }

        // Process parameters
        ParameterList *param_list = NULL;
        char *converted_sql = NULL;
        TypedParameter **ordered_params = NULL;
        size_t param_count = 0;
        char* message = NULL;
        
        json_t* params_json = json_object_get(query_obj, "params");
        
        enum MHD_Result param_result = handle_parameter_processing(connection, params_json, db_queue, cache_entry,
                                    database, query_ref, &param_list, &converted_sql,
                                    &ordered_params, &param_count, &message);
        if (param_result != MHD_YES) {
            log_this(SR_AUTH, "handle_conduit_alt_queries_request: Parameter processing failed for query %d", LOG_LEVEL_ERROR, 1, query_ref);
            submission_failed = true;
            break;
        }

        // Select queue
        DatabaseQueue *selected_queue = NULL;
        enum MHD_Result queue_result = handle_queue_selection(connection, database, query_ref, cache_entry,
                                   param_list, converted_sql, ordered_params, &selected_queue);
        if (queue_result != MHD_YES) {
            log_this(SR_AUTH, "handle_conduit_alt_queries_request: Queue selection failed for query %d", LOG_LEVEL_ERROR, 1, query_ref);
            free_parameter_list(param_list);
            free(converted_sql);
            if (ordered_params) {
                for (size_t j = 0; j < param_count; j++) {
                    free_typed_parameter(ordered_params[j]);
                }
                free(ordered_params);
            }
            if (message) free(message);
            submission_failed = true;
            break;
        }

        // Generate query ID
        char *query_id = NULL;
        enum MHD_Result id_result = handle_query_id_generation(connection, database, query_ref, param_list,
                                       converted_sql, ordered_params, &query_id);
        if (id_result != MHD_YES) {
            log_this(SR_AUTH, "handle_conduit_alt_queries_request: Query ID generation failed for query %d", LOG_LEVEL_ERROR, 1, query_ref);
            free_parameter_list(param_list);
            free(converted_sql);
            if (ordered_params) {
                for (size_t j = 0; j < param_count; j++) {
                    free_typed_parameter(ordered_params[j]);
                }
                free(ordered_params);
            }
            if (message) free(message);
            submission_failed = true;
            break;
        }

        // Register pending query
        PendingQueryResult *pending = NULL;
        enum MHD_Result pending_result = handle_pending_registration(connection, database, query_ref, query_id,
                                       param_list, converted_sql, ordered_params,
                                       cache_entry, &pending);
        if (pending_result != MHD_YES) {
            log_this(SR_AUTH, "handle_conduit_alt_queries_request: Pending registration failed for query %d", LOG_LEVEL_ERROR, 1, query_ref);
            free_parameter_list(param_list);
            free(converted_sql);
            free(query_id);
            if (ordered_params) {
                for (size_t j = 0; j < param_count; j++) {
                    free_typed_parameter(ordered_params[j]);
                }
                free(ordered_params);
            }
            if (message) free(message);
            submission_failed = true;
            break;
        }

        // Submit query
        enum MHD_Result submit_result = handle_query_submission(connection, database, query_ref, selected_queue,
                                   query_id, converted_sql, param_list, ordered_params,
                                   param_count, cache_entry);
        if (submit_result != MHD_YES) {
            log_this(SR_AUTH, "handle_conduit_alt_queries_request: Query submission failed for query %d", LOG_LEVEL_ERROR, 1, query_ref);
            free_parameter_list(param_list);
            free(converted_sql);
            free(query_id);
            if (ordered_params) {
                for (size_t j = 0; j < param_count; j++) {
                    free_typed_parameter(ordered_params[j]);
                }
                free(ordered_params);
            }
            if (message) free(message);
            // Need to free pending?
            submission_failed = true;
            break;
        }

        // Cleanup per-query resources
        free_parameter_list(param_list);
        free(converted_sql);
        free(query_id);
        if (ordered_params) {
            for (size_t j = 0; j < param_count; j++) {
                free_typed_parameter(ordered_params[j]);
            }
            free(ordered_params);
        }
        if (message) free(message);

        // Store for later
        pending_results[i] = pending;
        query_refs[i] = query_ref;
        cache_entries[i] = cache_entry;
        selected_queues[i] = selected_queue;
    }

    if (submission_failed) {
        for (size_t i = 0; i < query_count; i++) {
            if (pending_results[i]) {
                // Note: pending results are managed by the pending result manager
            }
        }
        free(pending_results);
        free(query_refs);
        free(cache_entries);
        free(selected_queues);
        free(mapping_array);
        free(is_duplicate);
        free(database);
        json_decref(deduplicated_queries);
        json_decref(queries_array);

        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Failed to submit queries"));
        return api_send_json_response(connection, error_response, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    // Step 6: Suspend webserver connection for long-running queries
    extern pthread_mutex_t webserver_suspend_lock;
    extern volatile bool webserver_thread_suspended;

    pthread_mutex_lock(&webserver_suspend_lock);
    webserver_thread_suspended = true;
    MHD_suspend_connection(connection);

    // Step 7: Wait for all queries to complete
    int collective_timeout = 30;  // Use a reasonable collective timeout
    int wait_result = pending_result_wait_multiple(pending_results, query_count, collective_timeout, SR_AUTH);
    if (wait_result != 0) {
        log_this(SR_AUTH, "handle_conduit_alt_queries_request: Some queries failed or timed out", LOG_LEVEL_ERROR, 0);
    }

    // Step 8: Resume connection processing
    MHD_resume_connection(connection);
    webserver_thread_suspended = false;
    pthread_mutex_unlock(&webserver_suspend_lock);

    // Step 9: Build responses for all queries (map back to original order)
    json_t *results_array = json_array();
    bool all_success = true;

    // Build results for unique queries first
    json_t **unique_results = calloc(unique_query_count, sizeof(json_t*));
    if (!unique_results) {
        log_this(SR_AUTH, "handle_conduit_alt_queries_request: Failed to allocate unique_results array", LOG_LEVEL_ERROR, 0);
        free(pending_results);
        free(query_refs);
        free(cache_entries);
        free(selected_queues);
        free(mapping_array);
        free(is_duplicate);
        json_decref(deduplicated_queries);
        json_decref(queries_array);
        free(database);

        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Internal server error"));
        return api_send_json_response(connection, error_response, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }
    
    for (size_t i = 0; i < unique_query_count; i++) {
        unique_results[i] = build_response_json(query_refs[i], database, cache_entries[i], selected_queues[i], pending_results[i], NULL);
    }

    // Map results back to original query order
    for (size_t i = 0; i < original_query_count; i++) {
        json_t *query_result;
        if (is_duplicate[i]) {
            // For duplicates, return an error
            query_result = json_object();
            json_object_set_new(query_result, "success", json_false());
            json_object_set_new(query_result, "error", json_string("Duplicate query"));
            all_success = false;
        } else {
            // Get result from unique results array
            size_t unique_idx = mapping_array[i];
            if (unique_idx < unique_query_count) {
                query_result = json_deep_copy(unique_results[unique_idx]);
                // Check if query succeeded
                json_t *success_field = json_object_get(query_result, "success");
                if (!success_field || !json_is_true(success_field)) {
                    all_success = false;
                }
            } else {
                query_result = json_object();
                json_object_set_new(query_result, "success", json_false());
                json_object_set_new(query_result, "error", json_string("Internal error: invalid query mapping"));
                all_success = false;
            }
        }
        json_array_append_new(results_array, query_result);
    }

    // Clean up unique results
    for (size_t i = 0; i < unique_query_count; i++) {
        json_decref(unique_results[i]);
    }
    free(unique_results);

    // Clean up
    free(pending_results);
    free(query_refs);
    free(cache_entries);
    free(selected_queues);
    free(mapping_array);
    free(is_duplicate);

    json_decref(deduplicated_queries);
    json_decref(queries_array);

    // Step 10: Calculate total execution time
    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    long total_time_ms = ((end_time.tv_sec - start_time.tv_sec) * 1000) +
                        ((end_time.tv_nsec - start_time.tv_nsec) / 1000000);

    // Step 11: Build response
    json_t *response_obj = json_object();
    json_object_set_new(response_obj, "success", json_boolean(all_success));
    json_object_set_new(response_obj, "results", results_array);
    json_object_set_new(response_obj, "database", json_string(database));
    json_object_set_new(response_obj, "total_execution_time_ms", json_integer(total_time_ms));

    // Add DQM statistics
    if (global_queue_manager) {
        json_t* dqm_stats = database_queue_manager_get_stats_json(global_queue_manager);
        if (dqm_stats) {
            json_object_set_new(response_obj, "dqm_statistics", dqm_stats);
        }
    }

    free(database);

    log_this(SR_AUTH, "handle_conduit_alt_queries_request: Request completed, original=%zu unique=%zu time=%ldms",
             LOG_LEVEL_DEBUG, 3, original_query_count, unique_query_count, total_time_ms);

    return api_send_json_response(connection, response_obj, MHD_HTTP_OK);
}
