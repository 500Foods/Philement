/*
 * SQLite Database Engine - Prepared Statement Management Implementation
 *
 * Implements SQLite prepared statement management functions with LRU cache and statistics.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/database/dbqueue/dbqueue.h>

 /* Local includes */
#include "types.h"
#include "connection.h"
#include "prepared.h"

// External declarations for sqlite3 function pointers (defined in connection.c)
extern sqlite3_prepare_v2_t sqlite3_prepare_v2_ptr;
extern sqlite3_finalize_t sqlite3_finalize_ptr;
extern sqlite3_errmsg_t sqlite3_errmsg_ptr;

// Forward declarations for utility functions
bool sqlite_evict_lru_prepared_statement(DatabaseHandle* connection, const SQLiteConnection* sqlite_conn, const char* new_stmt_name);
bool sqlite_add_prepared_statement_to_cache(DatabaseHandle* connection, PreparedStatement* stmt, size_t cache_size);

// Utility functions for prepared statement cache management
bool sqlite_initialize_prepared_statement_cache(DatabaseHandle* connection, size_t cache_size) {
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

bool sqlite_evict_lru_prepared_statement(DatabaseHandle* connection, const SQLiteConnection* sqlite_conn, const char* new_stmt_name) {
    if (!connection || !sqlite_conn || !sqlite_conn->db) return false;

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
    if (evicted_stmt && evicted_stmt->engine_specific_handle) {
        // Finalize the SQLite statement handle
        void* stmt = evicted_stmt->engine_specific_handle;
        if (sqlite3_finalize_ptr) {
            sqlite3_finalize_ptr(stmt);
        }
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
    log_this(SR_DATABASE, "Evicted LRU prepared statement to make room for: %s", LOG_LEVEL_TRACE, 1, new_stmt_name);
    return true;
}

bool sqlite_add_prepared_statement_to_cache(DatabaseHandle* connection, PreparedStatement* stmt, size_t cache_size) {
    if (!connection || !stmt) return false;

    // Check if cache is full - implement LRU eviction if needed
    if (connection->prepared_statement_count >= cache_size) {
        const SQLiteConnection* sqlite_conn = (const SQLiteConnection*)connection->connection_handle;
        if (!sqlite_evict_lru_prepared_statement(connection, sqlite_conn, stmt->name)) {
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
bool sqlite_prepare_statement(DatabaseHandle* connection, const char* name, const char* sql, PreparedStatement** stmt) {
    if (!connection || !name || !sql || !stmt || connection->engine_type != DB_ENGINE_SQLITE) {
        return false;
    }

    const SQLiteConnection* sqlite_conn = (const SQLiteConnection*)connection->connection_handle;
    if (!sqlite_conn || !sqlite_conn->db) {
        return false;
    }

    // Check if required function pointers are available
    if (!sqlite3_prepare_v2_ptr || !sqlite3_finalize_ptr || !sqlite3_errmsg_ptr) {
        log_this(SR_DATABASE, "SQLite function pointers not available", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Prepare the statement
    void* sqlite_stmt = NULL;
    int rc = sqlite3_prepare_v2_ptr(sqlite_conn->db, sql, -1, &sqlite_stmt, NULL);
    if (rc != SQLITE_OK) {
        log_this(SR_DATABASE, "SQLite PREPARE failed: %s", LOG_LEVEL_ERROR, 1, sqlite3_errmsg_ptr(sqlite_conn->db));
        return false;
    }

    // Create prepared statement structure
    PreparedStatement* prepared_stmt = calloc(1, sizeof(PreparedStatement));
    if (!prepared_stmt) {
        sqlite3_finalize_ptr(sqlite_stmt);
        return false;
    }

    prepared_stmt->name = strdup(name);
    prepared_stmt->sql_template = strdup(sql);
    prepared_stmt->engine_specific_handle = sqlite_stmt;
    prepared_stmt->created_at = time(NULL);
    prepared_stmt->usage_count = 0;

    // Get cache size from database configuration (default to 1000 if not set)
    size_t cache_size = 1000; // Default value
    if (connection->config && connection->config->prepared_statement_cache_size > 0) {
        cache_size = (size_t)connection->config->prepared_statement_cache_size;
    }

    // Initialize cache if this is the first prepared statement
    if (!connection->prepared_statements) {
        if (!sqlite_initialize_prepared_statement_cache(connection, cache_size)) {
            free(prepared_stmt->name);
            free(prepared_stmt->sql_template);
            free(prepared_stmt);
            sqlite3_finalize_ptr(sqlite_stmt);
            return false;
        }
    }

    // Add prepared statement to cache (handles LRU eviction if needed)
    if (!sqlite_add_prepared_statement_to_cache(connection, prepared_stmt, cache_size)) {
        free(prepared_stmt->name);
        free(prepared_stmt->sql_template);
        free(prepared_stmt);
        sqlite3_finalize_ptr(sqlite_stmt);
        return false;
    }

    *stmt = prepared_stmt;

    log_this(SR_DATABASE, "SQLite prepared statement created and added to connection", LOG_LEVEL_TRACE, 0);
    return true;
}

bool sqlite_unprepare_statement(DatabaseHandle* connection, PreparedStatement* stmt) {
    if (!connection || !stmt || connection->engine_type != DB_ENGINE_SQLITE) {
        return false;
    }

    const SQLiteConnection* sqlite_conn = (const SQLiteConnection*)connection->connection_handle;
    if (!sqlite_conn || !sqlite_conn->db) {
        return false;
    }

    // Finalize the SQLite statement handle if function pointer is available
    if (stmt->engine_specific_handle && sqlite3_finalize_ptr) {
        sqlite3_finalize_ptr(stmt->engine_specific_handle);
    }

    // Remove from connection's prepared statement array
    for (size_t i = 0; i < connection->prepared_statement_count; i++) {
        if (connection->prepared_statements[i] == stmt) {
            // Shift remaining elements
            for (size_t j = i; j < connection->prepared_statement_count - 1; j++) {
                connection->prepared_statements[j] = connection->prepared_statements[j + 1];
                // Only update LRU counter if array exists
                if (connection->prepared_statement_lru_counter) {
                    connection->prepared_statement_lru_counter[j] = connection->prepared_statement_lru_counter[j + 1];
                }
            }
            connection->prepared_statement_count--;
            break;
        }
    }

    // Free statement structure
    free(stmt->name);
    free(stmt->sql_template);
    free(stmt);

    log_this(SR_DATABASE, "SQLite prepared statement finalized and removed", LOG_LEVEL_TRACE, 0);
    return true;
}

// Update LRU counter when a prepared statement is used
void sqlite_update_prepared_lru_counter(DatabaseHandle* connection, const char* stmt_name) {
    if (!connection || !stmt_name) return;

    static uint64_t global_lru_counter = 0;

    for (size_t i = 0; i < connection->prepared_statement_count; i++) {
        if (connection->prepared_statements[i] && 
            strcmp(connection->prepared_statements[i]->name, stmt_name) == 0) {
            connection->prepared_statement_lru_counter[i] = ++global_lru_counter;
            connection->prepared_statements[i]->usage_count++;
            break;
        }
    }
}

// Legacy utility functions for compatibility with existing tests
// These operate on the old PreparedStatementCache structure and are stubs
bool sqlite_add_prepared_statement(PreparedStatementCache* cache, const char* name) {
    (void)cache;
    (void)name;
    // Stub implementation - actual caching is done through DatabaseHandle
    return true;
}

bool sqlite_remove_prepared_statement(PreparedStatementCache* cache, const char* name) {
    (void)cache;
    (void)name;
    // Stub implementation - actual caching is done through DatabaseHandle
    return true;
}