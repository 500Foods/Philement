/*
 * Database Parameter Processing Header
 *
 * Handles parsing of typed JSON parameters and conversion to database-specific formats.
 */

#ifndef DATABASE_PARAMS_H
#define DATABASE_PARAMS_H

// Project includes
#include <src/hydrogen.h>

// Database engine types
#include "database_types.h"

// Use the correct enum name from database_types.h
typedef DatabaseEngine DatabaseEngineType;

// Parameter type enumeration
typedef enum {
    PARAM_TYPE_INTEGER,
    PARAM_TYPE_STRING,
    PARAM_TYPE_BOOLEAN,
    PARAM_TYPE_FLOAT,
    PARAM_TYPE_TEXT,       // New - Large text fields (CLOBs, TEXT columns)
    PARAM_TYPE_DATE,       // New - Date values (YYYY-MM-DD)
    PARAM_TYPE_TIME,       // New - Time values (HH:MM:SS)
    PARAM_TYPE_DATETIME,   // New - Combined date and time (YYYY-MM-DD HH:MM:SS)
    PARAM_TYPE_TIMESTAMP   // New - Date, time with milliseconds (YYYY-MM-DD HH:MM:SS.fff)
} ParameterType;

// Typed parameter structure
typedef struct TypedParameter {
    char* name;              // Parameter name (e.g., "userId")
    ParameterType type;      // Data type
    union {
        long long int_value;
        char* string_value;
        bool bool_value;
        double float_value;
        char* text_value;       // For TEXT (large text fields)
        char* date_value;       // For DATE (format: YYYY-MM-DD)
        char* time_value;       // For TIME (format: HH:MM:SS)
        char* datetime_value;   // For DATETIME (format: YYYY-MM-DD HH:MM:SS)
        char* timestamp_value;  // For TIMESTAMP (format: YYYY-MM-DD HH:MM:SS.fff)
    } value;
} TypedParameter;

// Parameter list structure
typedef struct ParameterList {
    TypedParameter** params;
    size_t count;
} ParameterList;

// Function prototypes

// Parse typed JSON parameters into parameter list
ParameterList* parse_typed_parameters(const char* json_params, const char* dqm_label);

// Convert SQL template from named to positional parameters
// Returns modified SQL and ordered parameter array
char* convert_named_to_positional(
    const char* sql_template,
    ParameterList* params,
    DatabaseEngineType engine_type,
    TypedParameter*** ordered_params,
    size_t* param_count,
    const char* dqm_label
);

// Build parameter array in correct order for database execution
bool build_parameter_array(
    const char* sql_template,
    ParameterList* params,
    TypedParameter*** ordered_params,
    size_t* param_count,
    const char* dqm_label
);

// Cleanup functions
void free_typed_parameter(TypedParameter* param);
void free_parameter_list(ParameterList* params);

// Utility functions
const char* parameter_type_to_string(ParameterType type);
ParameterType string_to_parameter_type(const char* type_str);

#endif // DATABASE_PARAMS_H