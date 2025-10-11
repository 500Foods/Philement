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

        // Get column data
        char col_data[256] = {0};
        int data_len = 0;
        int fetch_result = SQLGetData_ptr ? SQLGetData_ptr(stmt_handle, col + 1, SQL_C_CHAR, col_data, sizeof(col_data), &data_len) : -1;

        if (fetch_result == SQL_SUCCESS || fetch_result == SQL_SUCCESS_WITH_INFO) {
            // Add column to JSON
            char col_json[1024];
            if (data_len == SQL_NULL_DATA) {
                snprintf(col_json, sizeof(col_json), "\"%s\":null", column_names[col]);
            } else {
                // Escape data using helper function
                char escaped_data[512] = {0};
                db2_json_escape_string(col_data, escaped_data, sizeof(escaped_data));
                snprintf(col_json, sizeof(col_json), "\"%s\":\"%s\"", column_names[col], escaped_data);
            }

            size_t col_json_len = strlen(col_json);
            if (!db2_ensure_json_buffer_capacity(json_buffer, *json_buffer_size, json_buffer_capacity, col_json_len + 1)) {
                return false;
            }
            strcat(*json_buffer, col_json);
            *json_buffer_size += col_json_len;
        }
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

    // Fetch all result rows
    size_t row_count = 0;
    char* json_buffer = NULL;
    size_t json_buffer_size = 0;
    size_t json_buffer_capacity = 1024;

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
            log_this(designator, "DB2 query execution failed - MESSAGE: %s", LOG_LEVEL_ERROR, 1, (char*)error_msg);
            log_this(designator, "DB2 query execution failed - SQLSTATE: %s, Native Error: %ld", LOG_LEVEL_ERROR, 2, (char*)sql_state, (long int)native_error);
            log_this(designator, "DB2 query execution failed - STATEMENT:\n%s", LOG_LEVEL_ERROR, 1, request->sql_template);

        } else {
            log_this(designator, "DB2 query execution failed - result: %d (could not get error details)", LOG_LEVEL_ERROR, 1, exec_result);
        }

        SQLFreeHandle_ptr(SQL_HANDLE_STMT, stmt_handle);
        return false;
    }

    // Process query results using helper function
    bool process_result = db2_process_query_results(stmt_handle, designator, start_time, result);
    
    // Clean up statement handle
    SQLFreeHandle_ptr(SQL_HANDLE_STMT, stmt_handle);
    
    if (process_result) {
        log_this(designator, "DB2 execute_query: Query completed successfully", LOG_LEVEL_TRACE, 0);
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
        log_this(designator, "DB2 prepared statement execution: No statement handle available", LOG_LEVEL_ERROR, 0);
        return false;
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