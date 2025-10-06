/*
  * MySQL Database Engine - Prepared Statement Management Implementation
  *
  * Implements MySQL prepared statement management functions.
  */
#include "../../hydrogen.h"
#include "../database.h"
#include "../database_queue.h"
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

    MySQLConnection* mysql_conn = (MySQLConnection*)connection->connection_handle;
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
    // Store MySQL statement handle in a way that can be retrieved later
    // For now, we'll use the name as identifier

    // Add to cache
    if (!mysql_add_prepared_statement(mysql_conn->prepared_statements, name)) {
        free(prepared_stmt->name);
        free(prepared_stmt->sql_template);
        free(prepared_stmt);
        mysql_stmt_close_ptr(mysql_stmt);
        return false;
    }

    *stmt = prepared_stmt;

    log_this(SR_DATABASE, "MySQL prepared statement created", LOG_LEVEL_TRACE, 0);
    return true;
}

bool mysql_unprepare_statement(DatabaseHandle* connection, PreparedStatement* stmt) {
    if (!connection || !stmt || connection->engine_type != DB_ENGINE_MYSQL) {
        return false;
    }

    MySQLConnection* mysql_conn = (MySQLConnection*)connection->connection_handle;
    if (!mysql_conn || !mysql_conn->connection) {
        return false;
    }

    // Check if prepared statement functions are available
    if (!mysql_stmt_close_ptr) {
        log_this(SR_DATABASE, "MySQL prepared statement functions not available for cleanup", LOG_LEVEL_TRACE, 0);
        // Still clean up our structures
        mysql_remove_prepared_statement(mysql_conn->prepared_statements, stmt->name);
        free(stmt->name);
        free(stmt->sql_template);
        free(stmt);
        return true;
    }

    // For now, we don't have a way to get the MySQL statement handle back
    // In a full implementation, we'd store the handle in the PreparedStatement structure
    // For this basic implementation, we'll just clean up our tracking structures

    // Remove from cache
    mysql_remove_prepared_statement(mysql_conn->prepared_statements, stmt->name);

    // Free statement structure
    free(stmt->name);
    free(stmt->sql_template);
    free(stmt);

    log_this(SR_DATABASE, "MySQL prepared statement removed", LOG_LEVEL_TRACE, 0);
    return true;
}