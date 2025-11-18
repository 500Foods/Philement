/*
  * DB2 Database Engine - Transaction Management Implementation
  *
  * Implements DB2 transaction management functions.
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
extern bool db2_check_timeout_expired(time_t start_time, int timeout_seconds);

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

    // Auto-commit is already OFF at connection level - start explicit transaction
    if (SQLEndTran_ptr) {
        int result = SQLEndTran_ptr(SQL_HANDLE_DBC, db2_conn->connection, SQL_COMMIT);
        if (result != SQL_SUCCESS) {
            log_this(log_subsystem, "DB2 failed to start transaction", LOG_LEVEL_ERROR, 0);
            return false;
        }
    }

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

    log_this(log_subsystem, "DB2 transaction started", LOG_LEVEL_TRACE, 0);
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

    // Commit transaction using SQLEndTran (connection-level timeout handles timing)
    if (SQLEndTran_ptr) {
        int result = SQLEndTran_ptr(SQL_HANDLE_DBC, db2_conn->connection, SQL_COMMIT);

        if (result != SQL_SUCCESS) {
            log_this(log_subsystem, "DB2 SQLEndTran commit failed", LOG_LEVEL_ERROR, 0);
            return false;
        }
    }

    // Auto-commit remains OFF - no need to restore

    transaction->active = false;
    connection->current_transaction = NULL;

    log_this(log_subsystem, "DB2 transaction committed", LOG_LEVEL_TRACE, 0);
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

    // Rollback transaction using SQLEndTran (connection-level timeout handles timing)
    if (SQLEndTran_ptr) {
        int result = SQLEndTran_ptr(SQL_HANDLE_DBC, db2_conn->connection, SQL_ROLLBACK);

        if (result != SQL_SUCCESS) {
            log_this(log_subsystem, "DB2 SQLEndTran rollback failed", LOG_LEVEL_ERROR, 0);
            return false;
        }
    }

    // Auto-commit remains OFF - no need to restore

    transaction->active = false;
    connection->current_transaction = NULL;

    log_this(log_subsystem, "DB2 transaction rolled back", LOG_LEVEL_TRACE, 0);
    return true;
}