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

    // Determine isolation level string (SQLite has limited isolation level support)
    const char* isolation_str = "";
    switch (level) {
        case DB_ISOLATION_READ_UNCOMMITTED:
            isolation_str = " READ UNCOMMITTED";
            break;
        case DB_ISOLATION_READ_COMMITTED:
            isolation_str = " READ COMMITTED";
            break;
        case DB_ISOLATION_REPEATABLE_READ:
            isolation_str = " REPEATABLE READ";
            break;
        case DB_ISOLATION_SERIALIZABLE:
            isolation_str = " SERIALIZABLE";
            break;
        default:
            isolation_str = ""; // SQLite default
    }

    // Begin transaction
    char query[256];
    snprintf(query, sizeof(query), "BEGIN%s;", isolation_str);

    char* error_msg = NULL;
    int result = sqlite3_exec_ptr(sqlite_conn->db, query, NULL, NULL, &error_msg);

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

    // Commit transaction
    char* error_msg = NULL;
    int result = sqlite3_exec_ptr(sqlite_conn->db, "COMMIT;", NULL, NULL, &error_msg);

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

    // Rollback transaction
    char* error_msg = NULL;
    int result = sqlite3_exec_ptr(sqlite_conn->db, "ROLLBACK;", NULL, NULL, &error_msg);

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