/*
 * Parameter Processing Helper Functions Header
 * Declarations for helper functions used in parameter processing.
 */

#ifndef PARAM_PROC_HELPERS_H
#define PARAM_PROC_HELPERS_H

#include <jansson.h>

// Forward declarations
typedef struct ParameterList ParameterList;

// Helper function prototypes
char** extract_required_parameters(const char* sql_template, size_t* count);
char** collect_provided_parameters(json_t* params_json, size_t* count);
char** collect_provided_from_param_list(ParameterList* param_list, size_t* count);
char** find_missing_parameters(char** required, size_t required_count, char** provided, size_t provided_count, size_t* missing_count);
char** find_unused_parameters(char** required, size_t required_count, char** provided, size_t provided_count, size_t* unused_count);
bool validate_single_parameter_type(json_t* param_value, int type_index);
char* format_type_error_message(const char* param_name, const char* actual_type, const char* expected_type, const char* verb);
size_t validate_parameter_types_to_buffer(json_t* params_json, char* buffer, size_t buffer_size);

#endif // PARAM_PROC_HELPERS_H