/*
  * SQLite Database Engine - Query Execution Implementation
  *
  * Implements SQLite query execution functions.
  */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/database/dbqueue/dbqueue.h>

// Local includes
#include "types.h"
#include "query.h"
#include "query_helpers.h"

// Forward declaration for numeric value checking
bool sqlite_is_numeric_value(const char* value);

// External declarations for libsqlite3 function pointers (defined in connection.c)
extern sqlite3_exec_t sqlite3_exec_ptr;
extern sqlite3_prepare_v2_t sqlite3_prepare_v2_ptr;
extern sqlite3_step_t sqlite3_step_ptr;
extern sqlite3_finalize_t sqlite3_finalize_ptr;
extern sqlite3_column_count_t sqlite3_column_count_ptr;
extern sqlite3_column_name_t sqlite3_column_name_ptr;
extern sqlite3_column_text_t sqlite3_column_text_ptr;
extern sqlite3_column_type_t sqlite3_column_type_ptr;
extern sqlite3_changes_t sqlite3_changes_ptr;
extern sqlite3_reset_t sqlite3_reset_ptr;
extern sqlite3_errmsg_t sqlite3_errmsg_ptr;

// Callback function for sqlite3_exec
// Note: This callback doesn't have access to column types, so all values are quoted as strings
// For type-aware JSON serialization, use prepared statements instead
int sqlite_exec_callback(void* data, int argc, char** argv, char** col_names) {
    QueryResult* result = (QueryResult*)data;

    // If this is the first row, set up column names
    if (result->row_count == 0 && result->column_count == 0) {
        result->column_count = (size_t)argc;
        if (result->column_count > 0) {
            result->column_names = calloc(result->column_count, sizeof(char*));
            if (result->column_names) {
                for (int i = 0; i < argc; i++) {
                    result->column_names[i] = col_names[i] ? strdup(col_names[i]) : strdup("");
                }
            }
        }
    }

    // Build JSON for this row with dynamic buffering
    if (result->row_count == 0) {
        result->data_json = calloc(1, 16384); // Larger initial allocation for query templates
        if (result->data_json) {
            strcpy(result->data_json, "[");
        }
    } else {
        // Add comma between rows
        size_t current_len = strlen(result->data_json);
        size_t needed = current_len + 2;
        // Check if we need to grow buffer
        if (needed > 16384) {
            char* new_json = realloc(result->data_json, needed + 16384);
            if (new_json) {
                result->data_json = new_json;
            }
        }
        if (result->data_json) {
            strcat(result->data_json, ",");
        }
    }

    if (result->data_json) {
        size_t row_start_pos = strlen(result->data_json);
        
        strcat(result->data_json, "{");
        for (int i = 0; i < argc; i++) {
            if (i > 0) strcat(result->data_json, ",");
            
            const char* col_name = result->column_names ? result->column_names[i] : "";
            const char* value = argv[i];
            
            // Calculate needed size for this column
            size_t value_len = value ? strlen(value) : 0;
            size_t needed_size = strlen(col_name) + (value_len * 2) + 20;
            
            // Ensure buffer has enough space
            size_t current_len = strlen(result->data_json);
            size_t total_needed = current_len + needed_size;
            if (total_needed > 16384) {
                char* new_json = realloc(result->data_json, total_needed + 1024);
                if (!new_json) continue; // Skip column on allocation failure
                result->data_json = new_json;
            }
            
            // Append column to JSON (always quoted since we don't have type info in callback)
            char* append_pos = result->data_json + current_len;
            
            if (value == NULL) {
                sprintf(append_pos, "\"%s\":null", col_name);
            } else {
                // Check if value should be treated as numeric (for SQLite's dynamic typing)
                bool is_numeric_value = sqlite_is_numeric_value(value);

                if (is_numeric_value && strlen(value) > 0) {
                    // Numeric value - no quotes around value
                    sprintf(append_pos, "\"%s\":%s", col_name, value);
                } else {
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
                }
            }
        }
        strcat(result->data_json, "}");
        
        // Debug: Log first row JSON for diagnosis
        if (result->row_count == 0) {
            size_t row_len = strlen(result->data_json) - row_start_pos;
            char* row_json = strndup(result->data_json + row_start_pos, row_len);
            if (row_json) {
                log_this(SR_DATABASE, "SQLite first row JSON: %s", LOG_LEVEL_DEBUG, 1, row_json);
                free(row_json);
            }
        }
    }

    result->row_count++;
    return 0; // Continue processing
}

// Query Execution Functions
bool sqlite_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result) {
    if (!connection || !request || !result || connection->engine_type != DB_ENGINE_SQLITE) {
        const char* designator = connection ? (connection->designator ? connection->designator : SR_DATABASE) : SR_DATABASE;
        log_this(designator, "SQLite execute_query: Invalid parameters", LOG_LEVEL_ERROR, 0);
        return false;
    }

    const char* designator = connection->designator ? connection->designator : SR_DATABASE;
    log_this(designator, "sqlite_execute_query: ENTER - connection=%p, request=%p, result=%p", LOG_LEVEL_TRACE, 3, (void*)connection, (void*)request, (void*)result);

    log_this(designator, "sqlite_execute_query: Parameters validated, proceeding", LOG_LEVEL_TRACE, 0);

    const SQLiteConnection* sqlite_conn = (const SQLiteConnection*)connection->connection_handle;
    if (!sqlite_conn || !sqlite_conn->db) {
        log_this(designator, "SQLite execute_query: Invalid connection handle", LOG_LEVEL_ERROR, 0);
        return false;
    }

    log_this(designator, "SQLite execute_query: Executing query: %s", LOG_LEVEL_TRACE, 1, request->sql_template);

    // Create result structure
    QueryResult* db_result = calloc(1, sizeof(QueryResult));
    if (!db_result) {
        return false;
    }

    db_result->success = false;

    // Start timing before query execution
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    // Execute query using sqlite3_exec for simple queries
    char* error_msg = NULL;
    int exec_result = sqlite3_exec_ptr(sqlite_conn->db, request->sql_template, sqlite_exec_callback, db_result, &error_msg);

    if (exec_result != SQLITE_OK) {
        log_this(designator, "SQLite query execution failed - result: %d", LOG_LEVEL_TRACE, 1, exec_result);
        if (error_msg) {
            log_this(designator, "SQLite query error: %s", LOG_LEVEL_TRACE, 1, error_msg);
            // Note: sqlite3_free is a macro that calls free() in most implementations
            free(error_msg);
        }
        free(db_result->data_json);
        free(db_result->column_names);
        free(db_result);
        return false;
    }

    if (error_msg) {
        // Note: sqlite3_free is a macro that calls free() in most implementations
        free(error_msg);
    }

    // Close JSON array
    if (db_result->data_json) {
        size_t len = strlen(db_result->data_json);
        if (len > 0 && db_result->data_json[len-1] != '[') {
            db_result->data_json = realloc(db_result->data_json, len + 2);
            if (db_result->data_json) {
                strcat(db_result->data_json, "]");
            }
        } else {
            // Empty result set
            strcpy(db_result->data_json, "[]");
        }
    } else {
        db_result->data_json = strdup("[]");
    }

    // Get affected rows if available
    if (sqlite3_changes_ptr) {
        db_result->affected_rows = (size_t)sqlite3_changes_ptr(sqlite_conn->db);
    } else {
        db_result->affected_rows = 0;
    }

    // End timing after all result processing is complete
    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    db_result->execution_time_ms = (end_time.tv_sec - start_time.tv_sec) * 1000 +
                                     (end_time.tv_nsec - start_time.tv_nsec) / 1000000;

    db_result->success = true;

    *result = db_result;

    log_this(designator, "SQLite execute_query: Query completed successfully", LOG_LEVEL_DEBUG, 0);
    return true;
}

    // cppcheck-suppress constParameterPointer
    // Justification: Database engine interface requires non-const QueryRequest* parameter
bool sqlite_execute_prepared(DatabaseHandle* connection, const PreparedStatement* stmt, QueryRequest* request, QueryResult** result) {
    if (!connection || !stmt || !request || !result || connection->engine_type != DB_ENGINE_SQLITE) {
        return false;
    }

    const char* designator = connection->designator ? connection->designator : SR_DATABASE;

    const SQLiteConnection* sqlite_conn = (const SQLiteConnection*)connection->connection_handle;
    if (!sqlite_conn || !sqlite_conn->db) {
        return false;
    }

    // Get the prepared statement handle
    void* stmt_handle = stmt->engine_specific_handle;
    if (!stmt_handle) {
        // Statement had no executable SQL (e.g., only comments after macro processing)
        // Return successful empty result instead of error
        log_this(designator, "SQLite prepared statement: No executable SQL (statement was not actionable)", LOG_LEVEL_DEBUG, 0);
        
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
    if (!sqlite3_step_ptr || !sqlite3_column_count_ptr || !sqlite3_column_name_ptr ||
        !sqlite3_column_text_ptr || !sqlite3_column_type_ptr || !sqlite3_reset_ptr) {
        log_this(designator, "SQLite prepared statement execution: Required functions not available", LOG_LEVEL_ERROR, 0);
        return false;
    }

    log_this(designator, "SQLite prepared statement execution: Executing prepared statement", LOG_LEVEL_TRACE, 0);

    // Create result structure
    QueryResult* db_result = calloc(1, sizeof(QueryResult));
    if (!db_result) {
        return false;
    }

    db_result->success = false;

    // Start timing before query execution
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    // Get column count
    int column_count = sqlite3_column_count_ptr(stmt_handle);
    db_result->column_count = (size_t)column_count;

    // Get column names using helper function
    char** column_names = sqlite_get_column_names(stmt_handle, column_count);
    if (column_count > 0 && !column_names) {
        free(db_result);
        return false;
    }
    db_result->column_names = column_names;

    // Fetch all result rows
    size_t row_count = 0;
    size_t json_buffer_capacity = 1024;
    char* json_buffer = calloc(1, json_buffer_capacity);
    if (!json_buffer) {
        sqlite_cleanup_column_names(column_names, column_count);
        free(db_result);
        return false;
    }

    // Start JSON array
    strcpy(json_buffer, "[");
    size_t json_buffer_size = 1;

    // Execute and fetch rows using helper function
    int step_result;
    while ((step_result = sqlite3_step_ptr(stmt_handle)) == SQLITE_ROW) {
        bool first_row = (row_count == 0);
        if (!sqlite_fetch_row_data(stmt_handle, column_names, column_count,
                                   &json_buffer, &json_buffer_size, &json_buffer_capacity, first_row)) {
            free(json_buffer);
            sqlite_cleanup_column_names(column_names, column_count);
            free(db_result);
            sqlite3_reset_ptr(stmt_handle);
            return false;
        }
        row_count++;
    }

    // Check if step failed (other than SQLITE_DONE)
    if (step_result != SQLITE_DONE && step_result != SQLITE_ROW) {
        log_this(designator, "SQLite prepared statement execution failed - result: %d", LOG_LEVEL_ERROR, 1, step_result);
        if (sqlite3_errmsg_ptr) {
            const char* error_msg = sqlite3_errmsg_ptr(sqlite_conn->db);
            if (error_msg) {
                log_this(designator, "SQLite prepared statement error: %s", LOG_LEVEL_ERROR, 1, error_msg);
            }
        }
        free(json_buffer);
        sqlite_cleanup_column_names(column_names, column_count);
        free(db_result);
        sqlite3_reset_ptr(stmt_handle);
        return false;
    }

    // End JSON array using helper function
    if (!sqlite_ensure_json_buffer_capacity(&json_buffer, json_buffer_size, &json_buffer_capacity, 2)) {
        free(json_buffer);
        sqlite_cleanup_column_names(column_names, column_count);
        free(db_result);
        sqlite3_reset_ptr(stmt_handle);
        return false;
    }
    strcat(json_buffer, "]");

    db_result->row_count = row_count;
    db_result->data_json = json_buffer;

    // Get affected rows if available
    if (sqlite3_changes_ptr) {
        db_result->affected_rows = (size_t)sqlite3_changes_ptr(sqlite_conn->db);
    } else {
        db_result->affected_rows = 0;
    }

    // Reset statement for reuse
    sqlite3_reset_ptr(stmt_handle);

    // End timing after all result processing is complete
    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    db_result->execution_time_ms = (end_time.tv_sec - start_time.tv_sec) * 1000 +
                                     (end_time.tv_nsec - start_time.tv_nsec) / 1000000;

    db_result->success = true;

    *result = db_result;

    log_this(designator, "SQLite prepared statement execution: Query completed successfully", LOG_LEVEL_TRACE, 0);
    return true;
}