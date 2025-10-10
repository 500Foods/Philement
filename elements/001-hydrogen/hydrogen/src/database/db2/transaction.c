/*
  * DB2 Database Engine - Transaction Management Implementation
  *
  * Implements DB2 transaction management functions.
  */
#include "../../hydrogen.h"
#include "../database.h"
#include "../queue/database_queue.h"
#include "types.h"
#include "transaction.h"

// External declarations for libdb2 function pointers (defined in connection.c)
extern SQLEndTran_t SQLEndTran_ptr;
extern SQLSetConnectAttr_t SQLSetConnectAttr_ptr;

// Transaction Management Functions
bool db2_begin_transaction(DatabaseHandle* connection, DatabaseIsolationLevel level, Transaction** transaction) {
    if (!connection || !transaction || connection->engine_type != DB_ENGINE_DB2) {
        return false;
    }

    DB2Connection* db2_conn = (DB2Connection*)connection->connection_handle;
    if (!db2_conn || !db2_conn->connection) {
        return false;
    }

    const char* log_subsystem = connection->designator ? connection->designator : SR_DATABASE;

    // Turn off auto-commit to start a transaction
    if (SQLSetConnectAttr_ptr) {
        int result = SQLSetConnectAttr_ptr(db2_conn->connection, SQL_ATTR_AUTOCOMMIT, (long)SQL_AUTOCOMMIT_OFF, 0);
        if (result != SQL_SUCCESS) {
            log_this(log_subsystem, "DB2 failed to turn off auto-commit for transaction", LOG_LEVEL_ERROR, 0);
            return false;
        }
    } else {
        log_this(log_subsystem, "DB2 SQLSetConnectAttr not available - cannot control auto-commit", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Create transaction structure
    Transaction* tx = calloc(1, sizeof(Transaction));
    if (!tx) {
        // Restore auto-commit on failure
        SQLSetConnectAttr_ptr(db2_conn->connection, SQL_ATTR_AUTOCOMMIT, (long)SQL_AUTOCOMMIT_ON, 0);
        return false;
    }

    tx->transaction_id = strdup("db2_tx");
    tx->isolation_level = level;
    tx->started_at = time(NULL);
    tx->active = true;

    *transaction = tx;
    connection->current_transaction = tx;

    log_this(log_subsystem, "DB2 transaction started (auto-commit disabled)", LOG_LEVEL_TRACE, 0);
    return true;
}

bool db2_commit_transaction(DatabaseHandle* connection, Transaction* transaction) {
    if (!connection || !transaction || connection->engine_type != DB_ENGINE_DB2) {
        return false;
    }

    DB2Connection* db2_conn = (DB2Connection*)connection->connection_handle;
    if (!db2_conn || !db2_conn->connection) {
        return false;
    }

    const char* log_subsystem = connection->designator ? connection->designator : SR_DATABASE;

    // Commit transaction using SQLEndTran if available
    if (SQLEndTran_ptr) {
        int result = SQLEndTran_ptr(SQL_HANDLE_DBC, db2_conn->connection, SQL_COMMIT);
        if (result != SQL_SUCCESS) {
            log_this(log_subsystem, "DB2 SQLEndTran commit failed", LOG_LEVEL_ERROR, 0);
            return false;
        }
    }

    // Restore auto-commit
    if (SQLSetConnectAttr_ptr) {
        SQLSetConnectAttr_ptr(db2_conn->connection, SQL_ATTR_AUTOCOMMIT, (long)SQL_AUTOCOMMIT_ON, 0);
    }

    transaction->active = false;
    connection->current_transaction = NULL;

    log_this(log_subsystem, "DB2 transaction committed (auto-commit restored)", LOG_LEVEL_TRACE, 0);
    return true;
}

bool db2_rollback_transaction(DatabaseHandle* connection, Transaction* transaction) {
    if (!connection || !transaction || connection->engine_type != DB_ENGINE_DB2) {
        return false;
    }

    DB2Connection* db2_conn = (DB2Connection*)connection->connection_handle;
    if (!db2_conn || !db2_conn->connection) {
        return false;
    }

    const char* log_subsystem = connection->designator ? connection->designator : SR_DATABASE;

    // Rollback transaction using SQLEndTran if available
    if (SQLEndTran_ptr) {
        int result = SQLEndTran_ptr(SQL_HANDLE_DBC, db2_conn->connection, SQL_ROLLBACK);
        if (result != SQL_SUCCESS) {
            log_this(log_subsystem, "DB2 SQLEndTran rollback failed", LOG_LEVEL_ERROR, 0);
            return false;
        }
    }

    // Restore auto-commit
    if (SQLSetConnectAttr_ptr) {
        SQLSetConnectAttr_ptr(db2_conn->connection, SQL_ATTR_AUTOCOMMIT, (long)SQL_AUTOCOMMIT_ON, 0);
    }

    transaction->active = false;
    connection->current_transaction = NULL;

    log_this(log_subsystem, "DB2 transaction rolled back (auto-commit restored)", LOG_LEVEL_TRACE, 0);
    return true;
}