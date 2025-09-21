/*
  * DB2 Database Engine - Query Execution Implementation
  *
  * Implements DB2 query execution functions.
  */
#include "../../hydrogen.h"
#include "../database.h"
#include "../database_queue.h"
#include "types.h"
#include "query.h"

// External declarations for libdb2 function pointers (defined in connection.c)
extern SQLAllocHandle_t SQLAllocHandle_ptr;
extern SQLExecDirect_t SQLExecDirect_ptr;
extern SQLFetch_t SQLFetch_ptr;
extern SQLGetData_t SQLGetData_ptr;
extern SQLNumResultCols_t SQLNumResultCols_ptr;
extern SQLRowCount_t SQLRowCount_ptr;
extern SQLFreeHandle_t SQLFreeHandle_ptr;
extern SQLFreeStmt_t SQLFreeStmt_ptr;
extern SQLDescribeCol_t SQLDescribeCol_ptr;

// Query Execution Functions
bool db2_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result) {
    if (!connection || !request || !result || connection->engine_type != DB_ENGINE_DB2) {
        const char* designator = connection ? (connection->designator ? connection->designator : SR_DATABASE) : SR_DATABASE;
        log_this(designator, "DB2 execute_query: Invalid parameters", LOG_LEVEL_ERROR, 0);
        return false;
    }

    const char* designator = connection->designator ? connection->designator : SR_DATABASE;
    log_this(designator, "db2_execute_query: ENTER - connection=%p, request=%p, result=%p", LOG_LEVEL_DEBUG, 3, (void*)connection, (void*)request, (void*)result);

    log_this(designator, "db2_execute_query: Parameters validated, proceeding", LOG_LEVEL_DEBUG, 0);

    const DB2Connection* db2_conn = (const DB2Connection*)connection->connection_handle;
    if (!db2_conn || !db2_conn->connection) {
        log_this(designator, "DB2 execute_query: Invalid connection handle", LOG_LEVEL_ERROR, 0);
        return false;
    }

    log_this(designator, "DB2 execute_query: Executing query: %s", LOG_LEVEL_DEBUG, 1, request->sql_template);

    // Allocate statement handle
    void* stmt_handle = NULL;
    if (SQLAllocHandle_ptr(SQL_HANDLE_STMT, db2_conn->connection, &stmt_handle) != SQL_SUCCESS) {
        log_this(designator, "DB2 execute_query: Failed to allocate statement handle", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Start timing before query execution
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    // Execute the query
    int exec_result = SQLExecDirect_ptr(stmt_handle, (char*)request->sql_template, SQL_NTS);
    if (exec_result != SQL_SUCCESS) {
        log_this(designator, "DB2 query execution failed - result: %d", LOG_LEVEL_ERROR, 1, exec_result);
        SQLFreeHandle_ptr(SQL_HANDLE_STMT, stmt_handle);
        return false;
    }

    // Create result structure
    QueryResult* db_result = calloc(1, sizeof(QueryResult));
    if (!db_result) {
        SQLFreeHandle_ptr(SQL_HANDLE_STMT, stmt_handle);
        return false;
    }

    db_result->success = true;

    // Get column count
    int column_count = 0;
    if (SQLNumResultCols_ptr(stmt_handle, &column_count) == SQL_SUCCESS) {
        db_result->column_count = (size_t)column_count;
    }

    // Get column names
    char** column_names = NULL;
    if (column_count > 0) {
        column_names = calloc((size_t)column_count, sizeof(char*));
        if (!column_names) {
            SQLFreeHandle_ptr(SQL_HANDLE_STMT, stmt_handle);
            free(db_result);
            return false;
        }

        for (int col = 0; col < column_count; col++) {
            char col_name[256] = {0};
            short col_name_len = 0;
            int desc_result = SQLDescribeCol_ptr(stmt_handle, col + 1, (unsigned char*)col_name, sizeof(col_name), &col_name_len, NULL, NULL, NULL, NULL);
            if (desc_result == SQL_SUCCESS || desc_result == SQL_SUCCESS_WITH_INFO) {
                column_names[col] = strdup(col_name);
                if (!column_names[col]) {
                    // Cleanup on failure
                    for (int i = 0; i < col; i++) {
                        free(column_names[i]);
                    }
                    free(column_names);
                    SQLFreeHandle_ptr(SQL_HANDLE_STMT, stmt_handle);
                    free(db_result);
                    return false;
                }
            } else {
                // Fallback to generic name if describe fails
                char fallback_name[32];
                snprintf(fallback_name, sizeof(fallback_name), "col%d", col + 1);
                column_names[col] = strdup(fallback_name);
                if (!column_names[col]) {
                    // Cleanup on failure
                    for (int i = 0; i < col; i++) {
                        free(column_names[i]);
                    }
                    free(column_names);
                    SQLFreeHandle_ptr(SQL_HANDLE_STMT, stmt_handle);
                    free(db_result);
                    return false;
                }
            }
        }
    }

    // Get row count
    int sql_row_count = 0;
    if (SQLRowCount_ptr(stmt_handle, &sql_row_count) == SQL_SUCCESS) {
        db_result->affected_rows = (size_t)sql_row_count;
    }

    // Fetch all result rows
    size_t row_count = 0;
    char* json_buffer = NULL;
    size_t json_buffer_size = 0;
    size_t json_buffer_capacity = 1024;

    json_buffer = calloc(1, json_buffer_capacity);
    if (!json_buffer) {
        // Cleanup column names
        if (column_names) {
            for (int i = 0; i < column_count; i++) {
                free(column_names[i]);
            }
            free(column_names);
        }
        SQLFreeHandle_ptr(SQL_HANDLE_STMT, stmt_handle);
        free(db_result);
        return false;
    }

    // Start JSON array
    strcpy(json_buffer, "[");
    json_buffer_size = 1;
    (void)json_buffer_size; // Suppress unused variable warning

    // Fetch rows
    while (SQLFetch_ptr(stmt_handle) == SQL_SUCCESS) {
        if (row_count > 0) {
            // Add comma between objects
            if (json_buffer_size + 2 >= json_buffer_capacity) {
                json_buffer_capacity *= 2;
                char* new_buffer = realloc(json_buffer, json_buffer_capacity);
                if (!new_buffer) {
                    free(json_buffer);
                    // Cleanup column names
                    if (column_names) {
                        for (int i = 0; i < column_count; i++) {
                            free(column_names[i]);
                        }
                        free(column_names);
                    }
                    SQLFreeHandle_ptr(SQL_HANDLE_STMT, stmt_handle);
                    free(db_result);
                    return false;
                }
                json_buffer = new_buffer;
            }
            strcat(json_buffer, ",");
            json_buffer_size++;
        }

        // Start JSON object for this row
        if (json_buffer_size + 2 >= json_buffer_capacity) {
            json_buffer_capacity *= 2;
            char* new_buffer = realloc(json_buffer, json_buffer_capacity);
            if (!new_buffer) {
                free(json_buffer);
                // Cleanup column names
                if (column_names) {
                    for (int i = 0; i < column_count; i++) {
                        free(column_names[i]);
                    }
                    free(column_names);
                }
                SQLFreeHandle_ptr(SQL_HANDLE_STMT, stmt_handle);
                free(db_result);
                return false;
            }
            json_buffer = new_buffer;
        }
        strcat(json_buffer, "{");
        json_buffer_size++;

        // Fetch each column
        for (int col = 0; col < column_count; col++) {
            if (col > 0) {
                if (json_buffer_size + 2 >= json_buffer_capacity) {
                    json_buffer_capacity *= 2;
                    char* new_buffer = realloc(json_buffer, json_buffer_capacity);
                    if (!new_buffer) {
                        free(json_buffer);
                        // Cleanup column names
                        if (column_names) {
                            for (int i = 0; i < column_count; i++) {
                                free(column_names[i]);
                            }
                            free(column_names);
                        }
                        SQLFreeHandle_ptr(SQL_HANDLE_STMT, stmt_handle);
                        free(db_result);
                        return false;
                    }
                    json_buffer = new_buffer;
                }
                strcat(json_buffer, ",");
                json_buffer_size++;
            }

            // Get column data (simplified - assuming string data for now)
            char col_data[256] = {0};
            int data_len = 0;
            int fetch_result = SQLGetData_ptr(stmt_handle, col + 1, SQL_C_CHAR, col_data, sizeof(col_data), &data_len);

            if (fetch_result == SQL_SUCCESS || fetch_result == SQL_SUCCESS_WITH_INFO) {
                // Add column to JSON
                char col_json[1024];
                if (data_len == SQL_NULL_DATA) {
                    snprintf(col_json, sizeof(col_json), "\"%s\":null", column_names[col]);
                } else {
                    // Escape quotes in data for JSON
                    char escaped_data[512] = {0};
                    const char* src = col_data;
                    char* dst = escaped_data;
                    while (*src && (size_t)(dst - escaped_data) < sizeof(escaped_data) - 2) {
                        if (*src == '"') {
                            *dst++ = '\\';
                            *dst++ = '"';
                        } else if (*src == '\\') {
                            *dst++ = '\\';
                            *dst++ = '\\';
                        } else {
                            *dst++ = *src;
                        }
                        src++;
                    }
                    snprintf(col_json, sizeof(col_json), "\"%s\":\"%s\"", column_names[col], escaped_data);
                }

                size_t col_json_len = strlen(col_json);
                if (json_buffer_size + col_json_len + 1 >= json_buffer_capacity) {
                    json_buffer_capacity = json_buffer_size + col_json_len + 1024;
                    char* new_buffer = realloc(json_buffer, json_buffer_capacity);
                    if (!new_buffer) {
                        free(json_buffer);
                        // Cleanup column names
                        if (column_names) {
                            for (int i = 0; i < column_count; i++) {
                                free(column_names[i]);
                            }
                            free(column_names);
                        }
                        SQLFreeHandle_ptr(SQL_HANDLE_STMT, stmt_handle);
                        free(db_result);
                        return false;
                    }
                    json_buffer = new_buffer;
                }
                strcat(json_buffer, col_json);
                json_buffer_size += col_json_len;
            }
        }

        // End JSON object for this row
        if (json_buffer_size + 2 >= json_buffer_capacity) {
            json_buffer_capacity *= 2;
            char* new_buffer = realloc(json_buffer, json_buffer_capacity);
            if (!new_buffer) {
                free(json_buffer);
                SQLFreeHandle_ptr(SQL_HANDLE_STMT, stmt_handle);
                free(db_result);
                return false;
            }
            json_buffer = new_buffer;
        }
        strcat(json_buffer, "}");
        json_buffer_size++;

        row_count++;
    }

    // End JSON array
    if (json_buffer_size + 2 >= json_buffer_capacity) {
        json_buffer_capacity *= 2;
        char* new_buffer = realloc(json_buffer, json_buffer_capacity);
        if (!new_buffer) {
            free(json_buffer);
            SQLFreeHandle_ptr(SQL_HANDLE_STMT, stmt_handle);
            free(db_result);
            return false;
        }
        json_buffer = new_buffer;
    }
    strcat(json_buffer, "]");
    json_buffer_size++;
    (void)json_buffer_size; // Suppress unused variable warning

    db_result->row_count = row_count;
    db_result->data_json = json_buffer;

    // End timing after all result processing is complete
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    db_result->execution_time_ms = (end_time.tv_sec - start_time.tv_sec) * 1000 +
                                    (end_time.tv_nsec - start_time.tv_nsec) / 1000000;

    log_this(designator, "DB2 execute_query: Query returned %zu columns, affected %d rows", LOG_LEVEL_DEBUG, 2,
        db_result->column_count, db_result->affected_rows);

    // Clean up column names
    if (column_names) {
        for (int i = 0; i < column_count; i++) {
            free(column_names[i]);
        }
        free(column_names);
    }

    // Clean up statement handle
    SQLFreeHandle_ptr(SQL_HANDLE_STMT, stmt_handle);

    *result = db_result;

    log_this(designator, "DB2 execute_query: Query completed successfully", LOG_LEVEL_DEBUG, 0);
    return true;
}

bool db2_execute_prepared(DatabaseHandle* connection, const PreparedStatement* stmt, QueryRequest* request, QueryResult** result) {
    if (!connection || !stmt || !request || !result || connection->engine_type != DB_ENGINE_DB2) {
        return false;
    }

    const char* designator = connection->designator ? connection->designator : SR_DATABASE;

    const DB2Connection* db2_conn = (const DB2Connection*)connection->connection_handle;
    if (!db2_conn || !db2_conn->connection) {
        return false;
    }

    // For now, execute as regular query (prepared statements can be implemented later)
    log_this(designator, "DB2 prepared statement execution: Using regular query execution", LOG_LEVEL_DEBUG, 0);
    return db2_execute_query(connection, request, result);
}