/*
  * SQLite Database Engine - Transaction Management Implementation
  *
  * Implements SQLite transaction management functions.
  */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/database/dbqueue/dbqueue.h>

// Local includes
#include "types.h"
#include "transaction.h"
#include "connection.h"

// External declarations for timeout checking (defined in connection.c)
extern bool sqlite_check_timeout_expired(time_t start_time, int timeout_seconds);

// External declarations for libsqlite3 function pointers (defined in connection.c)
extern sqlite3_exec_t sqlite3_exec_ptr;
extern sqlite3_errmsg_t sqlite3_errmsg_ptr;

// Transaction Management Functions
bool sqlite_begin_transaction(DatabaseHandle* connection, DatabaseIsolationLevel level, Transaction** transaction) {
    if (!connection || !transaction || connection->engine_type != DB_ENGINE_SQLITE) {
        return false;
    }

    SQLiteConnection* sqlite_conn = (SQLiteConnection*)connection->connection_handle;
    if (!sqlite_conn || !sqlite_conn->db) {
        return false;
    }

    // SQLite uses different transaction types instead of SQL-92 isolation levels:
    // - DEFERRED (default): lock acquired when first read/write
    // - IMMEDIATE: lock acquired immediately
    // - EXCLUSIVE: exclusive lock
    const char* transaction_type = "";
    switch (level) {
        case DB_ISOLATION_READ_UNCOMMITTED:
            // SQLite's closest match: DEFERRED with read_uncommitted pragma
            transaction_type = " DEFERRED";
            break;
        case DB_ISOLATION_READ_COMMITTED:
            // Use DEFERRED transaction (SQLite's default locking behavior)
            transaction_type = " DEFERRED";
            break;
        case DB_ISOLATION_REPEATABLE_READ:
            // Use IMMEDIATE to lock earlier
            transaction_type = " IMMEDIATE";
            break;
        case DB_ISOLATION_SERIALIZABLE:
            // Use EXCLUSIVE for strongest isolation
            transaction_type = " EXCLUSIVE";
            break;
        default:
            transaction_type = " DEFERRED"; // SQLite default
    }

    // Begin transaction with timeout protection
    char query[256];
    snprintf(query, sizeof(query), "BEGIN%s;", transaction_type);

    time_t start_time = time(NULL);
    char* error_msg = NULL;
    int result = sqlite3_exec_ptr(sqlite_conn->db, query, NULL, NULL, &error_msg);
    
    // Check if operation took too long
    if (sqlite_check_timeout_expired(start_time, 10)) {
        log_this(SR_DATABASE, "SQLite BEGIN TRANSACTION execution time exceeded 10 seconds", LOG_LEVEL_ERROR, 0);
        if (error_msg) {
            free(error_msg);
        }
        return false;
    }

    if (result != SQLITE_OK) {
        log_this(SR_DATABASE, "SQLite BEGIN TRANSACTION failed", LOG_LEVEL_ERROR, 0);
        if (error_msg) {
            log_this(SR_DATABASE, error_msg, LOG_LEVEL_ERROR, 0);
            // Note: sqlite3_free is a macro that calls free() in most implementations
            free(error_msg);
        }
        return false;
    }

    if (error_msg) {
        // Note: sqlite3_free is a macro that calls free() in most implementations
        free(error_msg);
    }

    // Create transaction structure
    Transaction* tx = calloc(1, sizeof(Transaction));
    if (!tx) {
        // Rollback on failure
        sqlite3_exec_ptr(sqlite_conn->db, "ROLLBACK;", NULL, NULL, NULL);
        return false;
    }

    tx->transaction_id = strdup("sqlite_tx");
    tx->isolation_level = level;
    tx->started_at = time(NULL);
    tx->active = true;

    *transaction = tx;
    connection->current_transaction = tx;

    log_this(SR_DATABASE, "SQLite transaction started", LOG_LEVEL_DEBUG, 0);
    return true;
}

bool sqlite_commit_transaction(DatabaseHandle* connection, Transaction* transaction) {
    if (!connection || !transaction || connection->engine_type != DB_ENGINE_SQLITE) {
        return false;
    }

    SQLiteConnection* sqlite_conn = (SQLiteConnection*)connection->connection_handle;
    if (!sqlite_conn || !sqlite_conn->db) {
        return false;
    }

    // Commit transaction with timeout protection
    time_t start_time = time(NULL);
    char* error_msg = NULL;
    int result = sqlite3_exec_ptr(sqlite_conn->db, "COMMIT;", NULL, NULL, &error_msg);
    
    // Check if commit took too long
    if (sqlite_check_timeout_expired(start_time, 10)) {
        log_this(SR_DATABASE, "SQLite COMMIT execution time exceeded 10 seconds", LOG_LEVEL_ERROR, 0);
        if (error_msg) {
            free(error_msg);
        }
        return false;
    }

    if (result != SQLITE_OK) {
        log_this(SR_DATABASE, "SQLite COMMIT failed", LOG_LEVEL_ERROR, 0);
        if (error_msg) {
            log_this(SR_DATABASE, error_msg, LOG_LEVEL_ERROR, 0);
            // Note: sqlite3_free is a macro that calls free() in most implementations
            free(error_msg);
        }
        return false;
    }

    if (error_msg) {
        // Note: sqlite3_free is a macro that calls free() in most implementations
        free(error_msg);
    }

    transaction->active = false;
    connection->current_transaction = NULL;

    log_this(SR_DATABASE, "SQLite transaction committed", LOG_LEVEL_DEBUG, 0);
    return true;
}

bool sqlite_rollback_transaction(DatabaseHandle* connection, Transaction* transaction) {
    if (!connection || !transaction || connection->engine_type != DB_ENGINE_SQLITE) {
        return false;
    }

    SQLiteConnection* sqlite_conn = (SQLiteConnection*)connection->connection_handle;
    if (!sqlite_conn || !sqlite_conn->db) {
        return false;
    }

    // Rollback transaction with timeout protection
    time_t start_time = time(NULL);
    char* error_msg = NULL;
    int result = sqlite3_exec_ptr(sqlite_conn->db, "ROLLBACK;", NULL, NULL, &error_msg);
    
    // Check if rollback took too long
    if (sqlite_check_timeout_expired(start_time, 10)) {
        log_this(SR_DATABASE, "SQLite ROLLBACK execution time exceeded 10 seconds", LOG_LEVEL_ERROR, 0);
        if (error_msg) {
            free(error_msg);
        }
        return false;
    }

    if (result != SQLITE_OK) {
        log_this(SR_DATABASE, "SQLite ROLLBACK failed", LOG_LEVEL_ERROR, 0);
        if (error_msg) {
            log_this(SR_DATABASE, error_msg, LOG_LEVEL_ERROR, 0);
            // Note: sqlite3_free is a macro that calls free() in most implementations
            free(error_msg);
        }
        return false;
    }

    if (error_msg) {
        // Note: sqlite3_free is a macro that calls free() in most implementations
        free(error_msg);
    }

    transaction->active = false;
    connection->current_transaction = NULL;

    log_this(SR_DATABASE, "SQLite transaction rolled back", LOG_LEVEL_DEBUG, 0);
    return true;
}