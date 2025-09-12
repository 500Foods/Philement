/*
  * SQLite Database Engine - Prepared Statement Management Implementation
  *
  * Implements SQLite prepared statement management functions.
  */
#include "../../hydrogen.h"
#include "../database.h"
#include "../database_queue.h"
#include "types.h"
#include "prepared.h"

// External declarations for libsqlite3 function pointers (defined in connection.c)
extern sqlite3_prepare_v2_t sqlite3_prepare_v2_ptr;
extern sqlite3_finalize_t sqlite3_finalize_ptr;
extern sqlite3_errmsg_t sqlite3_errmsg_ptr;

// Utility functions for prepared statement cache
bool sqlite_add_prepared_statement(PreparedStatementCache* cache, const char* name) {
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

bool sqlite_remove_prepared_statement(PreparedStatementCache* cache, const char* name) {
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
bool sqlite_prepare_statement(DatabaseHandle* connection, const char* name, const char* sql, PreparedStatement** stmt) {
    if (!connection || !name || !sql || !stmt || connection->engine_type != DB_ENGINE_SQLITE) {
        return false;
    }

    SQLiteConnection* sqlite_conn = (SQLiteConnection*)connection->connection_handle;
    if (!sqlite_conn || !sqlite_conn->db) {
        return false;
    }

    // Check if prepared statement functions are available
    if (!sqlite3_prepare_v2_ptr || !sqlite3_finalize_ptr) {
        log_this(SR_DATABASE, "SQLite prepared statement functions not available", LOG_LEVEL_DEBUG, 0);
        return false;
    }

    // Prepare the statement
    void* sqlite_stmt = NULL;
    const char* tail = NULL;
    int result = sqlite3_prepare_v2_ptr(sqlite_conn->db, sql, -1, &sqlite_stmt, &tail);

    if (result != SQLITE_OK) {
        log_this(SR_DATABASE, "SQLite sqlite3_prepare_v2 failed", LOG_LEVEL_ERROR, 0);
        if (sqlite3_errmsg_ptr) {
            const char* error_msg = sqlite3_errmsg_ptr(sqlite_conn->db);
            if (error_msg) {
                log_this(SR_DATABASE, error_msg, LOG_LEVEL_ERROR, 0);
            }
        }
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
    prepared_stmt->created_at = time(NULL);
    prepared_stmt->usage_count = 0;

    // Add to cache
    if (!sqlite_add_prepared_statement(sqlite_conn->prepared_statements, name)) {
        free(prepared_stmt->name);
        free(prepared_stmt->sql_template);
        free(prepared_stmt);
        sqlite3_finalize_ptr(sqlite_stmt);
        return false;
    }

    *stmt = prepared_stmt;

    log_this(SR_DATABASE, "SQLite prepared statement created", LOG_LEVEL_DEBUG, 0);
    return true;
}

bool sqlite_unprepare_statement(DatabaseHandle* connection, PreparedStatement* stmt) {
    if (!connection || !stmt || connection->engine_type != DB_ENGINE_SQLITE) {
        return false;
    }

    SQLiteConnection* sqlite_conn = (SQLiteConnection*)connection->connection_handle;
    if (!sqlite_conn || !sqlite_conn->db) {
        return false;
    }

    // Check if prepared statement functions are available
    if (!sqlite3_finalize_ptr) {
        log_this(SR_DATABASE, "SQLite prepared statement functions not available for cleanup", LOG_LEVEL_DEBUG, 0);
        // Still clean up our tracking structures
        sqlite_remove_prepared_statement(sqlite_conn->prepared_statements, stmt->name);
        free(stmt->name);
        free(stmt->sql_template);
        free(stmt);
        return true;
    }

    // For now, we don't have a way to get the SQLite statement handle back
    // In a full implementation, we'd store the handle in the PreparedStatement structure
    // For this basic implementation, we'll just clean up our tracking structures

    // Remove from cache
    sqlite_remove_prepared_statement(sqlite_conn->prepared_statements, stmt->name);

    // Free statement structure
    free(stmt->name);
    free(stmt->sql_template);
    free(stmt);

    log_this(SR_DATABASE, "SQLite prepared statement removed", LOG_LEVEL_DEBUG, 0);
    return true;
}