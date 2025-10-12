/*
  * MySQL Database Engine - Prepared Statement Management Implementation
  *
  * Implements MySQL prepared statement management functions.
  */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/database/dbqueue/dbqueue.h>

// Local includes
#include "types.h"
#include "connection.h"
#include "prepared.h"

// External declarations for timeout checking (defined in connection.c)
extern bool mysql_check_timeout_expired(time_t start_time, int timeout_seconds);

// External declarations for libmysqlclient function pointers (defined in connection.c)
extern mysql_stmt_init_t mysql_stmt_init_ptr;
extern mysql_stmt_prepare_t mysql_stmt_prepare_ptr;
extern mysql_stmt_execute_t mysql_stmt_execute_ptr;
extern mysql_stmt_close_t mysql_stmt_close_ptr;
extern mysql_error_t mysql_error_ptr;

// Helper functions for better testability
bool mysql_validate_prepared_statement_functions(void) {
    return mysql_stmt_init_ptr && mysql_stmt_prepare_ptr &&
           mysql_stmt_execute_ptr && mysql_stmt_close_ptr;
}

void* mysql_create_statement_handle(void* mysql_connection) {
    if (!mysql_stmt_init_ptr) return NULL;
    return mysql_stmt_init_ptr(mysql_connection);
}

bool mysql_prepare_statement_handle(void* stmt_handle, const char* sql) {
    if (!mysql_stmt_prepare_ptr) return false;
    return mysql_stmt_prepare_ptr(stmt_handle, sql, strlen(sql)) == 0;
}

bool mysql_initialize_prepared_statement_cache(DatabaseHandle* connection, size_t cache_size) {
    if (connection->prepared_statements) return true; // Already initialized

    connection->prepared_statements = calloc(cache_size, sizeof(PreparedStatement*));
    if (!connection->prepared_statements) return false;

    connection->prepared_statement_count = 0;

    if (!connection->prepared_statement_lru_counter) {
        connection->prepared_statement_lru_counter = calloc(cache_size, sizeof(uint64_t));
    }

    return connection->prepared_statements && connection->prepared_statement_lru_counter;
}

size_t mysql_find_lru_statement_index(DatabaseHandle* connection) {
    size_t lru_index = 0;
    uint64_t min_lru_value = UINT64_MAX;

    for (size_t i = 0; i < connection->prepared_statement_count; i++) {
        if (connection->prepared_statement_lru_counter[i] < min_lru_value) {
            min_lru_value = connection->prepared_statement_lru_counter[i];
            lru_index = i;
        }
    }

    return lru_index;
}

void mysql_evict_lru_statement(DatabaseHandle* connection, size_t lru_index) {
    PreparedStatement* evicted_stmt = connection->prepared_statements[lru_index];
    if (evicted_stmt) {
        if (mysql_stmt_close_ptr && evicted_stmt->engine_specific_handle) {
            mysql_stmt_close_ptr(evicted_stmt->engine_specific_handle);
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
}

bool mysql_add_statement_to_cache(DatabaseHandle* connection, PreparedStatement* stmt, size_t cache_size) {
    if (!mysql_initialize_prepared_statement_cache(connection, cache_size)) {
        return false;
    }

    // Check if cache is full - implement LRU eviction
    if (connection->prepared_statement_count >= cache_size) {
        size_t lru_index = mysql_find_lru_statement_index(connection);
        mysql_evict_lru_statement(connection, lru_index);
        log_this(SR_DATABASE, "Evicted LRU prepared statement to make room for: %s", LOG_LEVEL_TRACE, 1, stmt->name);
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

bool mysql_remove_statement_from_cache(DatabaseHandle* connection, const PreparedStatement* stmt) {
    for (size_t i = 0; i < connection->prepared_statement_count; i++) {
        if (connection->prepared_statements[i] == stmt) {
            // Shift remaining elements
            for (size_t j = i; j < connection->prepared_statement_count - 1; j++) {
                connection->prepared_statements[j] = connection->prepared_statements[j + 1];
            }
            connection->prepared_statement_count--;
            return true;
        }
    }
    return false;
}

void mysql_cleanup_prepared_statement(PreparedStatement* stmt) {
    if (stmt) {
        free(stmt->name);
        free(stmt->sql_template);
        free(stmt);
    }
}

// Utility functions for prepared statement cache
bool mysql_add_prepared_statement(PreparedStatementCache* cache, const char* name) {
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

bool mysql_remove_prepared_statement(PreparedStatementCache* cache, const char* name) {
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

/*
 * Prepared Statement Management
 */

bool mysql_prepare_statement(DatabaseHandle* connection, const char* name, const char* sql, PreparedStatement** stmt) {
    if (!connection || !name || !sql || !stmt || connection->engine_type != DB_ENGINE_MYSQL) {
        return false;
    }

    const MySQLConnection* mysql_conn = (const MySQLConnection*)connection->connection_handle;
    if (!mysql_conn || !mysql_conn->connection) {
        return false;
    }

    // Check if prepared statement functions are available
    if (!mysql_validate_prepared_statement_functions()) {
        log_this(SR_DATABASE, "MySQL prepared statement functions not available", LOG_LEVEL_TRACE, 0);
        return false;
    }

    // Initialize prepared statement
    void* mysql_stmt = mysql_create_statement_handle(mysql_conn->connection);
    if (!mysql_stmt) {
        log_this(SR_DATABASE, "MySQL mysql_stmt_init failed", LOG_LEVEL_ERROR, 0);
        if (mysql_error_ptr) {
            const char* error_msg = mysql_error_ptr(mysql_conn->connection);
            if (error_msg && strlen(error_msg) > 0) {
                log_this(SR_DATABASE, error_msg, LOG_LEVEL_ERROR, 0);
            }
        }
        return false;
    }

    // Prepare the statement with timeout protection
    time_t start_time = time(NULL);
    if (!mysql_prepare_statement_handle(mysql_stmt, sql)) {
        // Check if prepare took too long
        if (mysql_check_timeout_expired(start_time, 15)) {
            log_this(SR_DATABASE, "MySQL PREPARE execution time exceeded 15 seconds", LOG_LEVEL_ERROR, 0);
            mysql_stmt_close_ptr(mysql_stmt);
            return false;
        }

        log_this(SR_DATABASE, "MySQL mysql_stmt_prepare failed", LOG_LEVEL_ERROR, 0);
        if (mysql_error_ptr) {
            const char* error_msg = mysql_error_ptr(mysql_conn->connection);
            if (error_msg && strlen(error_msg) > 0) {
                log_this(SR_DATABASE, error_msg, LOG_LEVEL_ERROR, 0);
            }
        }
        mysql_stmt_close_ptr(mysql_stmt);
        return false;
    }

    // Create prepared statement structure
    PreparedStatement* prepared_stmt = calloc(1, sizeof(PreparedStatement));
    if (!prepared_stmt) {
        mysql_stmt_close_ptr(mysql_stmt);
        return false;
    }

    prepared_stmt->name = strdup(name);
    prepared_stmt->sql_template = strdup(sql);
    prepared_stmt->created_at = time(NULL);
    prepared_stmt->usage_count = 0;
    prepared_stmt->engine_specific_handle = mysql_stmt;  // Store MySQL statement handle

    // Get cache size from database configuration (default to 1000 if not set)
    size_t cache_size = 1000; // Default value
    if (connection->config && connection->config->prepared_statement_cache_size > 0) {
        cache_size = (size_t)connection->config->prepared_statement_cache_size;
    }

    // Add to cache using helper function
    if (!mysql_add_statement_to_cache(connection, prepared_stmt, cache_size)) {
        free(prepared_stmt->name);
        free(prepared_stmt->sql_template);
        free(prepared_stmt);
        mysql_stmt_close_ptr(mysql_stmt);
        return false;
    }

    *stmt = prepared_stmt;

    log_this(SR_DATABASE, "MySQL prepared statement created and added to connection", LOG_LEVEL_TRACE, 0);
    return true;
}

bool mysql_unprepare_statement(DatabaseHandle* connection, PreparedStatement* stmt) {
    if (!connection || !stmt || connection->engine_type != DB_ENGINE_MYSQL) {
        return false;
    }

    // cppcheck-suppress constVariablePointer
    // Justification: MySQL API requires non-const MYSQL* connection handle
    // cppcheck-suppress constVariablePointer
    // Justification: MySQL API requires non-const MYSQL* connection handle
    MySQLConnection* mysql_conn = (MySQLConnection*)connection->connection_handle;
    if (!mysql_conn || !mysql_conn->connection) {
        return false;
    }

    // Check if prepared statement functions are available
    if (!mysql_stmt_close_ptr) {
        log_this(SR_DATABASE, "MySQL prepared statement functions not available for cleanup", LOG_LEVEL_TRACE, 0);
        // Still clean up our structures using helper function
        mysql_remove_statement_from_cache(connection, stmt);
        mysql_cleanup_prepared_statement(stmt);
        return true;
    }

    // Close the MySQL statement handle if it exists
    if (stmt->engine_specific_handle) {
        mysql_stmt_close_ptr(stmt->engine_specific_handle);
    }

    // Remove from connection's prepared statement array using helper function
    mysql_remove_statement_from_cache(connection, stmt);

    // Free statement structure using helper function
    mysql_cleanup_prepared_statement(stmt);

    log_this(SR_DATABASE, "MySQL prepared statement removed", LOG_LEVEL_TRACE, 0);
    return true;
}