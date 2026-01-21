/*
 * Conduit Service Query Execution Helper Functions
 * 
 * Functions for query execution and result handling.
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
#include "../conduit_helpers.h"
#include "../conduit_service.h"

// Enable mock database queue functions for unit testing
#ifdef USE_MOCK_DBQUEUE
#include <unity/mocks/mock_dbqueue.h>
#endif

// Enable mock system functions for unit testing
#ifdef USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>
#endif

// Enable mock for generate_query_id
#ifdef USE_MOCK_GENERATE_QUERY_ID
extern char* mock_generate_query_id(void);
#endif

// Enable mock for select_query_queue
#ifdef USE_MOCK_SELECT_QUERY_QUEUE
extern DatabaseQueue* mock_select_query_queue(const char* database, const char* queue_type);
#endif

// Generate unique query ID
char* generate_query_id(void) {
#ifdef USE_MOCK_GENERATE_QUERY_ID
    return mock_generate_query_id();
#else
    static volatile unsigned long long counter = 0;
    unsigned long long id = __atomic_fetch_add(&counter, 1, __ATOMIC_SEQ_CST);

    char* query_id = calloc(32, sizeof(char));
    if (!query_id) return NULL;

    snprintf(query_id, 32, "conduit_%llu_%ld", id, time(NULL));
    return query_id;
#endif
}

// Prepare and submit database query
bool prepare_and_submit_query(DatabaseQueue* selected_queue, const char* query_id,
                                const char* sql_template, TypedParameter** ordered_params,
                                size_t param_count, const QueryCacheEntry* cache_entry) {
    // Validate parameter count to prevent excessive memory usage
    if (param_count > 100) {
        log_this(SR_API, "Parameter count too high: %zu", LOG_LEVEL_ERROR, 1, param_count);
        return false;
    }

    // Create query request
    DatabaseQuery db_query = {
        .query_id = (char*)query_id,
        .query_template = (char*)sql_template,
        .parameter_json = NULL,  // Will be built from ordered_params
        .queue_type_hint = database_queue_type_from_string(cache_entry->queue_type),
        .submitted_at = time(NULL),
        .processed_at = 0,
        .retry_count = 0,
        .error_message = NULL
    };

    // Build parameter JSON from ordered parameters, grouped by type
    if (param_count > 0) {
        json_t* param_json = json_object();
        json_t* integer_params = json_object();
        json_t* string_params = json_object();
        json_t* boolean_params = json_object();
        json_t* float_params = json_object();
        json_t* text_params = json_object();
        json_t* date_params = json_object();
        json_t* time_params = json_object();
        json_t* datetime_params = json_object();
        json_t* timestamp_params = json_object();

        for (size_t i = 0; i < param_count; i++) {
            TypedParameter* param = ordered_params[i];
            if (param) {
                switch (param->type) {
                    case PARAM_TYPE_INTEGER:
                        json_object_set_new(integer_params, param->name, json_integer(param->value.int_value));
                        break;
                    case PARAM_TYPE_STRING:
                        json_object_set_new(string_params, param->name, json_string(param->value.string_value));
                        break;
                    case PARAM_TYPE_BOOLEAN:
                        json_object_set_new(boolean_params, param->name, json_boolean(param->value.bool_value));
                        break;
                    case PARAM_TYPE_FLOAT:
                        json_object_set_new(float_params, param->name, json_real(param->value.float_value));
                        break;
                    case PARAM_TYPE_TEXT:
                        json_object_set_new(text_params, param->name, json_string(param->value.text_value));
                        break;
                    case PARAM_TYPE_DATE:
                        json_object_set_new(date_params, param->name, json_string(param->value.date_value));
                        break;
                    case PARAM_TYPE_TIME:
                        json_object_set_new(time_params, param->name, json_string(param->value.time_value));
                        break;
                    case PARAM_TYPE_DATETIME:
                        json_object_set_new(datetime_params, param->name, json_string(param->value.datetime_value));
                        break;
                    case PARAM_TYPE_TIMESTAMP:
                        json_object_set_new(timestamp_params, param->name, json_string(param->value.timestamp_value));
                        break;
                }
            }
        }

        // Only add type objects if they have parameters
        if (json_object_size(integer_params) > 0) {
            json_object_set_new(param_json, "INTEGER", integer_params);
        } else {
            json_decref(integer_params);
        }
        if (json_object_size(string_params) > 0) {
            json_object_set_new(param_json, "STRING", string_params);
        } else {
            json_decref(string_params);
        }
        if (json_object_size(boolean_params) > 0) {
            json_object_set_new(param_json, "BOOLEAN", boolean_params);
        } else {
            json_decref(boolean_params);
        }
        if (json_object_size(float_params) > 0) {
            json_object_set_new(param_json, "FLOAT", float_params);
        } else {
            json_decref(float_params);
        }
        if (json_object_size(text_params) > 0) {
            json_object_set_new(param_json, "TEXT", text_params);
        } else {
            json_decref(text_params);
        }
        if (json_object_size(date_params) > 0) {
            json_object_set_new(param_json, "DATE", date_params);
        } else {
            json_decref(date_params);
        }
        if (json_object_size(time_params) > 0) {
            json_object_set_new(param_json, "TIME", time_params);
        } else {
            json_decref(time_params);
        }
        if (json_object_size(datetime_params) > 0) {
            json_object_set_new(param_json, "DATETIME", datetime_params);
        } else {
            json_decref(datetime_params);
        }
        if (json_object_size(timestamp_params) > 0) {
            json_object_set_new(param_json, "TIMESTAMP", timestamp_params);
        } else {
            json_decref(timestamp_params);
        }

        db_query.parameter_json = json_dumps(param_json, JSON_COMPACT);
        json_decref(param_json);
    }

    // Submit query to selected queue
    bool submit_result = database_queue_submit_query(selected_queue, &db_query);
    return submit_result;
}

// Wait for query result
const QueryResult* wait_for_query_result(PendingQueryResult* pending) {
    int wait_result = pending_result_wait(pending, NULL);
    if (wait_result != 0) {
        return NULL; // Wait failed
    }
    return pending_result_get(pending);
}

// Parse query result data into JSON
json_t* parse_query_result_data(const QueryResult* result) {
    if (!result->data_json) {
        return json_array();
    }

    json_error_t parse_error;
    json_t* data = json_loads(result->data_json, 0, &parse_error);
    if (data) {
        return data;
    } else {
        return json_array(); // Return empty array on parse failure
    }
}

// Build success response JSON
json_t* build_success_response(int query_ref, const QueryCacheEntry* cache_entry,
                              const QueryResult* result, const DatabaseQueue* selected_queue, const char* message) {
    json_t* response = json_object();

    json_object_set_new(response, "success", json_true());
    json_object_set_new(response, "query_ref", json_integer(query_ref));
    json_object_set_new(response, "description", json_string(cache_entry->description ? cache_entry->description : ""));

    // Parse and set result data
    json_t* data = parse_query_result_data(result);
    json_object_set_new(response, "rows", data);

    json_object_set_new(response, "row_count", json_integer((json_int_t)result->row_count));
    json_object_set_new(response, "column_count", json_integer((json_int_t)result->column_count));
    json_object_set_new(response, "execution_time_ms", json_integer(result->execution_time_ms));
    json_object_set_new(response, "queue_used", json_string(selected_queue->queue_type));

    // Add message if provided
    if (message && strlen(message) > 0) {
        json_object_set_new(response, "message", json_string(message));
    }

    // DQM statistics are only included in status endpoints, not query endpoints

    return response;
}

// Build error response JSON
json_t* build_error_response(int query_ref, const char* database, const QueryCacheEntry* cache_entry,
                            const PendingQueryResult* pending, const QueryResult* result, const char* message) {
    json_t* response = json_object();

    json_object_set_new(response, "success", json_false());
    json_object_set_new(response, "query_ref", json_integer(query_ref));
    json_object_set_new(response, "database", json_string(database));

    if (pending_result_is_timed_out(pending)) {
        json_object_set_new(response, "error", json_string("Query execution timeout"));
        json_object_set_new(response, "timeout_seconds", json_integer(cache_entry->timeout_seconds));
    } else if (result && result->error_message) {
        json_object_set_new(response, "error", json_string("Database error"));
        json_object_set_new(response, "message", json_string(result->error_message));
    } else {
        json_object_set_new(response, "error", json_string("Query execution failed"));
    }

    // Add message if provided
    if (message && strlen(message) > 0) {
        json_object_set_new(response, "message", json_string(message));
    }

    return response;
}

// Build invalid queryref response JSON
json_t* build_invalid_queryref_response(int query_ref, const char* database, const char* message) {
    json_t* response = json_object();

    json_object_set_new(response, "success", json_string("fail"));
    json_object_set_new(response, "query_ref", json_integer(query_ref));
    json_object_set_new(response, "database", json_string(database));
    json_object_set_new(response, "rows", json_array());

    // Use provided message or default
    const char* msg = message && strlen(message) > 0 ? message : "queryref not found or not public";
    json_object_set_new(response, "message", json_string(msg));

    // DQM statistics are only included in status endpoints, not query endpoints

    return response;
}

// Build response JSON
json_t* build_response_json(int query_ref, const char* database, const QueryCacheEntry* cache_entry,
                            const DatabaseQueue* selected_queue, PendingQueryResult* pending, const char* message) {
    const QueryResult* result = wait_for_query_result(pending);

    if (result && result->success && !result->error_message) {
        return build_success_response(query_ref, cache_entry, result, selected_queue, message);
    } else {
        return build_error_response(query_ref, database, cache_entry, pending, result, message);
    }
}

// Determine HTTP status code based on result
unsigned int determine_http_status(const PendingQueryResult* pending, const QueryResult* result) {
    if (pending_result_is_timed_out(pending)) {
        return MHD_HTTP_REQUEST_TIMEOUT;
    } else if (result && result->error_message) {
        return MHD_HTTP_UNPROCESSABLE_ENTITY;
    } else {
        return MHD_HTTP_BAD_REQUEST;
    }
}

enum MHD_Result handle_query_id_generation(struct MHD_Connection *connection, const char* database,
                                            int query_ref, ParameterList* param_list, char* converted_sql,
                                            TypedParameter** ordered_params, char** query_id) {
    *query_id = generate_query_id();
    if (!*query_id) {
        free(converted_sql);
        free_parameter_list(param_list);
        if (ordered_params) free(ordered_params);
        json_t *error_response = create_processing_error_response("Failed to generate query ID", database, query_ref);

        api_send_json_response(connection, error_response, MHD_HTTP_INTERNAL_SERVER_ERROR);
        json_decref(error_response);
        return MHD_YES; // Response sent - processing complete
    }
    return MHD_YES; // Continue processing
}

enum MHD_Result handle_pending_registration(struct MHD_Connection *connection, const char* database,
                                             int query_ref, char* query_id, ParameterList* param_list,
                                             char* converted_sql, TypedParameter** ordered_params,
                                             const QueryCacheEntry* cache_entry, PendingQueryResult** pending) {
    PendingResultManager* pending_mgr = get_pending_result_manager();
    *pending = pending_result_register(pending_mgr, query_id, cache_entry->timeout_seconds, NULL);
    if (!*pending) {
        free(query_id);
        free(converted_sql);
        free_parameter_list(param_list);
        if (ordered_params) free(ordered_params);
        json_t *error_response = create_processing_error_response("Failed to register pending result", database, query_ref);

        api_send_json_response(connection, error_response, MHD_HTTP_INTERNAL_SERVER_ERROR);
        json_decref(error_response);
        return MHD_YES; // Response sent - processing complete
    }
    return MHD_YES; // Continue processing
}

enum MHD_Result handle_query_submission(struct MHD_Connection *connection, const char* database,
                                         int query_ref, DatabaseQueue* selected_queue, char* query_id,
                                         char* converted_sql, ParameterList* param_list,
                                         TypedParameter** ordered_params, size_t param_count,
                                         const QueryCacheEntry* cache_entry) {
    if (!prepare_and_submit_query(selected_queue, query_id, cache_entry->sql_template, ordered_params,
                                   param_count, cache_entry)) {
        // Cleanup on failure
        free(query_id);
        free(converted_sql);
        free_parameter_list(param_list);
        if (ordered_params) free(ordered_params);

        json_t *error_response = create_processing_error_response("Failed to submit query", database, query_ref);

        enum MHD_Result result = api_send_json_response(connection, error_response, MHD_HTTP_INTERNAL_SERVER_ERROR);
        json_decref(error_response);
        return result;
    }
    return MHD_YES; // Continue processing
}

enum MHD_Result handle_response_building(struct MHD_Connection *connection, int query_ref,
                                          const char* database, const QueryCacheEntry* cache_entry,
                                          const DatabaseQueue* selected_queue, PendingQueryResult* pending,
                                          char* query_id, char* converted_sql, ParameterList* param_list,
                                          TypedParameter** ordered_params, const char* message) {
    // Mark unused parameters
    (void)query_id;
    (void)converted_sql;
    (void)param_list;
    (void)ordered_params;

    // Wait for result and build response
    json_t* response = build_response_json(query_ref, database, cache_entry, selected_queue, pending, message);
    unsigned int http_status = json_is_true(json_object_get(response, "success")) ?
                                MHD_HTTP_OK : determine_http_status(pending, pending_result_get(pending));

    enum MHD_Result http_result = api_send_json_response(connection, response, http_status);
    json_decref(response);

    return http_result;
}