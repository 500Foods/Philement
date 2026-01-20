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
#include "../conduit_service.h"
#include "query.h"

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

// Forward declarations for helper functions
json_t* create_validation_error_response(const char* error_msg, const char* error_detail);
json_t* create_lookup_error_response(const char* error_msg, const char* database, int query_ref, bool include_query_ref, const char* message);
json_t* create_processing_error_response(const char* error_msg, const char* database, int query_ref);
json_t* parse_request_data_from_buffer(struct MHD_Connection* connection, ApiPostBuffer* buffer, json_error_t* json_error_out);
enum MHD_Result handle_request_parsing_with_buffer(struct MHD_Connection *connection, ApiPostBuffer* buffer, json_t** request_json);
// Backwards-compatible wrappers for unit tests (uses raw upload_data)
json_t* parse_request_data(struct MHD_Connection* connection, const char* method,
                           const char* upload_data, const size_t* upload_data_size);
enum MHD_Result handle_request_parsing(struct MHD_Connection *connection, const char* method,
                                       const char* upload_data, const size_t* upload_data_size,
                                       json_t** request_json);

// Forward declarations for request handling helper functions
// (Now declared in header file)

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

// Validate HTTP method - only POST is allowed
bool validate_http_method(const char* method) {
    if (!method) return false;
    return strcmp(method, "POST") == 0;
}

// Parse request data from either POST JSON body (using ApiPostBuffer) or GET query parameters
json_t* parse_request_data_from_buffer(struct MHD_Connection* connection, ApiPostBuffer* buffer, json_error_t* json_error_out) {
    json_t* request_json = NULL;
    json_error_t json_error;

    if (buffer->http_method == 'P') {
        // POST request - parse JSON body from buffer
        if (!buffer->data || buffer->size == 0) {
            return NULL; // Missing request body
        }

        request_json = json_loads(buffer->data, 0, &json_error);
        if (!request_json) {
            log_this(SR_API, "Failed to parse JSON in conduit query: %s", LOG_LEVEL_ERROR, 1, json_error.text);
            if (json_error_out) {
                *json_error_out = json_error;
            }
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
                if (json_error_out) {
                    *json_error_out = json_error;
                }
                return NULL; // Invalid params JSON
            }
        }
    }

    return request_json;
}

// Backwards-compatible wrapper for unit tests
// Creates a temporary ApiPostBuffer from raw upload_data and calls parse_request_data_from_buffer
json_t* parse_request_data(struct MHD_Connection* connection, const char* method,
                            const char* upload_data, const size_t* upload_data_size) {
    // Create temporary buffer structure
    ApiPostBuffer temp_buffer = {
        .data = NULL,
        .size = 0,
        .capacity = 0,
        .http_method = (method && strcmp(method, "POST") == 0) ? 'P' : 'G'
    };

    // Copy upload_data to temporary buffer if provided
    if (upload_data && upload_data_size && *upload_data_size > 0) {
        temp_buffer.data = malloc(*upload_data_size + 1);
        if (!temp_buffer.data) {
            return NULL;
        }
        memcpy(temp_buffer.data, upload_data, *upload_data_size);
        temp_buffer.data[*upload_data_size] = '\0';
        temp_buffer.size = *upload_data_size;
        temp_buffer.capacity = *upload_data_size + 1;
    }

    // Call the new function with the buffer (NULL for json_error_out to maintain backwards compatibility)
    json_t* result = parse_request_data_from_buffer(connection, &temp_buffer, NULL);

    // Free temporary buffer
    if (temp_buffer.data) {
        free(temp_buffer.data);
    }

    return result;
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

// Lookup database queue from global queue manager
DatabaseQueue* lookup_database_queue(const char* database) {
    return database_queue_manager_get_database(global_queue_manager, database);
}

// Lookup query cache entry from database queue
QueryCacheEntry* lookup_query_cache_entry(DatabaseQueue* db_queue, int query_ref) {
    if (!db_queue || !db_queue->query_cache) {
        return NULL;
    }
    return query_cache_lookup(db_queue->query_cache, query_ref, NULL);
}

// Lookup database queue and query cache entry
#ifndef USE_MOCK_LOOKUP_DATABASE_AND_QUERY
bool lookup_database_and_query(DatabaseQueue** db_queue, QueryCacheEntry** cache_entry,
                               const char* database, int query_ref) {
    if (!db_queue || !cache_entry || !database) {
        return false; // NULL pointer parameters not allowed
    }

    *db_queue = lookup_database_queue(database);
    if (!*db_queue) {
        *cache_entry = NULL; // Ensure cache_entry is NULL when database lookup fails
        return false;
    }

    *cache_entry = lookup_query_cache_entry(*db_queue, query_ref);
    if (!*cache_entry) {
        return false;
    }

    return true;
}
#endif

// Lookup database queue and public query cache entry (query_type_a28 = 10)
bool lookup_database_and_public_query(DatabaseQueue** db_queue, QueryCacheEntry** cache_entry,
                                      const char* database, int query_ref) {
    if (!db_queue || !cache_entry || !database) {
        return false; // NULL pointer parameters not allowed
    }

    *db_queue = lookup_database_queue(database);
    if (!*db_queue) {
        *cache_entry = NULL; // Ensure cache_entry is NULL when database lookup fails
        return false;
    }

    if (!(*db_queue)->query_cache) {
        *cache_entry = NULL;
        return false;
    }

    // Use the type-filtered lookup for public queries (type = 10)
    *cache_entry = query_cache_lookup_by_ref_and_type((*db_queue)->query_cache, query_ref, 10, SR_API);
    if (!*cache_entry) {
        return false;
    }

    return true;
}

// Parse and convert parameters
#ifndef USE_MOCK_PROCESS_PARAMETERS
bool process_parameters(json_t* params_json, ParameterList** param_list,
                       const char* sql_template, DatabaseEngineType engine_type,
                       char** converted_sql, TypedParameter*** ordered_params, size_t* param_count) {
   *param_list = NULL;
   if (params_json && json_is_object(params_json)) {
       char* params_str = json_dumps(params_json, JSON_COMPACT);
       if (params_str) {
           *param_list = parse_typed_parameters(params_str, NULL);
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
       param_count,
       NULL
   );

   return (*converted_sql != NULL);
}
#endif

// Generate parameter validation messages
char* generate_parameter_messages(const char* sql_template, json_t* params_json) {
    if (!sql_template) {
        return NULL;
    }

    // Collect required parameters from SQL template
    // Find all :paramName patterns
    const char* sql = sql_template;
    char** required_params = NULL;
    size_t required_count = 0;

    while ((sql = strchr(sql, ':')) != NULL) {
        sql++; // Skip the :
        const char* end = sql;
        while (*end && (isalnum(*end) || *end == '_')) {
            end++;
        }
        if (end > sql) {
            // Found a parameter name
            size_t len = (size_t)(end - sql);
            char* param_name = malloc(len + 1);
            if (param_name) {
                memcpy(param_name, sql, len);
                param_name[len] = '\0';

                // Check if already in list
                bool found = false;
                for (size_t i = 0; i < required_count; i++) {
                    if (strcmp(required_params[i], param_name) == 0) {
                        found = true;
                        free(param_name);
                        break;
                    }
                }
                if (!found) {
                    char** new_required_params = realloc(required_params, (required_count + 1) * sizeof(char*));
                    if (new_required_params) {
                        required_params = new_required_params;
                        required_params[required_count] = param_name;
                        required_count++;
                    } else {
                        free(param_name);
                        // Cleanup already allocated memory on failure
                        for (size_t i = 0; i < required_count; i++) {
                            free(required_params[i]);
                        }
                        free(required_params);
                        required_params = NULL;
                        required_count = 0;
                    }
                }
            }
        }
        sql = end;
    }

    // Collect provided parameters
    char** provided_params = NULL;
    size_t provided_count = 0;

    if (params_json && json_is_object(params_json)) {
        const char* type_keys[] = {"INTEGER", "STRING", "BOOLEAN", "FLOAT", "TEXT", "DATE", "TIME", "DATETIME", "TIMESTAMP"};
        size_t type_count = sizeof(type_keys) / sizeof(type_keys[0]);

        for (size_t i = 0; i < type_count; i++) {
            json_t* type_obj = json_object_get(params_json, type_keys[i]);
            if (type_obj && json_is_object(type_obj)) {
                const char* key;
                json_t* value;
                json_object_foreach(type_obj, key, value) {
                    // Check if already in list
                    bool found = false;
                    for (size_t j = 0; j < provided_count; j++) {
                        if (strcmp(provided_params[j], key) == 0) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        char** new_provided_params = realloc(provided_params, (provided_count + 1) * sizeof(char*));
                        if (new_provided_params) {
                            provided_params = new_provided_params;
                            provided_params[provided_count] = strdup(key);
                            if (provided_params[provided_count]) {
                                provided_count++;
                            } else {
                                // Cleanup on strdup failure
                                for (size_t prov_idx = 0; prov_idx < provided_count; prov_idx++) {
                                    free(provided_params[prov_idx]);
                                }
                                free(provided_params);
                                provided_params = NULL;
                                provided_count = 0;
                            }
                        } else {
                            // Cleanup on realloc failure
                            for (size_t prov_idx = 0; prov_idx < provided_count; prov_idx++) {
                                free(provided_params[prov_idx]);
                            }
                            free(provided_params);
                            provided_params = NULL;
                            provided_count = 0;
                        }
                    }
                }
            }
        }
    }

    // Generate messages
    char* messages = NULL;
    size_t messages_len = 0;

    // Find missing parameters
    for (size_t req_idx = 0; req_idx < required_count; req_idx++) {
        bool found = false;
        for (size_t prov_idx = 0; prov_idx < provided_count; prov_idx++) {
            if (strcmp(required_params[req_idx], provided_params[prov_idx]) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            const char* prefix = "Missing parameters: ";
            size_t prefix_len = strlen(prefix);
            size_t param_len = strlen(required_params[req_idx]);
            char* new_messages = realloc(messages, messages_len + prefix_len + param_len + 3); // ", " + null
            if (!new_messages) {
                // Cleanup on realloc failure
                for (size_t i = 0; i < required_count; i++) {
                    free(required_params[i]);
                }
                free(required_params);
                for (size_t i = 0; i < provided_count; i++) {
                    free(provided_params[i]);
                }
                free(provided_params);
                free(messages);
                return NULL;
            }
            messages = new_messages;
            if (messages_len == 0) {
                strcpy(messages, prefix);
                messages_len = prefix_len;
            } else {
                strcpy(messages + messages_len, ", ");
                messages_len += 2;
            }
            strcpy(messages + messages_len, required_params[req_idx]);
            messages_len += param_len;
        }
    }

    // Find unused parameters
    if (messages_len > 0) {
        for (size_t prov_idx = 0; prov_idx < provided_count; prov_idx++) {
            bool found = false;
            for (size_t req_idx = 0; req_idx < required_count; req_idx++) {
                if (strcmp(provided_params[prov_idx], required_params[req_idx]) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                const char* prefix = "Parameters unused: ";
                size_t prefix_len = strlen(prefix);
                size_t param_len = strlen(provided_params[prov_idx]);
                char* new_messages = realloc(messages, messages_len + prefix_len + param_len + 3);
                if (!new_messages) {
                    // Cleanup on realloc failure
                    for (size_t i = 0; i < required_count; i++) {
                        free(required_params[i]);
                    }
                    free(required_params);
                    for (size_t i = 0; i < provided_count; i++) {
                        free(provided_params[i]);
                    }
                    free(provided_params);
                    free(messages);
                    return NULL;
                }
                messages = new_messages;
                if (messages_len == 0) {
                    strcpy(messages, prefix);
                    messages_len = prefix_len;
                } else {
                    strcpy(messages + messages_len, ", ");
                    messages_len += 2;
                }
                strcpy(messages + messages_len, provided_params[prov_idx]);
                messages_len += param_len;
            }
        }
    }

    // Clean up
    for (size_t i = 0; i < required_count; i++) {
        free(required_params[i]);
    }
    free(required_params);

    for (size_t i = 0; i < provided_count; i++) {
        free(provided_params[i]);
    }
    free(provided_params);

    if (messages_len > 0) {
        messages[messages_len] = '\0';
        return messages;
    } else {
        return NULL;
    }
}

// Select optimal queue for query execution
DatabaseQueue* select_query_queue(const char* database, const char* queue_type) {
#ifdef USE_MOCK_SELECT_QUERY_QUEUE
    return mock_select_query_queue(database, queue_type);
#else
    return select_optimal_queue(database, queue_type, global_queue_manager);
#endif
}

// Prepare and submit database query
bool prepare_and_submit_query(DatabaseQueue* selected_queue, const char* query_id,
                              const char* sql_template, TypedParameter** ordered_params,
                              size_t param_count, const QueryCacheEntry* cache_entry) {
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

// Helper function to create error response for validation failures
json_t* create_validation_error_response(const char* error_msg, const char* error_detail) {
    json_t* response = json_object();
    json_object_set_new(response, "success", json_false());
    json_object_set_new(response, "error", json_string(error_msg));
    json_object_set_new(response, "message", json_string(error_detail));
    return response;
}
// Helper function to create error response for lookup failures
json_t* create_lookup_error_response(const char* error_msg, const char* database, int query_ref, bool include_query_ref, const char* message) {
    json_t* response = json_object();
    if (!response) return NULL;

    json_t* success_val = json_false();
    if (!success_val) {
        json_decref(response);
        return NULL;
    }
    json_object_set_new(response, "success", success_val);

    json_t* error_val = json_string(error_msg);
    if (!error_val) {
        json_decref(response);
        return NULL;
    }
    json_object_set_new(response, "error", error_val);

    if (database) {
        json_t* db_val = json_string(database);
        if (!db_val) {
            json_decref(response);
            return NULL;
        }
        json_object_set_new(response, "database", db_val);
    }

    if (include_query_ref) {
        json_t* ref_val = json_integer(query_ref);
        if (!ref_val) {
            json_decref(response);
            return NULL;
        }
        json_object_set_new(response, "query_ref", ref_val);
    }

    if (message) {
        json_t* msg_val = json_string(message);
        if (!msg_val) {
            json_decref(response);
            return NULL;
        }
        json_object_set_new(response, "message", msg_val);
    }

    return response;
}

// Helper function to create error response for processing failures
json_t* create_processing_error_response(const char* error_msg, const char* database, int query_ref) {
    json_t* response = json_object();
    json_object_set_new(response, "success", json_false());
    json_object_set_new(response, "error", json_string(error_msg));
    json_object_set_new(response, "query_ref", json_integer(query_ref));
    json_object_set_new(response, "database", json_string(database ? database : ""));
    return response;
}

// Wait for query result and build response
json_t* build_response_json(int query_ref, const char* database, const QueryCacheEntry* cache_entry,
                           const DatabaseQueue* selected_queue, PendingQueryResult* pending, const char* message) {
    const QueryResult* result = wait_for_query_result(pending);

    if (result && result->success) {
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

// Helper function to handle HTTP method validation
enum MHD_Result handle_method_validation(struct MHD_Connection *connection, const char* method) {
    if (!validate_http_method(method)) {
        json_t *error_response = create_validation_error_response(
            "Method not allowed",
            "Only POST requests are supported"
        );

        api_send_json_response(connection, error_response, MHD_HTTP_METHOD_NOT_ALLOWED);
        json_decref(error_response);
        return MHD_YES; // Response sent - processing complete
    }
    return MHD_YES; // Continue processing
}

// Helper function to handle request data parsing using ApiPostBuffer
enum MHD_Result handle_request_parsing_with_buffer(struct MHD_Connection *connection, ApiPostBuffer* buffer,
                                                     json_t** request_json) {
    json_error_t json_error = {0}; // Initialize to avoid uninitialized warnings
    *request_json = parse_request_data_from_buffer(connection, buffer, &json_error);
    if (!*request_json) {
        // Determine specific error based on method and data
        const char* error_msg = "Invalid JSON";
        const char* error_detail = "Request body contains invalid JSON";
        unsigned int http_status = MHD_HTTP_BAD_REQUEST;
        char detailed_error[256];

        if (buffer->http_method == 'P' && (!buffer->data || buffer->size == 0)) {
            error_msg = "Missing request body";
            error_detail = "POST requests must include a JSON body";
        } else if (buffer->http_method == 'P') {
            // Use detailed JSON error information from jansson
            snprintf(detailed_error, sizeof(detailed_error), "Unexpected token at position %d", json_error.position);
            error_detail = detailed_error;
        }

        json_t *error_response = create_validation_error_response(error_msg, error_detail);

        api_send_json_response(connection, error_response, http_status);
        json_decref(error_response);
        return MHD_YES; // Response sent - processing complete
    }
    return MHD_YES; // Continue processing
}

// Backwards-compatible wrapper for unit tests
// Creates a temporary ApiPostBuffer from raw upload_data and calls handle_request_parsing_with_buffer
enum MHD_Result handle_request_parsing(struct MHD_Connection *connection, const char* method,
                                       const char* upload_data, const size_t* upload_data_size,
                                       json_t** request_json) {
    // Create temporary buffer structure
    ApiPostBuffer temp_buffer = {
        .data = NULL,
        .size = 0,
        .capacity = 0,
        .http_method = (method && strcmp(method, "POST") == 0) ? 'P' : 'G'
    };
    
    // Copy upload_data to temporary buffer if provided
    if (upload_data && upload_data_size && *upload_data_size > 0) {
        temp_buffer.data = malloc(*upload_data_size + 1);
        if (!temp_buffer.data) {
            return MHD_NO;
        }
        memcpy(temp_buffer.data, upload_data, *upload_data_size);
        temp_buffer.data[*upload_data_size] = '\0';
        temp_buffer.size = *upload_data_size;
        temp_buffer.capacity = *upload_data_size + 1;
    }
    
    // Call the new function with the buffer
    enum MHD_Result result = handle_request_parsing_with_buffer(connection, &temp_buffer, request_json);
    
    // Free temporary buffer
    if (temp_buffer.data) {
        free(temp_buffer.data);
    }
    
    return result;
}

// Helper function to handle field extraction and validation
enum MHD_Result handle_field_extraction(struct MHD_Connection *connection, json_t* request_json,
                                        int* query_ref, const char** database, json_t** params_json) {
    if (!extract_request_fields(request_json, query_ref, database, params_json)) {
        const char* error_msg = !json_object_get(request_json, "query_ref") || !json_is_integer(json_object_get(request_json, "query_ref"))
                               ? "Missing or invalid query_ref" : "Missing or invalid database";
        const char* error_detail = !json_object_get(request_json, "query_ref") || !json_is_integer(json_object_get(request_json, "query_ref"))
                                  ? "query_ref must be an integer" : "database must be a string";

        json_t *error_response = create_validation_error_response(error_msg, error_detail);

        api_send_json_response(connection, error_response, MHD_HTTP_BAD_REQUEST);
        json_decref(error_response);
        return MHD_YES; // Response sent - processing complete
    }
    return MHD_YES; // Continue processing
}

// Helper function to handle database and query lookup
enum MHD_Result handle_database_lookup(struct MHD_Connection *connection, const char* database,
                                        int query_ref, DatabaseQueue** db_queue, QueryCacheEntry** cache_entry,
                                        bool* query_not_found, bool require_public) {
    bool lookup_success;
    if (require_public) {
        lookup_success = lookup_database_and_public_query(db_queue, cache_entry, database, query_ref);
    } else {
        lookup_success = lookup_database_and_query(db_queue, cache_entry, database, query_ref);
    }

    if (!lookup_success) {
        if (!*db_queue) {
            // Database not found - return 400 with appropriate message
            const char* error_msg = "Invalid database selection";
            unsigned int http_status = MHD_HTTP_BAD_REQUEST;
            const char* message = "The specified database does not exist or is not configured for queries";

            json_t *error_response = create_lookup_error_response(error_msg, database, query_ref, true, message);

            api_send_json_response(connection, error_response, http_status);
            json_decref(error_response);
            return MHD_YES; // Error response sent - main function will check db_queue
        } else {
            // Query not found - continue processing but mark as not found
            *query_not_found = true;
            *cache_entry = NULL;
            return MHD_YES; // Continue processing
        }
    }
    *query_not_found = false;
    return MHD_YES; // Continue processing
}

// Helper function to handle parameter processing
enum MHD_Result handle_parameter_processing(struct MHD_Connection *connection, json_t* params_json,
                                            const DatabaseQueue* db_queue, const QueryCacheEntry* cache_entry,
                                            const char* database, int query_ref,
                                            ParameterList** param_list, char** converted_sql,
                                            TypedParameter*** ordered_params, size_t* param_count) {
    if (!db_queue || !process_parameters(params_json, param_list, cache_entry->sql_template,
                           db_queue->engine_type, converted_sql, ordered_params, param_count)) {
        json_t *error_response = create_processing_error_response(
            !*param_list ? "Memory allocation failed" : "Parameter conversion failed",
            database, query_ref
        );

        api_send_json_response(connection, error_response, MHD_HTTP_BAD_REQUEST);
        json_decref(error_response);
        return MHD_YES; // Response sent - processing complete
    }
    return MHD_YES; // Continue processing
}

// Helper function to handle queue selection
enum MHD_Result handle_queue_selection(struct MHD_Connection *connection, const char* database,
                                        int query_ref, const QueryCacheEntry* cache_entry,
                                        ParameterList* param_list, char* converted_sql,
                                        TypedParameter** ordered_params,
                                        DatabaseQueue** selected_queue) {
    *selected_queue = select_query_queue(database, cache_entry->queue_type);
    if (!*selected_queue) {
        free(converted_sql);
        free_parameter_list(param_list);
        if (ordered_params) free(ordered_params);
        json_t *error_response = create_processing_error_response("No suitable queue available", database, query_ref);

        api_send_json_response(connection, error_response, MHD_HTTP_SERVICE_UNAVAILABLE);
        json_decref(error_response);
        return MHD_YES; // Response sent - processing complete
    }
    return MHD_YES; // Continue processing
}

// Helper function to handle query ID generation
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

// Helper function to handle pending result registration
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

// Helper function to handle query submission
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

// Helper function to handle response building and sending
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

    // Use common POST body buffering (handles both GET and POST)
    ApiPostBuffer *buffer = NULL;
    ApiBufferResult buf_result = api_buffer_post_data(method, upload_data, upload_data_size, con_cls, &buffer);
    
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
            break;
    }

    log_this(SR_API, "%s: Processing conduit query request", LOG_LEVEL_TRACE, 1, conduit_service_name());

    // Step 1: Validate HTTP method (GET and POST are allowed)
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

    log_this(SR_API, "%s: Request data parsed successfully", LOG_LEVEL_TRACE, 1, conduit_service_name());

    // Step 3: Extract and validate required fields
    int query_ref = 0;
    const char* database = NULL;
    json_t* params_json = NULL;
    result = handle_field_extraction(connection, request_json, &query_ref, &database, &params_json);
    if (result != MHD_YES) {
        json_decref(request_json);
        return result;
    }

    log_this(SR_API, "%s: Request fields extracted: query_ref=%d, database=%s", LOG_LEVEL_TRACE, 3, conduit_service_name(), query_ref, database);

    // Step 4: Lookup database queue and query cache entry
    DatabaseQueue* db_queue = NULL;
    QueryCacheEntry* cache_entry = NULL;
    bool query_not_found = false;
    result = handle_database_lookup(connection, database, query_ref, &db_queue, &cache_entry, &query_not_found, true);
    if (result != MHD_YES) {
        json_decref(request_json);
        return result;
    }

    // Check if database lookup failed (error response already sent)
    if (!db_queue) {
        json_decref(request_json);
        return MHD_YES;
    }

    // Handle invalid queryref case
    if (query_not_found) {
        json_t* response = build_invalid_queryref_response(query_ref, database, NULL);
        enum MHD_Result http_result = api_send_json_response(connection, response, MHD_HTTP_OK);
        json_decref(response);
        json_decref(request_json);
        return http_result;
    }

    log_this(SR_API, "%s: Database and query lookup successful", LOG_LEVEL_TRACE, 1, conduit_service_name());

    // Step 5: Parse and convert parameters
    ParameterList* param_list;
    char* converted_sql;
    TypedParameter** ordered_params;
    size_t param_count;
    result = handle_parameter_processing(connection, params_json, db_queue, cache_entry,
                                       database, query_ref, &param_list, &converted_sql,
                                       &ordered_params, &param_count);
    if (result != MHD_YES) {
        json_decref(request_json);
        return result;
    }

    // Generate parameter validation messages
    char* message = generate_parameter_messages(cache_entry->sql_template, params_json);

    // Step 6: Select optimal queue
    DatabaseQueue* selected_queue;
    result = handle_queue_selection(connection, database, query_ref, cache_entry,
                                   param_list, converted_sql, ordered_params, &selected_queue);
    if (result != MHD_YES) {
        json_decref(request_json);
        return result;
    }

    // Step 7: Generate unique query ID
    char* query_id;
    result = handle_query_id_generation(connection, database, query_ref, param_list,
                                       converted_sql, ordered_params, &query_id);
    if (result != MHD_YES) {
        json_decref(request_json);
        return result;
    }

    // Step 8: Register pending result
    PendingQueryResult* pending;
    result = handle_pending_registration(connection, database, query_ref, query_id,
                                       param_list, converted_sql, ordered_params,
                                       cache_entry, &pending);
    if (result != MHD_YES) {
        json_decref(request_json);
        return result;
    }

    // Step 9: Prepare and submit query
    result = handle_query_submission(connection, database, query_ref, selected_queue, query_id,
                                   converted_sql, param_list, ordered_params, param_count, cache_entry);
    if (result != MHD_YES) {
        json_decref(request_json);
        return result;
    }

    // Step 10: Wait for result and build response
    result = handle_response_building(connection, query_ref, database, cache_entry,
                                     selected_queue, pending, query_id, converted_sql,
                                     param_list, ordered_params, message);
    json_decref(request_json);

    // Clean up message
    if (message) free(message);

    log_this(SR_API, "%s: Conduit query request processing completed", LOG_LEVEL_TRACE, 1, conduit_service_name());

    return result;
}