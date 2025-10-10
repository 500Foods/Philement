/*
  * SQLite Database Engine - Query Execution Implementation
  *
  * Implements SQLite query execution functions.
  */
#include "../../hydrogen.h"
#include "../database.h"
#include "../queue/database_queue.h"
#include "types.h"
#include "query.h"

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
extern sqlite3_errmsg_t sqlite3_errmsg_ptr;

// Callback function for sqlite3_exec
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

    // Build JSON for this row
    if (result->row_count == 0) {
        result->data_json = calloc(1, 1024); // Initial allocation
        if (result->data_json) {
            strcpy(result->data_json, "[");
        }
    } else {
        // Extend JSON string
        size_t current_len = strlen(result->data_json);
        result->data_json = realloc(result->data_json, current_len + 1024);
        if (result->data_json) {
            strcat(result->data_json, ",");
        }
    }

    if (result->data_json) {
        strcat(result->data_json, "{");
        for (int i = 0; i < argc; i++) {
            if (i > 0) strcat(result->data_json, ",");
            char buffer[256];
            const char* value = argv[i] ? argv[i] : "";
            snprintf(buffer, sizeof(buffer), "\"%s\":\"%s\"",
                    result->column_names ? result->column_names[i] : "", value);
            strcat(result->data_json, buffer);
        }
        strcat(result->data_json, "}");
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
    log_this(designator, "sqlite_execute_query: ENTER - connection=%p, request=%p, result=%p", LOG_LEVEL_DEBUG, 3, (void*)connection, (void*)request, (void*)result);

    log_this(designator, "sqlite_execute_query: Parameters validated, proceeding", LOG_LEVEL_DEBUG, 0);

    const SQLiteConnection* sqlite_conn = (const SQLiteConnection*)connection->connection_handle;
    if (!sqlite_conn || !sqlite_conn->db) {
        log_this(designator, "SQLite execute_query: Invalid connection handle", LOG_LEVEL_ERROR, 0);
        return false;
    }

    log_this(designator, "SQLite execute_query: Executing query: %s", LOG_LEVEL_DEBUG, 1, request->sql_template);

    // Create result structure
    QueryResult* db_result = calloc(1, sizeof(QueryResult));
    if (!db_result) {
        return false;
    }

    db_result->success = false;
    db_result->execution_time_ms = 0; // TODO: Implement timing

    // Execute query using sqlite3_exec for simple queries
    char* error_msg = NULL;
    int exec_result = sqlite3_exec_ptr(sqlite_conn->db, request->sql_template, sqlite_exec_callback, db_result, &error_msg);

    if (exec_result != SQLITE_OK) {
        log_this(designator, "SQLite query execution failed - result: %d", LOG_LEVEL_ERROR, 1, exec_result);
        if (error_msg) {
            log_this(designator, "SQLite query error: %s", LOG_LEVEL_ERROR, 1, error_msg);
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

    db_result->success = true;


    *result = db_result;

    log_this(designator, "SQLite execute_query: Query completed successfully", LOG_LEVEL_DEBUG, 0);
    return true;
}

bool sqlite_execute_prepared(DatabaseHandle* connection, const PreparedStatement* stmt, QueryRequest* request, QueryResult** result) {
    if (!connection || !stmt || !request || !result || connection->engine_type != DB_ENGINE_SQLITE) {
        return false;
    }

    const char* designator = connection->designator ? connection->designator : SR_DATABASE;

    const SQLiteConnection* sqlite_conn = (const SQLiteConnection*)connection->connection_handle;
    if (!sqlite_conn || !sqlite_conn->db) {
        return false;
    }

    // For now, execute as regular query
    // TODO: Implement proper prepared statement execution with parameter binding
    log_this(designator, "SQLite prepared statement execution: Using regular query execution for now", LOG_LEVEL_DEBUG, 0);
    return sqlite_execute_query(connection, request, result);
}