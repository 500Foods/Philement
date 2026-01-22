/*
  * MySQL Database Engine - Query Execution Implementation
  *
  * Implements MySQL query execution functions.
  */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/database_params.h>

// Local includes
#include "types.h"
#include "connection.h"
#include "query.h"
#include "query_helpers.h"
#include "utils.h"

// External declarations for libmysqlclient function pointers (defined in connection.c)
extern mysql_store_result_t mysql_store_result_ptr;
extern mysql_num_rows_t mysql_num_rows_ptr;
extern mysql_num_fields_t mysql_num_fields_ptr;
extern mysql_fetch_row_t mysql_fetch_row_ptr;
extern mysql_fetch_fields_t mysql_fetch_fields_ptr;
extern mysql_free_result_t mysql_free_result_ptr;
extern mysql_error_t mysql_error_ptr;
extern mysql_query_t mysql_query_ptr;
extern mysql_affected_rows_t mysql_affected_rows_ptr;
extern mysql_stmt_execute_t mysql_stmt_execute_ptr;
extern mysql_stmt_result_metadata_t mysql_stmt_result_metadata_ptr;
extern mysql_stmt_fetch_t mysql_stmt_fetch_ptr;
extern mysql_stmt_bind_param_t mysql_stmt_bind_param_ptr;
extern mysql_stmt_error_t mysql_stmt_error_ptr;
extern mysql_stmt_affected_rows_t mysql_stmt_affected_rows_ptr;
extern mysql_stmt_store_result_t mysql_stmt_store_result_ptr;
extern mysql_stmt_free_result_t mysql_stmt_free_result_ptr;
extern mysql_stmt_field_count_t mysql_stmt_field_count_ptr;

/*
 * Helper Functions
 */

// Helper function to cleanup column names
void mysql_cleanup_column_names(char** column_names, size_t column_count) {
    if (column_names) {
        for (size_t i = 0; i < column_count; i++) {
            free(column_names[i]);
        }
        free(column_names);
    }
}

/*
 * MySQL Parameter Binding
 */

// MySQL type constants (since we can't include mysql.h)
#define MYSQL_TYPE_LONG 3
#define MYSQL_TYPE_STRING 254
#define MYSQL_TYPE_SHORT 2
#define MYSQL_TYPE_DOUBLE 5
#define MYSQL_TYPE_LONG_BLOB 251
#define MYSQL_TYPE_DATE 10
#define MYSQL_TYPE_TIME 11
#define MYSQL_TYPE_DATETIME 12
#define MYSQL_TYPE_TIMESTAMP 7

// Forward declaration for MYSQL_BIND
// cppcheck-suppress unusedStructMember
struct MYSQL_BIND_STRUCT;

// MYSQL_BIND structure (simplified version to match libmysqlclient)
typedef struct MYSQL_BIND_STRUCT {
    unsigned long* length;
    char* is_null;
    void* buffer;
    unsigned long* error;
    unsigned char* row_ptr;
    void (*store_param_func)(void*, struct MYSQL_BIND_STRUCT*, unsigned char**, unsigned char**);
    void (*fetch_result)(struct MYSQL_BIND_STRUCT*, unsigned int, unsigned char**);
    void (*skip_result)(struct MYSQL_BIND_STRUCT*, unsigned int, unsigned char**);
    unsigned long buffer_length;
    unsigned long offset;
    unsigned long length_value;
    unsigned int param_number;
    unsigned int pack_length;
    unsigned int buffer_type;  // MySQL type constant
    char error_value;
    char is_unsigned;
    char long_data_used;
    char is_null_value;
    void* extension;
} MYSQL_BIND;

// MySQL DATE_TIME structure for date/time binding
typedef struct {
    unsigned int year;
    unsigned int month;
    unsigned int day;
    unsigned int hour;
    unsigned int minute;
    unsigned int second;
    unsigned long second_part;  // microseconds
    char neg;
    unsigned int time_type;
} MYSQL_TIME;

// Helper function to bind a single parameter (will be used in Step 3)
// cppcheck-suppress unusedFunction
static bool mysql_bind_single_parameter(MYSQL_BIND* bind, unsigned int param_index, TypedParameter* param,
                                         void** bound_values, size_t total_param_count, const char* designator) __attribute__((unused));
static bool mysql_bind_single_parameter(MYSQL_BIND* bind, unsigned int param_index, TypedParameter* param,
                                         void** bound_values, size_t total_param_count, const char* designator) {
    if (!bind || !param || !bound_values) {
        log_this(designator, "mysql_bind_single_parameter: invalid parameters", LOG_LEVEL_ERROR, 0);
        return false;
    }

    log_this(designator, "Binding parameter %u: name=%s, type=%d", LOG_LEVEL_TRACE, 3,
             param_index, param->name, param->type);

    switch (param->type) {
        case PARAM_TYPE_INTEGER: {
            long long* int_val = malloc(sizeof(long long));
            if (!int_val) return false;
            *int_val = param->value.int_value;
            bound_values[param_index] = int_val;

            bind[param_index].buffer_type = MYSQL_TYPE_LONG;
            bind[param_index].buffer = int_val;
            bind[param_index].buffer_length = sizeof(long long);
            bind[param_index].is_null = NULL;
            bind[param_index].length = NULL;

            log_this(designator, "Bound INTEGER parameter %u: value=%lld", LOG_LEVEL_TRACE, 2,
                     param_index, *int_val);
            break;
        }
        case PARAM_TYPE_STRING: {
            const char* str_val = param->value.string_value ? param->value.string_value : "";
            size_t str_len = strlen(str_val);
            char* str_copy = strdup(str_val);
            if (!str_copy) return false;
            bound_values[param_index] = str_copy;

            unsigned long* length = malloc(sizeof(unsigned long));
            if (!length) {
                free(str_copy);
                return false;
            }
            *length = (unsigned long)str_len;
            bound_values[total_param_count + param_index] = length;  // Store length pointer in second half

            bind[param_index].buffer_type = MYSQL_TYPE_STRING;
            bind[param_index].buffer = str_copy;
            bind[param_index].buffer_length = (unsigned long)(str_len + 1);
            bind[param_index].is_null = NULL;
            bind[param_index].length = length;

            log_this(designator, "Bound STRING parameter %u: value='%s', len=%zu", LOG_LEVEL_TRACE, 3,
                     param_index, str_copy, str_len);
            break;
        }
        case PARAM_TYPE_BOOLEAN: {
            short* bool_val = malloc(sizeof(short));
            if (!bool_val) return false;
            *bool_val = param->value.bool_value ? 1 : 0;
            bound_values[param_index] = bool_val;

            bind[param_index].buffer_type = MYSQL_TYPE_SHORT;
            bind[param_index].buffer = bool_val;
            bind[param_index].buffer_length = sizeof(short);
            bind[param_index].is_null = NULL;
            bind[param_index].length = NULL;

            log_this(designator, "Bound BOOLEAN parameter %u: value=%d", LOG_LEVEL_TRACE, 2,
                     param_index, *bool_val);
            break;
        }
        case PARAM_TYPE_FLOAT: {
            double* float_val = malloc(sizeof(double));
            if (!float_val) return false;
            *float_val = param->value.float_value;
            bound_values[param_index] = float_val;

            bind[param_index].buffer_type = MYSQL_TYPE_DOUBLE;
            bind[param_index].buffer = float_val;
            bind[param_index].buffer_length = sizeof(double);
            bind[param_index].is_null = NULL;
            bind[param_index].length = NULL;

            log_this(designator, "Bound FLOAT parameter %u: value=%f", LOG_LEVEL_TRACE, 2,
                     param_index,*float_val);
            break;
        }
        case PARAM_TYPE_TEXT: {
            const char* text_val = param->value.text_value ? param->value.text_value : "";
            size_t text_len = strlen(text_val);
            char* text_copy = strdup(text_val);
            if (!text_copy) return false;
            bound_values[param_index] = text_copy;

            unsigned long* length = malloc(sizeof(unsigned long));
            if (!length) {
                free(text_copy);
                return false;
            }
            *length = (unsigned long)text_len;
            bound_values[total_param_count + param_index] = length;  // Store length pointer in second half

            bind[param_index].buffer_type = MYSQL_TYPE_LONG_BLOB;
            bind[param_index].buffer = text_copy;
            bind[param_index].buffer_length = (unsigned long)(text_len + 1);
            bind[param_index].is_null = NULL;
            bind[param_index].length = length;

            log_this(designator, "Bound TEXT parameter %u: len=%zu", LOG_LEVEL_TRACE, 2,
                     param_index, text_len);
            break;
        }
        case PARAM_TYPE_DATE: {
            MYSQL_TIME* date_time = calloc(1, sizeof(MYSQL_TIME));
            if (!date_time) return false;

            const char* date_value = param->value.date_value ? param->value.date_value : "1970-01-01";
            int year = 0, month = 0, day = 0;
            if (sscanf(date_value, "%d-%d-%d", &year, &month, &day) != 3) {
                log_this(designator, "Invalid DATE format (expected YYYY-MM-DD): %s", LOG_LEVEL_ERROR, 1, date_value);
                free(date_time);
                return false;
            }

            date_time->year = (unsigned int)year;
            date_time->month = (unsigned int)month;
            date_time->day = (unsigned int)day;
            date_time->hour = 0;
            date_time->minute = 0;
            date_time->second = 0;
            date_time->second_part = 0;
            date_time->neg = 0;
            date_time->time_type = 1;  // MYSQL_TIMESTAMP_DATE

            bound_values[param_index] = date_time;

            bind[param_index].buffer_type = MYSQL_TYPE_DATE;
            bind[param_index].buffer = date_time;
            bind[param_index].buffer_length = sizeof(MYSQL_TIME);
            bind[param_index].is_null = NULL;
            bind[param_index].length = NULL;

            log_this(designator, "Bound DATE parameter %u: %04d-%02d-%02d", LOG_LEVEL_TRACE, 4,
                     param_index, year, month, day);
            break;
        }
        case PARAM_TYPE_TIME: {
            MYSQL_TIME* date_time = calloc(1, sizeof(MYSQL_TIME));
            if (!date_time) return false;

            const char* time_value = param->value.time_value ? param->value.time_value : "00:00:00";
            int hour = 0, minute = 0, second = 0;
            if (sscanf(time_value, "%d:%d:%d", &hour, &minute, &second) != 3) {
                log_this(designator, "Invalid TIME format (expected HH:MM:SS): %s", LOG_LEVEL_ERROR, 1, time_value);
                free(date_time);
                return false;
            }

            date_time->year = 0;
            date_time->month = 0;
            date_time->day = 0;
            date_time->hour = (unsigned int)hour;
            date_time->minute = (unsigned int)minute;
            date_time->second = (unsigned int)second;
            date_time->second_part = 0;
            date_time->neg = 0;
            date_time->time_type = 2;  // MYSQL_TIMESTAMP_TIME

            bound_values[param_index] = date_time;

            bind[param_index].buffer_type = MYSQL_TYPE_TIME;
            bind[param_index].buffer = date_time;
            bind[param_index].buffer_length = sizeof(MYSQL_TIME);
            bind[param_index].is_null = NULL;
            bind[param_index].length = NULL;

            log_this(designator, "Bound TIME parameter %u: %02d:%02d:%02d", LOG_LEVEL_TRACE, 4,
                     param_index, hour, minute, second);
            break;
        }
        case PARAM_TYPE_DATETIME:
        case PARAM_TYPE_TIMESTAMP: {
            MYSQL_TIME* date_time = calloc(1, sizeof(MYSQL_TIME));
            if (!date_time) return false;

            const char* datetime_value = (param->type == PARAM_TYPE_DATETIME) ?
                (param->value.datetime_value ? param->value.datetime_value : "1970-01-01 00:00:00") :
                (param->value.timestamp_value ? param->value.timestamp_value : "1970-01-01 00:00:00.000");

            int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0, milliseconds = 0;
            int parsed = sscanf(datetime_value, "%d-%d-%d %d:%d:%d.%d",
                               &year, &month, &day, &hour, &minute, &second, &milliseconds);
            if (parsed < 6) {
                log_this(designator, "Invalid DATETIME/TIMESTAMP format: %s", LOG_LEVEL_ERROR, 1, datetime_value);
                free(date_time);
                return false;
            }

            date_time->year = (unsigned int)year;
            date_time->month = (unsigned int)month;
            date_time->day = (unsigned int)day;
            date_time->hour = (unsigned int)hour;
            date_time->minute = (unsigned int)minute;
            date_time->second = (unsigned int)second;
            date_time->second_part = (parsed >= 7) ? (unsigned long)(milliseconds * 1000) : 0;
            date_time->neg = 0;
            date_time->time_type = 3;  // MYSQL_TIMESTAMP_DATETIME

            bound_values[param_index] = date_time;

            bind[param_index].buffer_type = (param->type == PARAM_TYPE_DATETIME) ?
                MYSQL_TYPE_DATETIME : MYSQL_TYPE_TIMESTAMP;
            bind[param_index].buffer = date_time;
            bind[param_index].buffer_length = sizeof(MYSQL_TIME);
            bind[param_index].is_null = NULL;
            bind[param_index].length = NULL;

            log_this(designator, "Bound DATETIME/TIMESTAMP parameter %u: %04d-%02d-%02d %02d:%02d:%02d.%03d",
                     LOG_LEVEL_TRACE, 8, param_index, year, month, day, hour, minute, second, milliseconds);
            break;
        }
        default: {
            log_this(designator, "Unsupported parameter type %d for parameter %u", LOG_LEVEL_ERROR, 2,
                     param->type, param_index);
            return false;
        }
    }

    log_this(designator, "Successfully bound parameter %u", LOG_LEVEL_TRACE, 1, param_index);
    return true;
}

// Helper function to cleanup bound values (will be used in Step 3)
// cppcheck-suppress unusedFunction
static void mysql_cleanup_bound_values(void** bound_values, size_t count) __attribute__((unused));
static void mysql_cleanup_bound_values(void** bound_values, size_t count) {
    if (bound_values) {
        for (size_t i = 0; i < count; i++) {
            free(bound_values[i]);  // Free buffer
            free(bound_values[count + i]);  // Free length pointer if exists (stored in second half)
        }
        free(bound_values);
    }
}

/*
 * Query Execution
 */

bool mysql_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result) {
    if (!connection || !request || !result || connection->engine_type != DB_ENGINE_MYSQL) {
        const char* designator = connection ? (connection->designator ? connection->designator : SR_DATABASE) : SR_DATABASE;
        log_this(designator, "MySQL execute_query: Invalid parameters", LOG_LEVEL_ERROR, 0);
        return false;
    }

    const char* designator = connection->designator ? connection->designator : SR_DATABASE;
    log_this(designator, "mysql_execute_query: ENTER - connection=%p, request=%p, result=%p", LOG_LEVEL_TRACE, 3, (void*)connection, (void*)request, (void*)result);

    // cppcheck-suppress constVariablePointer
    // Justification: MySQL API requires non-const MYSQL* connection handle
    const MySQLConnection* mysql_conn = (const MySQLConnection*)connection->connection_handle;
    if (!mysql_conn || !mysql_conn->connection) {
        log_this(designator, "MySQL execute_query: Invalid connection handle", LOG_LEVEL_ERROR, 0);
        return false;
    }

    log_this(designator, "MySQL execute_query: Executing query: %s", LOG_LEVEL_TRACE, 1, request->sql_template);

    // Check if we have parameters to bind
    bool has_parameters = (request->parameters_json && strlen(request->parameters_json) > 2);  // More than "{}"
    
    if (has_parameters) {
        log_this(designator, "MySQL execute_query: Parameters detected, using prepared statement path", LOG_LEVEL_TRACE, 0);
        
        // Parse typed parameters
        ParameterList* param_list = parse_typed_parameters(request->parameters_json, designator);
        if (!param_list) {
            log_this(designator, "MySQL execute_query: Failed to parse parameters", LOG_LEVEL_ERROR, 0);
            return false;
        }
        
        // Convert named to positional parameters
        TypedParameter** ordered_params = NULL;
        size_t ordered_count = 0;
        
        char* positional_sql = convert_named_to_positional(request->sql_template, param_list, DB_ENGINE_MYSQL,
                                                            &ordered_params, &ordered_count, designator);
        
        if (!positional_sql) {
            log_this(designator, "MySQL execute_query: Failed to convert parameters", LOG_LEVEL_ERROR, 0);
            free_parameter_list(param_list);
            return false;
        }
        
        log_this(designator, "MySQL execute_query: Converted to positional SQL with %zu parameters", LOG_LEVEL_TRACE, 1, ordered_count);
        
        // Initialize prepared statement
        void* stmt = NULL;
        if (mysql_stmt_init_ptr) {
            stmt = mysql_stmt_init_ptr(mysql_conn->connection);
        }
        
        if (!stmt) {
            log_this(designator, "MySQL execute_query: Failed to initialize prepared statement", LOG_LEVEL_ERROR, 0);
            free(positional_sql);
            free(ordered_params);
            free_parameter_list(param_list);
            return false;
        }
        
        // Prepare statement
        if (!mysql_stmt_prepare_ptr || mysql_stmt_prepare_ptr(stmt, positional_sql, (unsigned long)strlen(positional_sql)) != 0) {
            log_this(designator, "MySQL execute_query: Failed to prepare statement", LOG_LEVEL_ERROR, 0);
            char* error_message = NULL;
            if (mysql_stmt_error_ptr) {
                const char* error_msg = mysql_stmt_error_ptr(stmt);
                if (error_msg && strlen(error_msg) > 0) {
                    error_message = strdup(error_msg);
                    log_this(designator, "MySQL prepare error: %s", LOG_LEVEL_ERROR, 1, error_msg);
                }
            }
            if (!error_message) {
                error_message = strdup("MySQL prepared statement preparation failed (no error details)");
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

            if (mysql_stmt_close_ptr) {
                mysql_stmt_close_ptr(stmt);
            }
            free(positional_sql);
            free(ordered_params);
            free_parameter_list(param_list);
            return (error_result != NULL);
        }
        
        // Allocate MYSQL_BIND array and bound values storage
        MYSQL_BIND* bind = calloc(ordered_count, sizeof(MYSQL_BIND));
        void** bound_values = calloc(ordered_count * 2, sizeof(void*));  // *2 for length indicators
        
        if (!bind || !bound_values) {
            log_this(designator, "MySQL execute_query: Failed to allocate binding structures", LOG_LEVEL_ERROR, 0);
            free(bind);
            free(bound_values);
            if (mysql_stmt_close_ptr) {
                mysql_stmt_close_ptr(stmt);
            }
            free(positional_sql);
            free(ordered_params);
            free_parameter_list(param_list);
            return false;
        }
        
        // Bind each parameter
        bool bind_success = true;
        for (size_t i = 0; i < ordered_count; i++) {
            if (!mysql_bind_single_parameter(bind, (unsigned int)i, ordered_params[i], bound_values, ordered_count, designator)) {
                log_this(designator, "MySQL execute_query: Failed to bind parameter %zu", LOG_LEVEL_ERROR, 1, i);
                bind_success = false;
                break;
            }
        }
        
        // Bind parameters to statement
        if (bind_success && mysql_stmt_bind_param_ptr) {
            if (mysql_stmt_bind_param_ptr(stmt, bind) != 0) {
                log_this(designator, "MySQL execute_query: mysql_stmt_bind_param failed", LOG_LEVEL_ERROR, 0);
                if (mysql_stmt_error_ptr) {
                    const char* error_msg = mysql_stmt_error_ptr(stmt);
                    if (error_msg && strlen(error_msg) > 0) {
                        log_this(designator, "MySQL bind error: %s", LOG_LEVEL_ERROR, 1, error_msg);
                    }
                }
                bind_success = false;
            }
        }
        
        // Execute prepared statement and process results
        QueryResult* db_result = NULL;
        if (bind_success && mysql_stmt_execute_ptr) {
            if (mysql_stmt_execute_ptr(stmt) != 0) {
                log_this(designator, "MySQL execute_query: Prepared statement execution failed", LOG_LEVEL_ERROR, 0);
                if (mysql_stmt_error_ptr) {
                    const char* error_msg = mysql_stmt_error_ptr(stmt);
                    if (error_msg && strlen(error_msg) > 0) {
                        log_this(designator, "MySQL execution error: %s", LOG_LEVEL_ERROR, 1, error_msg);
                    }
                }
                bind_success = false;
            } else {
                // Create result structure and use helper to process results
                db_result = calloc(1, sizeof(QueryResult));
                if (db_result && !mysql_process_prepared_stmt_result(stmt, db_result, designator)) {
                    free(db_result->data_json);
                    free(db_result);
                    db_result = NULL;
                    bind_success = false;
                }
            }
        }
        
        // Cleanup
        mysql_cleanup_bound_values(bound_values, ordered_count);
        free(bind);
        if (mysql_stmt_close_ptr) {
            mysql_stmt_close_ptr(stmt);
        }
        free(positional_sql);
        free(ordered_params);
        free_parameter_list(param_list);
        
        if (!bind_success || !db_result) {
            if (db_result) {
                free(db_result->data_json);
                free(db_result);
            }
            return false;
        }
        
        *result = db_result;
        log_this(designator, "MySQL execute_query: Prepared statement completed successfully", LOG_LEVEL_DEBUG, 0);
        return true;
    }
    
    // No parameters - use direct execution path
    log_this(designator, "MySQL execute_query: No parameters, using direct execution", LOG_LEVEL_TRACE, 0);
    
    // Execute query
    if (mysql_query_ptr(mysql_conn->connection, request->sql_template) != 0) {
        log_this(designator, "MySQL query execution failed", LOG_LEVEL_TRACE, 0);
        if (mysql_error_ptr) {
            const char* error_msg = mysql_error_ptr(mysql_conn->connection);
            if (error_msg && strlen(error_msg) > 0) {
                log_this(designator, "MySQL query error: %s", LOG_LEVEL_TRACE, 1, error_msg);
            }
        }
        return false;
    }

    // Store result
    void* mysql_result = mysql_store_result_ptr ? mysql_store_result_ptr(mysql_conn->connection) : NULL;

    // Create result structure and use helper to process results
    QueryResult* db_result = calloc(1, sizeof(QueryResult));
    if (!db_result) {
        if (mysql_result && mysql_free_result_ptr) {
            mysql_free_result_ptr(mysql_result);
        }
        return false;
    }

    // Use helper function to process the direct query result
    if (!mysql_process_direct_result(mysql_conn->connection, mysql_result, db_result, designator)) {
        free(db_result);
        return false;
    }

    *result = db_result;
    log_this(designator, "MySQL execute_query: Query completed successfully", LOG_LEVEL_DEBUG, 0);
    return true;
}

bool mysql_execute_prepared(DatabaseHandle* connection, const PreparedStatement* stmt, QueryRequest* request, QueryResult** result) {
    if (!connection || !stmt || !request || !result || connection->engine_type != DB_ENGINE_MYSQL) {
        const char* designator = connection ? (connection->designator ? connection->designator : SR_DATABASE) : SR_DATABASE;
        log_this(designator, "MySQL execute_prepared: Invalid parameters", LOG_LEVEL_ERROR, 0);
        return false;
    }

    const char* designator = connection->designator ? connection->designator : SR_DATABASE;
    log_this(designator, "mysql_execute_prepared: ENTER - connection=%p, stmt=%p, request=%p, result=%p", LOG_LEVEL_TRACE, 4, (void*)connection, (void*)stmt, (void*)request, (void*)result);

    // cppcheck-suppress constVariablePointer
    // Justification: MySQL API requires non-const MYSQL* connection handle
    MySQLConnection* mysql_conn = (MySQLConnection*)connection->connection_handle;
    if (!mysql_conn || !mysql_conn->connection) {
        log_this(designator, "MySQL execute_prepared: Invalid connection handle", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Get the prepared statement handle
    void* stmt_handle = stmt->engine_specific_handle;
    if (!stmt_handle) {
        // Statement had no executable SQL (e.g., only comments after macro processing)
        // Return successful empty result instead of error
        log_this(designator, "MySQL prepared statement: No executable SQL (statement was not actionable)", LOG_LEVEL_DEBUG, 0);
        
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

    // Check if required functions are available
    if (!mysql_stmt_execute_ptr) {
        log_this(designator, "MySQL execute_prepared: mysql_stmt_execute function not available", LOG_LEVEL_ERROR, 0);
        return false;
    }

    log_this(designator, "MySQL execute_prepared: Executing prepared statement", LOG_LEVEL_TRACE, 0);

    // Execute the prepared statement
    if (mysql_stmt_execute_ptr(stmt_handle) != 0) {
        log_this(designator, "MySQL prepared statement execution failed", LOG_LEVEL_ERROR, 0);
        if (mysql_stmt_error_ptr) {
            const char* error_msg = mysql_stmt_error_ptr(stmt_handle);
            if (error_msg && strlen(error_msg) > 0) {
                log_this(designator, "MySQL prepared statement error: %s", LOG_LEVEL_ERROR, 1, error_msg);
            }
        }
        return false;
    }

    // Create result structure and use helper to process results
    QueryResult* db_result = calloc(1, sizeof(QueryResult));
    if (!db_result) {
        return false;
    }

    // Use helper function to process prepared statement results
    if (!mysql_process_prepared_stmt_result(stmt_handle, db_result, designator)) {
        free(db_result->data_json);
        free(db_result);
        return false;
    }

    *result = db_result;
    log_this(designator, "MySQL execute_prepared: Query completed successfully", LOG_LEVEL_TRACE, 0);
    return true;
}
