/*
 * Conduit Service Parameter Processing Helper Functions
 * 
 * Functions for parameter parsing and processing.
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