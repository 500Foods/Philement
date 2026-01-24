/*
 * Parameter Processing Helper Functions Implementation
 * Helper functions for parameter parsing and processing.
 */

// Project includes
#include <src/hydrogen.h>

// Database subsystem includes
#include <src/database/database_params.h>

// Standard includes
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// JSON library
#include <jansson.h>

// Local includes
#include "param_proc_helpers.h"

// Helper function to extract required parameter names from SQL template
// Returns array of parameter names, caller must free each string and the array
char** extract_required_parameters(const char* sql_template, size_t* count) {
    *count = 0;
    if (!sql_template) {
        return NULL;
    }

    char** required_params = NULL;
    const char* sql = sql_template;

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

                // Check if already in list (avoid duplicates)
                bool found = false;
                for (size_t i = 0; i < *count; i++) {
                    if (strcmp(required_params[i], param_name) == 0) {
                        found = true;
                        free(param_name);
                        break;
                    }
                }
                if (!found) {
                    char** new_required_params = realloc(required_params, (*count + 1) * sizeof(char*));
                    if (new_required_params) {
                        required_params = new_required_params;
                        required_params[*count] = param_name;
                        (*count)++;
                    } else {
                        free(param_name);
                        // Cleanup on failure
                        for (size_t i = 0; i < *count; i++) {
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

    return required_params;
}

// Helper function to collect provided parameter names from JSON
// Returns array of parameter names, caller must free each string and the array
char** collect_provided_parameters(json_t* params_json, size_t* count) {
    *count = 0;
    if (!params_json || !json_is_object(params_json)) {
        return NULL;
    }

    char** provided_params = NULL;
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
                for (size_t j = 0; j < *count; j++) {
                    if (strcmp(provided_params[j], key) == 0) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    char** new_provided_params = realloc(provided_params, (*count + 1) * sizeof(char*));
                    if (new_provided_params) {
                        provided_params = new_provided_params;
                        provided_params[*count] = strdup(key);
                        if (provided_params[*count]) {
                            (*count)++;
                        } else {
                            // Cleanup on failure
                            for (size_t k = 0; k < *count; k++) {
                                free(provided_params[k]);
                            }
                            free(provided_params);
                            return NULL;
                        }
                    } else {
                        // Cleanup on failure
                        for (size_t k = 0; k < *count; k++) {
                            free(provided_params[k]);
                        }
                        free(provided_params);
                        return NULL;
                    }
                }
            }
        }
    }

    return provided_params;
}

// Helper function to collect provided parameter names from ParameterList
// Returns array of parameter names, caller must free each string and the array
char** collect_provided_from_param_list(ParameterList* param_list, size_t* count) {
    *count = 0;
    if (!param_list) {
        return NULL;
    }

    char** provided_params = NULL;

    for (size_t i = 0; i < param_list->count; i++) {
        const TypedParameter* param = param_list->params[i];
        if (param->name) {
            // Check if already in list
            bool found = false;
            for (size_t j = 0; j < *count; j++) {
                if (strcmp(provided_params[j], param->name) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                char** new_provided_params = realloc(provided_params, (*count + 1) * sizeof(char*));
                if (new_provided_params) {
                    provided_params = new_provided_params;
                    provided_params[*count] = strdup(param->name);
                    if (provided_params[*count]) {
                        (*count)++;
                    } else {
                        // Cleanup on failure
                        for (size_t k = 0; k < *count; k++) {
                            free(provided_params[k]);
                        }
                        free(provided_params);
                        return NULL;
                    }
                } else {
                    // Cleanup on failure
                    for (size_t k = 0; k < *count; k++) {
                        free(provided_params[k]);
                    }
                    free(provided_params);
                    return NULL;
                }
            }
        }
    }

    return provided_params;
}

// Helper function to find missing parameters
// Returns array of missing parameter names, caller must free each string and the array
char** find_missing_parameters(char** required, size_t required_count, char** provided, size_t provided_count, size_t* missing_count) {
    *missing_count = 0;
    char** missing_params = NULL;

    for (size_t req_idx = 0; req_idx < required_count; req_idx++) {
        bool found = false;
        for (size_t prov_idx = 0; prov_idx < provided_count; prov_idx++) {
            if (strcmp(required[req_idx], provided[prov_idx]) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            char** new_missing = realloc(missing_params, (*missing_count + 1) * sizeof(char*));
            if (!new_missing) {
                // Cleanup
                for (size_t i = 0; i < *missing_count; i++) {
                    free(missing_params[i]);
                }
                free(missing_params);
                return NULL;
            }
            missing_params = new_missing;
            missing_params[*missing_count] = strdup(required[req_idx]);
            if (!missing_params[*missing_count]) {
                // Cleanup
                for (size_t i = 0; i < *missing_count; i++) {
                    free(missing_params[i]);
                }
                free(missing_params);
                return NULL;
            }
            (*missing_count)++;
        }
    }

    return missing_params;
}

// Helper function to find unused parameters
// Returns array of unused parameter names, caller must free each string and the array
char** find_unused_parameters(char** required, size_t required_count, char** provided, size_t provided_count, size_t* unused_count) {
    *unused_count = 0;
    char** unused_params = NULL;

    for (size_t prov_idx = 0; prov_idx < provided_count; prov_idx++) {
        bool found = false;
        for (size_t req_idx = 0; req_idx < required_count; req_idx++) {
            if (strcmp(provided[prov_idx], required[req_idx]) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            char** new_unused = realloc(unused_params, (*unused_count + 1) * sizeof(char*));
            if (!new_unused) {
                // Cleanup
                for (size_t i = 0; i < *unused_count; i++) {
                    free(unused_params[i]);
                }
                free(unused_params);
                return NULL;
            }
            unused_params = new_unused;
            unused_params[*unused_count] = strdup(provided[prov_idx]);
            if (!unused_params[*unused_count]) {
                // Cleanup
                for (size_t i = 0; i < *unused_count; i++) {
                    free(unused_params[i]);
                }
                free(unused_params);
                return NULL;
            }
            (*unused_count)++;
        }
    }

    return unused_params;
}

// Helper function to validate a single parameter's type
// Returns true if valid, false if invalid
bool validate_single_parameter_type(json_t* param_value, int type_index) {
    if (!param_value) {
        return false;
    }

    switch (type_index) {
        case 0: // INTEGER
            return json_is_integer(param_value);
        case 1: // STRING
            return json_is_string(param_value);
        case 2: // BOOLEAN
            return json_is_boolean(param_value);
        case 3: // FLOAT
            return json_is_real(param_value) || json_is_integer(param_value);
        case 4: // TEXT
        case 5: // DATE
        case 6: // TIME
        case 7: // DATETIME
        case 8: // TIMESTAMP
            return json_is_string(param_value);
        default:
            return false;
    }
}

// Helper function to format type error message
// Returns allocated string, caller must free
char* format_type_error_message(const char* param_name, const char* actual_type, const char* expected_type, const char* verb) {
    // Calculate required buffer size
    size_t len = strlen(param_name) * 2 + strlen(actual_type) + strlen(expected_type) + strlen(verb) + strlen("()  ()") + 2;
    char* msg = malloc(len);
    if (msg) {
        snprintf(msg, len, "%s(%s) %s %s(%s)", param_name, actual_type, verb, param_name, expected_type);
    }
    return msg;
}

// Helper function to validate parameter types and append errors to buffer
// Returns the number of characters written to the buffer
size_t validate_parameter_types_to_buffer(json_t* params_json, char* buffer, size_t buffer_size) {
    if (!params_json || !json_is_object(params_json) || !buffer || buffer_size == 0) {
        return 0;
    }

    const char* type_keys[] = {"INTEGER", "STRING", "BOOLEAN", "FLOAT", "TEXT", "DATE", "TIME", "DATETIME", "TIMESTAMP"};
    size_t num_types = sizeof(type_keys) / sizeof(type_keys[0]);
    size_t pos = 0;

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
                    if (pos + msg_len + (pos > 0 ? 2 : 0) < buffer_size) {
                        if (pos > 0) {
                            strcpy(buffer + pos, ", ");
                            pos += 2;
                        }
                        strcpy(buffer + pos, invalid_msg);
                        pos += msg_len;
                    } else if (buffer_size > pos + 1) {
                        // If buffer is too small, write as much as possible (at least something)
                        if (pos > 0 && buffer_size > pos + 2) {
                            strcpy(buffer + pos, ", ");
                            pos += 2;
                        }
                        size_t write_len = buffer_size - pos - 1; // Leave space for null terminator
                        strncpy(buffer + pos, invalid_msg, write_len);
                        pos += write_len;
                    }
                    free(invalid_msg);
                }
            }

            iter = json_object_iter_next(type_obj, iter);
        }
    }

    // Null-terminate the buffer
    if (pos < buffer_size) {
        buffer[pos] = '\0';
    }

    return pos;
}