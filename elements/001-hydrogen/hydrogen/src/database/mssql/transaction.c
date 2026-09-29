/*
  * MSSQL Database Engine - Transaction Management Implementation
  *
  * Implements MSSQL transaction management functions.
  */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/database/dbqueue/dbqueue.h>
#include <src/utils/utils_uuid.h>

// Local includes
#include "types.h"
#include "transaction.h"
#include "connection.h"

// External declarations for timeout checking (defined in connection.c)
extern bool mssql_check_timeout_expired(time_t start_time, int timeout_seconds);

// External declarations for ODBC function pointers (defined in connection.c)
extern SQLEndTran_t mssql_SQLEndTran_ptr;
extern SQLSetConnectAttr_t mssql_SQLSetConnectAttr_ptr;

// Transaction Management Functions
bool mssql_begin_transaction(DatabaseHandle* connection, DatabaseIsolationLevel level, Transaction** transaction) {
    if (!connection || !transaction || connection->engine_type != DB_ENGINE_MSSQL) {
        return false;
    }

    MSSQLConnection* mssql_conn = (MSSQLConnection*)connection->connection_handle;
    if (!mssql_conn || !mssql_conn->connection) {
        return false;
    }

    const char* log_subsystem = connection->designator ? connection->designator : SR_DATABASE;

    // Auto-commit is already OFF at connection level - start explicit transaction
    if (mssql_SQLEndTran_ptr) {
        int result = mssql_SQLEndTran_ptr(SQL_HANDLE_DBC, mssql_conn->connection, SQL_COMMIT);
        if (result != SQL_SUCCESS) {
            log_this(log_subsystem, "MSSQL failed to start transaction", LOG_LEVEL_ERROR, 0);
            return false;
        }
    }

    // Create transaction structure
    Transaction* tx = calloc(1, sizeof(Transaction));
    if (!tx) {
        return false;
    }

    {
        char uuid_buf[UUID_STR_LEN];
        generate_uuid(uuid_buf);
        tx->transaction_id = strdup(uuid_buf);
    }
    if (!tx->transaction_id) {
        free(tx);
        return false;
    }
    tx->isolation_level = level;
    tx->started_at = time(NULL);
    tx->active = true;

    *transaction = tx;
    connection->current_transaction = tx;

    log_this(log_subsystem, "MSSQL transaction started", LOG_LEVEL_TRACE, 0);
    return true;
}

bool mssql_commit_transaction(DatabaseHandle* connection, Transaction* transaction) {
    if (!connection || !transaction || connection->engine_type != DB_ENGINE_MSSQL) {
        return false;
    }

    MSSQLConnection* mssql_conn = (MSSQLConnection*)connection->connection_handle;
    if (!mssql_conn || !mssql_conn->connection) {
        return false;
    }

    const char* log_subsystem = connection->designator ? connection->designator : SR_DATABASE;

    // Commit transaction using SQLEndTran (connection-level timeout handles timing)
    if (mssql_SQLEndTran_ptr) {
        int result = mssql_SQLEndTran_ptr(SQL_HANDLE_DBC, mssql_conn->connection, SQL_COMMIT);

        if (result != SQL_SUCCESS) {
            log_this(log_subsystem, "MSSQL SQLEndTran commit failed", LOG_LEVEL_ERROR, 0);
            return false;
        }
    }

    // Auto-commit remains OFF - no need to restore

    transaction->active = false;
    connection->current_transaction = NULL;

    log_this(log_subsystem, "MSSQL transaction committed", LOG_LEVEL_TRACE, 0);
    return true;
}

bool mssql_rollback_transaction(DatabaseHandle* connection, Transaction* transaction) {
    if (!connection || !transaction || connection->engine_type != DB_ENGINE_MSSQL) {
        return false;
    }

    MSSQLConnection* mssql_conn = (MSSQLConnection*)connection->connection_handle;
    if (!mssql_conn || !mssql_conn->connection) {
        return false;
    }

    const char* log_subsystem = connection->designator ? connection->designator : SR_DATABASE;

    // Rollback transaction using SQLEndTran (connection-level timeout handles timing)
    if (mssql_SQLEndTran_ptr) {
        int result = mssql_SQLEndTran_ptr(SQL_HANDLE_DBC, mssql_conn->connection, SQL_ROLLBACK);

        if (result != SQL_SUCCESS) {
            log_this(log_subsystem, "MSSQL SQLEndTran rollback failed", LOG_LEVEL_ERROR, 0);
            return false;
        }
    }

    // Auto-commit remains OFF - no need to restore

    transaction->active = false;
    connection->current_transaction = NULL;

    log_this(log_subsystem, "MSSQL transaction rolled back", LOG_LEVEL_TRACE, 0);
    return true;
}
