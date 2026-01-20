
/*
 * Database Parameter Processing Implementation
 *
 * Handles parsing of typed JSON parameters and conversion to database-specific formats.
 */

// Project includes
#include <src/hydrogen.h>

// Local includes
#include "database_params.h"

// Maximum parameter name length
#define MAX_PARAM_NAME_LEN 64

// Parameter type string constants
static const char* PARAM_TYPE_STRINGS[] = {
    "INTEGER",
    "STRING",
    "BOOLEAN",
    "FLOAT",
    "TEXT",
    "DATE",
    "TIME",
    "DATETIME",
    "TIMESTAMP"
};

// Parse typed JSON parameters into parameter list
ParameterList* parse_typed_parameters(const char* json_params, const char* dqm_label) {
    if (!json_params) {
        log_this(dqm_label ? dqm_label : SR_DATABASE, "NULL JSON parameters provided", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    json_error_t error;
    json_t* root = json_loads(json_params, 0, &error);
    if (!root) {
        log_this(dqm_label ? dqm_label : SR_DATABASE, "Failed to parse JSON parameters", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    if (!json_is_object(root)) {
        log_this(dqm_label ? dqm_label : SR_DATABASE, "JSON parameters must be an object", LOG_LEVEL_ERROR, 0);
        json_decref(root);
        return NULL;
    }

    ParameterList* param_list = (ParameterList*)malloc(sizeof(ParameterList));
    if (!param_list) {
        log_this(dqm_label ? dqm_label : SR_DATABASE, "Failed to allocate parameter list", LOG_LEVEL_ERROR, 0);
        json_decref(root);
        return NULL;
    }

    param_list->params = NULL;
    param_list->count = 0;

    // Count total parameters across all types
    size_t total_params = 0;
    const char* type_keys[] = {"INTEGER", "STRING", "BOOLEAN", "FLOAT", "TEXT", "DATE", "TIME", "DATETIME", "TIMESTAMP"};
    size_t num_types = sizeof(type_keys) / sizeof(type_keys[0]);

    for (size_t i = 0; i < num_types; i++) {
        json_t* type_obj = json_object_get(root, type_keys[i]);
        if (type_obj && json_is_object(type_obj)) {
            total_params += json_object_size(type_obj);
        }
    }

    if (total_params == 0) {
        log_this(dqm_label ? dqm_label : SR_DATABASE, "No parameters found in JSON", LOG_LEVEL_DEBUG, 0);
        json_decref(root);
        return param_list; // Return empty list
    }

    // Allocate parameter array
    param_list->params = (TypedParameter**)malloc(total_params * sizeof(TypedParameter*));
    if (!param_list->params) {
        log_this(dqm_label ? dqm_label : SR_DATABASE, "Failed to allocate parameter array", LOG_LEVEL_ERROR, 0);
        free(param_list);
        json_decref(root);
        return NULL;
    }

    // Parse each parameter type
    size_t param_index = 0;
    for (size_t type_idx = 0; type_idx < num_types; type_idx++) {
        const char* type_key = type_keys[type_idx];
        json_t* type_obj = json_object_get(root, type_key);

        if (!type_obj || !json_is_object(type_obj)) {
            continue; // Skip missing or invalid type sections
        }

        ParameterType param_type = (ParameterType)type_idx;

        // Iterate through parameters of this type
        const char* param_name;
        json_t* param_value;
        json_object_foreach(type_obj, param_name, param_value) {
            TypedParameter* param = (TypedParameter*)malloc(sizeof(TypedParameter));
            if (!param) {
                log_this(dqm_label ? dqm_label : SR_DATABASE, "Failed to allocate typed parameter", LOG_LEVEL_ERROR, 0);
                free_parameter_list(param_list);
                json_decref(root);
                return NULL;
            }

            // Initialize parameter (zero out the union to prevent double-free on error)
            memset(param, 0, sizeof(TypedParameter));
            param->name = strdup(param_name);
            param->type = param_type;

            if (!param->name) {
                log_this(dqm_label ? dqm_label : SR_DATABASE, "Failed to duplicate parameter name", LOG_LEVEL_ERROR, 0);
                free(param);
                free_parameter_list(param_list);
                json_decref(root);
                return NULL;
            }

            // Parse value based on type
            bool parse_success = false;
            switch (param_type) {
                case PARAM_TYPE_INTEGER:
                    if (json_is_integer(param_value)) {
                        param->value.int_value = json_integer_value(param_value);
                        parse_success = true;
                    }
                    break;

                case PARAM_TYPE_STRING:
                    if (json_is_string(param_value)) {
                        param->value.string_value = strdup(json_string_value(param_value));
                        if (param->value.string_value) {
                            parse_success = true;
                        }
                    }
                    break;

                case PARAM_TYPE_BOOLEAN:
                    if (json_is_boolean(param_value)) {
                        param->value.bool_value = json_boolean_value(param_value);
                        parse_success = true;
                    }
                    break;

                case PARAM_TYPE_FLOAT:
                    if (json_is_real(param_value)) {
                        param->value.float_value = json_real_value(param_value);
                        parse_success = true;
                    } else if (json_is_integer(param_value)) {
                        // Allow integers for float parameters
                        param->value.float_value = (double)json_integer_value(param_value);
                        parse_success = true;
                    }
                    break;

                case PARAM_TYPE_TEXT:
                case PARAM_TYPE_DATE:
                case PARAM_TYPE_TIME:
                case PARAM_TYPE_DATETIME:
                case PARAM_TYPE_TIMESTAMP:
                    if (json_is_string(param_value)) {
                        param->value.text_value = strdup(json_string_value(param_value));
                        if (param->value.text_value) {
                            parse_success = true;
                        }
                    }
                    break;
            }

            if (!parse_success) {
                log_this(dqm_label ? dqm_label : SR_DATABASE, "Invalid parameter value type", LOG_LEVEL_ERROR, 0);
                free_typed_parameter(param);
                free_parameter_list(param_list);
                json_decref(root);
                return NULL;
            }

            param_list->params[param_index++] = param;
        }
    }

    param_list->count = param_index;
    json_decref(root);

    log_this(dqm_label ? dqm_label : SR_DATABASE, "Successfully parsed typed parameters", LOG_LEVEL_DEBUG, 0);
    return param_list;
}

// Convert SQL template from named to positional parameters
char* convert_named_to_positional(
    const char* sql_template,
    ParameterList* params,
    DatabaseEngineType engine_type,
    TypedParameter*** ordered_params,
    size_t* param_count,
    const char* dqm_label
) {
    if (!sql_template || !params) {
        return NULL;
    }

    // First, build the ordered parameter array
    if (!build_parameter_array(sql_template, params, ordered_params, param_count, dqm_label)) {
        return NULL;
    }

    // Create a copy of the SQL template to modify
    char* modified_sql = strdup(sql_template);
    if (!modified_sql) {
        log_this(dqm_label ? dqm_label : SR_DATABASE, "Failed to duplicate SQL template", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // Replace named parameters with positional placeholders
    char* result = modified_sql;
    size_t param_index = 1; // 1-based for PostgreSQL, 0-based for others

    for (size_t i = 0; i < *param_count; i++) {
        const TypedParameter* param = (*ordered_params)[i];
        char param_placeholder[32];

        // Generate appropriate placeholder based on engine type
        switch (engine_type) {
            case DB_ENGINE_POSTGRESQL:
                snprintf(param_placeholder, sizeof(param_placeholder), "$%zu", param_index++);
                break;

            case DB_ENGINE_SQLITE:
            case DB_ENGINE_MYSQL:
            case DB_ENGINE_DB2:
            case DB_ENGINE_AI:
            case DB_ENGINE_MAX:
            default:
                snprintf(param_placeholder, sizeof(param_placeholder), "?");
                break;
        }

        // Replace :paramName with placeholder
        char named_param[256];
        snprintf(named_param, sizeof(named_param), ":%s", param->name);

        // Simple string replacement (could be optimized with more sophisticated replacement)
        const char* pos = strstr(result, named_param);
        if (pos) {
            size_t prefix_len = (size_t)(pos - result);
            size_t suffix_len = (size_t)strlen(pos + strlen(named_param));
            size_t placeholder_len = (size_t)strlen(param_placeholder);

            char* new_sql = (char*)malloc((size_t)strlen(result) + placeholder_len - (size_t)strlen(named_param) + 1);
            if (!new_sql) {
                log_this(dqm_label ? dqm_label : SR_DATABASE, "Failed to allocate modified SQL", LOG_LEVEL_ERROR, 0);
                free(modified_sql);
                return NULL;
            }

            memcpy(new_sql, result, prefix_len);
            memcpy(new_sql + prefix_len, param_placeholder, placeholder_len);
            memcpy(new_sql + prefix_len + placeholder_len, pos + strlen(named_param), suffix_len + 1);

            free(result);
            result = new_sql;
        }
    }

    return result;
}

// Build parameter array in correct order for database execution
bool build_parameter_array(
    const char* sql_template,
    ParameterList* params,
    TypedParameter*** ordered_params,
    size_t* param_count,
    const char* dqm_label
) {
    if (!sql_template || !params || !ordered_params || !param_count) {
        return false;
    }

    // Find all named parameters in SQL template
    // Use regex to find :paramName patterns, but exclude ${...} macro variables
    regex_t regex;
    int reti = regcomp(&regex, ":[a-zA-Z_][a-zA-Z0-9_]*", REG_EXTENDED);
    if (reti) {
        log_this(dqm_label ? dqm_label : SR_DATABASE, "Failed to compile regex for parameter matching", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Count matches
    size_t match_count = 0;
    regmatch_t match;
    const char* search_ptr = sql_template;

    while (regexec(&regex, search_ptr, 1, &match, 0) == 0) {
        // Check if this match is inside a ${} macro
        bool is_inside_macro = false;
        const char* current_ptr = sql_template;
        const char* match_start = search_ptr + match.rm_so;
        
        // Find if there's an unclosed ${ before the match
        int macro_depth = 0;
        while (current_ptr < match_start) {
            if (strncmp(current_ptr, "${", 2) == 0) {
                macro_depth++;
                current_ptr += 2;
            } else if (strncmp(current_ptr, "}", 1) == 0) {
                if (macro_depth > 0) {
                    macro_depth--;
                }
                current_ptr++;
            } else {
                current_ptr++;
            }
        }
        
        // If macro_depth > 0, the match is inside a ${}
        if (macro_depth > 0) {
            is_inside_macro = true;
        }
        
        if (!is_inside_macro) {
            match_count++;
        }
        
        search_ptr += match.rm_eo;
    }

    if (match_count == 0) {
        *ordered_params = NULL;
        *param_count = 0;
        regfree(&regex);
        return true; // No parameters to process
    }

    // Allocate ordered parameter array
    *ordered_params = (TypedParameter**)malloc(match_count * sizeof(TypedParameter*));
    if (!*ordered_params) {
        log_this(dqm_label ? dqm_label : SR_DATABASE, "Failed to allocate ordered parameter array", LOG_LEVEL_ERROR, 0);
        regfree(&regex);
        return false;
    }

    // Build ordered array by finding parameters in SQL order
    search_ptr = sql_template;
    size_t param_idx = 0;

    while (regexec(&regex, search_ptr, 1, &match, 0) == 0 && param_idx < match_count) {
        // Check if this match is inside a ${} macro
        bool is_inside_macro = false;
        const char* current_ptr = sql_template;
        const char* match_start = search_ptr + match.rm_so;
        
        // Find if there's an unclosed ${ before the match
        int macro_depth = 0;
        while (current_ptr < match_start) {
            if (strncmp(current_ptr, "${", 2) == 0) {
                macro_depth++;
                current_ptr += 2;
            } else if (strncmp(current_ptr, "}", 1) == 0) {
                if (macro_depth > 0) {
                    macro_depth--;
                }
                current_ptr++;
            } else {
                current_ptr++;
            }
        }
        
        // If macro_depth > 0, the match is inside a ${}
        if (macro_depth > 0) {
            is_inside_macro = true;
        }
        
        if (!is_inside_macro) {
            // Extract parameter name (skip the ':')
            size_t name_len = (size_t)(match.rm_eo - match.rm_so - 1);
            char param_name[MAX_PARAM_NAME_LEN];
            if (name_len >= sizeof(param_name)) {
                log_this(dqm_label ? dqm_label : SR_DATABASE, "Parameter name too long", LOG_LEVEL_ERROR, 0);
                free(*ordered_params);
                regfree(&regex);
                return false;
            }

            memcpy(param_name, search_ptr + match.rm_so + 1, name_len);
            param_name[name_len] = '\0';

            // Find matching parameter in our list
            TypedParameter* found_param = NULL;
            for (size_t i = 0; i < params->count; i++) {
                if (strcmp(params->params[i]->name, param_name) == 0) {
                    found_param = params->params[i];
                    break;
                }
            }

            if (!found_param) {
                log_this(dqm_label ? dqm_label : SR_DATABASE, "Parameter not found in parameter list: %s", LOG_LEVEL_ERROR, 1, param_name);
                free(*ordered_params);
                *ordered_params = NULL;
                regfree(&regex);
                return false;
            }

            (*ordered_params)[param_idx++] = found_param;
        }
        
        search_ptr += match.rm_eo;
    }

    *param_count = param_idx;
    regfree(&regex);

    return true;
}

// Cleanup functions
void free_typed_parameter(TypedParameter* param) {
    if (!param) return;

    free(param->name);
    if (param->type == PARAM_TYPE_STRING ||
        param->type == PARAM_TYPE_TEXT ||
        param->type == PARAM_TYPE_DATE ||
        param->type == PARAM_TYPE_TIME ||
        param->type == PARAM_TYPE_DATETIME ||
        param->type == PARAM_TYPE_TIMESTAMP) {
        free(param->value.string_value);  // All use same union position
    }
    free(param);
}

void free_parameter_list(ParameterList* params) {
    if (!params) return;

    if (params->params) {
        for (size_t i = 0; i < params->count; i++) {
            free_typed_parameter(params->params[i]);
        }
        free(params->params);
    }
    free(params);
}

// Utility functions
const char* parameter_type_to_string(ParameterType type) {
    if (type >= 0 && type < sizeof(PARAM_TYPE_STRINGS) / sizeof(PARAM_TYPE_STRINGS[0])) {
        return PARAM_TYPE_STRINGS[type];
    }
    return "UNKNOWN";
}

ParameterType string_to_parameter_type(const char* type_str) {
    if (!type_str) return PARAM_TYPE_INTEGER; // Default

    size_t num_types = sizeof(PARAM_TYPE_STRINGS) / sizeof(PARAM_TYPE_STRINGS[0]);
    for (size_t i = 0; i < num_types; i++) {
        if (strcmp(type_str, PARAM_TYPE_STRINGS[i]) == 0) {
            return (ParameterType)i;
        }
    }

    return PARAM_TYPE_INTEGER; // Default fallback
}