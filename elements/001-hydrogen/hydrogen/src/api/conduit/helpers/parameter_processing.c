/*
 * Conduit Service Parameter Processing Helper Functions
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
#include "param_proc_helpers.h"

// Enable mock database queue functions for unit testing
#ifdef USE_MOCK_DBQUEUE
#include <unity/mocks/mock_dbqueue.h>
#endif

// Enable mock system functions for unit testing
#ifdef USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>
#endif


// Analyze parameter validation and return structured information
bool analyze_parameter_validation(const char* sql_template, json_t* params_json,
                                   char*** missing_params, size_t* missing_count,
                                   char*** unused_params, size_t* unused_count,
                                   char* invalid_buffer, size_t invalid_buffer_size, size_t* invalid_pos);

// Simple parameter type validation - checks type mismatches only
char* validate_parameter_types_simple(json_t* params_json);
// Check for missing required parameters after parsing
char* check_missing_parameters_simple(const char* sql_template, ParameterList* param_list);
// Check for unused provided parameters (warning only)
char* check_unused_parameters_simple(const char* sql_template, ParameterList* param_list);

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

// Analyze parameter validation and return structured information
bool analyze_parameter_validation(const char* sql_template, json_t* params_json,
                                   char*** missing_params, size_t* missing_count,
                                   char*** unused_params, size_t* unused_count,
                                   char* invalid_buffer, size_t invalid_buffer_size, size_t* invalid_pos) {
    *missing_params = NULL;
    *missing_count = 0;
    *unused_params = NULL;
    *unused_count = 0;
    *invalid_pos = 0;

    if (!sql_template) {
        return true; // No template, no validation needed
    }

    // Collect required parameters from SQL template
    size_t required_count = 0;
    char** required_params = extract_required_parameters(sql_template, &required_count);
    if (!required_params && required_count > 0) {
        return false; // Memory allocation failed
    }

    // Collect provided parameters
    size_t provided_count = 0;
    char** provided_params = collect_provided_parameters(params_json, &provided_count);
    if (!provided_params && provided_count > 0) {
        // Cleanup required params
        for (size_t i = 0; i < required_count; i++) {
            free(required_params[i]);
        }
        free(required_params);
        return false; // Memory allocation failed
    }

    // Find missing parameters
    *missing_params = find_missing_parameters(required_params, required_count, provided_params, provided_count, missing_count);
    if (*missing_count > 0 && !*missing_params) {
        // Memory allocation failed
        for (size_t i = 0; i < required_count; i++) {
            free(required_params[i]);
        }
        free(required_params);
        for (size_t i = 0; i < provided_count; i++) {
            free(provided_params[i]);
        }
        free(provided_params);
        return false;
    }

    // Find unused parameters
    *unused_params = find_unused_parameters(required_params, required_count, provided_params, provided_count, unused_count);
    if (*unused_count > 0 && !*unused_params) {
        // Memory allocation failed
        for (size_t i = 0; i < *missing_count; i++) {
            free((*missing_params)[i]);
        }
        free(*missing_params);
        *missing_params = NULL;
        *missing_count = 0;
        for (size_t i = 0; i < required_count; i++) {
            free(required_params[i]);
        }
        free(required_params);
        for (size_t i = 0; i < provided_count; i++) {
            free(provided_params[i]);
        }
        free(provided_params);
        return false;
    }

    // Check for invalid parameters (type mismatches in JSON)
    if (params_json && json_is_object(params_json)) {
        const char* type_keys[] = {"INTEGER", "STRING", "BOOLEAN", "FLOAT", "TEXT", "DATE", "TIME", "DATETIME", "TIMESTAMP"};
        size_t num_types = sizeof(type_keys) / sizeof(type_keys[0]);

        for (size_t type_idx = 0; type_idx < num_types; type_idx++) {
            const char* type_key = type_keys[type_idx];
            json_t* type_obj = json_object_get(params_json, type_key);

            if (!type_obj || !json_is_object(type_obj)) {
                continue;
            }

            // Check each parameter in this type section
            json_t* iter = json_object_iter(type_obj);
            while (iter) {
                const char* param_name = json_object_iter_key(iter);
                json_t* param_value = json_object_iter_value(iter);

                if (!validate_single_parameter_type(param_value, (int)type_idx)) {
                    // This parameter has wrong type for its section
                    const char* actual_type = "unknown";

                    if (json_is_string(param_value)) actual_type = "string";
                    else if (json_is_integer(param_value)) actual_type = "integer";
                    else if (json_is_real(param_value)) actual_type = "float";
                    else if (json_is_boolean(param_value)) actual_type = "boolean";
                    else if (json_is_null(param_value)) actual_type = "null";

                    char* invalid_msg = format_type_error_message(param_name, actual_type, type_key, "should be");
                    if (invalid_msg) {
                        if (*invalid_pos < invalid_buffer_size - 1) {
                            size_t len = strlen(invalid_msg);
                            if (*invalid_pos + len + (*invalid_pos > 0 ? 2 : 0) < invalid_buffer_size) {
                                if (*invalid_pos > 0) {
                                    strcpy(invalid_buffer + *invalid_pos, ", ");
                                    *invalid_pos += 2;
                                }
                                strcpy(invalid_buffer + *invalid_pos, invalid_msg);
                                *invalid_pos += len;
                            }
                        }
                        free(invalid_msg);
                    }
                }

                iter = json_object_iter_next(type_obj, iter);
            }
        }
    }

    // Null-terminate the invalid buffer
    if (*invalid_pos < invalid_buffer_size) {
        invalid_buffer[*invalid_pos] = '\0';
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

    return true;
}

// Generate parameter validation messages (legacy function for backward compatibility)
char* generate_parameter_messages(const char* sql_template, json_t* params_json) {
    char** missing_params = NULL;
    size_t missing_count = 0;
    char** unused_params = NULL;
    size_t unused_count = 0;
    char invalid_buffer[4096] = {0};
    size_t invalid_pos = 0;

    if (!analyze_parameter_validation(sql_template, params_json, &missing_params, &missing_count,
                                       &unused_params, &unused_count, invalid_buffer, sizeof(invalid_buffer), &invalid_pos)) {
        return NULL;
    }

    // Calculate total message size
    size_t total_size = 1; // null terminator

    if (missing_count > 0) {
        total_size += strlen("Missing parameters: ");
        for (size_t i = 0; i < missing_count; i++) {
            if (missing_params[i]) {
                total_size += strlen(missing_params[i]);
            }
            if (i > 0) total_size += 2; // ", "
        }
    }

    if (invalid_pos > 0) {
        total_size += strlen("Invalid parameters: ");
        if (missing_count > 0) total_size += 2; // ". "
        total_size += invalid_pos; // Length of invalid buffer content
    }

    if (unused_count > 0) {
        total_size += strlen("Unused parameters: ");
        if (missing_count > 0 || invalid_pos > 0) total_size += 2; // ". "
        for (size_t i = 0; i < unused_count; i++) {
            if (unused_params[i]) {
                total_size += strlen(unused_params[i]);
            }
            if (i > 0) total_size += 2; // ", "
        }
    }

    if (total_size == 1) {
        // Clean up
        for (size_t i = 0; i < missing_count; i++) free(missing_params[i]);
        free(missing_params);
        for (size_t i = 0; i < unused_count; i++) free(unused_params[i]);
        free(unused_params);
        return NULL;
    }

    // Allocate message buffer
    char* message = malloc(total_size);
    if (!message) {
        // Clean up
        for (size_t i = 0; i < missing_count; i++) free(missing_params[i]);
        free(missing_params);
        for (size_t i = 0; i < unused_count; i++) free(unused_params[i]);
        free(unused_params);
        return NULL;
    }

    size_t pos = 0;

    // Add missing parameters
    if (missing_count > 0) {
        strcpy(message + pos, "Missing parameters: ");
        pos += strlen("Missing parameters: ");

        for (size_t i = 0; i < missing_count; i++) {
            if (i > 0) {
                strcpy(message + pos, ", ");
                pos += 2;
            }
            size_t param_len = strlen(missing_params[i]);
            strcpy(message + pos, missing_params[i]);
            pos += param_len;
        }
    }

    // Add invalid parameters
    if (invalid_pos > 0) {
        if (missing_count > 0) {
            strcpy(message + pos, ". ");
            pos += 2;
        }
        strcpy(message + pos, "Invalid parameters: ");
        pos += strlen("Invalid parameters: ");
        strcpy(message + pos, invalid_buffer);
        pos += invalid_pos;
    }

    // Add unused parameters
    if (unused_count > 0) {
        if (missing_count > 0 || invalid_pos > 0) {
            strcpy(message + pos, ". ");
            pos += 2;
        }
        strcpy(message + pos, "Unused parameters: ");
        pos += strlen("Unused parameters: ");

        for (size_t i = 0; i < unused_count; i++) {
            if (i > 0) {
                strcpy(message + pos, ", ");
                pos += 2;
            }
            size_t param_len = strlen(unused_params[i]);
            strcpy(message + pos, unused_params[i]);
            pos += param_len;
        }
    }

    message[pos] = '\0';

    // Clean up
    for (size_t i = 0; i < missing_count; i++) free(missing_params[i]);
    free(missing_params);
    for (size_t i = 0; i < unused_count; i++) free(unused_params[i]);
    free(unused_params);

    return message;
}

// Simple parameter type validation - checks type mismatches only
char* validate_parameter_types_simple(json_t* params_json) {
    if (!params_json || !json_is_object(params_json)) {
        return NULL; // No parameters to validate
    }

    const char* type_keys[] = {"INTEGER", "STRING", "BOOLEAN", "FLOAT", "TEXT", "DATE", "TIME", "DATETIME", "TIMESTAMP"};
    size_t num_types = sizeof(type_keys) / sizeof(type_keys[0]);
    char* error_message = NULL;
    size_t error_pos = 0;
    size_t buffer_size = 1024;

    error_message = malloc(buffer_size);
    if (!error_message) {
        return NULL;
    }
    error_message[0] = '\0';

    for (size_t type_idx = 0; type_idx < num_types; type_idx++) {
        const char* type_key = type_keys[type_idx];
        json_t* type_obj = json_object_get(params_json, type_key);

        if (!type_obj || !json_is_object(type_obj)) {
            continue;
        }

        // Check each parameter in this type section
        json_t* iter = json_object_iter(type_obj);
        while (iter) {
            const char* param_name = json_object_iter_key(iter);
            json_t* param_value = json_object_iter_value(iter);

            if (!validate_single_parameter_type(param_value, (int)type_idx)) {
                // This parameter has wrong type for its section
                const char* actual_type = "unknown";

                if (json_is_string(param_value)) actual_type = "string";
                else if (json_is_integer(param_value)) actual_type = "integer";
                else if (json_is_real(param_value)) actual_type = "float";
                else if (json_is_boolean(param_value)) actual_type = "boolean";
                else if (json_is_null(param_value)) actual_type = "null";

                char* invalid_msg = format_type_error_message(param_name, actual_type, type_key, "is not");
                if (invalid_msg) {
                    size_t msg_len = strlen(invalid_msg);
                    if (error_pos + msg_len + (error_pos > 0 ? 2 : 0) + 1 < buffer_size) {
                        if (error_pos > 0) {
                            strcpy(error_message + error_pos, ", ");
                            error_pos += 2;
                        }
                        strcpy(error_message + error_pos, invalid_msg);
                        error_pos += msg_len;
                    }
                    free(invalid_msg);
                }
            }

            iter = json_object_iter_next(type_obj, iter);
        }
    }

    if (error_pos == 0) {
        free(error_message);
        return NULL; // No type errors
    }

    return error_message;
}

// Check for missing required parameters after parsing
char* check_missing_parameters_simple(const char* sql_template, ParameterList* param_list) {
    if (!sql_template) {
        return NULL; // No template, no missing parameters
    }

    // Collect required parameters from SQL template
    size_t required_count = 0;
    char** required_params = extract_required_parameters(sql_template, &required_count);
    if (!required_params && required_count > 0) {
        return NULL; // Memory allocation failed
    }

    // Collect provided parameters from ParameterList
    size_t provided_count = 0;
    char** provided_params = collect_provided_from_param_list(param_list, &provided_count);
    if (!provided_params && provided_count > 0) {
        // Cleanup required params
        for (size_t i = 0; i < required_count; i++) free(required_params[i]);
        free(required_params);
        return NULL; // Memory allocation failed
    }

    // Find missing parameters
    size_t missing_count = 0;
    char** missing_params = find_missing_parameters(required_params, required_count, provided_params, provided_count, &missing_count);
    if (missing_count > 0 && !missing_params) {
        // Cleanup
        for (size_t i = 0; i < required_count; i++) free(required_params[i]);
        free(required_params);
        for (size_t i = 0; i < provided_count; i++) free(provided_params[i]);
        free(provided_params);
        return NULL;
    }

    // Format message
    char* missing_message = NULL;
    if (missing_count > 0) {
        size_t buffer_size = 512;
        missing_message = malloc(buffer_size);
        if (missing_message) {
            size_t pos = 0;
            for (size_t i = 0; i < missing_count; i++) {
                size_t param_len = strlen(missing_params[i]);
                if (pos + param_len + (pos > 0 ? 2 : 0) + 1 < buffer_size) {
                    if (pos > 0) {
                        strcpy(missing_message + pos, ", ");
                        pos += 2;
                    }
                    strcpy(missing_message + pos, missing_params[i]);
                    pos += param_len;
                }
            }
            missing_message[pos] = '\0';
        }
    }

    // Cleanup
    for (size_t i = 0; i < required_count; i++) free(required_params[i]);
    free(required_params);
    for (size_t i = 0; i < provided_count; i++) free(provided_params[i]);
    free(provided_params);
    for (size_t i = 0; i < missing_count; i++) free(missing_params[i]);
    free(missing_params);

    return missing_message;
}

// Check for unused provided parameters (warning only)
char* check_unused_parameters_simple(const char* sql_template, ParameterList* param_list) {
    if (!sql_template || !param_list) {
        return NULL;
    }

    // Collect required parameters from SQL template
    size_t required_count = 0;
    char** required_params = extract_required_parameters(sql_template, &required_count);
    if (!required_params && required_count > 0) {
        return NULL; // Memory allocation failed
    }

    // Collect provided parameters from ParameterList
    size_t provided_count = 0;
    char** provided_params = collect_provided_from_param_list(param_list, &provided_count);
    if (!provided_params && provided_count > 0) {
        // Cleanup required params
        for (size_t i = 0; i < required_count; i++) free(required_params[i]);
        free(required_params);
        return NULL; // Memory allocation failed
    }

    // Find unused parameters
    size_t unused_count = 0;
    char** unused_params = find_unused_parameters(required_params, required_count, provided_params, provided_count, &unused_count);
    if (unused_count > 0 && !unused_params) {
        // Cleanup
        for (size_t i = 0; i < required_count; i++) free(required_params[i]);
        free(required_params);
        for (size_t i = 0; i < provided_count; i++) free(provided_params[i]);
        free(provided_params);
        return NULL;
    }

    // Format message
    char* unused_message = NULL;
    if (unused_count > 0) {
        size_t buffer_size = 512;
        unused_message = malloc(buffer_size);
        if (unused_message) {
            strcpy(unused_message, "Unused Parameters: ");
            size_t pos = strlen("Unused Parameters: ");
            for (size_t i = 0; i < unused_count; i++) {
                size_t param_len = strlen(unused_params[i]);
                if (pos + param_len + (pos > strlen("Unused Parameters: ") ? 2 : 0) + 1 < buffer_size) {
                    if (pos > strlen("Unused Parameters: ")) {
                        strcpy(unused_message + pos, ", ");
                        pos += 2;
                    }
                    strcpy(unused_message + pos, unused_params[i]);
                    pos += param_len;
                }
            }
            unused_message[pos] = '\0';
        }
    }

    // Cleanup
    for (size_t i = 0; i < required_count; i++) free(required_params[i]);
    free(required_params);
    for (size_t i = 0; i < provided_count; i++) free(provided_params[i]);
    free(provided_params);
    for (size_t i = 0; i < unused_count; i++) free(unused_params[i]);
    free(unused_params);

    return unused_message;
}

enum MHD_Result handle_parameter_processing(struct MHD_Connection *connection, json_t* params_json,
                                             const DatabaseQueue* db_queue, const QueryCacheEntry* cache_entry,
                                             const char* database, int query_ref,
                                             ParameterList** param_list, char** converted_sql,
                                             TypedParameter*** ordered_params, size_t* param_count,
                                             char** message) {
    // Check for NULL db_queue
    if (!db_queue) {
        json_t *error_response = create_processing_error_response("Database queue not available", database, query_ref);
        api_send_json_response(connection, error_response, MHD_HTTP_INTERNAL_SERVER_ERROR);
        json_decref(error_response);
        *converted_sql = NULL;
        return MHD_NO;
    }

    // (B) Validate Params: Check type mismatches
    char* type_error = validate_parameter_types_simple(params_json);
    if (type_error) {
        json_t *error_response = create_processing_error_response("Parameter type mismatch", database, query_ref);
        json_object_set_new(error_response, "message", json_string(type_error));
        api_send_json_response(connection, error_response, MHD_HTTP_BAD_REQUEST);
        json_decref(error_response);
        free(type_error);
        *converted_sql = NULL;
        return MHD_YES;
    }

    // (C) Check for missing required parameters BEFORE processing
    // Parse parameters first to check what's provided
    ParameterList* temp_param_list = NULL;
    if (params_json && json_is_object(params_json)) {
        char* params_str = json_dumps(params_json, JSON_COMPACT);
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
        api_send_json_response(connection, error_response, MHD_HTTP_BAD_REQUEST);
        json_decref(error_response);
        free(missing_error);
        *converted_sql = NULL;
        return MHD_YES;
    }

    // (D) Assign Parameters: Parse and convert now that we know parameters are complete
    bool processing_success = process_parameters(params_json, param_list, cache_entry->sql_template, db_queue->engine_type, converted_sql, ordered_params, param_count);

    if (!processing_success) {
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

        json_t *error_response = create_processing_error_response("Parameter processing failed", database, query_ref);
        api_send_json_response(connection, error_response, MHD_HTTP_BAD_REQUEST);
        json_decref(error_response);
        *converted_sql = NULL;
        return MHD_YES;
    }

    // Check for unused parameters (warning only) - now using the final param_list
    char* unused_warning = check_unused_parameters_simple(cache_entry->sql_template, *param_list);
    if (unused_warning) {
        log_this(SR_API, "%s: Query %d has unused parameters: %s", LOG_LEVEL_ALERT, 3, conduit_service_name(), query_ref, unused_warning);
        // Set message for response
        *message = unused_warning;
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

    return MHD_YES; // Continue processing
}