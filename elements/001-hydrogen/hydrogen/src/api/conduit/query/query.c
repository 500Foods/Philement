/*
 * Conduit Query API endpoint implementation for the Hydrogen Project.
 * Executes pre-defined database queries by reference.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/api/api_utils.h>

// Database subsystem includes
#include <src/database/database_cache.h>
#include <src/database/database_params.h>
#include <src/database/database_pending.h>
#include <src/database/database_queue_select.h>
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/database.h>

// Local includes
#include "query.h"

// Generate unique query ID
char* generate_query_id(void) {
    static volatile unsigned long long counter = 0;
    unsigned long long id = __atomic_fetch_add(&counter, 1, __ATOMIC_SEQ_CST);

    char* query_id = calloc(32, sizeof(char));
    if (!query_id) return NULL;

    snprintf(query_id, 32, "conduit_%llu_%ld", id, time(NULL));
    return query_id;
}

// Validate HTTP method - only GET and POST are allowed
bool validate_http_method(const char* method) {
    if (!method) return false;
    return strcmp(method, "GET") == 0 || strcmp(method, "POST") == 0;
}

// Parse request data from either POST JSON body or GET query parameters
json_t* parse_request_data(struct MHD_Connection* connection, const char* method,
                          const char* upload_data, const size_t* upload_data_size) {
    json_t* request_json = NULL;
    json_error_t json_error;

    if (strcmp(method, "POST") == 0) {
        // POST request - parse JSON body
        if (!upload_data || *upload_data_size == 0) {
            return NULL; // Missing request body
        }

        request_json = json_loads(upload_data, 0, &json_error);
        if (!request_json) {
            return NULL; // Invalid JSON
        }
    } else {
        // GET request - parse query parameters
        request_json = json_object();

        // Extract query parameters
        const char* query_ref_str = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "query_ref");
        const char* database = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "database");
        const char* params_json = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "params");

        if (query_ref_str) json_object_set_new(request_json, "query_ref", json_integer(atoll(query_ref_str)));
        if (database) json_object_set_new(request_json, "database", json_string(database));
        if (params_json) {
            json_t* params_parsed = json_loads(params_json, 0, &json_error);
            if (params_parsed) {
                json_object_set_new(request_json, "params", params_parsed);
            } else {
                json_decref(request_json);
                return NULL; // Invalid params JSON
            }
        }
    }

    return request_json;
}

// Extract and validate required fields from request JSON
bool extract_request_fields(json_t* request_json, int* query_ref, const char** database, json_t** params) {
    json_t* query_ref_json = json_object_get(request_json, "query_ref");
    json_t* database_json = json_object_get(request_json, "database");
    *params = json_object_get(request_json, "params");

    if (!query_ref_json || !json_is_integer(query_ref_json)) {
        return false;
    }

    if (!database_json || !json_is_string(database_json)) {
        return false;
    }

    *query_ref = (int)json_integer_value(query_ref_json);
    *database = json_string_value(database_json);
    return true;
}

// Lookup database queue and query cache entry
bool lookup_database_and_query(DatabaseQueue** db_queue, QueryCacheEntry** cache_entry,
                              const char* database, int query_ref) {
    *db_queue = database_queue_manager_get_database(global_queue_manager, database);
    if (!*db_queue) {
        return false;
    }

    *cache_entry = query_cache_lookup((*db_queue)->query_cache, query_ref);
    if (!*cache_entry) {
        return false;
    }

    return true;
}

// Parse and convert parameters
bool process_parameters(json_t* params_json, ParameterList** param_list,
                       const char* sql_template, DatabaseEngineType engine_type,
                       char** converted_sql, TypedParameter*** ordered_params, size_t* param_count) {
    *param_list = NULL;
    if (params_json && json_is_object(params_json)) {
        char* params_str = json_dumps(params_json, JSON_COMPACT);
        if (params_str) {
            *param_list = parse_typed_parameters(params_str);
            free(params_str);
        }
    }

    if (!*param_list) {
        // Create empty parameter list if none provided
        *param_list = calloc(1, sizeof(ParameterList));
        if (!*param_list) {
            return false;
        }
    }

    // Convert named parameters to positional
    *converted_sql = convert_named_to_positional(
        sql_template,
        *param_list,
        engine_type,
        ordered_params,
        param_count
    );

    return (*converted_sql != NULL);
}

// Select optimal queue for query execution
DatabaseQueue* select_query_queue(const char* database, const char* queue_type) {
    return select_optimal_queue(database, queue_type, global_queue_manager);
}

// Prepare and submit database query
bool prepare_and_submit_query(DatabaseQueue* selected_queue, const char* query_id,
                             const char* converted_sql, TypedParameter** ordered_params,
                             size_t param_count, const QueryCacheEntry* cache_entry) {
    // Create query request
    DatabaseQuery db_query = {
        .query_id = (char*)query_id,
        .query_template = (char*)converted_sql,
        .parameter_json = NULL,  // Will be built from ordered_params
        .queue_type_hint = database_queue_type_from_string(cache_entry->queue_type),
        .submitted_at = time(NULL),
        .processed_at = 0,
        .retry_count = 0,
        .error_message = NULL
    };

    // Build parameter JSON from ordered parameters
    if (param_count > 0) {
        json_t* param_json = json_object();
        for (size_t i = 0; i < param_count; i++) {
            TypedParameter* param = ordered_params[i];
            if (param) {
                switch (param->type) {
                    case PARAM_TYPE_INTEGER:
                        json_object_set_new(param_json, param->name, json_integer(param->value.int_value));
                        break;
                    case PARAM_TYPE_STRING:
                        json_object_set_new(param_json, param->name, json_string(param->value.string_value));
                        break;
                    case PARAM_TYPE_BOOLEAN:
                        json_object_set_new(param_json, param->name, json_boolean(param->value.bool_value));
                        break;
                    case PARAM_TYPE_FLOAT:
                        json_object_set_new(param_json, param->name, json_real(param->value.float_value));
                        break;
                }
            }
        }
        db_query.parameter_json = json_dumps(param_json, JSON_COMPACT);
        json_decref(param_json);
    }

    // Submit query to selected queue
    return database_queue_submit_query(selected_queue, &db_query);
}

// Wait for query result and build response
json_t* build_response_json(int query_ref, const char* database, const QueryCacheEntry* cache_entry,
                           const DatabaseQueue* selected_queue, PendingQueryResult* pending) {
    int wait_result = pending_result_wait(pending);
    QueryResult* result = pending_result_get(pending);

    json_t* response = json_object();

    if (wait_result == 0 && result && result->success) {
        // Success response
        json_object_set_new(response, "success", json_true());
        json_object_set_new(response, "query_ref", json_integer(query_ref));
        json_object_set_new(response, "description", json_string(cache_entry->description ? cache_entry->description : ""));

        // Parse result data
        if (result->data_json) {
            json_error_t parse_error;
            json_t* data = json_loads(result->data_json, 0, &parse_error);
            if (data) {
                json_object_set_new(response, "rows", data);
            } else {
                json_object_set_new(response, "rows", json_array());
            }
        } else {
            json_object_set_new(response, "rows", json_array());
        }

        json_object_set_new(response, "row_count", json_integer((json_int_t)result->row_count));
        json_object_set_new(response, "column_count", json_integer((json_int_t)result->column_count));
        json_object_set_new(response, "execution_time_ms", json_integer(result->execution_time_ms));
        json_object_set_new(response, "queue_used", json_string(selected_queue->queue_type));
    } else {
        // Error response
        json_object_set_new(response, "success", json_false());
        json_object_set_new(response, "query_ref", json_integer(query_ref));
        json_object_set_new(response, "database", json_string(database));

        if (pending_result_is_timed_out(pending)) {
            json_object_set_new(response, "error", json_string("Query execution timeout"));
            json_object_set_new(response, "timeout_seconds", json_integer(cache_entry->timeout_seconds));
        } else if (result && result->error_message) {
            json_object_set_new(response, "error", json_string("Database error"));
            json_object_set_new(response, "database_error", json_string(result->error_message));
        } else {
            json_object_set_new(response, "error", json_string("Query execution failed"));
        }
    }

    return response;
}

// Determine HTTP status code based on result
unsigned int determine_http_status(const PendingQueryResult* pending, const QueryResult* result) {
    if (pending_result_is_timed_out(pending)) {
        return MHD_HTTP_REQUEST_TIMEOUT;
    } else if (result && result->error_message) {
        return MHD_HTTP_INTERNAL_SERVER_ERROR;
    } else {
        return MHD_HTTP_BAD_REQUEST;
    }
}

/**
 * Handle the /api/conduit/query endpoint.
 * Executes pre-defined database queries by reference with typed parameters.
 *
 * Supports both GET (query parameters) and POST (JSON body) methods.
 *
 * Implementation:
 * 1. Parse request parameters (GET query string or POST JSON body)
 * 2. Lookup query in Query Table Cache (QTC)
 * 3. Parse and validate typed parameters
 * 4. Convert named parameters to positional format
 * 5. Select optimal database queue (DQM)
 * 6. Submit query and wait for result
 * 7. Return JSON response with results
 */
enum MHD_Result handle_conduit_query_request(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *upload_data,
    // cppcheck-suppress[constParameterPointer] - upload_data_size parameter must match libmicrohttpd callback signature
    size_t *upload_data_size,
    void **con_cls
  ) {
    (void)url;              // Unused parameter
    (void)con_cls;          // Will be used when implemented

    // Validate HTTP method
    if (!validate_http_method(method)) {
        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Method not allowed"));
        json_object_set_new(error_response, "message", json_string("Only GET and POST requests are supported"));

        enum MHD_Result result = api_send_json_response(connection, error_response, MHD_HTTP_METHOD_NOT_ALLOWED);
        json_decref(error_response);
        return result;
    }

    // Parse request data
    json_t *request_json = parse_request_data(connection, method, upload_data, upload_data_size);
    if (!request_json) {
        // Determine specific error based on method and data
        const char* error_msg = "Invalid JSON";
        const char* error_detail = "Request body contains invalid JSON";
        unsigned int http_status = MHD_HTTP_BAD_REQUEST;

        if (strcmp(method, "POST") == 0 && (!upload_data || *upload_data_size == 0)) {
            error_msg = "Missing request body";
            error_detail = "POST requests must include a JSON body";
        }

        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string(error_msg));
        json_object_set_new(error_response, "message", json_string(error_detail));

        enum MHD_Result result = api_send_json_response(connection, error_response, http_status);
        json_decref(error_response);
        return result;
    }

    // Extract and validate required fields
    int query_ref;
    const char* database;
    json_t* params_json;
    if (!extract_request_fields(request_json, &query_ref, &database, &params_json)) {
        json_decref(request_json);
        const char* error_msg = !json_object_get(request_json, "query_ref") || !json_is_integer(json_object_get(request_json, "query_ref"))
                               ? "Missing or invalid query_ref" : "Missing or invalid database";
        const char* error_detail = !json_object_get(request_json, "query_ref") || !json_is_integer(json_object_get(request_json, "query_ref"))
                                  ? "query_ref must be an integer" : "database must be a string";

        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string(error_msg));
        json_object_set_new(error_response, "message", json_string(error_detail));

        enum MHD_Result result = api_send_json_response(connection, error_response, MHD_HTTP_BAD_REQUEST);
        json_decref(error_response);
        return result;
    }

    // Lookup database queue and query cache entry
    DatabaseQueue* db_queue;
    QueryCacheEntry* cache_entry;
    if (!lookup_database_and_query(&db_queue, &cache_entry, database, query_ref)) {
        json_decref(request_json);
        const char* error_msg = !db_queue ? "Database not found" : "Query not found";
        unsigned int http_status = MHD_HTTP_NOT_FOUND;

        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string(error_msg));
        if (!db_queue) {
            json_object_set_new(error_response, "database", json_string(database));
        } else {
            json_object_set_new(error_response, "query_ref", json_integer(query_ref));
            json_object_set_new(error_response, "database", json_string(database));
        }

        enum MHD_Result result = api_send_json_response(connection, error_response, http_status);
        json_decref(error_response);
        return result;
    }

    // Parse and convert parameters
    ParameterList* param_list;
    char* converted_sql;
    TypedParameter** ordered_params;
    size_t param_count;
    if (!process_parameters(params_json, &param_list, cache_entry->sql_template,
                           db_queue->engine_type, &converted_sql, &ordered_params, &param_count)) {
        json_decref(request_json);
        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string(!param_list ? "Memory allocation failed" : "Parameter conversion failed"));
        json_object_set_new(error_response, "query_ref", json_integer(query_ref));
        json_object_set_new(error_response, "database", json_string(database));

        enum MHD_Result result = api_send_json_response(connection, error_response, MHD_HTTP_BAD_REQUEST);
        json_decref(error_response);
        return result;
    }

    // Select optimal queue
    DatabaseQueue* selected_queue = select_query_queue(database, cache_entry->queue_type);
    if (!selected_queue) {
        free(converted_sql);
        free_parameter_list(param_list);
        if (ordered_params) free(ordered_params);
        json_decref(request_json);
        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("No suitable queue available"));
        json_object_set_new(error_response, "query_ref", json_integer(query_ref));
        json_object_set_new(error_response, "database", json_string(database));

        enum MHD_Result result = api_send_json_response(connection, error_response, MHD_HTTP_SERVICE_UNAVAILABLE);
        json_decref(error_response);
        return result;
    }

    // Generate unique query ID
    char* query_id = generate_query_id();
    if (!query_id) {
        free(converted_sql);
        free_parameter_list(param_list);
        if (ordered_params) free(ordered_params);
        json_decref(request_json);
        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Failed to generate query ID"));

        enum MHD_Result result = api_send_json_response(connection, error_response, MHD_HTTP_INTERNAL_SERVER_ERROR);
        json_decref(error_response);
        return result;
    }

    // Register pending result
    PendingResultManager* pending_mgr = get_pending_result_manager();
    PendingQueryResult* pending = pending_result_register(pending_mgr, query_id, cache_entry->timeout_seconds);
    if (!pending) {
        free(query_id);
        free(converted_sql);
        free_parameter_list(param_list);
        if (ordered_params) free(ordered_params);
        json_decref(request_json);
        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Failed to register pending result"));

        enum MHD_Result result = api_send_json_response(connection, error_response, MHD_HTTP_INTERNAL_SERVER_ERROR);
        json_decref(error_response);
        return result;
    }

    // Prepare and submit query
    if (!prepare_and_submit_query(selected_queue, query_id, converted_sql, ordered_params,
                                  param_count, cache_entry)) {
        // Cleanup on failure
        free(query_id);
        free(converted_sql);
        free_parameter_list(param_list);
        if (ordered_params) free(ordered_params);
        json_decref(request_json);

        json_t *error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Failed to submit query"));
        json_object_set_new(error_response, "query_ref", json_integer(query_ref));
        json_object_set_new(error_response, "database", json_string(database));

        enum MHD_Result result = api_send_json_response(connection, error_response, MHD_HTTP_INTERNAL_SERVER_ERROR);
        json_decref(error_response);
        return result;
    }

    // Wait for result and build response
    json_t* response = build_response_json(query_ref, database, cache_entry, selected_queue, pending);
    unsigned int http_status = json_is_true(json_object_get(response, "success")) ?
                               MHD_HTTP_OK : determine_http_status(pending, pending_result_get(pending));

    enum MHD_Result http_result = api_send_json_response(connection, response, http_status);
    json_decref(response);

    // Cleanup
    free(query_id);
    free(converted_sql);
    free_parameter_list(param_list);
    if (ordered_params) free(ordered_params);
    json_decref(request_json);

    return http_result;
}