/*
  * MySQL Database Engine - Query Execution Implementation
  *
  * Implements MySQL query execution functions.
  */
#include "../../hydrogen.h"
#include "../database.h"
#include "../database_queue.h"
#include "types.h"
#include "connection.h"
#include "query.h"

// External declarations for libmysqlclient function pointers (defined in connection.c)
extern mysql_store_result_t mysql_store_result_ptr;
extern mysql_num_rows_t mysql_num_rows_ptr;
extern mysql_num_fields_t mysql_num_fields_ptr;
extern mysql_fetch_row_t mysql_fetch_row_ptr;
extern mysql_fetch_fields_t mysql_fetch_fields_ptr;
extern mysql_free_result_t mysql_free_result_ptr;
extern mysql_error_t mysql_error_ptr;
extern mysql_query_t mysql_query_ptr;

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

    log_this(designator, "mysql_execute_query: Parameters validated, proceeding", LOG_LEVEL_TRACE, 0);

    // cppcheck-suppress constVariablePointer
    // Justification: MySQL API requires non-const MYSQL* connection handle
    const MySQLConnection* mysql_conn = (const MySQLConnection*)connection->connection_handle;
    if (!mysql_conn || !mysql_conn->connection) {
        log_this(designator, "MySQL execute_query: Invalid connection handle", LOG_LEVEL_ERROR, 0);
        return false;
    }

    log_this(designator, "MySQL execute_query: Executing query: %s", LOG_LEVEL_TRACE, 1, request->sql_template);

    // Execute query
    if (mysql_query_ptr(mysql_conn->connection, request->sql_template) != 0) {
        log_this(designator, "MySQL query execution failed", LOG_LEVEL_ERROR, 0);
        if (mysql_error_ptr) {
            const char* error_msg = mysql_error_ptr(mysql_conn->connection);
            if (error_msg && strlen(error_msg) > 0) {
                log_this(designator, "MySQL query error: %s", LOG_LEVEL_ERROR, 1, error_msg);
            }
        }
        return false;
    }

    // Store result
    void* mysql_result = NULL;
    if (mysql_store_result_ptr) {
        mysql_result = mysql_store_result_ptr(mysql_conn->connection);
        if (!mysql_result) {
            log_this(designator, "MySQL execute_query: No result set returned", LOG_LEVEL_TRACE, 0);
        }
    }

    // Create result structure
    QueryResult* db_result = calloc(1, sizeof(QueryResult));
    if (!db_result) {
        if (mysql_result && mysql_free_result_ptr) {
            mysql_free_result_ptr(mysql_result);
        }
        return false;
    }

    db_result->success = true;
    db_result->execution_time_ms = 0; // TODO: Implement timing

    // Get result metadata
    if (mysql_result) {
        if (mysql_num_rows_ptr && mysql_num_fields_ptr) {
            db_result->row_count = (size_t)mysql_num_rows_ptr(mysql_result);
            db_result->column_count = (size_t)mysql_num_fields_ptr(mysql_result);


            // Extract column names
            if (db_result->column_count > 0 && mysql_fetch_fields_ptr) {
                const void* fields = mysql_fetch_fields_ptr(mysql_result);
                if (fields) {
                    db_result->column_names = calloc(db_result->column_count, sizeof(char*));
                    if (db_result->column_names) {
                        // MySQL field structure: we need to access field names
                        // This is a simplified approach - in practice we'd need proper field structure handling
                        for (size_t i = 0; i < db_result->column_count; i++) {
                            // Placeholder - need proper field name extraction
                            char col_name[32];
                            snprintf(col_name, sizeof(col_name), "col_%zu", i);
                            db_result->column_names[i] = strdup(col_name);
                        }
                    }
                }
            }

            // Convert result to JSON
            if (db_result->row_count > 0 && db_result->column_count > 0 && mysql_fetch_row_ptr) {
                size_t json_size = 1024 * db_result->row_count;
                db_result->data_json = calloc(1, json_size);
                if (db_result->data_json) {
                    strcpy(db_result->data_json, "[");
                    for (size_t row = 0; row < db_result->row_count; row++) {
                        if (row > 0) strcat(db_result->data_json, ",");

                        const void* row_data = mysql_fetch_row_ptr(mysql_result);
                        if (row_data) {
                            strcat(db_result->data_json, "{");
                            // Simplified - in practice need to handle row data properly
                            for (size_t col = 0; col < db_result->column_count; col++) {
                                if (col > 0) strcat(db_result->data_json, ",");
                                char buffer[256];
                                // Placeholder - need proper row data extraction
                                snprintf(buffer, sizeof(buffer), "\"%s\":\"value_%zu_%zu\"",
                                    db_result->column_names ? db_result->column_names[col] : "unknown", row, col);
                                strcat(db_result->data_json, buffer);
                            }
                            strcat(db_result->data_json, "}");
                        }
                    }
                    strcat(db_result->data_json, "]");

                    log_this(designator, "MySQL execute_query: Generated result JSON", LOG_LEVEL_TRACE, 0);
                }
            } else {
                db_result->data_json = strdup("[]");
                log_this(designator, "MySQL execute_query: Query returned no data", LOG_LEVEL_TRACE, 0);
            }
        }

        // Free result
        if (mysql_free_result_ptr) {
            mysql_free_result_ptr(mysql_result);
        }
    } else {
        // No result set (e.g., INSERT, UPDATE, DELETE)
        db_result->row_count = 0;
        db_result->column_count = 0;
        db_result->data_json = strdup("[]");
        // TODO: Get affected rows from mysql_affected_rows if available
        db_result->affected_rows = 0;
    }

    *result = db_result;

    log_this(designator, "MySQL execute_query: Query completed successfully", LOG_LEVEL_TRACE, 0);
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

    // For now, execute the SQL directly since MySQL prepared statement infrastructure is not fully loaded
    // TODO: Implement proper MySQL prepared statement execution when mysql_stmt_* functions are available
    log_this(designator, "MySQL execute_prepared: Executing SQL directly (prepared statements not fully implemented)", LOG_LEVEL_TRACE, 0);

    // Create a temporary QueryRequest with the prepared statement's SQL
    QueryRequest temp_request = *request;
    temp_request.sql_template = stmt->sql_template;
    temp_request.use_prepared_statement = false;
    temp_request.prepared_statement_name = NULL;

    return mysql_execute_query(connection, &temp_request, result);
}