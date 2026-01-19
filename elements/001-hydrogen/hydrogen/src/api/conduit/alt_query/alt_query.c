/**
 * @file alt_query.c
 * @brief Alternative Authenticated Conduit Query API endpoint implementation
 *
 * This module implements the authenticated database query execution endpoint
 * with database override capability. It validates JWT tokens before executing
 * queries and allows specifying a different database than the one in the JWT claims.
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

// Third-party libraries
#include <jansson.h>

// Project headers
#include <src/api/api_service.h>
#include <src/api/api_utils.h>  // For api_send_json_response
#include <src/api/conduit/alt_query/alt_query.h>
#include <src/api/conduit/query/query.h>
#include <src/api/auth/auth_service.h>
#include <src/api/auth/auth_service_jwt.h>
#include <src/config/config.h>
#include <src/logging/logging.h>

/**
 * @brief Validate JWT token for authentication (without extracting database)
 *
 * Validates the provided JWT token for authentication purposes only.
 * Unlike auth_query, this does not extract the database from the token.
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
        char *response_str = json_dumps(error_response, JSON_COMPACT);
        json_decref(error_response);

        struct MHD_Response *response = MHD_create_response_from_buffer(
            strlen(response_str), response_str, MHD_RESPMEM_MUST_FREE);
        MHD_add_response_header(response, "Content-Type", "application/json");
        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
        MHD_destroy_response(response);
        return ret;
    }

    // Validate JWT token - pass NULL since database comes from request
    jwt_validation_result_t result = validate_jwt(token, NULL);
    if (!result.valid || !result.claims) {
        log_this(SR_AUTH, "validate_jwt_for_auth: JWT validation failed", LOG_LEVEL_ALERT, 0);
        free_jwt_validation_result(&result);

        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Invalid or expired JWT token"));
        char *response_str = json_dumps(error_response, JSON_COMPACT);
        json_decref(error_response);

        struct MHD_Response *response = MHD_create_response_from_buffer(
            strlen(response_str), response_str, MHD_RESPMEM_MUST_FREE);
        MHD_add_response_header(response, "Content-Type", "application/json");
        MHD_queue_response(connection, MHD_HTTP_UNAUTHORIZED, response);
        MHD_destroy_response(response);
        return MHD_NO;
    }

    log_this(SR_AUTH, "validate_jwt_for_auth: JWT validated successfully", LOG_LEVEL_DEBUG, 0);
    free_jwt_validation_result(&result);
    return MHD_YES;
}

/**
 * @brief Parse alternative authenticated request and extract fields
 *
 * Parses the request JSON (from GET params or POST body) and extracts
 * the JWT token, database, query_ref, and optional params fields.
 *
 * @param connection The HTTP connection for error responses
 * @param method The HTTP method (GET or POST)
 * @param upload_data Request body data
 * @param upload_data_size Size of request body
 * @param token Output parameter for JWT token
 * @param database Output parameter for database name
 * @param query_ref Output parameter for query reference ID
 * @param params_json Output parameter for query parameters
 * @return MHD_YES on success, MHD_NO on parsing failure
 */
static enum MHD_Result parse_alt_request(
    struct MHD_Connection *connection,
    const char *method,
    const char *upload_data,
    const size_t *upload_data_size,
    char **token,
    char **database,
    int *query_ref,
    json_t **params_json)
{
    if (!method || !token || !database || !query_ref || !params_json) {
        log_this(SR_AUTH, "parse_alt_request: NULL parameters", LOG_LEVEL_ERROR, 0);
        return MHD_NO;
    }

    // Parse request data (handles both GET and POST)
    json_t *request_json = parse_request_data(connection, method, upload_data, upload_data_size);
    if (!request_json) {
        log_this(SR_AUTH, "parse_alt_request: Failed to parse request data", LOG_LEVEL_ERROR, 0);
        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Invalid request format"));
        char *response_str = json_dumps(error_response, JSON_COMPACT);
        json_decref(error_response);

        struct MHD_Response *response = MHD_create_response_from_buffer(
            strlen(response_str), response_str, MHD_RESPMEM_MUST_FREE);
        MHD_add_response_header(response, "Content-Type", "application/json");
        MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
        MHD_destroy_response(response);
        return MHD_NO;
    }

    // Extract token field
    json_t *token_json = json_object_get(request_json, "token");
    if (!token_json || !json_is_string(token_json)) {
        log_this(SR_AUTH, "parse_alt_request: Missing or invalid token field", LOG_LEVEL_ERROR, 0);
        json_decref(request_json);

        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Missing required parameter: token"));
        char *response_str = json_dumps(error_response, JSON_COMPACT);
        json_decref(error_response);

        struct MHD_Response *response = MHD_create_response_from_buffer(
            strlen(response_str), response_str, MHD_RESPMEM_MUST_FREE);
        MHD_add_response_header(response, "Content-Type", "application/json");
        MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
        MHD_destroy_response(response);
        return MHD_NO;
    }

    *token = strdup(json_string_value(token_json));
    if (!*token) {
        log_this(SR_AUTH, "parse_alt_request: Failed to allocate token string", LOG_LEVEL_ERROR, 0);
        json_decref(request_json);
        return MHD_NO;
    }

    // Extract database field
    json_t *database_json = json_object_get(request_json, "database");
    if (!database_json || !json_is_string(database_json)) {
        log_this(SR_AUTH, "parse_alt_request: Missing or invalid database field", LOG_LEVEL_ERROR, 0);
        free(*token);
        json_decref(request_json);

        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Missing required parameter: database"));
        char *response_str = json_dumps(error_response, JSON_COMPACT);
        json_decref(error_response);

        struct MHD_Response *response = MHD_create_response_from_buffer(
            strlen(response_str), response_str, MHD_RESPMEM_MUST_FREE);
        MHD_add_response_header(response, "Content-Type", "application/json");
        MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
        MHD_destroy_response(response);
        return MHD_NO;
    }

    *database = strdup(json_string_value(database_json));
    if (!*database) {
        log_this(SR_AUTH, "parse_alt_request: Failed to allocate database string", LOG_LEVEL_ERROR, 0);
        free(*token);
        json_decref(request_json);
        return MHD_NO;
    }

    // Extract query_ref field
    json_t *query_ref_json = json_object_get(request_json, "query_ref");
    if (!query_ref_json || !json_is_integer(query_ref_json)) {
        log_this(SR_AUTH, "parse_alt_request: Missing or invalid query_ref field", LOG_LEVEL_ERROR, 0);
        free(*token);
        free(*database);
        json_decref(request_json);

        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Missing required parameter: query_ref"));
        char *response_str = json_dumps(error_response, JSON_COMPACT);
        json_decref(error_response);

        struct MHD_Response *response = MHD_create_response_from_buffer(
            strlen(response_str), response_str, MHD_RESPMEM_MUST_FREE);
        MHD_add_response_header(response, "Content-Type", "application/json");
        MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
        MHD_destroy_response(response);
        return MHD_NO;
    }

    *query_ref = (int)json_integer_value(query_ref_json);

    // Extract optional params field
    json_t *params = json_object_get(request_json, "params");
    if (params && json_is_object(params)) {
        *params_json = json_deep_copy(params);
    } else {
        *params_json = NULL;
    }

    json_decref(request_json);
    log_this(SR_AUTH, "parse_alt_request: Successfully parsed, database=%s, query_ref=%d", LOG_LEVEL_DEBUG, 2, *database, *query_ref);
    return MHD_YES;
}

/**
 * @brief Handle alternative authenticated conduit query request
 *
 * Main handler for the /api/conduit/alt_query endpoint. Validates JWT token,
 * extracts database from request body (overriding JWT claims), and executes
 * the requested query.
 *
 * @param connection The HTTP connection
 * @param url The request URL
 * @param method The HTTP method (GET or POST)
 * @param upload_data Request body data
 * @param upload_data_size Size of request body
 * @param con_cls Connection-specific data
 * @return HTTP response code
 */
enum MHD_Result handle_conduit_alt_query_request(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *upload_data,
    const size_t *upload_data_size,
    void **con_cls)
{
    (void)url;
    (void)con_cls;

    log_this(SR_AUTH, "handle_conduit_alt_query_request: Processing alternative authenticated query request", LOG_LEVEL_DEBUG, 0);

    // Step 1: Validate HTTP method
    enum MHD_Result result = handle_method_validation(connection, method);
    if (result != MHD_YES) {
        return result;
    }

    // Step 2: Parse request and extract token, database, query_ref, params
    char *token = NULL;
    char *database = NULL;
    int query_ref = 0;
    json_t *params_json = NULL;

    result = parse_alt_request(connection, method, upload_data, upload_data_size,
                               &token, &database, &query_ref, &params_json);
    if (result != MHD_YES) {
        return result;
    }

    // Step 3: Validate JWT token for authentication
    result = validate_jwt_for_auth(connection, token);
    free(token);  // Token no longer needed after validation
    token = NULL;

    if (result != MHD_YES) {
        free(database);
        if (params_json) {
            json_decref(params_json);
        }
        return result;
    }

    // Step 4: Look up database queue and query cache entry
    DatabaseQueue *db_queue = NULL;
    QueryCacheEntry *cache_entry = NULL;
    bool query_not_found = false;
    result = handle_database_lookup(connection, database, query_ref, &db_queue, &cache_entry, &query_not_found);
    if (result != MHD_YES) {
        free(database);
        if (params_json) {
            json_decref(params_json);
        }
        return result;
    }

    // Handle invalid queryref case
    if (query_not_found) {
        json_t* response = build_invalid_queryref_response(query_ref, database);
        enum MHD_Result http_result = api_send_json_response(connection, response, MHD_HTTP_OK);
        json_decref(response);
        free(database);
        if (params_json) {
            json_decref(params_json);
        }
        return http_result;
    }

    // Step 5: Process parameters
    ParameterList *param_list = NULL;
    char *converted_sql = NULL;
    TypedParameter **ordered_params = NULL;
    size_t param_count = 0;

    result = handle_parameter_processing(connection, params_json, db_queue, cache_entry,
                                       database, query_ref, &param_list, &converted_sql,
                                       &ordered_params, &param_count);
    if (params_json) {
        json_decref(params_json);
        params_json = NULL;
    }

    if (result != MHD_YES) {
        free(database);
        return result;
    }

    // Step 6: Select appropriate queue
    DatabaseQueue *selected_queue = NULL;
    result = handle_queue_selection(connection, database, query_ref, cache_entry,
                                   param_list, converted_sql, ordered_params, &selected_queue);
    if (result != MHD_YES) {
        free(database);
        free_parameter_list(param_list);
        free(converted_sql);
        if (ordered_params) {
            for (size_t i = 0; i < param_count; i++) {
                if (ordered_params[i]) {
                    free_typed_parameter(ordered_params[i]);
                }
            }
            free(ordered_params);
        }
        return result;
    }

    // Step 7: Generate query ID
    char *query_id = NULL;
    result = handle_query_id_generation(connection, database, query_ref, param_list,
                                       converted_sql, ordered_params, &query_id);
    if (result != MHD_YES) {
        free(database);
        free_parameter_list(param_list);
        free(converted_sql);
        if (ordered_params) {
            for (size_t i = 0; i < param_count; i++) {
                if (ordered_params[i]) {
                    free_typed_parameter(ordered_params[i]);
                }
            }
            free(ordered_params);
        }
        return result;
    }

    // Step 8: Register pending query
    PendingQueryResult *pending = NULL;
    result = handle_pending_registration(connection, database, query_ref, query_id,
                                       param_list, converted_sql, ordered_params,
                                       cache_entry, &pending);
    if (result != MHD_YES) {
        free(database);
        free(query_id);
        free_parameter_list(param_list);
        free(converted_sql);
        if (ordered_params) {
            for (size_t i = 0; i < param_count; i++) {
                if (ordered_params[i]) {
                    free_typed_parameter(ordered_params[i]);
                }
            }
            free(ordered_params);
        }
        return result;
    }

    // Step 9: Submit query to database queue
    result = handle_query_submission(connection, database, query_ref, selected_queue,
                                   query_id, converted_sql, param_list, ordered_params,
                                   param_count, cache_entry);
    if (result != MHD_YES) {
        free(database);
        free(query_id);
        free_parameter_list(param_list);
        free(converted_sql);
        if (ordered_params) {
            for (size_t i = 0; i < param_count; i++) {
                if (ordered_params[i]) {
                    free_typed_parameter(ordered_params[i]);
                }
            }
            free(ordered_params);
        }
        return result;
    }

    // Step 10: Suspend webserver connection for long-running queries
    extern pthread_mutex_t webserver_suspend_lock;
    extern volatile bool webserver_thread_suspended;

    pthread_mutex_lock(&webserver_suspend_lock);
    webserver_thread_suspended = true;
    MHD_suspend_connection(connection);

    // Step 11: Wait for result
    int wait_result = pending_result_wait(pending, NULL);
    if (wait_result != 0) {
        log_this(SR_AUTH, "handle_conduit_alt_query_request: Query failed or timed out", LOG_LEVEL_ERROR, 0);
    }

    // Step 12: Resume connection processing
    MHD_resume_connection(connection);
    webserver_thread_suspended = false;
    pthread_mutex_unlock(&webserver_suspend_lock);

    // Step 13: Build response
    json_t* response = build_response_json(query_ref, database, cache_entry, selected_queue, pending);
    unsigned int http_status = json_is_true(json_object_get(response, "success")) ?
                              MHD_HTTP_OK : determine_http_status(pending, pending_result_get(pending));

    enum MHD_Result http_result = api_send_json_response(connection, response, http_status);
    json_decref(response);

    // Clean up
    free(database);
    free(query_id);
    free_parameter_list(param_list);
    free(converted_sql);
    if (ordered_params) {
        for (size_t i = 0; i < param_count; i++) {
            if (ordered_params[i]) {
                free_typed_parameter(ordered_params[i]);
            }
        }
        free(ordered_params);
    }

    log_this(SR_AUTH, "handle_conduit_alt_query_request: Request completed", LOG_LEVEL_DEBUG, 0);
    return http_result;
}