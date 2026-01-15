/**
 * @file auth_query.c
 * @brief Authenticated Conduit Query API endpoint implementation
 *
 * This module implements the authenticated database query execution endpoint.
 * It validates JWT tokens before executing queries and extracts the database
 * name from JWT claims for secure routing.
 *
 * @author Hydrogen Framework
 * @date 2026-01-10
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
#include <src/api/conduit/auth_query/auth_query.h>
#include <src/api/conduit/query/query.h>
#include <src/api/auth/auth_service.h>
#include <src/api/auth/auth_service_jwt.h>
#include <src/config/config.h>
#include <src/logging/logging.h>

/**
 * @brief Validate JWT token and extract database name
 *
 * Validates the provided JWT token and extracts the database name from
 * the token claims. This ensures secure database routing based on
 * authenticated user sessions.
 *
 * @param connection The HTTP connection for error responses
 * @param token The JWT token string to validate
 * @param database Output parameter for extracted database name
 * @return MHD_YES on success, MHD_NO on validation failure
 */
static enum MHD_Result validate_token_and_extract_database(
    struct MHD_Connection *connection,
    const char *token,
    char **database)
{
    if (!token || !database) {
        log_this(SR_AUTH, "validate_token_and_extract_database: NULL parameters", LOG_LEVEL_ERROR, 0);
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

    // Validate JWT token - pass "unknown" as placeholder since database comes from token
    jwt_validation_result_t result = validate_jwt(token, "unknown");
    if (!result.valid || !result.claims) {
        log_this(SR_AUTH, "validate_token_and_extract_database: JWT validation failed", LOG_LEVEL_ALERT, 0);
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

    // Extract database from JWT claims
    if (!result.claims->database || strlen(result.claims->database) == 0) {
        log_this(SR_AUTH, "validate_token_and_extract_database: No database in JWT claims", LOG_LEVEL_ERROR, 0);
        free_jwt_validation_result(&result);
        
        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("JWT token missing database information"));
        char *response_str = json_dumps(error_response, JSON_COMPACT);
        json_decref(error_response);
        
        struct MHD_Response *response = MHD_create_response_from_buffer(
            strlen(response_str), response_str, MHD_RESPMEM_MUST_FREE);
        MHD_add_response_header(response, "Content-Type", "application/json");
        MHD_queue_response(connection, MHD_HTTP_UNAUTHORIZED, response);
        MHD_destroy_response(response);
        return MHD_NO;
    }

    // Allocate and copy database name
    *database = strdup(result.claims->database);
    if (!*database) {
        log_this(SR_AUTH, "validate_token_and_extract_database: Failed to allocate database string", LOG_LEVEL_ERROR, 0);
        free_jwt_validation_result(&result);
        
        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Internal server error"));
        char *response_str = json_dumps(error_response, JSON_COMPACT);
        json_decref(error_response);
        
        struct MHD_Response *response = MHD_create_response_from_buffer(
            strlen(response_str), response_str, MHD_RESPMEM_MUST_FREE);
        MHD_add_response_header(response, "Content-Type", "application/json");
        MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
        MHD_destroy_response(response);
        return MHD_NO;
    }

    log_this(SR_AUTH, "validate_token_and_extract_database: JWT validated, database=%s", LOG_LEVEL_DEBUG, 1, *database);
    free_jwt_validation_result(&result);
    return MHD_YES;
}

/**
 * @brief Parse authenticated request and extract fields
 *
 * Parses the request JSON (from GET params or POST body) and extracts
 * the JWT token, query_ref, and optional params fields.
 *
 * @param connection The HTTP connection for error responses
 * @param method The HTTP method (GET or POST)
 * @param upload_data Request body data
 * @param upload_data_size Size of request body
 * @param token Output parameter for JWT token
 * @param query_ref Output parameter for query reference ID
 * @param params_json Output parameter for query parameters
 * @return MHD_YES on success, MHD_NO on parsing failure
 */
static enum MHD_Result parse_auth_request(
    struct MHD_Connection *connection,
    const char *method,
    const char *upload_data,
    const size_t *upload_data_size,
    char **token,
    int *query_ref,
    json_t **params_json)
{
    if (!method || !token || !query_ref || !params_json) {
        log_this(SR_AUTH, "parse_auth_request: NULL parameters", LOG_LEVEL_ERROR, 0);
        return MHD_NO;
    }

    // Parse request data (handles both GET and POST)
    json_t *request_json = parse_request_data(connection, method, upload_data, upload_data_size);
    if (!request_json) {
        log_this(SR_AUTH, "parse_auth_request: Failed to parse request data", LOG_LEVEL_ERROR, 0);
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
        log_this(SR_AUTH, "parse_auth_request: Missing or invalid token field", LOG_LEVEL_ERROR, 0);
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
        log_this(SR_AUTH, "parse_auth_request: Failed to allocate token string", LOG_LEVEL_ERROR, 0);
        json_decref(request_json);
        return MHD_NO;
    }

    // Extract query_ref field
    json_t *query_ref_json = json_object_get(request_json, "query_ref");
    if (!query_ref_json || !json_is_integer(query_ref_json)) {
        log_this(SR_AUTH, "parse_auth_request: Missing or invalid query_ref field", LOG_LEVEL_ERROR, 0);
        free(*token);
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
    log_this(SR_AUTH, "parse_auth_request: Successfully parsed, query_ref=%d", LOG_LEVEL_DEBUG, 1, *query_ref);
    return MHD_YES;
}

/**
 * @brief Handle authenticated conduit query request
 *
 * Main handler for the /api/conduit/auth_query endpoint. Validates JWT token,
 * extracts database from token claims, and executes the requested query.
 *
 * @param connection The HTTP connection
 * @param url The request URL
 * @param method The HTTP method (GET or POST)
 * @param upload_data Request body data
 * @param upload_data_size Size of request body
 * @param con_cls Connection-specific data
 * @return HTTP response code
 */
enum MHD_Result handle_conduit_auth_query_request(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *upload_data,
    const size_t *upload_data_size,
    void **con_cls)
{
    (void)url;
    (void)con_cls;

    log_this(SR_AUTH, "handle_conduit_auth_query_request: Processing authenticated query request", LOG_LEVEL_DEBUG, 0);

    // Step 1: Validate HTTP method
    enum MHD_Result result = handle_method_validation(connection, method);
    if (result != MHD_YES) {
        return result;
    }

    // Step 2: Parse request and extract token, query_ref, params
    char *token = NULL;
    int query_ref = 0;
    json_t *params_json = NULL;
    
    result = parse_auth_request(connection, method, upload_data, upload_data_size, 
                                &token, &query_ref, &params_json);
    if (result != MHD_YES) {
        return result;
    }

    // Step 3: Validate JWT token and extract database
    char *database = NULL;
    result = validate_token_and_extract_database(connection, token, &database);
    free(token);  // Token no longer needed after validation
    token = NULL;
    
    if (result != MHD_YES) {
        if (params_json) {
            json_decref(params_json);
        }
        return result;
    }

    // Step 4: Look up database queue and query cache entry
    DatabaseQueue *db_queue = NULL;
    QueryCacheEntry *cache_entry = NULL;
    result = handle_database_lookup(connection, database, query_ref, &db_queue, &cache_entry);
    if (result != MHD_YES) {
        free(database);
        if (params_json) {
            json_decref(params_json);
        }
        return result;
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

    // Step 10: Wait for result and build response
    result = handle_response_building(connection, query_ref, database, cache_entry,
                                     selected_queue, pending, query_id, converted_sql,
                                     param_list, ordered_params);
    
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

    log_this(SR_AUTH, "handle_conduit_auth_query_request: Request completed", LOG_LEVEL_DEBUG, 0);
    return result;
}
