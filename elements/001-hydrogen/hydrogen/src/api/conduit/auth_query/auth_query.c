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
#include <src/api/api_utils.h>  // For api_send_json_response
#include <src/api/conduit/auth_query/auth_query.h>
#include <src/api/conduit/query/query.h>
#include <src/api/auth/auth_service.h>
#include <src/api/auth/auth_service_jwt.h>
#include <src/config/config.h>
#include <src/logging/logging.h>

// Database subsystem includes
#include <src/database/database_cache.h>
#include <src/database/database_params.h>
#include <src/database/database_pending.h>
#include <src/database/database_queue_select.h>
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/database.h>

// Local includes
#include "../conduit_service.h"
#include "../conduit_helpers.h"

/**
 * Handle the result of api_buffer_post_data.
 * Returns MHD_YES to continue processing, or an error result to return immediately.
 */
enum MHD_Result handle_auth_query_buffer_result(struct MHD_Connection *connection, ApiBufferResult buf_result, void **con_cls) {
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
                "Method not allowed - use GET or POST", MHD_HTTP_METHOD_NOT_ALLOWED);

        case API_BUFFER_COMPLETE:
            // All data received (or GET request), continue with processing
            return MHD_YES;

        default:
            // Should not happen, but handle gracefully
            return api_send_error_and_cleanup(connection, con_cls,
                "Unknown buffer result", MHD_HTTP_INTERNAL_SERVER_ERROR);
    }
}

/**
 * @brief Validate JWT token from Authorization header and extract database name and claims
 *
 * Extracts the JWT token from the Authorization header, validates it,
 * and extracts the database name from the token claims. This ensures
 * secure database routing based on authenticated user sessions.
 *
 * @param connection The HTTP connection for error responses
 * @param database Output parameter for extracted database name
 * @param jwt_result Output parameter for JWT validation result (contains claims)
 * @return MHD_YES on success, MHD_NO on validation failure
 */
static enum MHD_Result validate_jwt_from_request(
    struct MHD_Connection *connection,
    json_t *request_json,
    jwt_validation_result_t **jwt_result)
{
    if (!connection || !request_json || !jwt_result) {
        log_this(SR_AUTH, "validate_jwt_from_request: NULL parameters", LOG_LEVEL_ERROR, 0);
        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Internal server error"));
        char *response_str = json_dumps(error_response, JSON_COMPACT);
        json_decref(error_response);

        if (!response_str) {
            log_this(SR_AUTH, "validate_jwt_from_request: Failed to serialize error response", LOG_LEVEL_ERROR, 0);
            return MHD_NO;
        }

        struct MHD_Response *response = MHD_create_response_from_buffer(
            strlen(response_str), response_str, MHD_RESPMEM_MUST_FREE);
        MHD_add_response_header(response, "Content-Type", "application/json");
        MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
        MHD_destroy_response(response);
        return MHD_NO;
    }

    // Extract token from request JSON
    json_t* token_json = json_object_get(request_json, "token");
    if (!token_json || !json_is_string(token_json)) {
        log_this(SR_AUTH, "validate_jwt_from_request: Missing or invalid token", LOG_LEVEL_ERROR, 0);
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

    const char *token = json_string_value(token_json);

    // Validate JWT token - pass NULL since database comes from token
    jwt_validation_result_t result = validate_jwt(token, NULL);
    if (!result.valid) {
        log_this(SR_AUTH, "validate_jwt_from_request: JWT validation failed", LOG_LEVEL_ALERT, 0);
        free_jwt_validation_result(&result);

        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Invalid or expired JWT token"));
        char *response_str = json_dumps(error_response, JSON_COMPACT);
        json_decref(error_response);

        if (!response_str) {
            log_this(SR_AUTH, "validate_jwt_from_request: Failed to serialize error response", LOG_LEVEL_ERROR, 0);
            return MHD_NO;
        }

        struct MHD_Response *response = MHD_create_response_from_buffer(
            strlen(response_str), response_str, MHD_RESPMEM_MUST_FREE);
        MHD_add_response_header(response, "Content-Type", "application/json");
        MHD_queue_response(connection, MHD_HTTP_UNAUTHORIZED, response);
        MHD_destroy_response(response);
        return MHD_NO;
    }

    // Extract database from JWT claims
    if (!result.claims || !result.claims->database || strlen(result.claims->database) == 0) {
        log_this(SR_AUTH, "validate_jwt_from_request: No database in JWT claims", LOG_LEVEL_ERROR, 0);
        free_jwt_validation_result(&result);

        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("JWT token missing database information"));
        char *response_str = json_dumps(error_response, JSON_COMPACT);
        json_decref(error_response);

        if (!response_str) {
            log_this(SR_AUTH, "validate_jwt_from_request: Failed to serialize error response", LOG_LEVEL_ERROR, 0);
            return MHD_NO;
        }

        struct MHD_Response *response = MHD_create_response_from_buffer(
            strlen(response_str), response_str, MHD_RESPMEM_MUST_FREE);
        MHD_add_response_header(response, "Content-Type", "application/json");
        MHD_queue_response(connection, MHD_HTTP_UNAUTHORIZED, response);
        MHD_destroy_response(response);
        return MHD_NO;
    }

    // Allocate and copy database name
    char *database = strdup(result.claims->database);
    if (!database) {
        log_this(SR_AUTH, "validate_jwt_from_request: Failed to allocate database string", LOG_LEVEL_ERROR, 0);
        free_jwt_validation_result(&result);

        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Internal server error"));
        char *response_str = json_dumps(error_response, JSON_COMPACT);
        json_decref(error_response);

        if (!response_str) {
            log_this(SR_AUTH, "validate_jwt_from_request: Failed to serialize error response", LOG_LEVEL_ERROR, 0);
            return MHD_NO;
        }

        struct MHD_Response *response = MHD_create_response_from_buffer(
            strlen(response_str), response_str, MHD_RESPMEM_MUST_FREE);
        MHD_add_response_header(response, "Content-Type", "application/json");
        MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
        MHD_destroy_response(response);
        return MHD_NO;
    }

    // Add database to request JSON for field extraction
    json_object_set_new(request_json, "database", json_string(database));
    free(database);

    // Return the JWT result for use by caller
    *jwt_result = malloc(sizeof(jwt_validation_result_t));
    if (!*jwt_result) {
        log_this(SR_AUTH, "validate_jwt_from_request: Failed to allocate JWT result", LOG_LEVEL_ERROR, 0);
        free_jwt_validation_result(&result);

        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Internal server error"));
        char *response_str = json_dumps(error_response, JSON_COMPACT);
        json_decref(error_response);

        if (!response_str) {
            log_this(SR_AUTH, "validate_jwt_from_request: Failed to serialize error response", LOG_LEVEL_ERROR, 0);
            return MHD_NO;
        }

        struct MHD_Response *response = MHD_create_response_from_buffer(
            strlen(response_str), response_str, MHD_RESPMEM_MUST_FREE);
        MHD_add_response_header(response, "Content-Type", "application/json");
        MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
        MHD_destroy_response(response);
        return MHD_NO;
    }
    **jwt_result = result; // Copy the struct

    log_this(SR_AUTH, "validate_jwt_from_request: JWT validated, database=%s", LOG_LEVEL_DEBUG, 1, result.claims->database);
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
    // cppcheck-suppress[constParameterPointer] - upload_data_size parameter must match libmicrohttpd callback signature
    size_t *upload_data_size,
    void **con_cls)
{
    (void)url;

    // Validate required parameters
    if (!connection || !method || !upload_data_size) {
        return MHD_NO;
    }

    // Use common POST body buffering (handles both GET and POST)
    ApiPostBuffer *buffer = NULL;
    ApiBufferResult buf_result = api_buffer_post_data(method, upload_data, upload_data_size, con_cls, &buffer);

    enum MHD_Result buffer_result = handle_auth_query_buffer_result(connection, buf_result, con_cls);
    if (buffer_result != MHD_YES) {
        return buffer_result;
    }

    log_this(SR_AUTH, "handle_conduit_auth_query_request: Processing authenticated query request", LOG_LEVEL_DEBUG, 0);

    // Step 1: Validate HTTP method
    enum MHD_Result result = handle_method_validation(connection, method);
    if (result != MHD_YES) {
        api_free_post_buffer(con_cls);
        return result;
    }

    // Step 2: Parse request data from buffer (handles GET query params and POST JSON body)
    json_t *request_json = NULL;
    result = handle_request_parsing_with_buffer(connection, buffer, &request_json);

    // Free the buffer now that we've parsed the data
    api_free_post_buffer(con_cls);

    if (result != MHD_YES) return result;

    // Step 3: Extract and validate required fields
    int query_ref = 0;
    const char* database = NULL;
    json_t* params_json = NULL;
    result = handle_field_extraction(connection, request_json, &query_ref, &database, &params_json);
    
    // For auth_query, database should come from JWT claims, not request body
    if (database) {
        log_this(SR_AUTH, "handle_conduit_auth_query_request: Database field found in request body - ignoring, using JWT claims instead", LOG_LEVEL_DEBUG, 0);
        database = NULL; // Clear database from request body
    }
    if (result != MHD_YES) {
        json_decref(request_json);
        return result;
    }

    log_this(SR_AUTH, "handle_conduit_auth_query_request: Request fields extracted: query_ref=%d, database=%s", LOG_LEVEL_TRACE, 2, query_ref, database);

    // Step 4: Validate JWT token and extract database from claims
    jwt_validation_result_t *jwt_result = NULL;
    result = validate_jwt_from_request(connection, request_json, &jwt_result);
    if (result != MHD_YES) {
        json_decref(request_json);
        return result;
    }

    // Step 5: Look up database queue and query cache entry
    DatabaseQueue *db_queue = NULL;
    QueryCacheEntry *cache_entry = NULL;
    bool query_not_found = false;
    result = handle_database_lookup(connection, database, query_ref, &db_queue, &cache_entry, &query_not_found, false);
    if (result != MHD_YES) {
        json_decref(request_json);
        free_jwt_validation_result(jwt_result);
        free(jwt_result);
        return result;
    }

    // Check if database lookup failed (error response already sent)
    if (!db_queue) {
        json_decref(request_json);
        free_jwt_validation_result(jwt_result);
        free(jwt_result);
        return MHD_YES;
    }

    // Handle invalid queryref case
    if (query_not_found) {
        json_t* response = build_invalid_queryref_response(query_ref, database, NULL);
        enum MHD_Result http_result = api_send_json_response(connection, response, MHD_HTTP_OK);
        json_decref(response);
        json_decref(request_json);
        free_jwt_validation_result(jwt_result);
        free(jwt_result);
        return http_result;
    }

    log_this(SR_AUTH, "handle_conduit_auth_query_request: Database and query lookup successful", LOG_LEVEL_TRACE, 1, conduit_service_name());

    // Step 6: Process parameters
    ParameterList *param_list = NULL;
    char *converted_sql = NULL;
    TypedParameter **ordered_params = NULL;
    size_t param_count = 0;

    char* message = NULL;
    result = handle_parameter_processing(connection, params_json, db_queue, cache_entry,
                                        database, query_ref, &param_list, &converted_sql,
                                        &ordered_params, &param_count, &message);
    if (params_json) {
        json_decref(params_json);
        params_json = NULL;
    }

    if (result != MHD_YES || !converted_sql) {
        json_decref(request_json);
        free_jwt_validation_result(jwt_result);
        free(jwt_result);
        return result;
    }

    // Step 7: Select appropriate queue
    DatabaseQueue *selected_queue = NULL;
    result = handle_queue_selection(connection, database, query_ref, cache_entry,
                                   param_list, converted_sql, ordered_params, &selected_queue);
    if (result != MHD_YES) {
        json_decref(request_json);
        free_jwt_validation_result(jwt_result);
        free(jwt_result);
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

    // Step 8: Generate query ID
    char *query_id = NULL;
    result = handle_query_id_generation(connection, database, query_ref, param_list,
                                       converted_sql, ordered_params, &query_id);
    if (result != MHD_YES) {
        json_decref(request_json);
        free_jwt_validation_result(jwt_result);
        free(jwt_result);
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

    // Step 9: Register pending result
    PendingQueryResult *pending = NULL;
    result = handle_pending_registration(connection, database, query_ref, query_id,
                                        param_list, converted_sql, ordered_params,
                                        cache_entry, &pending);
    if (result != MHD_YES) {
        json_decref(request_json);
        free_jwt_validation_result(jwt_result);
        free(jwt_result);
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

    // Step 10: Submit query to database queue
    result = handle_query_submission(connection, database, query_ref, selected_queue, query_id,
                                    converted_sql, param_list, ordered_params, param_count, cache_entry);
    if (result != MHD_YES) {
        json_decref(request_json);
        free_jwt_validation_result(jwt_result);
        free(jwt_result);
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

    // Step 11: Wait for result and build response
    result = handle_response_building(connection, query_ref, database, cache_entry,
                                     selected_queue, pending, query_id, converted_sql,
                                     param_list, ordered_params, message);

    // Clean up
    json_decref(request_json);
    free_jwt_validation_result(jwt_result);
    free(jwt_result);
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
    if (message) free(message);

    log_this(SR_AUTH, "handle_conduit_auth_query_request: Request completed", LOG_LEVEL_DEBUG, 0);
    return result;
}
