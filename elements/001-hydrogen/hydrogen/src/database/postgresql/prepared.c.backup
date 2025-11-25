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

// Forward declarations for utility functions
bool postgresql_initialize_prepared_statement_cache(DatabaseHandle* connection, size_t cache_size);
bool postgresql_evict_lru_prepared_statement(DatabaseHandle* connection, const PostgresConnection* pg_conn, const char* new_stmt_name);
bool postgresql_add_prepared_statement_to_cache(DatabaseHandle* connection, PreparedStatement* stmt, size_t cache_size);

// Utility functions for prepared statement cache management
bool postgresql_initialize_prepared_statement_cache(DatabaseHandle* connection, size_t cache_size) {
    if (!connection) return false;

    // Initialize prepared statement array
    connection->prepared_statements = calloc(cache_size, sizeof(PreparedStatement*));
    if (!connection->prepared_statements) {
        return false;
    }

    // Initialize LRU counter array
    connection->prepared_statement_lru_counter = calloc(cache_size, sizeof(uint64_t));
    if (!connection->prepared_statement_lru_counter) {
        free(connection->prepared_statements);
        connection->prepared_statements = NULL;
        return false;
    }

    connection->prepared_statement_count = 0;
    return true;
}

bool postgresql_evict_lru_prepared_statement(DatabaseHandle* connection, const PostgresConnection* pg_conn, const char* new_stmt_name) {
    if (!connection || !pg_conn || !pg_conn->connection) return false;

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
        // PostgreSQL automatically deallocates prepared statements when connection closes
        // No explicit DEALLOCATE needed during LRU eviction
        free(evicted_stmt->name);
        free(evicted_stmt->sql_template);
        free(evicted_stmt);
    }

    // Shift remaining elements to fill the gap
    for (size_t i = lru_index; i < connection->prepared_statement_count - 1; i++) {
        connection->prepared_statements[i] = connection->prepared_statements[i + 1];
        connection->prepared_statement_lru_counter[i] = connection->prepared_statement_lru_counter[i + 1];
    }

    // NULL out the last element to prevent dangling pointer issues
    connection->prepared_statements[connection->prepared_statement_count - 1] = NULL;
    if (connection->prepared_statement_lru_counter) {
        connection->prepared_statement_lru_counter[connection->prepared_statement_count - 1] = 0;
    }

    connection->prepared_statement_count--;
    log_this(SR_DATABASE, "Evicted LRU prepared statement to make room for: %s", LOG_LEVEL_TRACE, 1, new_stmt_name);
    return true;
}

bool postgresql_add_prepared_statement_to_cache(DatabaseHandle* connection, PreparedStatement* stmt, size_t cache_size) {
    if (!connection || !stmt) return false;

    // Check if cache is full - implement LRU eviction if needed
    if (connection->prepared_statement_count >= cache_size) {
        const PostgresConnection* pg_conn = (const PostgresConnection*)connection->connection_handle;
        if (!postgresql_evict_lru_prepared_statement(connection, pg_conn, stmt->name)) {
            return false;
        }
    }

    // Store the new prepared statement at the end
    size_t new_index = connection->prepared_statement_count;
    connection->prepared_statements[new_index] = stmt;

    // Update LRU counter for this statement (global counter for recency tracking)
    static uint64_t global_lru_counter = 0;
    connection->prepared_statement_lru_counter[new_index] = ++global_lru_counter;
    connection->prepared_statement_count++;

    return true;
}

// Prepared Statement Management Functions
bool postgresql_prepare_statement(DatabaseHandle* connection, const char* name, const char* sql, PreparedStatement** stmt) {
    if (!connection || !name || !sql || !stmt || connection->engine_type != DB_ENGINE_POSTGRESQL) {
        return false;
    }

    const PostgresConnection* pg_conn = (const PostgresConnection*)connection->connection_handle;
    if (!pg_conn || !pg_conn->connection) {
        return false;
    }

    // Note: Timeout is now set once per connection in postgresql_connect()
    // No need to set it before each PREPARE operation

    // Prepare the statement (timeout already set at connection level)
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
        // PostgreSQL automatically deallocates prepared statements when connection closes
        // No explicit DEALLOCATE needed in error cleanup
        return false;
    }

    prepared_stmt->name = strdup(name);
    prepared_stmt->sql_template = strdup(sql);
    prepared_stmt->created_at = time(NULL);
    prepared_stmt->usage_count = 0;

    // Add to connection's prepared statement array with LRU tracking
    // Since each thread owns its connection exclusively, no mutex needed

    // Get cache size from database configuration
    size_t cache_size = (size_t)connection->config->prepared_statement_cache_size;

    // Initialize cache if this is the first prepared statement
    if (!connection->prepared_statements) {
        if (!postgresql_initialize_prepared_statement_cache(connection, cache_size)) {
            free(prepared_stmt->name);
            free(prepared_stmt->sql_template);
            free(prepared_stmt);
            // PostgreSQL automatically deallocates prepared statements when connection closes
            // No explicit DEALLOCATE needed in error cleanup
            return false;
        }
    }

    // NOTE: Do NOT add to cache here - database_engine_execute() will handle caching
    // via store_prepared_statement() to avoid double-caching
    *stmt = prepared_stmt;

    log_this(connection->designator ? connection->designator : SR_DATABASE,
             "PostgreSQL prepared statement created and added to connection", LOG_LEVEL_TRACE, 0);
    return true;
}

bool postgresql_unprepare_statement(DatabaseHandle* connection, PreparedStatement* stmt) {
    if (!connection || !stmt || connection->engine_type != DB_ENGINE_POSTGRESQL) {
        return false;
    }

    const PostgresConnection* pg_conn = (const PostgresConnection*)connection->connection_handle;
    if (!pg_conn || !pg_conn->connection) {
        return false;
    }

    // Execute DEALLOCATE to explicitly deallocate the prepared statement
    char deallocate_query[256];
    snprintf(deallocate_query, sizeof(deallocate_query), "DEALLOCATE %s", stmt->name);

    time_t start_time = time(NULL);
    void* res = PQexec_ptr(pg_conn->connection, deallocate_query);

    // Check if deallocate took too long
    if (check_timeout_expired(start_time, 15)) {
        log_this(SR_DATABASE, "PostgreSQL DEALLOCATE execution time exceeded 15 seconds", LOG_LEVEL_ERROR, 0);
        if (res) PQclear_ptr(res);
        return false;
    }

    if (PQresultStatus_ptr(res) != PGRES_COMMAND_OK) {
        log_this(SR_DATABASE, "PostgreSQL DEALLOCATE failed", LOG_LEVEL_TRACE, 0);
        log_this(SR_DATABASE, PQerrorMessage_ptr(pg_conn->connection), LOG_LEVEL_TRACE, 0);
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
            // NULL out the last element to prevent dangling pointer issues
            connection->prepared_statements[connection->prepared_statement_count - 1] = NULL;
            if (connection->prepared_statement_lru_counter) {
                connection->prepared_statement_lru_counter[connection->prepared_statement_count - 1] = 0;
            }
            connection->prepared_statement_count--;
            break;
        }
    }

    // Free statement structure
    free(stmt->name);
    free(stmt->sql_template);
    free(stmt);


    log_this(connection->designator ? connection->designator : SR_DATABASE,
             "PostgreSQL prepared statement deallocated and removed", LOG_LEVEL_TRACE, 0);
    return true;
}