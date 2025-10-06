/*
 * PostgreSQL Database Engine - Prepared Statement Management Implementation
 *
 * Implements PostgreSQL prepared statement management functions.
 */

 #include "../../hydrogen.h"
#include "../database.h"
#include "../database_queue.h"
#include "types.h"
#include "connection.h"
#include "prepared.h"

// External declarations for libpq function pointers (defined in connection.c)
extern PQprepare_t PQprepare_ptr;
extern PQexec_t PQexec_ptr;
extern PQresultStatus_t PQresultStatus_ptr;
extern PQclear_t PQclear_ptr;
extern PQerrorMessage_t PQerrorMessage_ptr;


// Prepared Statement Management Functions
bool postgresql_prepare_statement(DatabaseHandle* connection, const char* name, const char* sql, PreparedStatement** stmt) {
    if (!connection || !name || !sql || !stmt || connection->engine_type != DB_ENGINE_POSTGRESQL) {
        return false;
    }

    PostgresConnection* pg_conn = (PostgresConnection*)connection->connection_handle;
    if (!pg_conn || !pg_conn->connection) {
        return false;
    }

    // Set timeout for prepare operation
    void* timeout_result = PQexec_ptr(pg_conn->connection, "SET statement_timeout = 15000"); // 15 seconds
    if (timeout_result) {
        PQclear_ptr(timeout_result);
    }

    // Prepare the statement with timeout protection
    time_t start_time = time(NULL);
    void* res = PQprepare_ptr(pg_conn->connection, name, sql, 0, NULL);

    // Check if prepare took too long
    if (check_timeout_expired(start_time, 15)) {
        log_this(SR_DATABASE, "PostgreSQL PREPARE execution time exceeded 15 seconds", LOG_LEVEL_ERROR, 0);
        if (res) PQclear_ptr(res);
        return false;
    }

    if (PQresultStatus_ptr(res) != PGRES_COMMAND_OK) {
        log_this(SR_DATABASE, "PostgreSQL PREPARE failed", LOG_LEVEL_ERROR, 0);
        log_this(SR_DATABASE, PQerrorMessage_ptr(pg_conn->connection), LOG_LEVEL_ERROR, 0);
        PQclear_ptr(res);
        return false;
    }
    PQclear_ptr(res);

    // Add to cache
    if (!add_prepared_statement(pg_conn->prepared_statements, name)) {
        // Deallocate the prepared statement with timeout protection
        char dealloc_query[256];
        snprintf(dealloc_query, sizeof(dealloc_query), "DEALLOCATE %s", name);

        {
            // Set timeout for deallocate operation
            void* dealloc_timeout_result = PQexec_ptr(pg_conn->connection, "SET statement_timeout = 5000"); // 5 seconds
            if (dealloc_timeout_result) {
                PQclear_ptr(dealloc_timeout_result);
            }

            time_t dealloc_start = time(NULL);
            void* dealloc_res = PQexec_ptr(pg_conn->connection, dealloc_query);

            // Check if dealloc took too long
            if (check_timeout_expired(dealloc_start, 5)) {
                log_this(SR_DATABASE, "PostgreSQL DEALLOCATE on failure execution time exceeded 5 seconds", LOG_LEVEL_ERROR, 0);
            }

            if (dealloc_res) PQclear_ptr(dealloc_res);
        }
        return false;
    }

    // Create prepared statement structure
    PreparedStatement* prepared_stmt = calloc(1, sizeof(PreparedStatement));
    if (!prepared_stmt) {
        // Cleanup with timeout protection
        remove_prepared_statement(pg_conn->prepared_statements, name);
        char dealloc_query[256];
        snprintf(dealloc_query, sizeof(dealloc_query), "DEALLOCATE %s", name);

        {
            // Set timeout for deallocate operation
            void* stmt_timeout_result = PQexec_ptr(pg_conn->connection, "SET statement_timeout = 5000"); // 5 seconds
            if (stmt_timeout_result) {
                PQclear_ptr(stmt_timeout_result);
            }

            time_t dealloc_start = time(NULL);
            void* dealloc_res = PQexec_ptr(pg_conn->connection, dealloc_query);

            // Check if dealloc took too long
            if (check_timeout_expired(dealloc_start, 5)) {
                log_this(SR_DATABASE, "PostgreSQL DEALLOCATE on prepared statement failure execution time exceeded 5 seconds", LOG_LEVEL_ERROR, 0);
            }

            if (dealloc_res) PQclear_ptr(dealloc_res);
        }
        return false;
    }

    prepared_stmt->name = strdup(name);
    prepared_stmt->sql_template = strdup(sql);
    prepared_stmt->created_at = time(NULL);
    prepared_stmt->usage_count = 0;

    *stmt = prepared_stmt;

    log_this(SR_DATABASE, "PostgreSQL prepared statement created", LOG_LEVEL_TRACE, 0);
    return true;
}

bool postgresql_unprepare_statement(DatabaseHandle* connection, PreparedStatement* stmt) {
    if (!connection || !stmt || connection->engine_type != DB_ENGINE_POSTGRESQL) {
        return false;
    }

    PostgresConnection* pg_conn = (PostgresConnection*)connection->connection_handle;
    if (!pg_conn || !pg_conn->connection) {
        return false;
    }

    // Deallocate from database with timeout protection
    char dealloc_query[256];
    snprintf(dealloc_query, sizeof(dealloc_query), "DEALLOCATE %s", stmt->name);

    // Set timeout for deallocate operation
    void* timeout_result = PQexec_ptr(pg_conn->connection, "SET statement_timeout = 10000"); // 10 seconds
    if (timeout_result) {
        PQclear_ptr(timeout_result);
    }

    time_t start_time = time(NULL);
    void* res = PQexec_ptr(pg_conn->connection, dealloc_query);

    // Check if dealloc took too long
    if (check_timeout_expired(start_time, 10)) {
        log_this(SR_DATABASE, "PostgreSQL DEALLOCATE execution time exceeded 10 seconds", LOG_LEVEL_ERROR, 0);
        if (res) PQclear_ptr(res);
        return false;
    }

    if (PQresultStatus_ptr(res) != PGRES_COMMAND_OK) {
        log_this(SR_DATABASE, "PostgreSQL DEALLOCATE failed", LOG_LEVEL_ERROR, 0);
        PQclear_ptr(res);
        return false;
    }
    PQclear_ptr(res);

    // Remove from cache
    remove_prepared_statement(pg_conn->prepared_statements, stmt->name);

    // Free statement structure
    free(stmt->name);
    free(stmt->sql_template);
    free(stmt);

    log_this(SR_DATABASE, "PostgreSQL prepared statement removed", LOG_LEVEL_TRACE, 0);
    return true;
}