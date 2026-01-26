/**
 * @file queries.c
 * @brief Public Conduit Queries API endpoint implementation
 *
 * This module implements the public database queries execution endpoint.
 * It executes multiple pre-defined queries in parallel without authentication,
 * requiring an explicit database parameter in the request.
 *
 * @author Hydrogen Framework
 * @date 2026-01-14
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
#include <src/api/conduit/queries/queries.h>
#include <src/api/conduit/conduit_helpers.h>
#include <src/api/conduit/conduit_service.h>
#include <src/config/config.h>
#include <src/config/config_databases.h>
#include <src/logging/logging.h>

/**
 * @brief Deduplicate queries and validate rate limits
 *
 * Processes the queries array to remove duplicates by query_ref and validates
 * against the MaxQueriesPerRequest limit for the specified database.
 *
 * @param queries_array The input queries array from the request
 * @param database The database name to check rate limits against
 * @param deduplicated_queries Output array of unique query objects (caller must decref)
 * @param mapping_array Output array mapping original indices to deduplicated indices
 * @return MHD_YES on success, MHD_NO on rate limit exceeded
 */
enum MHD_Result deduplicate_and_validate_queries(
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
        log_this(SR_API, "deduplicate_and_validate_queries: NULL parameters", LOG_LEVEL_ERROR, 0);
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
        // This handles cases where the database parameter is not the connection name
        for (int i = 0; i < app_config->databases.connection_count; i++) {
            const DatabaseConnection *conn = &app_config->databases.connections[i];
            if (conn->enabled && conn->connection_name && strcmp(conn->connection_name, database) == 0) {
                db_conn = conn;
                break;
            }
        }
        if (!db_conn) {
            log_this(SR_API, "deduplicate_and_validate_queries: Database connection not found: %s", LOG_LEVEL_ALERT, 1, database);
            if (result_code) {
                *result_code = DEDUP_DATABASE_NOT_FOUND;
            }
            return MHD_NO;  // Let the caller handle the error
        }
    }

    // Allocate memory for duplicate tracking
    *is_duplicate = calloc(original_count, sizeof(bool));
    if (!*is_duplicate) {
        log_this(SR_API, "deduplicate_and_validate_queries: Failed to allocate memory for duplicate tracking", LOG_LEVEL_ERROR, 0);
        return MHD_NO;
    }

    // Create arrays to track unique queries (query_ref + params) and their mapping
    int *query_refs = calloc(original_count, sizeof(int));
    json_t **query_params = calloc(original_count, sizeof(json_t*));
    size_t *first_occurrence = calloc(original_count, sizeof(size_t));
    size_t unique_count = 0;

    if (!query_refs || !query_params || !first_occurrence) {
        log_this(SR_API, "deduplicate_and_validate_queries: Failed to allocate memory", LOG_LEVEL_ERROR, 0);
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
            query_params[unique_count] = params;  // We don't need to decref since it's a reference to the original
            first_occurrence[unique_count] = i;
            unique_count++;
        }
    }

    // Check rate limit
    if ((int)unique_count > db_conn->max_queries_per_request) {
        log_this(SR_API, "deduplicate_and_validate_queries: Rate limit exceeded: %zu unique queries > %d max for database %s",
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
        log_this(SR_API, "deduplicate_and_validate_queries: Failed to allocate output arrays", LOG_LEVEL_ERROR, 0);
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

    log_this(SR_API, "deduplicate_and_validate_queries: Deduplicated %zu queries to %zu unique queries",
             LOG_LEVEL_DEBUG, 2, original_count, unique_count);

    free(query_refs);
    free(query_params);
    free(first_occurrence);

    return MHD_YES;
}

/**
 * @brief Execute a single query using existing helpers
 *
 * Executes a single query from the queries array using the existing
 * conduit infrastructure helpers from query.h.
 *
 * @param database Database name from request
 * @param query_obj Query object containing query_ref and optional params
 * @return JSON object with query result or error
 */
json_t* execute_single_query(const char *database, json_t *query_obj)
{
    if (!database || !query_obj) {
        log_this(SR_API, "execute_single_query: NULL parameters", LOG_LEVEL_ERROR, 0);
        json_t *error_result = json_object();
        json_object_set_new(error_result, "success", json_false());
        json_object_set_new(error_result, "error", json_string("Invalid query object"));
        return error_result;
    }

    // Extract query_ref
    json_t *query_ref_json = json_object_get(query_obj, "query_ref");
    if (!query_ref_json || !json_is_integer(query_ref_json)) {
        log_this(SR_API, "execute_single_query: Missing or invalid query_ref", LOG_LEVEL_ERROR, 0);
        json_t *error_result = json_object();
        json_object_set_new(error_result, "success", json_false());
        json_object_set_new(error_result, "error", json_string("Missing required field: query_ref"));
        return error_result;
    }

    int query_ref = (int)json_integer_value(query_ref_json);
    json_t *params = json_object_get(query_obj, "params");

    // Look up database queue and cache entry using new helper function
    DatabaseQueue *db_queue = NULL;
    QueryCacheEntry *cache_entry = NULL;
    
    if (!lookup_database_and_public_query(&db_queue, &cache_entry, database, query_ref)) {
        const char *error_msg = !db_queue ? "Database not available" : "Public query not found";
        const char *message = !db_queue ? "Database is not available" : NULL;
        json_t *error_result = create_lookup_error_response(error_msg, database, query_ref, !db_queue, message);
        return error_result;
    }

    // Process parameters with validation
    ParameterList *param_list = NULL;
    char *converted_sql = NULL;
    TypedParameter **ordered_params = NULL;
    size_t param_count = 0;
    char* message = NULL;

    // (1) Validate parameter types
    char* type_error = validate_parameter_types_simple(params);
    if (type_error) {
        json_t *error_response = create_processing_error_response("Parameter type mismatch", database, query_ref);
        json_object_set_new(error_response, "message", json_string(type_error));
        free(type_error);
        return error_response;
    }

    // (2) Check for missing parameters
    ParameterList* temp_param_list = NULL;
    if (params && json_is_object(params)) {
        char* params_str = json_dumps(params, JSON_COMPACT);
        if (params_str) {
            temp_param_list = parse_typed_parameters(params_str, NULL);
            free(params_str);
        }
    }
    if (!temp_param_list) {
        temp_param_list = calloc(1, sizeof(ParameterList));
    }

    char* missing_error = check_missing_parameters_simple(cache_entry->sql_template, temp_param_list);
    if (missing_error) {
        // Clean up temp list
        if (temp_param_list) {
            if (temp_param_list->params) {
                for (size_t i = 0; i < temp_param_list->count; i++) {
                    if (temp_param_list->params[i]) {
                        free(temp_param_list->params[i]->name);
                        free(temp_param_list->params[i]);
                    }
                }
                free(temp_param_list->params);
            }
            free(temp_param_list);
        }
        
        json_t *error_response = create_processing_error_response("Missing parameters", database, query_ref);
        json_object_set_new(error_response, "message", json_string(missing_error));
        free(missing_error);
        return error_response;
    }

    // (3) Process parameters
    if (!process_parameters(params, &param_list, cache_entry->sql_template,
                            db_queue->engine_type, &converted_sql, &ordered_params, &param_count)) {
        // Clean up temp list
        if (temp_param_list) {
            if (temp_param_list->params) {
                for (size_t i = 0; i < temp_param_list->count; i++) {
                    if (temp_param_list->params[i]) {
                        free(temp_param_list->params[i]->name);
                        free(temp_param_list->params[i]);
                    }
                }
                free(temp_param_list->params);
            }
            free(temp_param_list);
        }
        
        return create_processing_error_response("Parameter processing failed", database, query_ref);
    }

    // (4) Check for unused parameters (warning only)
    char* unused_warning = check_unused_parameters_simple(cache_entry->sql_template, param_list);
    if (unused_warning) {
        log_this(SR_API, "%s: Query %d has unused parameters: %s", LOG_LEVEL_ALERT, 3, conduit_service_name(), query_ref, unused_warning);
        message = unused_warning;
    }

    // Clean up temp list
    if (temp_param_list) {
        if (temp_param_list->params) {
            for (size_t i = 0; i < temp_param_list->count; i++) {
                if (temp_param_list->params[i]) {
                    free(temp_param_list->params[i]->name);
                    free(temp_param_list->params[i]);
                }
            }
            free(temp_param_list->params);
        }
        free(temp_param_list);
    }

    // Generate parameter validation messages
    char* validation_message = generate_parameter_messages(cache_entry->sql_template, params);
    if (validation_message) {
        if (message) {
            // Combine the messages
            size_t combined_length = strlen(message) + strlen(validation_message) + 3; // +3 for " | " and null terminator
            char* combined_message = malloc(combined_length);
            if (combined_message) {
                snprintf(combined_message, combined_length, "%s | %s", message, validation_message);
                free(message);
                free(validation_message);
                message = combined_message;
            }
        } else {
            message = validation_message;
        }
    }

    // Select queue using new helper function
    DatabaseQueue *selected_queue = select_query_queue(database, cache_entry->queue_type);
    if (!selected_queue) {
        free(converted_sql);
        free_parameter_list(param_list);
        if (ordered_params) free(ordered_params);
        if (message) free(message);
        return create_processing_error_response("No suitable queue available", database, query_ref);
    }

    // Generate query ID using new helper function
    char *query_id = generate_query_id();
    if (!query_id) {
        free(converted_sql);
        free_parameter_list(param_list);
        if (ordered_params) free(ordered_params);
        if (message) free(message);
        return create_processing_error_response("Failed to generate query ID", database, query_ref);
    }

    // Register pending result
    PendingResultManager *pending_mgr = get_pending_result_manager();
    PendingQueryResult *pending = pending_result_register(pending_mgr, query_id, cache_entry->timeout_seconds, NULL);
    if (!pending) {
        free(query_id);
        free(converted_sql);
        free_parameter_list(param_list);
        if (ordered_params) free(ordered_params);
        if (message) free(message);
        return create_processing_error_response("Failed to register pending result", database, query_ref);
    }

    // Submit query using new helper function
    if (!prepare_and_submit_query(selected_queue, query_id, cache_entry->sql_template, ordered_params,
                                  param_count, cache_entry)) {
        free(query_id);
        free(converted_sql);
        free_parameter_list(param_list);
        if (ordered_params) free(ordered_params);
        if (message) free(message);
        return create_processing_error_response("Failed to submit query", database, query_ref);
    }

    // Build response using new helper function
    json_t *result = build_response_json(query_ref, database, cache_entry, selected_queue, pending, message);

    // Clean up
    free(query_id);
    free(converted_sql);
    free_parameter_list(param_list);
    if (ordered_params) free(ordered_params);
    if (message) free(message);

    log_this(SR_API, "execute_single_query: Query completed, query_ref=%d",
             LOG_LEVEL_DEBUG, 1, query_ref);

    return result;
}



/**
 * @brief Handle public conduit queries request
 *
 * Main handler for the /api/conduit/queries endpoint. Executes multiple
 * pre-defined queries in parallel without authentication.
 *
 * Request format (POST JSON):
 * {
 *   "database": "database_name",
 *   "queries": [
 *     {
 *       "query_ref": 1234,
 *       "params": { "INTEGER": {...}, "STRING": {...} }
 *     },
 *     {
 *       "query_ref": 5678,
 *       "params": { "INTEGER": {...}, "STRING": {...} }
 *     }
 *   ]
 * }
 *
 * Response format:
 * {
 *   "success": true,
 *   "results": [ {...}, {...} ],
 *   "database": "database_name",
 *   "total_execution_time_ms": 123
 * }
 *
 * @param connection The HTTP connection
 * @param url The request URL
 * @param method The HTTP method (GET or POST)
 * @param upload_data Request body data
 * @param upload_data_size Size of request body
 * @param con_cls Connection-specific data
 * @return HTTP response code
 */
enum MHD_Result handle_conduit_queries_request(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *upload_data,
    const size_t *upload_data_size,
    void **con_cls)
{
    (void)url;  // Unused parameter

    // Use common POST body buffering (handles POST requests)
    ApiPostBuffer *buffer = NULL;
    ApiBufferResult buf_result = api_buffer_post_data(method, upload_data, (size_t*)upload_data_size, con_cls, &buffer);

    switch (buf_result) {
        case API_BUFFER_CONTINUE:
            // More data expected for POST, continue receiving
            return MHD_YES;

        case API_BUFFER_ERROR:
            // Error occurred during buffering
            return api_send_error_and_cleanup(connection, con_cls,
                "Request processing error", MHD_HTTP_INTERNAL_SERVER_ERROR);

        case API_BUFFER_METHOD_ERROR:
            // Unsupported HTTP method
            return api_send_error_and_cleanup(connection, con_cls,
                "Method not allowed - use POST", MHD_HTTP_METHOD_NOT_ALLOWED);

        case API_BUFFER_COMPLETE:
            // All data received, continue with processing
            break;
    }

    log_this(SR_API, "%s: Processing public queries request", LOG_LEVEL_DEBUG, 1, conduit_service_name());

    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    log_this(SR_API, "%s: Step 1 - Validate HTTP method", LOG_LEVEL_DEBUG, 1, conduit_service_name());

    // Step 1: Validate HTTP method using new helper function
    enum MHD_Result result = handle_method_validation(connection, method);
    if (result != MHD_YES) {
        api_free_post_buffer(con_cls);
        log_this(SR_API, "%s: Method validation failed", LOG_LEVEL_ERROR, 1, conduit_service_name());
        return result;
    }

    log_this(SR_API, "%s: Step 2 - Parse request", LOG_LEVEL_DEBUG, 1, conduit_service_name());

    // Step 2: Parse request using new helper function
    json_t *request_json = NULL;
    result = handle_request_parsing_with_buffer(connection, buffer, &request_json);

    // Free the buffer now that we've parsed the data
    api_free_post_buffer(con_cls);

    if (result != MHD_YES) {
        log_this(SR_API, "%s: Request parsing failed", LOG_LEVEL_ERROR, 1, conduit_service_name());
        return result;
    }

    log_this(SR_API, "handle_conduit_queries_request: Step 3 - Extract database field", LOG_LEVEL_DEBUG, 0);

    // Step 3: Extract database field
    json_t *database_json = json_object_get(request_json, "database");
    if (!database_json || !json_is_string(database_json)) {
        log_this(SR_API, "handle_conduit_queries_request: Missing or invalid database field", LOG_LEVEL_ERROR, 0);
        json_decref(request_json);

        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Missing required parameter: database"));
        return api_send_json_response(connection, error_response, MHD_HTTP_BAD_REQUEST);
    }

    const char *database = json_string_value(database_json);
    log_this(SR_API, "handle_conduit_queries_request: Database = %s", LOG_LEVEL_DEBUG, 1, database);

    log_this(SR_API, "handle_conduit_queries_request: Step 4 - Extract queries array", LOG_LEVEL_DEBUG, 0);

    // Step 4: Extract queries array
    json_t *queries_array = json_object_get(request_json, "queries");
    if (!queries_array || !json_is_array(queries_array)) {
        log_this(SR_API, "handle_conduit_queries_request: Missing or invalid queries field", LOG_LEVEL_ERROR, 0);
        json_decref(request_json);

        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Missing required parameter: queries (must be array)"));
        return api_send_json_response(connection, error_response, MHD_HTTP_BAD_REQUEST);
    }

    size_t original_query_count = json_array_size(queries_array);
    log_this(SR_API, "handle_conduit_queries_request: Found %zu queries in array", LOG_LEVEL_DEBUG, 1, original_query_count);

    if (original_query_count == 0) {
        log_this(SR_API, "handle_conduit_queries_request: Empty queries array", LOG_LEVEL_ERROR, 0);
        json_decref(request_json);

        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Queries array cannot be empty"));
        json_object_set_new(error_response, "results", json_array());
        json_object_set_new(error_response, "database", json_string(database));
        json_object_set_new(error_response, "total_execution_time_ms", json_integer(0));
        return api_send_json_response(connection, error_response, MHD_HTTP_OK);
    }

    log_this(SR_API, "%s: Step 5 - Deduplicate queries and validate rate limits", LOG_LEVEL_DEBUG, 1, conduit_service_name());

    // Step 5: Deduplicate queries and validate rate limits
    json_t *deduplicated_queries = NULL;
    size_t *mapping_array = NULL;
    bool *is_duplicate = NULL;
    DeduplicationResult dedup_code;
    enum MHD_Result dedup_result = deduplicate_and_validate_queries(connection, queries_array, database, &deduplicated_queries, &mapping_array, &is_duplicate, &dedup_code);
    
    bool rate_limit_exceeded = false;
    int max_queries_per_request = 0;
    size_t unique_query_count = 0;
    
    if (dedup_result != MHD_YES) {
        log_this(SR_API, "%s: Validation failed with code %d", LOG_LEVEL_ERROR, 2, conduit_service_name(), dedup_code);
        
        if (dedup_code == DEDUP_RATE_LIMIT) {
            // Get the max queries limit for the database
            const DatabaseConnection *db_conn = find_database_connection(&app_config->databases, database);
            if (db_conn) {
                max_queries_per_request = db_conn->max_queries_per_request;
            }
            rate_limit_exceeded = true;
            
            // When rate limit is exceeded, we still want to execute queries up to the limit
            // So we need to manually create the deduplicated queries array with only the first max_queries_per_request queries
            unique_query_count = (size_t)max_queries_per_request;
            
            // Create deduplicated_queries array with only the queries up to the limit
            deduplicated_queries = json_array();
            for (size_t i = 0; i < unique_query_count; i++) {
                json_t *query = json_array_get(queries_array, i);
                json_array_append_new(deduplicated_queries, json_deep_copy(query));
            }
            
            // Create mapping array and is_duplicate array
            mapping_array = malloc(original_query_count * sizeof(size_t));
            is_duplicate = malloc(original_query_count * sizeof(bool));
            
            for (size_t i = 0; i < original_query_count; i++) {
                if (i < unique_query_count) {
                    mapping_array[i] = i;  // Map to itself
                    is_duplicate[i] = false;
                } else {
                    mapping_array[i] = 0;  // Map to first query (will be handled as exceeded)
                    is_duplicate[i] = true;
                }
            }
        } else {
            // For other validation errors, return immediately
            json_decref(request_json);

            json_t *error_response = json_object();
            json_object_set_new(error_response, "success", json_false());
            
            const char *error_msg = "Validation failed";
            unsigned int http_status = MHD_HTTP_BAD_REQUEST;
            
            switch (dedup_code) {
                case DEDUP_DATABASE_NOT_FOUND:
                    error_msg = "Invalid database";
                    http_status = MHD_HTTP_BAD_REQUEST;
                    break;
                case DEDUP_RATE_LIMIT:
                    error_msg = "Rate limit exceeded: too many unique queries in request";
                    http_status = MHD_HTTP_TOO_MANY_REQUESTS;
                    break;
                case DEDUP_OK:
                    // Shouldn't get here since dedup_result == MHD_NO
                    error_msg = "Validation failed";
                    http_status = MHD_HTTP_BAD_REQUEST;
                    break;
                case DEDUP_ERROR:
                    error_msg = "Validation failed";
                    http_status = MHD_HTTP_BAD_REQUEST;
                    break;
            }
            
            json_object_set_new(error_response, "error", json_string(error_msg));
            return api_send_json_response(connection, error_response, http_status);
        }
    } else {
        // Normal case - deduplication succeeded
        unique_query_count = json_array_size(deduplicated_queries);
    }

    log_this(SR_API, "%s: Deduplicated %zu queries to %zu unique queries",
             LOG_LEVEL_DEBUG, 3, conduit_service_name(), original_query_count, unique_query_count);

    log_this(SR_API, "%s: Step 6 - Execute queries", LOG_LEVEL_DEBUG, 1, conduit_service_name());

    // Step 6: Execute all unique queries using execute_single_query which now uses new helpers
    json_t *results_array = json_array();
    bool all_success = true;

    // Execute unique queries and store results
    json_t **unique_results = calloc(unique_query_count, sizeof(json_t*));
    
    if (!unique_results) {
        log_this(SR_API, "%s: Failed to allocate memory for query results", LOG_LEVEL_ERROR, 1, conduit_service_name());
        json_decref(deduplicated_queries);
        free(mapping_array);
        free(is_duplicate);
        json_decref(request_json);
        
        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Internal server error"));
        return api_send_json_response(connection, error_response, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }
    
    for (size_t i = 0; i < unique_query_count; i++) {
        log_this(SR_API, "%s: Executing unique query %zu", LOG_LEVEL_DEBUG, 2, conduit_service_name(), i);
        
        json_t *query_obj = json_array_get(deduplicated_queries, i);
        json_t *query_result = execute_single_query(database, query_obj);
        
        if (!query_result) {
            log_this(SR_API, "%s: Failed to execute unique query %zu", LOG_LEVEL_ERROR, 2, conduit_service_name(), i);
            query_result = json_object();
            json_object_set_new(query_result, "success", json_false());
            json_object_set_new(query_result, "error", json_string("Query execution failed"));
            all_success = false;
        } else {
            // Check if query succeeded
            json_t *success_field = json_object_get(query_result, "success");
            if (!success_field || !json_is_true(success_field)) {
                all_success = false;
                log_this(SR_API, "%s: Unique query %zu failed", LOG_LEVEL_DEBUG, 2, conduit_service_name(), i);
            }
        }
        
        unique_results[i] = query_result;
        log_this(SR_API, "%s: Unique query %zu completed", LOG_LEVEL_DEBUG, 2, conduit_service_name(), i);
    }
    
    // Map results back to original query order
    for (size_t i = 0; i < original_query_count; i++) {
        if (rate_limit_exceeded && (int)i >= max_queries_per_request) {
            // For queries beyond the rate limit, return an error with limit information
            json_t *rate_limit_error = json_object();
            json_object_set_new(rate_limit_error, "success", json_false());
            json_object_set_new(rate_limit_error, "error", json_string("Rate limit exceeded"));
            
            // Add informative message with the limit
            char limit_msg[100];
            snprintf(limit_msg, sizeof(limit_msg), "Query limit of %d unique queries per request exceeded", max_queries_per_request);
            json_object_set_new(rate_limit_error, "message", json_string(limit_msg));
            
            json_array_append_new(results_array, rate_limit_error);
            all_success = false;
        } else if (is_duplicate[i]) {
            // For duplicates, return an error instead of executing the query
            json_t *duplicate_error = json_object();
            json_object_set_new(duplicate_error, "success", json_false());
            json_object_set_new(duplicate_error, "error", json_string("Duplicate query"));
            json_array_append_new(results_array, duplicate_error);
            all_success = false;
        } else {
            // Ensure we don't access beyond the unique_results array bounds
            if (mapping_array[i] < unique_query_count) {
                json_array_append_new(results_array, unique_results[mapping_array[i]]);
            } else {
                // This shouldn't happen, but handle it gracefully
                json_t *error_response = json_object();
                json_object_set_new(error_response, "success", json_false());
                json_object_set_new(error_response, "error", json_string("Internal error: invalid query mapping"));
                json_array_append_new(results_array, error_response);
                all_success = false;
            }
        }
    }
    
    // Clean up
    free(unique_results);
    json_decref(deduplicated_queries);
    free(mapping_array);
    free(is_duplicate);

    json_decref(request_json);

    // Step 6: Calculate total execution time
    log_this(SR_API, "%s: Calculating total execution time", LOG_LEVEL_DEBUG, 1, conduit_service_name());
    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    long total_time_ms = ((end_time.tv_sec - start_time.tv_sec) * 1000) +
                        ((end_time.tv_nsec - start_time.tv_nsec) / 1000000);

    // Step 7: Determine appropriate HTTP status code
    log_this(SR_API, "%s: Determining HTTP status code", LOG_LEVEL_DEBUG, 1, conduit_service_name());
    unsigned int http_status = MHD_HTTP_OK;
    
    if (!all_success) {
        // Check if we encountered a rate limit error
        bool rate_limit = false;
        bool has_parameter_errors = false;
        bool has_database_errors = false;
        bool has_duplicate_errors = false;
        
        for (size_t i = 0; i < original_query_count; i++) {
            json_t *single_result = json_array_get(results_array, i);
            if (!single_result) continue;
            
            json_t *error = json_object_get(single_result, "error");
            if (error && json_is_string(error)) {
                const char *error_str = json_string_value(error);
                
                if (strstr(error_str, "Rate limit")) {
                    rate_limit = true;
                    http_status = MHD_HTTP_TOO_MANY_REQUESTS;
                } else if (strstr(error_str, "Duplicate")) {
                    has_duplicate_errors = true;
                    // Don't set http_status here, duplicates alone shouldn't cause non-200 status
                } else if (strstr(error_str, "Parameter") ||
                           strstr(error_str, "Missing") ||
                           strstr(error_str, "Invalid")) {
                    has_parameter_errors = true;
                    http_status = MHD_HTTP_BAD_REQUEST;
                } else if (strstr(error_str, "Auth") ||
                           strstr(error_str, "Permission") ||
                           strstr(error_str, "Unauthorized")) {
                    http_status = MHD_HTTP_UNAUTHORIZED;
                } else if (strstr(error_str, "Not found")) {
                    http_status = MHD_HTTP_NOT_FOUND;
                } else {
                    // Database execution errors and other issues
                    has_database_errors = true;
                    http_status = MHD_HTTP_UNPROCESSABLE_ENTITY;
                }
            }
        }
        
        // Apply highest return code strategy
        // If we have rate limiting, that takes precedence (429)
        if (rate_limit) {
            http_status = MHD_HTTP_TOO_MANY_REQUESTS;
        }
        // Parameter errors take precedence over database errors (400 > 422)
        else if (has_parameter_errors) {
            http_status = MHD_HTTP_BAD_REQUEST;
        }
        // Database execution errors (422)
        else if (has_database_errors) {
            http_status = MHD_HTTP_UNPROCESSABLE_ENTITY;
        }
        // If only duplicates exist, keep HTTP 200 (duplicates are not errors)
        else if (has_duplicate_errors) {
            http_status = MHD_HTTP_OK;
        }
    }

    // Step 8: Build response
    log_this(SR_API, "%s: Building final response object", LOG_LEVEL_DEBUG, 1, conduit_service_name());
    json_t *response_obj = json_object();
    if (!response_obj) {
        log_this(SR_API, "%s: ERROR - Failed to create response object", LOG_LEVEL_ERROR, 1, conduit_service_name());
        json_decref(results_array);
        return MHD_NO;
    }

    json_object_set_new(response_obj, "success", json_boolean(all_success));
    json_object_set_new(response_obj, "results", results_array);
    json_object_set_new(response_obj, "database", json_string(database));
    json_object_set_new(response_obj, "total_execution_time_ms", json_integer(total_time_ms));

    log_this(SR_API, "%s: Request completed, queries=%zu, time=%ldms, status=%d",
             LOG_LEVEL_DEBUG, 4, conduit_service_name(), original_query_count, total_time_ms, http_status);

    log_this(SR_API, "%s: Calling api_send_json_response", LOG_LEVEL_DEBUG, 1, conduit_service_name());
    enum MHD_Result send_result = api_send_json_response(connection, response_obj, http_status);
    log_this(SR_API, "%s: api_send_json_response returned %d", LOG_LEVEL_DEBUG, 1, conduit_service_name(), send_result);

    return send_result;
}
