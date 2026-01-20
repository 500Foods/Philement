/*
 * PostgreSQL Database Engine - Query Execution Implementation
 *
 * Implements PostgreSQL query execution functions.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/database/database_serialize.h>
#include <src/database/database_params.h>
#include <src/database/dbqueue/dbqueue.h>

// Local includes
#include "types.h"
#include "connection.h"
#include "query.h"

// External declarations for libpq function pointers (defined in connection.c)
typedef int (*PQftype_t)(void* res, int column_number);

extern PQexec_t PQexec_ptr;
extern PQexecParams_t PQexecParams_ptr;
extern PQresultStatus_t PQresultStatus_ptr;
extern PQclear_t PQclear_ptr;
extern PQntuples_t PQntuples_ptr;
extern PQnfields_t PQnfields_ptr;
extern PQfname_t PQfname_ptr;
extern PQftype_t PQftype_ptr;
extern PQgetvalue_t PQgetvalue_ptr;
extern PQcmdTuples_t PQcmdTuples_ptr;
extern PQerrorMessage_t PQerrorMessage_ptr;
extern PQexecPrepared_t PQexecPrepared_ptr;

// PostgreSQL type OIDs for common numeric types
#define POSTGRES_INT2OID 21    // smallint
#define POSTGRES_INT4OID 23    // integer
#define POSTGRES_INT8OID 20    // bigint
#define POSTGRES_FLOAT4OID 700  // real
#define POSTGRES_FLOAT8OID 701  // double precision
#define POSTGRES_NUMERICOID 1700 // numeric/decimal

// Helper function to check if PostgreSQL type is numeric
static bool postgresql_is_numeric_type(int oid) {
    switch (oid) {
        case POSTGRES_INT2OID:
        case POSTGRES_INT4OID:
        case POSTGRES_INT8OID:
        case POSTGRES_FLOAT4OID:
        case POSTGRES_FLOAT8OID:
        case POSTGRES_NUMERICOID:
            return true;
        default:
            return false;
    }
}

// External declarations for constants (defined in connection.c)
extern bool check_timeout_expired(time_t start_time, int timeout_seconds);

// Helper function to convert TypedParameter to PostgreSQL string format
// PostgreSQL PQexecParams accepts all parameters as text strings
// Returns allocated string that caller must free
char* postgresql_convert_param_value(const TypedParameter* param, const char* designator) {
    if (!param) {
        log_this(designator, "postgresql_convert_param_value: NULL parameter", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    char buffer[256];
    
    switch (param->type) {
        case PARAM_TYPE_INTEGER:
            snprintf(buffer, sizeof(buffer), "%lld", param->value.int_value);
            return strdup(buffer);
            
        case PARAM_TYPE_STRING:
            return param->value.string_value ? strdup(param->value.string_value) : strdup("");
            
        case PARAM_TYPE_BOOLEAN:
            return strdup(param->value.bool_value ? "true" : "false");
            
        case PARAM_TYPE_FLOAT:
            snprintf(buffer, sizeof(buffer), "%.15g", param->value.float_value);
            return strdup(buffer);
            
        case PARAM_TYPE_TEXT:
            return param->value.text_value ? strdup(param->value.text_value) : strdup("");
            
        case PARAM_TYPE_DATE:
            return param->value.date_value ? strdup(param->value.date_value) : strdup("");
            
        case PARAM_TYPE_TIME:
            return param->value.time_value ? strdup(param->value.time_value) : strdup("");
            
        case PARAM_TYPE_DATETIME:
            return param->value.datetime_value ? strdup(param->value.datetime_value) : strdup("");
            
        case PARAM_TYPE_TIMESTAMP:
            return param->value.timestamp_value ? strdup(param->value.timestamp_value) : strdup("");
            
        default:
            log_this(designator, "postgresql_convert_param_value: Unknown parameter type %d", LOG_LEVEL_ERROR, 1, param->type);
            return NULL;
    }
}

// Query Execution Functions
bool postgresql_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result) {
    if (!connection || !request || !result || connection->engine_type != DB_ENGINE_POSTGRESQL) {
        const char* designator = connection ? (connection->designator ? connection->designator : SR_DATABASE) : SR_DATABASE;
        log_this(designator, "PostgreSQL execute_query: Invalid parameters", LOG_LEVEL_ERROR, 0);
        return false;
    }

    const char* designator = connection->designator ? connection->designator : SR_DATABASE;
    log_this(designator, "postgresql_execute_query: ENTER - connection=%p, request=%p, result=%p", LOG_LEVEL_TRACE, 3, (void*)connection, (void*)request, (void*)result);

    log_this(designator, "postgresql_execute_query: Parameters validated, proceeding", LOG_LEVEL_TRACE, 0);

    const PostgresConnection* pg_conn = (const PostgresConnection*)connection->connection_handle;
    if (!pg_conn || !pg_conn->connection) {
        log_this(designator, "PostgreSQL execute_query: Invalid connection handle", LOG_LEVEL_ERROR, 0);
        return false;
    }

    log_this(designator, "PostgreSQL execute_query: Executing query: %s", LOG_LEVEL_TRACE, 1, request->sql_template);
    log_this(designator, "PostgreSQL execute_query: Query timeout: %d seconds", LOG_LEVEL_TRACE, 1, request->timeout_seconds);

    // Set PostgreSQL statement timeout before executing query
    int query_timeout = request->timeout_seconds > 0 ? request->timeout_seconds : 30;
    char timeout_sql[256];
    snprintf(timeout_sql, sizeof(timeout_sql), "SET statement_timeout = %d", query_timeout * 1000); // Convert to milliseconds

    log_this(designator, "PostgreSQL execute_query: Setting statement timeout to %d seconds", LOG_LEVEL_TRACE, 1, query_timeout);

    // Set the timeout
    void* timeout_result = PQexec_ptr(pg_conn->connection, timeout_sql);
    if (timeout_result) {
        // int timeout_status = PQresultStatus_ptr(timeout_result);
        // log_this(designator, "PostgreSQL execute_query: Timeout setting result status: %d", LOG_LEVEL_TRACE, 1, timeout_status);
        PQclear_ptr(timeout_result);
    } else {
        log_this(designator, "PostgreSQL execute_query: Failed to set statement timeout", LOG_LEVEL_ERROR, 0);
    }

    // Parse and process parameters if provided
    ParameterList* params = NULL;
    char* positional_sql = NULL;
    TypedParameter** ordered_params = NULL;
    size_t ordered_param_count = 0;
    char** param_values = NULL;
    
    if (request->parameters_json && strlen(request->parameters_json) > 0) {
        // Parse typed parameters from JSON
        params = parse_typed_parameters(request->parameters_json, designator);
        if (params) {
            // Convert named parameters to PostgreSQL positional format ($1, $2, ...)
            positional_sql = convert_named_to_positional(
                request->sql_template,
                params,
                DB_ENGINE_POSTGRESQL,
                &ordered_params,
                &ordered_param_count,
                designator
            );
            
            if (positional_sql && ordered_params && ordered_param_count > 0) {
                // Build parameter value array for PQexecParams
                param_values = calloc(ordered_param_count, sizeof(char*));
                if (param_values) {
                    for (size_t i = 0; i < ordered_param_count; i++) {
                        param_values[i] = postgresql_convert_param_value(ordered_params[i], designator);
                    }
                }
            }
        }
    }
    
    // Execute the query (now with PostgreSQL-level timeout protection)
    time_t query_start_time = time(NULL);
    void* pg_result = NULL;
    
    if (positional_sql && param_values && ordered_param_count > 0 && PQexecParams_ptr) {
        // Use parameterized execution
        log_this(designator, "PostgreSQL execute_query: Executing with %zu parameters", LOG_LEVEL_TRACE, 1, ordered_param_count);
        pg_result = PQexecParams_ptr(pg_conn->connection, positional_sql, (int)ordered_param_count,
                                      NULL, (const char* const*)param_values, NULL, NULL, 0);
    } else {
        // Fall back to direct execution
        log_this(designator, "PostgreSQL execute_query: Executing without parameters", LOG_LEVEL_TRACE, 0);
        pg_result = PQexec_ptr(pg_conn->connection, request->sql_template);
    }
    
    // Cleanup parameter resources
    if (param_values) {
        for (size_t i = 0; i < ordered_param_count; i++) {
            free(param_values[i]);
        }
        free(param_values);
    }
    if (ordered_params) {
        free(ordered_params);
    }
    if (positional_sql) {
        free(positional_sql);
    }
    if (params) {
        free_parameter_list(params);
    }

    // Check if query took too long (timing now handled at engine abstraction layer, but we still need timeout checking)
    if (check_timeout_expired(query_start_time, query_timeout)) {
        log_this(designator, "PostgreSQL execute_query: Query execution time exceeded %d seconds", LOG_LEVEL_ERROR, 1, query_timeout);
        if (pg_result) {
            log_this(designator, "PostgreSQL execute_query: Cleaning up failed query result", LOG_LEVEL_TRACE, 0);
            PQclear_ptr(pg_result);
        }
        return false;
    }

    if (!pg_result) {
        log_this(designator, "PostgreSQL execute_query: PQexec returned NULL", LOG_LEVEL_ERROR, 0);
        return false;
    }

    int result_status = PQresultStatus_ptr(pg_result);
    // log_this(designator, "PostgreSQL execute_query: Result status: %d", LOG_LEVEL_DEBUG, 1, result_status);

    if (result_status != PGRES_TUPLES_OK && result_status != PGRES_COMMAND_OK) {
        log_this(designator, "PostgreSQL query execution failed - status: %d", LOG_LEVEL_TRACE, 1, result_status);
        char* error_msg = PQerrorMessage_ptr(pg_conn->connection);
        char* error_message = NULL;
        if (error_msg && strlen(error_msg) > 0) {
            error_message = strdup(error_msg);
            log_this(designator, "PostgreSQL query error: %s", LOG_LEVEL_TRACE, 1, error_msg);
        } else {
            error_message = strdup("PostgreSQL query execution failed (no error details)");
        }

        // Create error result
        QueryResult* error_result = calloc(1, sizeof(QueryResult));
        if (error_result) {
            error_result->success = false;
            error_result->error_message = error_message;
            error_result->row_count = 0;
            error_result->column_count = 0;
            error_result->data_json = strdup("[]");
            error_result->execution_time_ms = 0;
            error_result->affected_rows = 0;
            *result = error_result;
        } else {
            free(error_message);
        }

        PQclear_ptr(pg_result);
        return (error_result != NULL);
    }

    // Create result structure
    QueryResult* db_result = calloc(1, sizeof(QueryResult));
    if (!db_result) {
        PQclear_ptr(pg_result);
        return false;
    }

    db_result->success = true;
    db_result->row_count = (size_t)PQntuples_ptr(pg_result);
    db_result->column_count = (size_t)PQnfields_ptr(pg_result);
    // db_result->execution_time_ms is now set by the engine abstraction layer
    db_result->affected_rows = atoi(PQcmdTuples_ptr(pg_result));

    // Extract column names
    if (db_result->column_count > 0) {
        db_result->column_names = calloc(db_result->column_count, sizeof(char*));
        if (!db_result->column_names) {
            PQclear_ptr(pg_result);
            free(db_result);
            return false;
        }

        for (size_t i = 0; i < db_result->column_count; i++) {
            db_result->column_names[i] = strdup(PQfname_ptr(pg_result, (int)i));
            if (!db_result->column_names[i]) {
                // Cleanup on failure
                for (size_t j = 0; j < i; j++) {
                    free(db_result->column_names[j]);
                }
                free(db_result->column_names);
                PQclear_ptr(pg_result);
                free(db_result);
                return false;
            }
        }
    }

    // Convert result to JSON
    if (db_result->row_count > 0 && db_result->column_count > 0) {
        // Simple JSON array of objects
        // Increase buffer size to handle large query templates (migration SQL can be 10KB+)
        size_t json_size = 16384 * db_result->row_count; // Larger estimate for query templates
        db_result->data_json = calloc(1, json_size);
        if (!db_result->data_json) {
            // Cleanup
            for (size_t i = 0; i < db_result->column_count; i++) {
                free(db_result->column_names[i]);
            }
            free(db_result->column_names);
            PQclear_ptr(pg_result);
            free(db_result);
            return false;
        }

        strcpy(db_result->data_json, "[");
        for (size_t row = 0; row < db_result->row_count; row++) {
            if (row > 0) strcat(db_result->data_json, ",");
            
            // Track start of this row for debug logging
            size_t row_start_pos = strlen(db_result->data_json);
            
            strcat(db_result->data_json, "{");

            for (size_t col = 0; col < db_result->column_count; col++) {
                if (col > 0) strcat(db_result->data_json, ",");
                
                // Get column type to determine if we should quote the value
                int col_type_oid = PQftype_ptr ? PQftype_ptr(pg_result, (int)col) : 0;
                bool is_numeric = postgresql_is_numeric_type(col_type_oid);
                
                const char* value = PQgetvalue_ptr(pg_result, (int)row, (int)col);
                const char* col_name = db_result->column_names[col];
                
                // Calculate needed size for this column (name + value + escaping overhead)
                size_t value_len = value ? strlen(value) : 0;
                size_t needed_size = strlen(col_name) + (value_len * 2) + 20; // Extra for escaping
                
                // Ensure buffer has enough space
                size_t current_len = strlen(db_result->data_json);
                if (current_len + needed_size > json_size) {
                    json_size = (current_len + needed_size) * 2;
                    char* new_json = realloc(db_result->data_json, json_size);
                    if (!new_json) continue; // Skip column on allocation failure
                    db_result->data_json = new_json;
                }
                
                // Append column to JSON
                char* append_pos = db_result->data_json + current_len;
                
                if (is_numeric && value && strlen(value) > 0) {
                    // Numeric type - no quotes around value
                    sprintf(append_pos, "\"%s\":%s", col_name, value);
                } else if (value) {
                    // String type - escape and quote
                    sprintf(append_pos, "\"%s\":\"", col_name);
                    char* dst = append_pos + strlen(append_pos);
                    const char* src = value;
                    while (*src) {
                        if (*src == '"' || *src == '\\') {
                            *dst++ = '\\';
                            *dst++ = *src++;
                        } else if (*src == '\n') {
                            *dst++ = '\\';
                            *dst++ = 'n';
                            src++;
                        } else if (*src == '\r') {
                            *dst++ = '\\';
                            *dst++ = 'r';
                            src++;
                        } else if (*src == '\t') {
                            *dst++ = '\\';
                            *dst++ = 't';
                            src++;
                        } else {
                            *dst++ = *src++;
                        }
                    }
                    *dst++ = '"';
                    *dst = '\0';
                } else {
                    // NULL value
                    sprintf(append_pos, "\"%s\":null", col_name);
                }

                // Log the actual data values for debugging
                // log_this(designator, "PostgreSQL execute_query: Row %zu, Column %zu (%s): %s", LOG_LEVEL_TRACE, 4,
                //     row,
                //     col,
                //     db_result->column_names[col],
                //     value ? value : "NULL");
            }
            strcat(db_result->data_json, "}");
            
            // Debug: Log first row JSON for diagnosis
            if (row == 0) {
                size_t row_len = strlen(db_result->data_json) - row_start_pos;
                char* row_json = strndup(db_result->data_json + row_start_pos, row_len);
                if (row_json) {
                    log_this(designator, "PostgreSQL first row JSON: %s", LOG_LEVEL_DEBUG, 1, row_json);
                    free(row_json);
                }
            }
        }
        strcat(db_result->data_json, "]");

        // log_this(designator, "PostgreSQL execute_query: Complete result JSON: %s", LOG_LEVEL_DEBUG, 1, db_result->data_json);
    } else {
        log_this(designator, "PostgreSQL execute_query: Query returned no data (0 rows or 0 columns)", LOG_LEVEL_DEBUG, 0);
    }

    PQclear_ptr(pg_result);
    *result = db_result;

    log_this(SR_DATABASE, "PostgreSQL query executed successfully", LOG_LEVEL_DEBUG, 0);
    return true;
}

bool postgresql_execute_prepared(DatabaseHandle* connection, const PreparedStatement* stmt, QueryRequest* request, QueryResult** result) {
    if (!connection || !stmt || !request || !result || connection->engine_type != DB_ENGINE_POSTGRESQL) {
        const char* designator = connection ? (connection->designator ? connection->designator : SR_DATABASE) : SR_DATABASE;
        log_this(designator, "PostgreSQL execute_prepared: Invalid parameters", LOG_LEVEL_ERROR, 0);
        return false;
    }

    const char* designator = connection->designator ? connection->designator : SR_DATABASE;
    log_this(designator, "postgresql_execute_prepared: ENTER - connection=%p, stmt=%p, request=%p, result=%p", LOG_LEVEL_TRACE, 4, (void*)connection, (void*)stmt, (void*)request, (void*)result);

    const PostgresConnection* pg_conn = (const PostgresConnection*)connection->connection_handle;
    if (!pg_conn || !pg_conn->connection) {
        log_this(designator, "PostgreSQL execute_prepared: Invalid connection handle", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Check if statement name is valid
    if (!stmt->name || strlen(stmt->name) == 0) {
        // Statement had no executable SQL (e.g., only comments after macro processing)
        // Return successful empty result instead of error
        log_this(designator, "PostgreSQL prepared statement: No executable SQL (statement was not actionable)", LOG_LEVEL_DEBUG, 0);
        
        QueryResult* db_result = calloc(1, sizeof(QueryResult));
        if (!db_result) {
            return false;
        }
        
        db_result->success = true;
        db_result->row_count = 0;
        db_result->column_count = 0;
        db_result->affected_rows = 0;
        db_result->execution_time_ms = 0;
        db_result->data_json = strdup("[]");
        
        *result = db_result;
        return true;
    }

    log_this(designator, "PostgreSQL execute_prepared: Executing prepared statement: %s", LOG_LEVEL_TRACE, 1, stmt->name);

    // Set PostgreSQL statement timeout before executing prepared statement
    int query_timeout = request->timeout_seconds > 0 ? request->timeout_seconds : 30;
    char timeout_sql[256];
    snprintf(timeout_sql, sizeof(timeout_sql), "SET statement_timeout = %d", query_timeout * 1000); // Convert to milliseconds

    log_this(designator, "PostgreSQL execute_prepared: Setting statement timeout to %d seconds", LOG_LEVEL_TRACE, 1, query_timeout);

    // Set the timeout
    void* timeout_result = PQexec_ptr(pg_conn->connection, timeout_sql);
    if (timeout_result) {
        PQclear_ptr(timeout_result);
    } else {
        log_this(designator, "PostgreSQL execute_prepared: Failed to set statement timeout", LOG_LEVEL_ERROR, 0);
    }

    // Parse and process parameters if provided
    ParameterList* params = NULL;
    size_t ordered_param_count = 0;
    char** param_values = NULL;
    
    if (request->parameters_json && strlen(request->parameters_json) > 0) {
        // Parse typed parameters from JSON
        params = parse_typed_parameters(request->parameters_json, designator);
        if (params && params->count > 0) {
            // For prepared statements, parameters are already in positional order
            // Just convert the parameter values to string format for PQexecPrepared
            ordered_param_count = params->count;
            param_values = calloc(ordered_param_count, sizeof(char*));
            if (param_values) {
                for (size_t i = 0; i < ordered_param_count; i++) {
                    param_values[i] = postgresql_convert_param_value(params->params[i], designator);
                }
            }
        }
    }

    // Execute the prepared statement using PQexecPrepared API
    time_t query_start_time = time(NULL);
    void* pg_result = NULL;

    if (PQexecPrepared_ptr) {
        // Use true prepared statement execution with PQexecPrepared
        if (param_values && ordered_param_count > 0) {
            log_this(designator, "PostgreSQL execute_prepared: Using PQexecPrepared with %zu parameters", LOG_LEVEL_TRACE, 1, ordered_param_count);
            pg_result = PQexecPrepared_ptr(pg_conn->connection, stmt->name, (int)ordered_param_count,
                                          (const char* const*)param_values, NULL, NULL, 0);
        } else {
            // For queries with no parameters, use nParams=0 and NULL parameter arrays
            log_this(designator, "PostgreSQL execute_prepared: Using PQexecPrepared without parameters", LOG_LEVEL_TRACE, 0);
            pg_result = PQexecPrepared_ptr(pg_conn->connection, stmt->name, 0, NULL, NULL, NULL, 0);
        }
    } else {
        // Fallback to simple execution if PQexecPrepared is not available
        log_this(designator, "PostgreSQL execute_prepared: Falling back to PQexec (PQexecPrepared not available)", LOG_LEVEL_TRACE, 0);
        pg_result = PQexec_ptr(pg_conn->connection, stmt->name);
    }
    
    // Cleanup parameter resources
    if (param_values) {
        for (size_t i = 0; i < ordered_param_count; i++) {
            free(param_values[i]);
        }
        free(param_values);
    }
    if (params) {
        free_parameter_list(params);
    }

    // Check if query took too long (timing now handled at engine abstraction layer, but we still need timeout checking)
    if (check_timeout_expired(query_start_time, query_timeout)) {
        log_this(designator, "PostgreSQL execute_prepared: Query execution time exceeded %d seconds", LOG_LEVEL_ERROR, 1, query_timeout);
        if (pg_result) {
            log_this(designator, "PostgreSQL execute_prepared: Cleaning up failed query result", LOG_LEVEL_TRACE, 0);
            PQclear_ptr(pg_result);
        }
        return false;
    }

    if (!pg_result) {
        log_this(designator, "PostgreSQL execute_prepared: PQexec returned NULL", LOG_LEVEL_ERROR, 0);
        return false;
    }

    int result_status = PQresultStatus_ptr(pg_result);
    if (result_status != PGRES_TUPLES_OK && result_status != PGRES_COMMAND_OK) {
        log_this(designator, "PostgreSQL prepared statement execution failed - status: %d", LOG_LEVEL_ERROR, 1, result_status);
        char* error_msg = PQerrorMessage_ptr(pg_conn->connection);
        char* error_message = NULL;
        if (error_msg && strlen(error_msg) > 0) {
            error_message = strdup(error_msg);
            log_this(designator, "PostgreSQL prepared statement error: %s", LOG_LEVEL_ERROR, 1, error_msg);
        } else {
            error_message = strdup("PostgreSQL prepared statement execution failed (no error details)");
        }

        // Create error result
        QueryResult* error_result = calloc(1, sizeof(QueryResult));
        if (error_result) {
            error_result->success = false;
            error_result->error_message = error_message;
            error_result->row_count = 0;
            error_result->column_count = 0;
            error_result->data_json = strdup("[]");
            error_result->execution_time_ms = 0;
            error_result->affected_rows = 0;
            *result = error_result;
        } else {
            free(error_message);
        }

        PQclear_ptr(pg_result);
        return (error_result != NULL);
    }

    // Create result structure
    QueryResult* db_result = calloc(1, sizeof(QueryResult));
    if (!db_result) {
        PQclear_ptr(pg_result);
        return false;
    }

    db_result->success = true;
    db_result->row_count = (size_t)PQntuples_ptr(pg_result);
    db_result->column_count = (size_t)PQnfields_ptr(pg_result);
    // db_result->execution_time_ms is now set by the engine abstraction layer
    db_result->affected_rows = atoi(PQcmdTuples_ptr(pg_result));

    // Extract column names
    if (db_result->column_count > 0) {
        db_result->column_names = calloc(db_result->column_count, sizeof(char*));
        if (!db_result->column_names) {
            PQclear_ptr(pg_result);
            free(db_result);
            return false;
        }

        for (size_t i = 0; i < db_result->column_count; i++) {
            db_result->column_names[i] = strdup(PQfname_ptr(pg_result, (int)i));
            if (!db_result->column_names[i]) {
                // Cleanup on failure
                for (size_t j = 0; j < i; j++) {
                    free(db_result->column_names[j]);
                }
                free(db_result->column_names);
                PQclear_ptr(pg_result);
                free(db_result);
                return false;
            }
        }
    }

    // Convert result to JSON
    if (db_result->row_count > 0 && db_result->column_count > 0) {
        // Simple JSON array of objects
        // Increase buffer size to handle large query templates (migration SQL can be 10KB+)
        size_t json_size = 16384 * db_result->row_count; // Larger estimate for query templates
        db_result->data_json = calloc(1, json_size);
        if (!db_result->data_json) {
            // Cleanup
            for (size_t i = 0; i < db_result->column_count; i++) {
                free(db_result->column_names[i]);
            }
            free(db_result->column_names);
            PQclear_ptr(pg_result);
            free(db_result);
            return false;
        }

        strcpy(db_result->data_json, "[");
        for (size_t row = 0; row < db_result->row_count; row++) {
            if (row > 0) strcat(db_result->data_json, ",");

            // Track start of this row for debug logging
            size_t row_start_pos = strlen(db_result->data_json);

            strcat(db_result->data_json, "{");

            for (size_t col = 0; col < db_result->column_count; col++) {
                if (col > 0) strcat(db_result->data_json, ",");

                // Get column type to determine if we should quote the value
                int col_type_oid = PQftype_ptr ? PQftype_ptr(pg_result, (int)col) : 0;
                bool is_numeric = postgresql_is_numeric_type(col_type_oid);

                const char* value = PQgetvalue_ptr(pg_result, (int)row, (int)col);
                const char* col_name = db_result->column_names[col];

                // Calculate needed size for this column (name + value + escaping overhead)
                size_t value_len = value ? strlen(value) : 0;
                size_t needed_size = strlen(col_name) + (value_len * 2) + 20; // Extra for escaping

                // Ensure buffer has enough space
                size_t current_len = strlen(db_result->data_json);
                if (current_len + needed_size > json_size) {
                    json_size = (current_len + needed_size) * 2;
                    char* new_json = realloc(db_result->data_json, json_size);
                    if (!new_json) continue; // Skip column on allocation failure
                    db_result->data_json = new_json;
                }

                // Append column to JSON
                char* append_pos = db_result->data_json + current_len;

                if (is_numeric && value && strlen(value) > 0) {
                    // Numeric type - no quotes around value
                    sprintf(append_pos, "\"%s\":%s", col_name, value);
                } else if (value) {
                    // String type - escape and quote
                    sprintf(append_pos, "\"%s\":\"", col_name);
                    char* dst = append_pos + strlen(append_pos);
                    const char* src = value;
                    while (*src) {
                        if (*src == '"' || *src == '\\') {
                            *dst++ = '\\';
                            *dst++ = *src++;
                        } else if (*src == '\n') {
                            *dst++ = '\\';
                            *dst++ = 'n';
                            src++;
                        } else if (*src == '\r') {
                            *dst++ = '\\';
                            *dst++ = 'r';
                            src++;
                        } else if (*src == '\t') {
                            *dst++ = '\\';
                            *dst++ = 't';
                            src++;
                        } else {
                            *dst++ = *src++;
                        }
                    }
                    *dst++ = '"';
                    *dst = '\0';
                } else {
                    // NULL value
                    sprintf(append_pos, "\"%s\":null", col_name);
                }
            }
            strcat(db_result->data_json, "}");

            // Debug: Log first row JSON for diagnosis
            if (row == 0) {
                size_t row_len = strlen(db_result->data_json) - row_start_pos;
                char* row_json = strndup(db_result->data_json + row_start_pos, row_len);
                if (row_json) {
                    log_this(designator, "PostgreSQL first row JSON: %s", LOG_LEVEL_DEBUG, 1, row_json);
                    free(row_json);
                }
            }
        }
        strcat(db_result->data_json, "]");

        // log_this(designator, "PostgreSQL execute_prepared: Complete result JSON: %s", LOG_LEVEL_DEBUG, 1, db_result->data_json);
    } else {
        log_this(designator, "PostgreSQL execute_prepared: Query returned no data (0 rows or 0 columns)", LOG_LEVEL_TRACE, 0);
    }

    PQclear_ptr(pg_result);
    *result = db_result;

    log_this(designator, "PostgreSQL execute_prepared: Prepared statement executed successfully", LOG_LEVEL_TRACE, 0);
    return true;
}