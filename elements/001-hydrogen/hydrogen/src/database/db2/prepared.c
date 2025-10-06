/*
  * DB2 Database Engine - Prepared Statement Management Implementation
  *
  * Implements DB2 prepared statement management functions.
  */
#include "../../hydrogen.h"
#include "../database.h"
#include "../database_queue.h"
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

    DB2Connection* db2_conn = (DB2Connection*)connection->connection_handle;
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

    // Add to cache
    if (!db2_add_prepared_statement(db2_conn->prepared_statements, name)) {
        free(prepared_stmt->name);
        free(prepared_stmt->sql_template);
        free(prepared_stmt);
        SQLFreeHandle_ptr(SQL_HANDLE_STMT, stmt_handle);
        return false;
    }

    *stmt = prepared_stmt;

    log_this(SR_DATABASE, "DB2 prepared statement created", LOG_LEVEL_TRACE, 0);
    return true;
}

bool db2_unprepare_statement(DatabaseHandle* connection, PreparedStatement* stmt) {
    if (!connection || !stmt || connection->engine_type != DB_ENGINE_DB2) {
        return false;
    }

    DB2Connection* db2_conn = (DB2Connection*)connection->connection_handle;
    if (!db2_conn || !db2_conn->connection) {
        return false;
    }

    // Check if prepared statement functions are available
    if (!SQLFreeHandle_ptr) {
        log_this(SR_DATABASE, "DB2 prepared statement functions not available for cleanup", LOG_LEVEL_TRACE, 0);
        // Still clean up our tracking structures
        db2_remove_prepared_statement(db2_conn->prepared_statements, stmt->name);
        free(stmt->name);
        free(stmt->sql_template);
        free(stmt);
        return true;
    }

    // For now, we don't have a way to get the DB2 statement handle back
    // In a full implementation, we'd store the handle in the PreparedStatement structure
    // For this basic implementation, we'll just clean up our tracking structures

    // Remove from cache
    db2_remove_prepared_statement(db2_conn->prepared_statements, stmt->name);

    // Free statement structure
    free(stmt->name);
    free(stmt->sql_template);
    free(stmt);

    log_this(SR_DATABASE, "DB2 prepared statement removed", LOG_LEVEL_TRACE, 0);
    return true;
}