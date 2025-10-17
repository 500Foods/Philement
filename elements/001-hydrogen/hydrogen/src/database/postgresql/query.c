/*
 * PostgreSQL Database Engine - Query Execution Implementation
 *
 * Implements PostgreSQL query execution functions.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/database/dbqueue/dbqueue.h>

// Local includes
#include "types.h"
#include "connection.h"
#include "query.h"

// External declarations for libpq function pointers (defined in connection.c)
extern PQexec_t PQexec_ptr;
extern PQresultStatus_t PQresultStatus_ptr;
extern PQclear_t PQclear_ptr;
extern PQntuples_t PQntuples_ptr;
extern PQnfields_t PQnfields_ptr;
extern PQfname_t PQfname_ptr;
extern PQgetvalue_t PQgetvalue_ptr;
extern PQcmdTuples_t PQcmdTuples_ptr;
extern PQerrorMessage_t PQerrorMessage_ptr;
extern PQexecPrepared_t PQexecPrepared_ptr;

// External declarations for constants (defined in connection.c)
extern bool check_timeout_expired(time_t start_time, int timeout_seconds);

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

    // Execute the query (now with PostgreSQL-level timeout protection)
    time_t query_start_time = time(NULL);
    void* pg_result = PQexec_ptr(pg_conn->connection, request->sql_template);

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
        log_this(designator, "PostgreSQL query execution failed - status: %d", LOG_LEVEL_ERROR, 1, result_status);
        char* error_msg = PQerrorMessage_ptr(pg_conn->connection);
        if (error_msg && strlen(error_msg) > 0) {
            log_this(designator, "PostgreSQL query error: %s", LOG_LEVEL_ERROR, 1, error_msg);
        }
        PQclear_ptr(pg_result);
        return false;
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
        size_t json_size = 1024 * db_result->row_count; // Estimate size
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
            strcat(db_result->data_json, "{");

            for (size_t col = 0; col < db_result->column_count; col++) {
                if (col > 0) strcat(db_result->data_json, ",");
                const char* value = PQgetvalue_ptr(pg_result, (int)row, (int)col);
                char buffer[256];
                snprintf(buffer, sizeof(buffer), "\"%s\":\"%s\"",
                        db_result->column_names[col], value ? value : "");
                strcat(db_result->data_json, buffer);

                // Log the actual data values for debugging
                // log_this(designator, "PostgreSQL execute_query: Row %zu, Column %zu (%s,4,3,2,1,0): %s", LOG_LEVEL_TRACE, 4,
                //     row,
                //     col,
                //     db_result->column_names[col],
                //     value ? value : "NULL");
            }
            strcat(db_result->data_json, "}");
        }
        strcat(db_result->data_json, "]");

        // log_this(designator, "PostgreSQL execute_query: Complete result JSON: %s", LOG_LEVEL_DEBUG, 1, db_result->data_json);
    } else {
        log_this(designator, "PostgreSQL execute_query: Query returned no data (0 rows or 0 columns)", LOG_LEVEL_TRACE, 0);
    }

    PQclear_ptr(pg_result);
    *result = db_result;

    // log_this(SR_DATABASE, "PostgreSQL query executed successfully", LOG_LEVEL_TRACE, 0);
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

    // Execute the prepared statement using PQexecPrepared API
    time_t query_start_time = time(NULL);
    void* pg_result = NULL;

    if (PQexecPrepared_ptr) {
        // Use true prepared statement execution with PQexecPrepared
        // For migration queries with no parameters, use nParams=0 and NULL parameter arrays
        log_this(designator, "PostgreSQL execute_prepared: Using PQexecPrepared API", LOG_LEVEL_TRACE, 0);
        pg_result = PQexecPrepared_ptr(pg_conn->connection, stmt->name, 0, NULL, NULL, NULL, 0);
    } else {
        // Fallback to simple execution if PQexecPrepared is not available
        log_this(designator, "PostgreSQL execute_prepared: Falling back to PQexec (PQexecPrepared not available)", LOG_LEVEL_TRACE, 0);
        pg_result = PQexec_ptr(pg_conn->connection, stmt->name);
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
        if (error_msg && strlen(error_msg) > 0) {
            log_this(designator, "PostgreSQL prepared statement error: %s", LOG_LEVEL_ERROR, 1, error_msg);
        }
        PQclear_ptr(pg_result);
        return false;
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
        size_t json_size = 1024 * db_result->row_count; // Estimate size
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
            strcat(db_result->data_json, "{");

            for (size_t col = 0; col < db_result->column_count; col++) {
                if (col > 0) strcat(db_result->data_json, ",");
                const char* value = PQgetvalue_ptr(pg_result, (int)row, (int)col);
                char buffer[256];
                snprintf(buffer, sizeof(buffer), "\"%s\":\"%s\"",
                        db_result->column_names[col], value ? value : "");
                strcat(db_result->data_json, buffer);
            }
            strcat(db_result->data_json, "}");
        }
        strcat(db_result->data_json, "]");
    } else {
        log_this(designator, "PostgreSQL execute_prepared: Query returned no data (0 rows or 0 columns)", LOG_LEVEL_TRACE, 0);
    }

    PQclear_ptr(pg_result);
    *result = db_result;

    log_this(designator, "PostgreSQL execute_prepared: Prepared statement executed successfully", LOG_LEVEL_TRACE, 0);
    return true;
}