/*
 * DB2 Database Engine - Prepared Statement Management Implementation
 *
 * Implements DB2 prepared statement management functions with LRU cache.
 */

/* Project includes */
#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/database/dbqueue/dbqueue.h>

/* Local includes */
#include "types.h"
#include "connection.h"
#include "prepared.h"

// ODBC type definitions for DB2
typedef short SQLSMALLINT;
typedef long SQLINTEGER;
typedef unsigned long SQLUINTEGER;
typedef unsigned char SQLCHAR;
typedef short SQLRETURN;
typedef void* SQLHANDLE;

// ODBC constants
#define SQL_MAX_MESSAGE_LENGTH 512

// External declarations for DB2 function pointers (defined in connection.c)
extern SQLAllocHandle_t SQLAllocHandle_ptr;
extern SQLPrepare_t SQLPrepare_ptr;
extern SQLFreeHandle_t SQLFreeHandle_ptr;
extern SQLGetDiagRec_t SQLGetDiagRec_ptr;

// Forward declarations for utility functions
bool db2_initialize_prepared_statement_cache(DatabaseHandle* connection, size_t cache_size);
bool db2_evict_lru_prepared_statement(DatabaseHandle* connection, const DB2Connection* db2_conn, const char* new_stmt_name);
bool db2_add_prepared_statement_to_cache(DatabaseHandle* connection, PreparedStatement* stmt, size_t cache_size);

// Utility functions for prepared statement cache management
bool db2_initialize_prepared_statement_cache(DatabaseHandle* connection, size_t cache_size) {
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

bool db2_evict_lru_prepared_statement(DatabaseHandle* connection, const DB2Connection* db2_conn, const char* new_stmt_name) {
    if (!connection || !db2_conn) return false;

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
        // Free the DB2 statement handle
        void* stmt_handle = evicted_stmt->engine_specific_handle;
        if (SQLFreeHandle_ptr) {
            SQLFreeHandle_ptr(SQL_HANDLE_STMT, stmt_handle);
        } else {
            log_this(SR_DATABASE, "SQLFreeHandle_ptr not available for freeing statement", LOG_LEVEL_ERROR, 0);
            return false;
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

    // NULL out the last element to prevent dangling pointer issues
    connection->prepared_statements[connection->prepared_statement_count - 1] = NULL;
    if (connection->prepared_statement_lru_counter) {
        connection->prepared_statement_lru_counter[connection->prepared_statement_count - 1] = 0;
    }

    connection->prepared_statement_count--;
    log_this(SR_DATABASE, "Evicted LRU prepared statement to make room for: %s", LOG_LEVEL_TRACE, 1, new_stmt_name);
    return true;
}

bool db2_add_prepared_statement_to_cache(DatabaseHandle* connection, PreparedStatement* stmt, size_t cache_size) {
    if (!connection || !stmt) return false;

    // Check if cache is full - implement LRU eviction if needed
    if (connection->prepared_statement_count >= cache_size) {
        const DB2Connection* db2_conn = (const DB2Connection*)connection->connection_handle;
        if (!db2_evict_lru_prepared_statement(connection, db2_conn, stmt->name)) {
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
bool db2_prepare_statement(DatabaseHandle* connection, const char* name, const char* sql, PreparedStatement** stmt, bool add_to_cache) {
    if (!connection || !name || !sql || !stmt || connection->engine_type != DB_ENGINE_DB2) {
        return false;
    }

    const DB2Connection* db2_conn = (const DB2Connection*)connection->connection_handle;
    if (!db2_conn || !db2_conn->connection) {
        return false;
    }

    // Check if required function pointers are available
    if (!SQLAllocHandle_ptr || !SQLPrepare_ptr || !SQLFreeHandle_ptr) {
        log_this(SR_DATABASE, "DB2 function pointers not available", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Allocate statement handle
    void* stmt_handle = NULL;
    SQLRETURN rc = (SQLRETURN)SQLAllocHandle_ptr(SQL_HANDLE_STMT, db2_conn->connection, &stmt_handle);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
        log_this(SR_DATABASE, "DB2 statement handle allocation failed", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Prepare the statement
    rc = (SQLRETURN)SQLPrepare_ptr(stmt_handle, (SQLCHAR*)sql, SQL_NTS);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
        SQLCHAR sqlstate[6];
        SQLCHAR message[SQL_MAX_MESSAGE_LENGTH];
        SQLINTEGER native_error;
        SQLSMALLINT text_length;
        
        SQLGetDiagRec_ptr(SQL_HANDLE_STMT, stmt_handle, 1, sqlstate, &native_error, 
                         message, sizeof(message), &text_length);
        log_this(SR_DATABASE, "DB2 PREPARE failed: %s", LOG_LEVEL_ERROR, 1, (char*)message);
        if (SQLFreeHandle_ptr) {
            SQLFreeHandle_ptr(SQL_HANDLE_STMT, stmt_handle);
        } else {
            log_this(SR_DATABASE, "SQLFreeHandle_ptr not available for freeing statement", LOG_LEVEL_ERROR, 0);
            return false;
        }
        return false;
    }

    // Create prepared statement structure
    PreparedStatement* prepared_stmt = calloc(1, sizeof(PreparedStatement));
    if (!prepared_stmt) {
        if (SQLFreeHandle_ptr) {
            SQLFreeHandle_ptr(SQL_HANDLE_STMT, stmt_handle);
        } else {
            log_this(SR_DATABASE, "SQLFreeHandle_ptr not available for freeing statement", LOG_LEVEL_ERROR, 0);
            return false;
        }
        return false;
    }

    prepared_stmt->name = strdup(name);
    prepared_stmt->sql_template = strdup(sql);
    prepared_stmt->engine_specific_handle = stmt_handle;
    prepared_stmt->created_at = time(NULL);
    prepared_stmt->usage_count = 0;

    // Get cache size from database configuration
    size_t cache_size = (size_t)connection->config->prepared_statement_cache_size;

    // Initialize cache if this is the first prepared statement
    if (!connection->prepared_statements) {
        if (!db2_initialize_prepared_statement_cache(connection, cache_size)) {
            free(prepared_stmt->name);
            free(prepared_stmt->sql_template);
            free(prepared_stmt);
            if (SQLFreeHandle_ptr) {
                SQLFreeHandle_ptr(SQL_HANDLE_STMT, stmt_handle);
            } else {
                log_this(SR_DATABASE, "SQLFreeHandle_ptr not available for freeing statement", LOG_LEVEL_ERROR, 0);
                return false;
            }
            return false;
        }
    }

    // Add to cache if requested (for unit tests) or let database_engine_execute handle it
    if (add_to_cache) {
        // Add statement to cache
        if (!db2_add_prepared_statement_to_cache(connection, prepared_stmt, cache_size)) {
            // Adding to cache failed, but statement is still valid
            // Just log or continue - the statement can still be used
            log_this(connection->designator ? connection->designator : SR_DATABASE,
                     "Failed to add prepared statement to cache, but statement is still valid", LOG_LEVEL_TRACE, 0);
        }
    }
    
    *stmt = prepared_stmt;

    log_this(connection->designator ? connection->designator : SR_DATABASE, 
             "DB2 prepared statement created and added to connection", LOG_LEVEL_TRACE, 0);
    return true;
}

bool db2_unprepare_statement(DatabaseHandle* connection, PreparedStatement* stmt) {
    if (!connection || !stmt || connection->engine_type != DB_ENGINE_DB2) {
        return false;
    }

    const DB2Connection* db2_conn = (const DB2Connection*)connection->connection_handle;
    if (!db2_conn || !db2_conn->connection) {
        return false;
    }

    // Free the DB2 statement handle if function pointer is available
    if (stmt->engine_specific_handle && SQLFreeHandle_ptr) {
        SQLFreeHandle_ptr(SQL_HANDLE_STMT, stmt->engine_specific_handle);
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
             "DB2 prepared statement freed and removed", LOG_LEVEL_TRACE, 0);
    return true;
}

// Update LRU counter when a prepared statement is used
void db2_update_prepared_lru_counter(DatabaseHandle* connection, const char* stmt_name) {
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
bool db2_add_prepared_statement(PreparedStatementCache* cache, const char* name) {
    (void)cache;
    (void)name;
    // Stub implementation - actual caching is done through DatabaseHandle
    return true;
}

bool db2_remove_prepared_statement(PreparedStatementCache* cache, const char* name) {
    (void)cache;
    (void)name;
    // Stub implementation - actual caching is done through DatabaseHandle
    return true;
}