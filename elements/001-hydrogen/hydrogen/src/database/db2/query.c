/*
  * DB2 Database Engine - Query Execution Implementation
  *
  * Implements DB2 query execution functions.
  */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/database_params.h>

// Local includes
#include "types.h"
#include "query.h"
#include "query_helpers.h"

// External declarations for libdb2 function pointers (defined in connection.c)
extern SQLAllocHandle_t SQLAllocHandle_ptr;
extern SQLExecDirect_t SQLExecDirect_ptr;
extern SQLExecute_t SQLExecute_ptr;
extern SQLFetch_t SQLFetch_ptr;
extern SQLGetData_t SQLGetData_ptr;
extern SQLNumResultCols_t SQLNumResultCols_ptr;
extern SQLRowCount_t SQLRowCount_ptr;
extern SQLFreeHandle_t SQLFreeHandle_ptr;
extern SQLFreeStmt_t SQLFreeStmt_ptr;
extern SQLDescribeCol_t SQLDescribeCol_ptr;
extern SQLGetDiagRec_t SQLGetDiagRec_ptr;
extern SQLPrepare_t SQLPrepare_ptr;
extern SQLBindParameter_t SQLBindParameter_ptr;

// Helper function to cleanup column names (non-static for testing)
void db2_cleanup_column_names(char** column_names, int column_count) {
    if (column_names) {
        for (int i = 0; i < column_count; i++) {
            free(column_names[i]);
        }
        free(column_names);
    }
}

// Helper function to get column names (non-static for testing)
char** db2_get_column_names(void* stmt_handle, int column_count) {
    if (column_count <= 0) {
        return NULL;
    }
    
    char** column_names = calloc((size_t)column_count, sizeof(char*));
    if (!column_names) {
        return NULL;
    }

    for (int col = 0; col < column_count; col++) {
        if (!db2_get_column_name(stmt_handle, col, &column_names[col])) {
            // Cleanup on failure
            for (int i = 0; i < col; i++) {
                free(column_names[i]);
            }
            free(column_names);
            return NULL;
        }
    }
    
    return column_names;
}

// Helper function to fetch and format a single row (non-static for testing)
bool db2_fetch_row_data(void* stmt_handle, char** column_names, int column_count,
                        char** json_buffer, size_t* json_buffer_size, size_t* json_buffer_capacity, bool first_row) {
    if (!stmt_handle || !json_buffer || !json_buffer_size || !json_buffer_capacity) {
        return false;
    }
    
    // Add comma between rows if not first row
    if (!first_row) {
        if (!db2_ensure_json_buffer_capacity(json_buffer, *json_buffer_size, json_buffer_capacity, 2)) {
            return false;
        }
        strcat(*json_buffer, ",");
        (*json_buffer_size)++;
    }

    // Start JSON object for this row
    if (!db2_ensure_json_buffer_capacity(json_buffer, *json_buffer_size, json_buffer_capacity, 2)) {
        return false;
    }
    strcat(*json_buffer, "{");
    (*json_buffer_size)++;

    // Fetch each column
    for (int col = 0; col < column_count; col++) {
        if (col > 0) {
            if (!db2_ensure_json_buffer_capacity(json_buffer, *json_buffer_size, json_buffer_capacity, 2)) {
                return false;
            }
            strcat(*json_buffer, ",");
            (*json_buffer_size)++;
        }

        // Get column type to determine if we should quote the value
        int sql_type = 0;
        bool got_type = db2_get_column_type(stmt_handle, col, &sql_type);
        bool is_numeric = got_type && db2_is_numeric_type(sql_type);

        // Get column data - use dynamic allocation for large columns (like migration SQL)
        int data_len = 0;
        
        // First call to get the actual data length
        (void)SQLGetData_ptr (stmt_handle, col + 1, SQL_C_CHAR, NULL, 0, &data_len);
        
        char* col_data = NULL;
        int fetch_result = -1;
        
        if (data_len == SQL_NULL_DATA) {
            // Data is NULL - handle it directly
            fetch_result = SQL_SUCCESS;
        } else if (data_len > 0) {
            // Allocate buffer for actual data length + null terminator
            col_data = calloc(1, (size_t)data_len + 1);
            if (!col_data) {
                return false;
            }
            
            // Second call to get the actual data
            fetch_result = SQLGetData_ptr(stmt_handle, col + 1, SQL_C_CHAR, col_data, data_len + 1, &data_len);
        } else {
            // Fallback for empty or small data
            col_data = calloc(1, 256);
            if (!col_data) {
                return false;
            }
            fetch_result = SQLGetData_ptr ? SQLGetData_ptr(stmt_handle, col + 1, SQL_C_CHAR, col_data, 256, &data_len) : -1;
        }

        if (fetch_result == SQL_SUCCESS || fetch_result == SQL_SUCCESS_WITH_INFO) {
            // Calculate needed space for JSON (column name + value + quotes + escaping)
            // Use strlen(col_data) only if data_len indicates valid data (> 0)
            // Don't trust data_len if it's negative or zero as it's an SQL indicator
            size_t actual_data_len = (col_data && data_len > 0) ? strlen(col_data) : 0;
            size_t needed_json_space = strlen(column_names[col]) + (actual_data_len * 2) + 20;
            
            // Ensure we have enough capacity
            if (!db2_ensure_json_buffer_capacity(json_buffer, *json_buffer_size, json_buffer_capacity, needed_json_space)) {
                free(col_data);
                return false;
            }
            
            // Build JSON for this column
            char* current_pos = *json_buffer + *json_buffer_size;
            
            if (data_len == SQL_NULL_DATA) {
                int written = snprintf(current_pos, needed_json_space, "\"%s\":null", column_names[col]);
                // Check for truncation - snprintf returns what WOULD be written, not what WAS written
                if (written >= (int)needed_json_space) {
                    written = (int)needed_json_space - 1; // Actual bytes written (excluding null terminator)
                }
                *json_buffer_size += (size_t)written;
            } else if (is_numeric) {
                // Numeric types - no quotes around value
                int written = snprintf(current_pos, needed_json_space, "\"%s\":%s", column_names[col], col_data);
                // Check for truncation - snprintf returns what WOULD be written, not what WAS written
                if (written >= (int)needed_json_space) {
                    written = (int)needed_json_space - 1; // Actual bytes written (excluding null terminator)
                }
                *json_buffer_size += (size_t)written;
            } else {
                // String types - quote and escape the value
                // For large strings (like migration SQL), use dynamic allocation for escaped data
                size_t escaped_size = (col_data ? strlen(col_data) * 2 : 0) + 1;
                char* escaped_data = calloc(1, escaped_size);
                if (!escaped_data) {
                    free(col_data);
                    return false;
                }
                
                database_json_escape_string(col_data, escaped_data, escaped_size);
                int written = snprintf(current_pos, needed_json_space, "\"%s\":\"%s\"", column_names[col], escaped_data);
                // Check for truncation - snprintf returns what WOULD be written, not what WAS written
                if (written >= (int)needed_json_space) {
                    written = (int)needed_json_space - 1; // Actual bytes written (excluding null terminator)
                }
                *json_buffer_size += (size_t)written;
                
                free(escaped_data);
            }
        }
        
        free(col_data);
    }

    // End JSON object for this row
    if (!db2_ensure_json_buffer_capacity(json_buffer, *json_buffer_size, json_buffer_capacity, 2)) {
        return false;
    }
    strcat(*json_buffer, "}");
    (*json_buffer_size)++;
    
    return true;
}

// Helper function to process query results (non-static for testing)
bool db2_process_query_results(void* stmt_handle, const char* designator, struct timespec start_time, QueryResult** result) {
    if (!stmt_handle || !designator || !result) {
        return false;
    }

    // Create result structure
    QueryResult* db_result = calloc(1, sizeof(QueryResult));
    if (!db_result) {
        return false;
    }

    db_result->success = true;

    // Get column count
    int column_count = 0;
    if (SQLNumResultCols_ptr && SQLNumResultCols_ptr(stmt_handle, &column_count) == SQL_SUCCESS) {
        db_result->column_count = (size_t)column_count;
    }

    // Get column names using helper function
    char** column_names = db2_get_column_names(stmt_handle, column_count);
    if (column_count > 0 && !column_names) {
        free(db_result);
        return false;
    }

    // Get row count
    int sql_row_count = 0;
    if (SQLRowCount_ptr && SQLRowCount_ptr(stmt_handle, &sql_row_count) == SQL_SUCCESS) {
        db_result->affected_rows = (size_t)sql_row_count;
    }

    // Fetch all result rows - only if there are result columns
    size_t row_count = 0;
    char* json_buffer = NULL;
    size_t json_buffer_size = 0;
    size_t json_buffer_capacity = 1024;

    // Check if this statement returns result columns (not DDL statements)
    if (column_count > 0) {
        json_buffer = calloc(1, json_buffer_capacity);
        if (!json_buffer) {
            // Cleanup column names using helper
            db2_cleanup_column_names(column_names, column_count);
            free(db_result);
            return false;
        }

        // Start JSON array
        strcpy(json_buffer, "[");
        json_buffer_size = 1;

        // Fetch rows using helper function
        while (SQLFetch_ptr && SQLFetch_ptr(stmt_handle) == SQL_SUCCESS) {
            bool first_row = (row_count == 0);
            if (!db2_fetch_row_data(stmt_handle, column_names, column_count,
                                    &json_buffer, &json_buffer_size, &json_buffer_capacity, first_row)) {
                free(json_buffer);
                db2_cleanup_column_names(column_names, column_count);
                free(db_result);
                return false;
            }
            row_count++;
        }
    } else {
        // DDL statement or statement with no result columns - create empty JSON array
        json_buffer = strdup("[]");
        if (!json_buffer) {
            db2_cleanup_column_names(column_names, column_count);
            free(db_result);
            return false;
        }
        json_buffer_size = 2; // Length of "[]"
        json_buffer_capacity = 3; // CRITICAL FIX: Update capacity to match actual allocation (2 chars + null terminator)
    }

    // End JSON array - only for queries with result columns
    // For DDL statements, "[]" is already complete, don't append anything
    if (column_count > 0) {
        if (!db2_ensure_json_buffer_capacity(&json_buffer, json_buffer_size, &json_buffer_capacity, 2)) {
            free(json_buffer);
            db2_cleanup_column_names(column_names, column_count);
            free(db_result);
            return false;
        }
        strcat(json_buffer, "]");
    }

    db_result->row_count = row_count;
    db_result->data_json = json_buffer;

    // End timing after all result processing is complete
    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    db_result->execution_time_ms = (end_time.tv_sec - start_time.tv_sec) * 1000 +
                                     (end_time.tv_nsec - start_time.tv_nsec) / 1000000;

    log_this(designator, "DB2 query results: %zu columns, %zu rows, %d affected", LOG_LEVEL_TRACE, 3,
        db_result->column_count, db_result->row_count, db_result->affected_rows);

    // Clean up column names using helper
    db2_cleanup_column_names(column_names, column_count);

    *result = db_result;
    return true;
}

// Helper function to bind a single parameter
static bool db2_bind_single_parameter(void* stmt_handle, unsigned short param_index, TypedParameter* param,
                                       void** bound_values, long* str_len_indicators, const char* designator) {
    if (!SQLBindParameter_ptr) {
        log_this(designator, "SQLBindParameter function not available", LOG_LEVEL_ERROR, 0);
        return false;
    }

    int bind_result = SQL_SUCCESS;

    log_this(designator, "Binding parameter %u: name=%s, type=%d", LOG_LEVEL_TRACE, 3,
             (unsigned int)param_index, param->name, param->type);

    switch (param->type) {
        case PARAM_TYPE_INTEGER: {
            bound_values[param_index - 1] = malloc(sizeof(int));
            if (!bound_values[param_index - 1]) return false;
            *(int*)bound_values[param_index - 1] = (int)param->value.int_value;
            str_len_indicators[param_index - 1] = 0;
            log_this(designator, "Binding INTEGER parameter %u: value=%d", LOG_LEVEL_TRACE, 2,
                     (unsigned int)param_index, (int)param->value.int_value);
            bind_result = SQLBindParameter_ptr(stmt_handle, param_index, SQL_PARAM_INPUT,
                                               SQL_C_LONG, SQL_INTEGER, 0, 0,
                                               bound_values[param_index - 1], 0,
                                               &str_len_indicators[param_index - 1]);
            break;
        }
        case PARAM_TYPE_STRING: {
            size_t str_len = param->value.string_value ? strlen(param->value.string_value) : 0;
            bound_values[param_index - 1] = param->value.string_value ? strdup(param->value.string_value) : strdup("");
            if (!bound_values[param_index - 1]) return false;
            str_len_indicators[param_index - 1] = (long)str_len;
            log_this(designator, "Binding STRING parameter %u: value='%s', len=%zu", LOG_LEVEL_TRACE, 3,
                     (unsigned int)param_index, (char*)bound_values[param_index - 1], str_len);
            bind_result = SQLBindParameter_ptr(stmt_handle, param_index, SQL_PARAM_INPUT,
                                               SQL_C_CHAR, SQL_CHAR, str_len > 0 ? str_len : 1, 0,
                                               bound_values[param_index - 1], (long)(str_len + 1),
                                               &str_len_indicators[param_index - 1]);
            break;
        }
        case PARAM_TYPE_BOOLEAN: {
            bound_values[param_index - 1] = malloc(sizeof(short));
            if (!bound_values[param_index - 1]) return false;
            *(short*)bound_values[param_index - 1] = param->value.bool_value ? 1 : 0;
            str_len_indicators[param_index - 1] = 0;
            log_this(designator, "Binding BOOLEAN parameter %u: value=%d", LOG_LEVEL_TRACE, 2,
                     (unsigned int)param_index, param->value.bool_value ? 1 : 0);
            bind_result = SQLBindParameter_ptr(stmt_handle, param_index, SQL_PARAM_INPUT,
                                               SQL_C_SHORT, SQL_SMALLINT, 0, 0,
                                               bound_values[param_index - 1], 0,
                                               &str_len_indicators[param_index - 1]);
            break;
        }
        case PARAM_TYPE_FLOAT: {
            bound_values[param_index - 1] = malloc(sizeof(double));
            if (!bound_values[param_index - 1]) return false;
            *(double*)bound_values[param_index - 1] = param->value.float_value;
            str_len_indicators[param_index - 1] = 0;
            log_this(designator, "Binding FLOAT parameter %u: value=%f", LOG_LEVEL_TRACE, 2,
                     (unsigned int)param_index, param->value.float_value);
            bind_result = SQLBindParameter_ptr(stmt_handle, param_index, SQL_PARAM_INPUT,
                                               SQL_C_DOUBLE, SQL_DOUBLE, 0, 0,
                                               bound_values[param_index - 1], 0,
                                               &str_len_indicators[param_index - 1]);
            break;
        }
        case PARAM_TYPE_TEXT: {
            size_t text_len = param->value.text_value ? strlen(param->value.text_value) : 0;
            bound_values[param_index - 1] = param->value.text_value ? strdup(param->value.text_value) : strdup("");
            if (!bound_values[param_index - 1]) return false;
            str_len_indicators[param_index - 1] = (long)text_len;
            log_this(designator, "Binding TEXT parameter %u: len=%zu", LOG_LEVEL_TRACE, 2,
                     (unsigned int)param_index, text_len);
            bind_result = SQLBindParameter_ptr(stmt_handle, param_index, SQL_PARAM_INPUT,
                                               SQL_C_CHAR, SQL_LONGVARCHAR, text_len > 0 ? text_len : 1, 0,
                                               bound_values[param_index - 1], (long)(text_len + 1),
                                               &str_len_indicators[param_index - 1]);
            break;
        }
        case PARAM_TYPE_DATE: {
            // DATE_STRUCT: year, month, day
            // Allocate DATE_STRUCT
            SQL_DATE_STRUCT* date_struct = malloc(sizeof(SQL_DATE_STRUCT));
            if (!date_struct) return false;
            
            // Parse date string (YYYY-MM-DD format)
            const char* date_value = param->value.date_value ? param->value.date_value : "1970-01-01";
            int year = 0, month = 0, day = 0;
            if (sscanf(date_value, "%d-%d-%d", &year, &month, &day) != 3) {
                log_this(designator, "Invalid DATE format (expected YYYY-MM-DD): %s", LOG_LEVEL_ERROR, 1, date_value);
                free(date_struct);
                return false;
            }
            
            date_struct->year = (short)year;
            date_struct->month = (unsigned short)month;
            date_struct->day = (unsigned short)day;
            
            bound_values[param_index - 1] = date_struct;
            str_len_indicators[param_index - 1] = 0;
            log_this(designator, "Binding DATE parameter %u: %04d-%02d-%02d", LOG_LEVEL_TRACE, 4,
                     (unsigned int)param_index, year, month, day);
            bind_result = SQLBindParameter_ptr(stmt_handle, param_index, SQL_PARAM_INPUT,
                                               SQL_C_TYPE_DATE, SQL_TYPE_DATE, 0, 0,
                                               bound_values[param_index - 1], 0,
                                               &str_len_indicators[param_index - 1]);
            break;
        }
        case PARAM_TYPE_TIME: {
            // TIME_STRUCT: hour, minute, second
            // Allocate TIME_STRUCT
            SQL_TIME_STRUCT* time_struct = malloc(sizeof(SQL_TIME_STRUCT));
            if (!time_struct) return false;
            
            // Parse time string (HH:MM:SS format)
            const char* time_value = param->value.time_value ? param->value.time_value : "00:00:00";
            int hour = 0, minute = 0, second = 0;
            if (sscanf(time_value, "%d:%d:%d", &hour, &minute, &second) != 3) {
                log_this(designator, "Invalid TIME format (expected HH:MM:SS): %s", LOG_LEVEL_ERROR, 1, time_value);
                free(time_struct);
                return false;
            }
            
            time_struct->hour = (unsigned short)hour;
            time_struct->minute = (unsigned short)minute;
            time_struct->second = (unsigned short)second;
            
            bound_values[param_index - 1] = time_struct;
            str_len_indicators[param_index - 1] = 0;
            log_this(designator, "Binding TIME parameter %u: %02d:%02d:%02d", LOG_LEVEL_TRACE, 4,
                     (unsigned int)param_index, hour, minute, second);
            bind_result = SQLBindParameter_ptr(stmt_handle, param_index, SQL_PARAM_INPUT,
                                               SQL_C_TYPE_TIME, SQL_TYPE_TIME, 0, 0,
                                               bound_values[param_index - 1], 0,
                                               &str_len_indicators[param_index - 1]);
            break;
        }
        case PARAM_TYPE_DATETIME: {
            // TIMESTAMP_STRUCT: year, month, day, hour, minute, second, fraction
            // Allocate TIMESTAMP_STRUCT
            SQL_TIMESTAMP_STRUCT* timestamp_struct = malloc(sizeof(SQL_TIMESTAMP_STRUCT));
            if (!timestamp_struct) return false;
            
            // Parse datetime string (YYYY-MM-DD HH:MM:SS format)
            const char* datetime_value = param->value.datetime_value ? param->value.datetime_value : "1970-01-01 00:00:00";
            int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
            if (sscanf(datetime_value, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second) != 6) {
                log_this(designator, "Invalid DATETIME format (expected YYYY-MM-DD HH:MM:SS): %s", LOG_LEVEL_ERROR, 1, datetime_value);
                free(timestamp_struct);
                return false;
            }
            
            timestamp_struct->year = (short)year;
            timestamp_struct->month = (unsigned short)month;
            timestamp_struct->day = (unsigned short)day;
            timestamp_struct->hour = (unsigned short)hour;
            timestamp_struct->minute = (unsigned short)minute;
            timestamp_struct->second = (unsigned short)second;
            timestamp_struct->fraction = 0; // No fractional seconds in this format
            
            bound_values[param_index - 1] = timestamp_struct;
            str_len_indicators[param_index - 1] = 0;
            log_this(designator, "Binding DATETIME parameter %u: %04d-%02d-%02d %02d:%02d:%02d", LOG_LEVEL_TRACE, 7,
                     (unsigned int)param_index, year, month, day, hour, minute, second);
            bind_result = SQLBindParameter_ptr(stmt_handle, param_index, SQL_PARAM_INPUT,
                                               SQL_C_TYPE_TIMESTAMP, SQL_TYPE_TIMESTAMP, 0, 0,
                                               bound_values[param_index - 1], 0,
                                               &str_len_indicators[param_index - 1]);
            break;
        }
        case PARAM_TYPE_TIMESTAMP: {
            // TIMESTAMP_STRUCT: year, month, day, hour, minute, second, fraction (milliseconds)
            // Allocate TIMESTAMP_STRUCT
            SQL_TIMESTAMP_STRUCT* timestamp_struct = malloc(sizeof(SQL_TIMESTAMP_STRUCT));
            if (!timestamp_struct) return false;
            
            // Parse timestamp string (YYYY-MM-DD HH:MM:SS.fff format)
            const char* timestamp_value = param->value.timestamp_value ? param->value.timestamp_value : "1970-01-01 00:00:00.000";
            int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0, milliseconds = 0;
            int parsed = sscanf(timestamp_value, "%d-%d-%d %d:%d:%d.%d", &year, &month, &day, &hour, &minute, &second, &milliseconds);
            if (parsed < 6) {
                log_this(designator, "Invalid TIMESTAMP format (expected YYYY-MM-DD HH:MM:SS.fff): %s", LOG_LEVEL_ERROR, 1, timestamp_value);
                free(timestamp_struct);
                return false;
            }
            // If only 6 fields parsed, milliseconds is 0 (fractional seconds are optional)
            
            timestamp_struct->year = (short)year;
            timestamp_struct->month = (unsigned short)month;
            timestamp_struct->day = (unsigned short)day;
            timestamp_struct->hour = (unsigned short)hour;
            timestamp_struct->minute = (unsigned short)minute;
            timestamp_struct->second = (unsigned short)second;
            // Convert milliseconds to nanoseconds (fraction field is nanoseconds)
            timestamp_struct->fraction = (unsigned long)(milliseconds * 1000000);
            
            bound_values[param_index - 1] = timestamp_struct;
            str_len_indicators[param_index - 1] = 0;
            log_this(designator, "Binding TIMESTAMP parameter %u: %04d-%02d-%02d %02d:%02d:%02d.%03d", LOG_LEVEL_TRACE, 8,
                     (unsigned int)param_index, year, month, day, hour, minute, second, milliseconds);
            bind_result = SQLBindParameter_ptr(stmt_handle, param_index, SQL_PARAM_INPUT,
                                               SQL_C_TYPE_TIMESTAMP, SQL_TYPE_TIMESTAMP, 0, 0,
                                               bound_values[param_index - 1], 0,
                                               &str_len_indicators[param_index - 1]);
            break;
        }
    }

    if (bind_result != SQL_SUCCESS && bind_result != SQL_SUCCESS_WITH_INFO) {
        log_this(designator, "Failed to bind parameter %u (type %d) - result: %d", LOG_LEVEL_ERROR, 3,
                 (unsigned int)param_index, param->type, bind_result);
        return false;
    }

    log_this(designator, "Successfully bound parameter %u", LOG_LEVEL_TRACE, 1, (unsigned int)param_index);
    return true;
}

// Helper function to cleanup bound values
static void db2_cleanup_bound_values(void** bound_values, size_t count) {
    if (bound_values) {
        for (size_t i = 0; i < count; i++) {
            free(bound_values[i]);
        }
        free(bound_values);
    }
}

// Query Execution Functions
bool db2_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result) {
    if (!connection || !request || !result || connection->engine_type != DB_ENGINE_DB2) {
        const char* designator = connection ? (connection->designator ? connection->designator : SR_DATABASE) : SR_DATABASE;
        log_this(designator, "DB2 execute_query: Invalid parameters", LOG_LEVEL_ERROR, 0);
        return false;
    }

    const char* designator = connection->designator ? connection->designator : SR_DATABASE;
    log_this(designator, "db2_execute_query: ENTER - connection=%p, request=%p, result=%p", LOG_LEVEL_TRACE, 3, (void*)connection, (void*)request, (void*)result);

    log_this(designator, "db2_execute_query: Parameters validated, proceeding", LOG_LEVEL_TRACE, 0);

    const DB2Connection* db2_conn = (const DB2Connection*)connection->connection_handle;
    if (!db2_conn || !db2_conn->connection) {
        log_this(designator, "DB2 execute_query: Invalid connection handle", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // log_this(designator, "DB2 execute_query:\n%s", LOG_LEVEL_TRACE, 1, request->sql_template);

    // Allocate statement handle
    void* stmt_handle = NULL;
    if (SQLAllocHandle_ptr(SQL_HANDLE_STMT, db2_conn->connection, &stmt_handle) != SQL_SUCCESS) {
        log_this(designator, "DB2 execute_query: Failed to allocate statement handle", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Start timing before query execution
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    // Variables for parameter binding
    ParameterList* param_list = NULL;
    TypedParameter** ordered_params = NULL;
    size_t param_count = 0;
    char* positional_sql = NULL;
    void** bound_values = NULL;
    long* str_len_indicators = NULL;
    int exec_result = -1;

    // Check if we have parameters to bind
    bool has_params = request->parameters_json && strlen(request->parameters_json) > 2; // More than "{}"
    
    if (has_params && SQLPrepare_ptr && SQLBindParameter_ptr) {
        // Parse parameters
        log_this(designator, "DB2 execute_query: Parsing parameters: %s", LOG_LEVEL_TRACE, 1, request->parameters_json);
        param_list = parse_typed_parameters(request->parameters_json, designator);
        
        if (param_list && param_list->count > 0) {
            // Convert named parameters to positional
            positional_sql = convert_named_to_positional(
                request->sql_template, param_list, DB_ENGINE_DB2,
                &ordered_params, &param_count, designator
            );
            
            if (!positional_sql) {
                log_this(designator, "DB2 execute_query: Failed to convert named to positional parameters", LOG_LEVEL_ERROR, 0);
                free_parameter_list(param_list);
                SQLFreeHandle_ptr(SQL_HANDLE_STMT, stmt_handle);
                return false;
            }
            
            log_this(designator, "DB2 execute_query: Converted SQL: %s", LOG_LEVEL_TRACE, 1, positional_sql);
            log_this(designator, "DB2 execute_query: Parameter count: %zu", LOG_LEVEL_TRACE, 1, param_count);
            
            // Prepare the statement
            int prepare_result = SQLPrepare_ptr(stmt_handle, (unsigned char*)positional_sql, SQL_NTS);
            if (prepare_result != SQL_SUCCESS && prepare_result != SQL_SUCCESS_WITH_INFO) {
                log_this(designator, "DB2 execute_query: SQLPrepare failed with result %d", LOG_LEVEL_ERROR, 1, prepare_result);
                free(positional_sql);
                free(ordered_params);
                free_parameter_list(param_list);
                SQLFreeHandle_ptr(SQL_HANDLE_STMT, stmt_handle);
                return false;
            }
            
            // Allocate arrays for bound values and indicators
            bound_values = calloc(param_count, sizeof(void*));
            str_len_indicators = calloc(param_count, sizeof(long));
            if (!bound_values || !str_len_indicators) {
                log_this(designator, "DB2 execute_query: Failed to allocate binding arrays", LOG_LEVEL_ERROR, 0);
                free(bound_values);
                free(str_len_indicators);
                free(positional_sql);
                free(ordered_params);
                free_parameter_list(param_list);
                SQLFreeHandle_ptr(SQL_HANDLE_STMT, stmt_handle);
                return false;
            }
            
            // Bind each parameter
            for (size_t i = 0; i < param_count; i++) {
                if (!db2_bind_single_parameter(stmt_handle, (unsigned short)(i + 1), ordered_params[i],
                                                bound_values, str_len_indicators, designator)) {
                    log_this(designator, "DB2 execute_query: Failed to bind parameter %zu", LOG_LEVEL_ERROR, 1, i + 1);
                    db2_cleanup_bound_values(bound_values, i);
                    free(str_len_indicators);
                    free(positional_sql);
                    free(ordered_params);
                    free_parameter_list(param_list);
                    SQLFreeHandle_ptr(SQL_HANDLE_STMT, stmt_handle);
                    return false;
                }
            }
            
            // Execute the prepared statement
            exec_result = SQLExecute_ptr(stmt_handle);
            
            // Cleanup binding resources
            db2_cleanup_bound_values(bound_values, param_count);
            free(str_len_indicators);
            free(positional_sql);
            free(ordered_params);
            free_parameter_list(param_list);
        } else {
            // No actual parameters or parsing failed, fall back to direct execution
            if (param_list) free_parameter_list(param_list);
            exec_result = SQLExecDirect_ptr(stmt_handle, (char*)request->sql_template, SQL_NTS);
        }
    } else {
        // No parameters, use direct execution
        exec_result = SQLExecDirect_ptr(stmt_handle, (char*)request->sql_template, SQL_NTS);
    }

    if (exec_result != SQL_SUCCESS && exec_result != SQL_SUCCESS_WITH_INFO) {
        // Get detailed error information
        unsigned char sql_state[6] = {0};
        long int native_error = 0;
        unsigned char error_msg[1024] = {0};
        short msg_len = 0;

        // Get the first error diagnostic
        int diag_result = SQLGetDiagRec_ptr ? SQLGetDiagRec_ptr(SQL_HANDLE_STMT, stmt_handle, 1,
            sql_state, &native_error, error_msg, (short)sizeof(error_msg), &msg_len) : -1;

        if (diag_result == SQL_SUCCESS || diag_result == SQL_SUCCESS_WITH_INFO) {
            char *msg = (char*)error_msg;
            while (*msg) {
                if (*msg == '\n') {
                    *msg = ' ';
                }
                msg++;
            }
            log_this(designator, "DB2 query execution failed - MESSAGE: %s", LOG_LEVEL_TRACE, 1, (char*)error_msg);
            log_this(designator, "DB2 query execution failed - SQLSTATE: %s, Native Error: %ld", LOG_LEVEL_TRACE, 2, (char*)sql_state, (long int)native_error);
            log_this(designator, "DB2 query execution failed - STATEMENT:\n%s", LOG_LEVEL_TRACE, 1, request->sql_template);

        } else {
            log_this(designator, "DB2 query execution failed - result: %d (could not get error details)", LOG_LEVEL_TRACE, 1, exec_result);
        }

        SQLFreeHandle_ptr(SQL_HANDLE_STMT, stmt_handle);
        return false;
    }

    // Process query results using helper function
    bool process_result = db2_process_query_results(stmt_handle, designator, start_time, result);
    
    // Clean up statement handle
    SQLFreeHandle_ptr(SQL_HANDLE_STMT, stmt_handle);
    
    if (process_result) {
        log_this(designator, "DB2 execute_query: Query completed successfully", LOG_LEVEL_DEBUG, 0);
    }
    
    return process_result;
}

    // cppcheck-suppress constParameterPointer
    // Justification: Database engine interface requires non-const QueryRequest* parameter
bool db2_execute_prepared(DatabaseHandle* connection, const PreparedStatement* stmt, QueryRequest* request, QueryResult** result) {
    if (!connection || !stmt || !request || !result || connection->engine_type != DB_ENGINE_DB2) {
        return false;
    }

    const char* designator = connection->designator ? connection->designator : SR_DATABASE;

    const DB2Connection* db2_conn = (const DB2Connection*)connection->connection_handle;
    if (!db2_conn || !db2_conn->connection) {
        return false;
    }

    // Get the prepared statement handle
    void* stmt_handle = stmt->engine_specific_handle;
    if (!stmt_handle) {
        // Statement had no executable SQL (e.g., only comments after macro processing)
        // Return successful empty result instead of error
        log_this(designator, "DB2 prepared statement: No executable SQL (statement was not actionable)", LOG_LEVEL_DEBUG, 0);
        
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

    // Check if SQLExecute is available
    if (!SQLExecute_ptr) {
        log_this(designator, "DB2 prepared statement execution: SQLExecute function not available", LOG_LEVEL_ERROR, 0);
        return false;
    }

    log_this(designator, "DB2 prepared statement execution: Executing prepared statement", LOG_LEVEL_TRACE, 0);

    // Start timing before query execution
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    // Execute the prepared statement
    int exec_result = SQLExecute_ptr(stmt_handle);
    if (exec_result != SQL_SUCCESS && exec_result != SQL_SUCCESS_WITH_INFO) {
        // Get detailed error information
        unsigned char sql_state[6] = {0};
        long int native_error = 0;
        unsigned char error_msg[1024] = {0};
        short msg_len = 0;

        // Get the first error diagnostic
        int diag_result = SQLGetDiagRec_ptr ? SQLGetDiagRec_ptr(SQL_HANDLE_STMT, stmt_handle, 1,
            sql_state, &native_error, error_msg, (short)sizeof(error_msg), &msg_len) : -1;

        if (diag_result == SQL_SUCCESS || diag_result == SQL_SUCCESS_WITH_INFO) {
            char *msg = (char*)error_msg;
            while (*msg) {
                if (*msg == '\n') {
                    *msg = ' ';
                }
            msg++;
        }
            log_this(designator, "DB2 prepared statement execution failed - MESSAGE: %s", LOG_LEVEL_ERROR, 1, (char*)error_msg);
            log_this(designator, "DB2 prepared statement execution failed - SQLSTATE: %s, Native Error: %ld", LOG_LEVEL_ERROR, 2, (char*)sql_state, (long int)native_error);
        } else {
            log_this(designator, "DB2 prepared statement execution failed - result: %d (could not get error details)", LOG_LEVEL_ERROR, 1, exec_result);
        }

        return false;
    }

    // Process query results using helper function
    bool process_result = db2_process_query_results(stmt_handle, designator, start_time, result);
    
    if (process_result) {
        log_this(designator, "DB2 prepared statement execution: Query completed successfully", LOG_LEVEL_TRACE, 0);
    }
    
    return process_result;
}