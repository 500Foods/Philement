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

// External declarations for libmysqlclient function pointers (defined in connection.c)
extern mysql_stmt_init_t mysql_stmt_init_ptr;
extern mysql_stmt_prepare_t mysql_stmt_prepare_ptr;
extern mysql_stmt_execute_t mysql_stmt_execute_ptr;
extern mysql_stmt_close_t mysql_stmt_close_ptr;
extern mysql_error_t mysql_error_ptr;

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
    if (!mysql_stmt_init_ptr || !mysql_stmt_prepare_ptr || !mysql_stmt_execute_ptr || !mysql_stmt_close_ptr) {
        log_this(SR_DATABASE, "MySQL prepared statement functions not available", LOG_LEVEL_TRACE, 0);
        return false;
    }

    // Initialize prepared statement
    void* mysql_stmt = mysql_stmt_init_ptr(mysql_conn->connection);
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

    // Prepare the statement
    if (mysql_stmt_prepare_ptr(mysql_stmt, sql, strlen(sql)) != 0) {
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

    // Add to connection's prepared statement array
    MUTEX_LOCK(&connection->connection_lock, SR_DATABASE);

    // Expand array if needed (simple approach - just add one more)
    PreparedStatement** new_array = realloc(connection->prepared_statements,
                                           (connection->prepared_statement_count + 1) * sizeof(PreparedStatement*));
    if (!new_array) {
        MUTEX_UNLOCK(&connection->connection_lock, SR_DATABASE);
        free(prepared_stmt->name);
        free(prepared_stmt->sql_template);
        free(prepared_stmt);
        mysql_stmt_close_ptr(mysql_stmt);
        return false;
    }
    connection->prepared_statements = new_array;
    connection->prepared_statements[connection->prepared_statement_count] = prepared_stmt;
    connection->prepared_statement_count++;

    MUTEX_UNLOCK(&connection->connection_lock, SR_DATABASE);

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
        // Still clean up our structures
        MUTEX_LOCK(&connection->connection_lock, SR_DATABASE);
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
        MUTEX_UNLOCK(&connection->connection_lock, SR_DATABASE);
        free(stmt->name);
        free(stmt->sql_template);
        free(stmt);
        return true;
    }

    // Close the MySQL statement handle if it exists
    if (stmt->engine_specific_handle) {
        mysql_stmt_close_ptr(stmt->engine_specific_handle);
    }

    // Remove from connection's prepared statement array
    MUTEX_LOCK(&connection->connection_lock, SR_DATABASE);
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
    MUTEX_UNLOCK(&connection->connection_lock, SR_DATABASE);

    // Free statement structure
    free(stmt->name);
    free(stmt->sql_template);
    free(stmt);

    log_this(SR_DATABASE, "MySQL prepared statement removed", LOG_LEVEL_TRACE, 0);
    return true;
}