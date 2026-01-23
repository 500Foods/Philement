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
                        return false;
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
                                // Cleanup on realloc failure
                                for (size_t prov_idx = 0; prov_idx < provided_count; prov_idx++) {
                                    free(provided_params[prov_idx]);
                                }
                                free(provided_params);
                                // Cleanup required params
                                for (size_t req_cleanup_idx2 = 0; req_cleanup_idx2 < required_count; req_cleanup_idx2++) {
                                    free(required_params[req_cleanup_idx2]);
                                }
                                free(required_params);
                                return false;
                            }
                        } else {
                            // Cleanup on realloc failure
                            for (size_t prov_idx = 0; prov_idx < provided_count; prov_idx++) {
                                free(provided_params[prov_idx]);
                            }
                            free(provided_params);
                            // Cleanup required params
                            for (size_t req_cleanup_idx = 0; req_cleanup_idx < required_count; req_cleanup_idx++) {
                                free(required_params[req_cleanup_idx]);
                            }
                            free(required_params);
                            return false;
                        }
                    }
                }
            }
        }
    }

    // Find missing parameters
    for (size_t req_idx = 0; req_idx < required_count; req_idx++) {
        bool found = false;
        for (size_t prov_idx2 = 0; prov_idx2 < provided_count; prov_idx2++) {
            if (strcmp(required_params[req_idx], provided_params[prov_idx2]) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            char** new_missing = realloc(*missing_params, (*missing_count + 1) * sizeof(char*));
            if (!new_missing) {
                // Cleanup
                for (size_t i = 0; i < *missing_count; i++) {
                    free((*missing_params)[i]);
                }
                free(*missing_params);
                *missing_params = NULL;
                *missing_count = 0;
                // Continue cleanup
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
            *missing_params = new_missing;
            (*missing_params)[*missing_count] = strdup(required_params[req_idx]);
            if (!(*missing_params)[*missing_count]) {
                // Cleanup
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
            (*missing_count)++;
        }
    }

    // Find unused parameters
    for (size_t prov_idx = 0; prov_idx < provided_count; prov_idx++) {
        bool found = false;
        for (size_t req_idx2 = 0; req_idx2 < required_count; req_idx2++) {
            if (strcmp(provided_params[prov_idx], required_params[req_idx2]) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            char** new_unused = realloc(*unused_params, (*unused_count + 1) * sizeof(char*));
            if (!new_unused) {
                // Cleanup
                for (size_t i = 0; i < *missing_count; i++) {
                    free((*missing_params)[i]);
                }
                free(*missing_params);
                *missing_params = NULL;
                *missing_count = 0;
                for (size_t i = 0; i < *unused_count; i++) {
                    free((*unused_params)[i]);
                }
                free(*unused_params);
                *unused_params = NULL;
                *unused_count = 0;
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
            *unused_params = new_unused;
            (*unused_params)[*unused_count] = strdup(provided_params[prov_idx]);
            if (!(*unused_params)[*unused_count]) {
                // Cleanup
                for (size_t i = 0; i < *missing_count; i++) {
                    free((*missing_params)[i]);
                }
                free(*missing_params);
                *missing_params = NULL;
                *missing_count = 0;
                for (size_t i = 0; i < *unused_count; i++) {
                    free((*unused_params)[i]);
                }
                free(*unused_params);
                *unused_params = NULL;
                *unused_count = 0;
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
            (*unused_count)++;
        }
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
            const char* param_name;
            json_t* param_value;
            json_object_foreach(type_obj, param_name, param_value) {
                bool is_valid_type = false;

                switch (type_idx) {
                    case 0: // INTEGER
                        is_valid_type = json_is_integer(param_value);
                        break;
                    case 1: // STRING
                        is_valid_type = json_is_string(param_value);
                        break;
                    case 2: // BOOLEAN
                        is_valid_type = json_is_boolean(param_value);
                        break;
                    case 3: // FLOAT
                        is_valid_type = json_is_real(param_value) || json_is_integer(param_value);
                        break;
                    case 4: // TEXT
                    case 5: // DATE
                    case 6: // TIME
                    case 7: // DATETIME
                    case 8: // TIMESTAMP
                        is_valid_type = json_is_string(param_value);
                        break;
                }

                if (!is_valid_type) {
                    // This parameter has wrong type for its section
                    const char* actual_type = "unknown";

                    if (json_is_string(param_value)) actual_type = "string";
                    else if (json_is_integer(param_value)) actual_type = "integer";
                    else if (json_is_real(param_value)) actual_type = "float";
                    else if (json_is_boolean(param_value)) actual_type = "boolean";
                    else if (json_is_null(param_value)) actual_type = "null";

                    // Format: "PARAM_NAME(actual_type) should be PARAM_NAME(expected_type)"
                    char invalid_msg[256];
                    snprintf(invalid_msg, sizeof(invalid_msg), "%s(%s) should be %s(%s)",
                            param_name, actual_type, param_name, type_key);

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
                }
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
        const char* param_name;
        json_t* param_value;
        json_object_foreach(type_obj, param_name, param_value) {
            bool is_valid_type = false;

            switch (type_idx) {
                case 0: // INTEGER
                    is_valid_type = json_is_integer(param_value);
                    break;
                case 1: // STRING
                    is_valid_type = json_is_string(param_value);
                    break;
                case 2: // BOOLEAN
                    is_valid_type = json_is_boolean(param_value);
                    break;
                case 3: // FLOAT
                    is_valid_type = json_is_real(param_value) || json_is_integer(param_value);
                    break;
                case 4: // TEXT
                case 5: // DATE
                case 6: // TIME
                case 7: // DATETIME
                case 8: // TIMESTAMP
                    is_valid_type = json_is_string(param_value);
                    break;
            }

            if (!is_valid_type) {
                // This parameter has wrong type for its section
                const char* actual_type = "unknown";

                if (json_is_string(param_value)) actual_type = "string";
                else if (json_is_integer(param_value)) actual_type = "integer";
                else if (json_is_real(param_value)) actual_type = "float";
                else if (json_is_boolean(param_value)) actual_type = "boolean";
                else if (json_is_null(param_value)) actual_type = "null";

                // Format: "PARAM_NAME(actual_type) is not PARAM_NAME(expected_type)"
                char invalid_msg[256];
                snprintf(invalid_msg, sizeof(invalid_msg), "%s(%s) is not %s(%s)",
                        param_name, actual_type, param_name, type_key);

                size_t msg_len = strlen(invalid_msg);
                if (error_pos + msg_len + (error_pos > 0 ? 2 : 0) + 1 < buffer_size) {
                    if (error_pos > 0) {
                        strcpy(error_message + error_pos, ", ");
                        error_pos += 2;
                    }
                    strcpy(error_message + error_pos, invalid_msg);
                    error_pos += msg_len;
                }
            }
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
                        // Cleanup already allocated memory
                        for (size_t i = 0; i < required_count; i++) {
                            free(required_params[i]);
                        }
                        free(required_params);
                        return NULL;
                    }
                }
            }
        }
        sql = end;
    }

    // Check provided parameters
    char** provided_params = NULL;
    size_t provided_count = 0;

    if (param_list) {
        for (size_t i = 0; i < param_list->count; i++) {
            const TypedParameter* param = param_list->params[i];
            if (param->name) {
                // Check if already in list
                bool found = false;
                for (size_t j = 0; j < provided_count; j++) {
                    if (strcmp(provided_params[j], param->name) == 0) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    char** new_provided_params = realloc(provided_params, (provided_count + 1) * sizeof(char*));
                    if (new_provided_params) {
                        provided_params = new_provided_params;
                        provided_params[provided_count] = strdup(param->name);
                        if (provided_params[provided_count]) {
                            provided_count++;
                        } else {
                            // Cleanup on failure
                            for (size_t k = 0; k < provided_count; k++) {
                                free(provided_params[k]);
                            }
                            free(provided_params);
                            // Cleanup required params
                            for (size_t k = 0; k < required_count; k++) {
                                free(required_params[k]);
                            }
                            free(required_params);
                            return NULL;
                        }
                    } else {
                        // Cleanup on failure
                        for (size_t k = 0; k < provided_count; k++) {
                            free(provided_params[k]);
                        }
                        free(provided_params);
                        // Cleanup required params
                        for (size_t k = 0; k < required_count; k++) {
                            free(required_params[k]);
                        }
                        free(required_params);
                        return NULL;
                    }
                }
            }
        }
    }

    // Find missing parameters
    char* missing_message = NULL;
    size_t missing_pos = 0;
    size_t buffer_size = 512;

    for (size_t req_idx = 0; req_idx < required_count; req_idx++) {
        bool found = false;
        for (size_t prov_idx = 0; prov_idx < provided_count; prov_idx++) {
            if (strcmp(required_params[req_idx], provided_params[prov_idx]) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            // Add to missing message
            if (!missing_message) {
                missing_message = malloc(buffer_size);
                if (!missing_message) {
                    // Cleanup
                    for (size_t i = 0; i < required_count; i++) free(required_params[i]);
                    free(required_params);
                    for (size_t i = 0; i < provided_count; i++) free(provided_params[i]);
                    free(provided_params);
                    return NULL;
                }
                missing_message[0] = '\0';
            }

            size_t param_len = strlen(required_params[req_idx]);
            if (missing_pos + param_len + (missing_pos > 0 ? 2 : 0) + 1 < buffer_size) {
                if (missing_pos > 0) {
                    strcpy(missing_message + missing_pos, ", ");
                    missing_pos += 2;
                }
                strcpy(missing_message + missing_pos, required_params[req_idx]);
                missing_pos += param_len;
            }
        }
    }

    // Cleanup
    for (size_t i = 0; i < required_count; i++) free(required_params[i]);
    free(required_params);
    for (size_t i = 0; i < provided_count; i++) free(provided_params[i]);
    free(provided_params);

    return missing_message;
}

// Check for unused provided parameters (warning only)
char* check_unused_parameters_simple(const char* sql_template, ParameterList* param_list) {
    if (!sql_template || !param_list) {
        return NULL;
    }

    // Collect required parameters from SQL template
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
                        // Cleanup already allocated memory
                        for (size_t i = 0; i < required_count; i++) {
                            free(required_params[i]);
                        }
                        free(required_params);
                        return NULL;
                    }
                }
            }
        }
        sql = end;
    }

    // Find unused parameters
    char* unused_message = NULL;
    size_t unused_pos = 0;
    size_t buffer_size = 512;
    size_t prefix_len = strlen("Unused Parameters: ");

    for (size_t i = 0; i < param_list->count; i++) {
        const TypedParameter* param = param_list->params[i];
        if (param->name) {
            bool found = false;
            for (size_t j = 0; j < required_count; j++) {
                if (strcmp(param->name, required_params[j]) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                // Add to unused message
                if (!unused_message) {
                    unused_message = malloc(buffer_size);
                    if (!unused_message) {
                        // Cleanup
                        for (size_t k = 0; k < required_count; k++) free(required_params[k]);
                        free(required_params);
                        return NULL;
                    }
                    strcpy(unused_message, "Unused Parameters: ");
                    unused_pos = prefix_len;
                }

                size_t param_len = strlen(param->name);
                if (unused_pos + param_len + (unused_pos > prefix_len ? 2 : 0) + 1 < buffer_size) {
                    if (unused_pos > prefix_len) {
                        strcpy(unused_message + unused_pos, ", ");
                        unused_pos += 2;
                    }
                    strcpy(unused_message + unused_pos, param->name);
                    unused_pos += param_len;
                }
            }
        }
    }

    // Cleanup
    for (size_t i = 0; i < required_count; i++) free(required_params[i]);
    free(required_params);

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