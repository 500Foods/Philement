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
#include "connection.h"

// External declarations for timeout checking (defined in connection.c)
extern bool db2_check_timeout_expired(time_t start_time, int timeout_seconds);

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

    // Use connection's designator for consistent logging
    const char* log_subsystem = connection->designator ? connection->designator : SR_DATABASE;

    // Check if prepared statement functions are available
    if (!SQLAllocHandle_ptr || !SQLPrepare_ptr || !SQLFreeHandle_ptr) {
        log_this(log_subsystem, "DB2 prepared statement functions not available", LOG_LEVEL_TRACE, 0);
        return false;
    }

    // Allocate statement handle
    void* stmt_handle = NULL;
    if (SQLAllocHandle_ptr(SQL_HANDLE_STMT, db2_conn->connection, &stmt_handle) != SQL_SUCCESS) {
        log_this(log_subsystem, "DB2 prepare_statement: Failed to allocate statement handle", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Prepare the statement with timeout protection
    time_t start_time = time(NULL);
    int result = SQLPrepare_ptr(stmt_handle, (char*)sql, SQL_NTS);
    
    // Check if prepare took too long
    if (db2_check_timeout_expired(start_time, 15)) {
        log_this(log_subsystem, "DB2 PREPARE execution time exceeded 15 seconds", LOG_LEVEL_ERROR, 0);
        SQLFreeHandle_ptr(SQL_HANDLE_STMT, stmt_handle);
        return false;
    }
    
    if (result != SQL_SUCCESS) {
        log_this(log_subsystem, "DB2 SQLPrepare failed", LOG_LEVEL_ERROR, 0);
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

    // Add to connection's prepared statement array
    // Since each thread owns its connection exclusively, no mutex needed

    // Get cache size from database configuration (default to 1000 to match database_engine.c)
    // CRITICAL: This default MUST match the default in database_engine.c:store_prepared_statement
    size_t cache_size = 1000;
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
    }

    // Initialize LRU counter array if needed
    if (!connection->prepared_statement_lru_counter) {
        connection->prepared_statement_lru_counter = calloc(cache_size, sizeof(uint64_t));
        if (!connection->prepared_statement_lru_counter) {
            free(prepared_stmt->name);
            free(prepared_stmt->sql_template);
            free(prepared_stmt);
            SQLFreeHandle_ptr(SQL_HANDLE_STMT, stmt_handle);
            return false;
        }
    }

    // Static global LRU counter (monotonically increasing)
    static uint64_t global_lru_counter = 0;
    
    // LRU cache management - evict least recently used statement if cache is full
    if (connection->prepared_statement_count >= cache_size) {
        // Find the statement with the lowest LRU counter (least recently used)
        size_t lru_index = 0;
        uint64_t lowest_counter = connection->prepared_statement_lru_counter[0];
        
        for (size_t i = 1; i < connection->prepared_statement_count; i++) {
            if (connection->prepared_statement_lru_counter[i] < lowest_counter) {
                lowest_counter = connection->prepared_statement_lru_counter[i];
                lru_index = i;
            }
        }
        
        // Evict the LRU statement
        PreparedStatement* evicted_stmt = connection->prepared_statements[lru_index];
        log_this(log_subsystem, "Evicting LRU prepared statement: %s", LOG_LEVEL_TRACE, 1,
                 evicted_stmt->name ? evicted_stmt->name : "unknown");
        
        // Free the DB2 statement handle
        if (SQLFreeHandle_ptr && evicted_stmt->engine_specific_handle) {
            SQLFreeHandle_ptr(SQL_HANDLE_STMT, evicted_stmt->engine_specific_handle);
        }
        
        // Free the statement structure
        free(evicted_stmt->name);
        free(evicted_stmt->sql_template);
        free(evicted_stmt);
        
        // Shift remaining statements to fill the gap
        for (size_t i = lru_index; i < connection->prepared_statement_count - 1; i++) {
            connection->prepared_statements[i] = connection->prepared_statements[i + 1];
            connection->prepared_statement_lru_counter[i] = connection->prepared_statement_lru_counter[i + 1];
        }
        
        // Decrement count (we'll increment it back when adding the new statement)
        connection->prepared_statement_count--;
    }

    // Store the new prepared statement at the end and assign LRU counter
    connection->prepared_statements[connection->prepared_statement_count] = prepared_stmt;
    connection->prepared_statement_lru_counter[connection->prepared_statement_count] = ++global_lru_counter;
    connection->prepared_statement_count++;

    *stmt = prepared_stmt;

    log_this(log_subsystem, "DB2 prepared statement created and added to connection", LOG_LEVEL_TRACE, 0);
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

    // Use connection's designator for consistent logging
    const char* log_subsystem = connection->designator ? connection->designator : SR_DATABASE;

    // Free the DB2 statement handle if it exists
    if (SQLFreeHandle_ptr && stmt->engine_specific_handle) {
        SQLFreeHandle_ptr(SQL_HANDLE_STMT, stmt->engine_specific_handle);
    }

    // Remove from connection's prepared statement array
    // Since each thread owns its connection exclusively, no mutex needed
    for (size_t i = 0; i < connection->prepared_statement_count; i++) {
        if (connection->prepared_statements[i] == stmt) {
            // Shift remaining elements to fill the gap
            for (size_t j = i; j < connection->prepared_statement_count - 1; j++) {
                connection->prepared_statements[j] = connection->prepared_statements[j + 1];
            }
            connection->prepared_statements[connection->prepared_statement_count - 1] = NULL; // Clear last element
            connection->prepared_statement_count--;
            break;
        }
    }

    // Free statement structure
    free(stmt->name);
    free(stmt->sql_template);
    free(stmt);

    log_this(log_subsystem, "DB2 prepared statement removed", LOG_LEVEL_TRACE, 0);
    return true;
}