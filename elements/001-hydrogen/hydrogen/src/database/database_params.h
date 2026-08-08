/*
 * Database Parameter Processing Header
 *
 * Typed JSON parameter parse + named-to-positional SQL conversion for all
 * engines. JSON groups values by type key (INTEGER, STRING, …); SQL uses
 * :name markers. See docs/H/database/PARAMETER_TYPES.md.
 */

#ifndef DATABASE_PARAMS_H
#define DATABASE_PARAMS_H

// Project includes
#include <src/hydrogen.h>

// Database engine types
#include "database_types.h"

// Use the correct enum name from database_types.h
typedef DatabaseEngine DatabaseEngineType;

// Parameter type enumeration (JSON type-group keys match parameter_type_to_string)
typedef enum {
    PARAM_TYPE_INTEGER,
    PARAM_TYPE_STRING,
    PARAM_TYPE_BOOLEAN,
    PARAM_TYPE_FLOAT,
    PARAM_TYPE_TEXT,       // Large text / CLOB-style values
    PARAM_TYPE_DATE,       // YYYY-MM-DD
    PARAM_TYPE_TIME,       // HH:MM:SS
    PARAM_TYPE_DATETIME,   // YYYY-MM-DD HH:MM:SS
    PARAM_TYPE_TIMESTAMP   // YYYY-MM-DD HH:MM:SS.fff
} ParameterType;

// Typed parameter structure
typedef struct TypedParameter {
    char* name;              // Parameter name (e.g., "userId")
    ParameterType type;      // Data type
    bool is_null;            // True when JSON value was null (SQL NULL bind)
    union {
        long long int_value;
        char* string_value;
        bool bool_value;
        double float_value;
        char* text_value;       // TEXT
        char* date_value;       // DATE YYYY-MM-DD
        char* time_value;       // TIME HH:MM:SS
        char* datetime_value;   // DATETIME YYYY-MM-DD HH:MM:SS
        char* timestamp_value;  // TIMESTAMP YYYY-MM-DD HH:MM:SS.fff
    } value;
} TypedParameter;

// Parameter list structure
typedef struct ParameterList {
    TypedParameter** params;
    size_t count;
} ParameterList;

// Function prototypes

// Parse typed JSON parameters into parameter list (caller frees with free_parameter_list)
ParameterList* parse_typed_parameters(const char* json_params, const char* dqm_label);

// Replace :name with engine placeholders ($N or ?); fill ordered_params (caller frees array only)
char* convert_named_to_positional(
    const char* sql_template,
    ParameterList* params,
    DatabaseEngineType engine_type,
    TypedParameter*** ordered_params,
    size_t* param_count,
    const char* dqm_label
);

// Scan SQL for :name occurrences (skips string literals); build ordered bind list
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

/* ----------------------------------------------------------------------------
 * The following helper is NOT part of the stable public API. It is exposed
 * (non-static) solely so the Unity test framework can call it directly.
 * -------------------------------------------------------------------------- */
bool database_params_is_inside_string_literal(const char* sql, const char* position);

#endif // DATABASE_PARAMS_H