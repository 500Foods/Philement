/*
  * DB2 Database Engine - Transaction Management Implementation
  *
  * Implements DB2 transaction management functions.
  */
#include "../../hydrogen.h"
#include "../database.h"
#include "../database_queue.h"
#include "types.h"
#include "transaction.h"

// External declarations for libdb2 function pointers (defined in connection.c)
extern SQLEndTran_t SQLEndTran_ptr;

// Transaction Management Functions
bool db2_begin_transaction(DatabaseHandle* connection, DatabaseIsolationLevel level, Transaction** transaction) {
    if (!connection || !transaction || connection->engine_type != DB_ENGINE_DB2) {
        return false;
    }

    const DB2Connection* db2_conn = (const DB2Connection*)connection->connection_handle;
    if (!db2_conn || !db2_conn->connection) {
        return false;
    }

    // DB2 handles transactions automatically by default
    // We can set isolation level if needed, but for basic implementation, just create transaction structure

    // Create transaction structure
    Transaction* tx = calloc(1, sizeof(Transaction));
    if (!tx) {
        return false;
    }

    tx->transaction_id = strdup("db2_tx");
    tx->isolation_level = level;
    tx->started_at = time(NULL);
    tx->active = true;

    *transaction = tx;
    connection->current_transaction = tx;

    log_this(SR_DATABASE, "DB2 transaction started", LOG_LEVEL_TRACE, 0);
    return true;
}

bool db2_commit_transaction(DatabaseHandle* connection, Transaction* transaction) {
    if (!connection || !transaction || connection->engine_type != DB_ENGINE_DB2) {
        return false;
    }

    const DB2Connection* db2_conn = (const DB2Connection*)connection->connection_handle;
    if (!db2_conn || !db2_conn->connection) {
        return false;
    }

    // Commit transaction using SQLEndTran if available
    if (SQLEndTran_ptr) {
        int result = SQLEndTran_ptr(SQL_HANDLE_DBC, db2_conn->connection, SQL_COMMIT);
        if (result != SQL_SUCCESS) {
            log_this(SR_DATABASE, "DB2 SQLEndTran commit failed", LOG_LEVEL_ERROR, 0);
            return false;
        }
    }

    transaction->active = false;
    connection->current_transaction = NULL;

    log_this(SR_DATABASE, "DB2 transaction committed", LOG_LEVEL_TRACE, 0);
    return true;
}

bool db2_rollback_transaction(DatabaseHandle* connection, Transaction* transaction) {
    if (!connection || !transaction || connection->engine_type != DB_ENGINE_DB2) {
        return false;
    }

    const DB2Connection* db2_conn = (const DB2Connection*)connection->connection_handle;
    if (!db2_conn || !db2_conn->connection) {
        return false;
    }

    // Rollback transaction using SQLEndTran if available
    if (SQLEndTran_ptr) {
        int result = SQLEndTran_ptr(SQL_HANDLE_DBC, db2_conn->connection, SQL_ROLLBACK);
        if (result != SQL_SUCCESS) {
            log_this(SR_DATABASE, "DB2 SQLEndTran rollback failed", LOG_LEVEL_ERROR, 0);
            return false;
        }
    }

    transaction->active = false;
    connection->current_transaction = NULL;

    log_this(SR_DATABASE, "DB2 transaction rolled back", LOG_LEVEL_TRACE, 0);
    return true;
}