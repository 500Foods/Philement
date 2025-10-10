/*
  * DB2 Database Engine - Prepared Statement Management Implementation
  *
  * Implements DB2 prepared statement management functions.
  */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/database/dbqueue/dbqueue.h>

// Local includes
#include "types.h"
#include "prepared.h"

// External declarations for libdb2 function pointers (defined in connection.c)
extern SQLAllocHandle_t SQLAllocHandle_ptr;
extern SQLPrepare_t SQLPrepare_ptr;
extern SQLFreeHandle_t SQLFreeHandle_ptr;

// Utility functions for prepared statement cache
bool db2_add_prepared_statement(PreparedStatementCache* cache, const char* name) {
    if (!cache || !name) return false;

    pthread_mutex_lock(&cache->lock);

    // Check if already exists
    for (size_t i = 0; i < cache->count; i++) {
        if (strcmp(cache->names[i], name) == 0) {
            pthread_mutex_unlock(&cache->lock);
            return true; // Already exists
        }
    }

    // Expand capacity if needed
    if (cache->count >= cache->capacity) {
        size_t new_capacity = cache->capacity * 2;
        char** new_names = realloc(cache->names, new_capacity * sizeof(char*));
        if (!new_names) {
            pthread_mutex_unlock(&cache->lock);
            return false;
        }
        cache->names = new_names;
        cache->capacity = new_capacity;
    }

    cache->names[cache->count] = strdup(name);
    if (!cache->names[cache->count]) {
        pthread_mutex_unlock(&cache->lock);
        return false;
    }

    cache->count++;
    pthread_mutex_unlock(&cache->lock);
    return true;
}

bool db2_remove_prepared_statement(PreparedStatementCache* cache, const char* name) {
    if (!cache || !name) return false;

    pthread_mutex_lock(&cache->lock);

    for (size_t i = 0; i < cache->count; i++) {
        if (strcmp(cache->names[i], name) == 0) {
            free(cache->names[i]);
            // Shift remaining elements
            for (size_t j = i; j < cache->count - 1; j++) {
                cache->names[j] = cache->names[j + 1];
            }
            cache->count--;
            pthread_mutex_unlock(&cache->lock);
            return true;
        }
    }

    pthread_mutex_unlock(&cache->lock);
    return false;
}

// Prepared Statement Management Functions
bool db2_prepare_statement(DatabaseHandle* connection, const char* name, const char* sql, PreparedStatement** stmt) {
    if (!connection || !name || !sql || !stmt || connection->engine_type != DB_ENGINE_DB2) {
        return false;
    }

    const DB2Connection* db2_conn = (const DB2Connection*)connection->connection_handle;
    if (!db2_conn || !db2_conn->connection) {
        return false;
    }

    // Check if prepared statement functions are available
    if (!SQLAllocHandle_ptr || !SQLPrepare_ptr || !SQLFreeHandle_ptr) {
        log_this(SR_DATABASE, "DB2 prepared statement functions not available", LOG_LEVEL_TRACE, 0);
        return false;
    }

    // Allocate statement handle
    void* stmt_handle = NULL;
    if (SQLAllocHandle_ptr(SQL_HANDLE_STMT, db2_conn->connection, &stmt_handle) != SQL_SUCCESS) {
        log_this(SR_DATABASE, "DB2 prepare_statement: Failed to allocate statement handle", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Prepare the statement
    int result = SQLPrepare_ptr(stmt_handle, (char*)sql, SQL_NTS);
    if (result != SQL_SUCCESS) {
        log_this(SR_DATABASE, "DB2 SQLPrepare failed", LOG_LEVEL_ERROR, 0);
        SQLFreeHandle_ptr(SQL_HANDLE_STMT, stmt_handle);
        return false;
    }

    // Create prepared statement structure
    PreparedStatement* prepared_stmt = calloc(1, sizeof(PreparedStatement));
    if (!prepared_stmt) {
        SQLFreeHandle_ptr(SQL_HANDLE_STMT, stmt_handle);
        return false;
    }

    prepared_stmt->name = strdup(name);
    prepared_stmt->sql_template = strdup(sql);
    prepared_stmt->created_at = time(NULL);
    prepared_stmt->usage_count = 0;
    prepared_stmt->engine_specific_handle = stmt_handle;  // Store the DB2 statement handle

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
            SQLFreeHandle_ptr(SQL_HANDLE_STMT, stmt_handle);
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
            // Clean up the evicted statement
            if (SQLFreeHandle_ptr && evicted_stmt->engine_specific_handle) {
                SQLFreeHandle_ptr(SQL_HANDLE_STMT, evicted_stmt->engine_specific_handle);
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

    log_this(SR_DATABASE, "DB2 prepared statement created and added to connection", LOG_LEVEL_TRACE, 0);
    return true;
}

bool db2_unprepare_statement(DatabaseHandle* connection, PreparedStatement* stmt) {
    if (!connection || !stmt || connection->engine_type != DB_ENGINE_DB2) {
        return false;
    }

    // cppcheck-suppress constVariablePointer
    // Justification: DB2 API requires non-const SQLHDBC connection handle
    DB2Connection* db2_conn = (DB2Connection*)connection->connection_handle;
    if (!db2_conn || !db2_conn->connection) {
        return false;
    }

    // Check if prepared statement functions are available
    if (!SQLFreeHandle_ptr) {
        log_this(SR_DATABASE, "DB2 prepared statement functions not available for cleanup", LOG_LEVEL_TRACE, 0);
        // Clean up our tracking structures
        // Since each thread owns its connection exclusively, no mutex needed
        // Find and remove from connection's array
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
        free(stmt->name);
        free(stmt->sql_template);
        free(stmt);
        return true;
    }

    // Free the DB2 statement handle if it exists
    if (stmt->engine_specific_handle) {
        SQLFreeHandle_ptr(SQL_HANDLE_STMT, stmt->engine_specific_handle);
    }

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

    log_this(SR_DATABASE, "DB2 prepared statement removed", LOG_LEVEL_TRACE, 0);
    return true;
}