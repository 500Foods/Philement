/*
 * PostgreSQL Database Engine - Prepared Statement Management Implementation
 *
 * Implements PostgreSQL prepared statement management functions.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/database/dbqueue/dbqueue.h>

// Local includes
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

    // Create prepared statement structure
    PreparedStatement* prepared_stmt = calloc(1, sizeof(PreparedStatement));
    if (!prepared_stmt) {
        // Cleanup with timeout protection
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

    // Add to connection's prepared statement array with LRU tracking
    // Since each thread owns its connection exclusively, no mutex needed

    // Get cache size from database configuration (default to 1000 if not set)
    size_t cache_size = 1000; // Default value
    if (connection->config && connection->config->prepared_statement_cache_size > 0) {
        cache_size = (size_t)connection->config->prepared_statement_cache_size;
    }

    // Initialize array if this is the first prepared statement
    if (!connection->prepared_statements) {
        connection->prepared_statements = calloc(cache_size, sizeof(PreparedStatement*));
        if (!connection->prepared_statements) {
            free(prepared_stmt->name);
            free(prepared_stmt->sql_template);
            free(prepared_stmt);
            // Cleanup with timeout protection
            char dealloc_query[256];
            snprintf(dealloc_query, sizeof(dealloc_query), "DEALLOCATE %s", name);
            void* dealloc_res = PQexec_ptr(pg_conn->connection, dealloc_query);
            if (dealloc_res) PQclear_ptr(dealloc_res);
            return false;
        }
        connection->prepared_statement_count = 0;
        // Initialize LRU counter
        if (!connection->prepared_statement_lru_counter) {
            connection->prepared_statement_lru_counter = calloc(cache_size, sizeof(uint64_t));
        }
    }

    // Check if cache is full - implement LRU eviction
    if (connection->prepared_statement_count >= cache_size) {
        // Find least recently used prepared statement (lowest LRU counter)
        size_t lru_index = 0;
        uint64_t min_lru_value = UINT64_MAX;

        for (size_t i = 0; i < connection->prepared_statement_count; i++) {
            if (connection->prepared_statement_lru_counter[i] < min_lru_value) {
                min_lru_value = connection->prepared_statement_lru_counter[i];
                lru_index = i;
            }
        }

        // Evict the LRU prepared statement
        PreparedStatement* evicted_stmt = connection->prepared_statements[lru_index];
        if (evicted_stmt) {
            // Clean up the evicted statement - deallocate from database
            char dealloc_query[256];
            snprintf(dealloc_query, sizeof(dealloc_query), "DEALLOCATE %s", evicted_stmt->name);
            void* dealloc_res = PQexec_ptr(pg_conn->connection, dealloc_query);
            if (dealloc_res) PQclear_ptr(dealloc_res);

            free(evicted_stmt->name);
            free(evicted_stmt->sql_template);
            free(evicted_stmt);
        }

        // Shift remaining elements to fill the gap
        for (size_t i = lru_index; i < connection->prepared_statement_count - 1; i++) {
            connection->prepared_statements[i] = connection->prepared_statements[i + 1];
            connection->prepared_statement_lru_counter[i] = connection->prepared_statement_lru_counter[i + 1];
        }

        connection->prepared_statement_count--;
        log_this(SR_DATABASE, "Evicted LRU prepared statement to make room for: %s", LOG_LEVEL_TRACE, 1, name);
    }

    // Store the new prepared statement at the end
    size_t new_index = connection->prepared_statement_count;
    connection->prepared_statements[new_index] = prepared_stmt;

    // Update LRU counter for this statement (global counter for recency tracking)
    static uint64_t global_lru_counter = 0;
    connection->prepared_statement_lru_counter[new_index] = ++global_lru_counter;
    connection->prepared_statement_count++;

    *stmt = prepared_stmt;

    log_this(SR_DATABASE, "PostgreSQL prepared statement created and added to connection", LOG_LEVEL_TRACE, 0);
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

    // Remove from connection's prepared statement array
    // Since each thread owns its connection exclusively, no mutex needed
    for (size_t i = 0; i < connection->prepared_statement_count; i++) {
        if (connection->prepared_statements[i] == stmt) {
            // Shift remaining elements
            for (size_t j = i; j < connection->prepared_statement_count - 1; j++) {
                connection->prepared_statements[j] = connection->prepared_statements[j + 1];
            }
            connection->prepared_statement_count--;
            break;
        }
    }

    // Free statement structure
    free(stmt->name);
    free(stmt->sql_template);
    free(stmt);

    log_this(SR_DATABASE, "PostgreSQL prepared statement removed", LOG_LEVEL_TRACE, 0);
    return true;
}