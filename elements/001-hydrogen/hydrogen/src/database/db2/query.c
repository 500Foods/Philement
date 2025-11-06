/*
  * DB2 Database Engine - Query Execution Implementation
  *
  * Implements DB2 query execution functions.
  */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/database/dbqueue/dbqueue.h>

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
                
                db2_json_escape_string(col_data, escaped_data, escaped_size);
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
    }

    // End JSON array
    if (!db2_ensure_json_buffer_capacity(&json_buffer, json_buffer_size, &json_buffer_capacity, 2)) {
        free(json_buffer);
        db2_cleanup_column_names(column_names, column_count);
        free(db_result);
        return false;
    }
    strcat(json_buffer, "]");

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

    // Execute the query
    int exec_result = SQLExecDirect_ptr(stmt_handle, (char*)request->sql_template, SQL_NTS);
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