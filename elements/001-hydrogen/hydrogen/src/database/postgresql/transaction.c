/*
 * PostgreSQL Database Engine - Transaction Management Implementation
 *
 * Implements PostgreSQL transaction management functions.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/database/dbqueue/dbqueue.h>

// Local includes
#include "types.h"
#include "connection.h"
#include "transaction.h"

// External declarations for libpq function pointers (defined in connection.c)
extern PQexec_t PQexec_ptr;
extern PQresultStatus_t PQresultStatus_ptr;
extern PQclear_t PQclear_ptr;

// External declarations for constants (defined in connection.c)
extern bool check_timeout_expired(time_t start_time, int timeout_seconds);

// Transaction Management Functions
bool postgresql_begin_transaction(DatabaseHandle* connection, DatabaseIsolationLevel level, Transaction** transaction) {
    if (!connection || !transaction || connection->engine_type != DB_ENGINE_POSTGRESQL) {
        return false;
    }

    PostgresConnection* pg_conn = (PostgresConnection*)connection->connection_handle;
    if (!pg_conn || !pg_conn->connection || pg_conn->in_transaction) {
        return false;
    }

    // Begin transaction with simplified command (default isolation level is READ COMMITTED)
    // This eliminates unnecessary command complexity and parsing overhead
    // Note: PostgreSQL's default isolation level is READ COMMITTED, so no explicit level needed
    const char* query = "BEGIN";

    // Note: Timeout is now set once per connection in postgresql_connect()
    // No need to set it before each transaction operation

    time_t start_time = time(NULL);
    void* res = PQexec_ptr(pg_conn->connection, query);

    // Check if query took too long (approximate check)
    if (check_timeout_expired(start_time, 10)) {
        log_this(SR_DATABASE, "PostgreSQL BEGIN TRANSACTION execution time exceeded 10 seconds", LOG_LEVEL_ERROR, 0);
        if (res) PQclear_ptr(res);
        return false;
    }

    if (PQresultStatus_ptr(res) != PGRES_COMMAND_OK) {
        log_this(SR_DATABASE, "PostgreSQL BEGIN TRANSACTION failed", LOG_LEVEL_ERROR, 0);
        PQclear_ptr(res);
        return false;
    }
    PQclear_ptr(res);

    pg_conn->in_transaction = true;

    // Variables that might be affected by longjmp - declare ALL before any setjmp
    Transaction* tx = NULL;
    void* rollback_res = NULL;
    char* transaction_id = NULL;

    // Create transaction structure
    tx = calloc(1, sizeof(Transaction));
    if (!tx) {
        // Rollback on failure (timeout already set at connection level)
        rollback_res = PQexec_ptr(pg_conn->connection, "ROLLBACK");
        if (rollback_res) PQclear_ptr(rollback_res);
        pg_conn->in_transaction = false;
        return false;
    }

    transaction_id = strdup("postgresql_tx"); // TODO: Generate unique ID
    tx->transaction_id = transaction_id;
    tx->isolation_level = level;
    tx->started_at = time(NULL);
    tx->active = true;

    *transaction = tx;
    connection->current_transaction = tx;

    // log_this(SR_DATABASE, "PostgreSQL transaction started", LOG_LEVEL_TRACE, 0);
    return true;
}

bool postgresql_commit_transaction(DatabaseHandle* connection, Transaction* transaction) {
    if (!connection || !transaction || connection->engine_type != DB_ENGINE_POSTGRESQL) {
        return false;
    }

    PostgresConnection* pg_conn = (PostgresConnection*)connection->connection_handle;
    if (!pg_conn || !pg_conn->connection || !pg_conn->in_transaction) {
        return false;
    }

    // Note: Timeout is now set once per connection in postgresql_connect()
    // No need to set it before each commit operation

    time_t start_time = time(NULL);
    void* res = PQexec_ptr(pg_conn->connection, "COMMIT");

    // Check if commit took too long
    if (check_timeout_expired(start_time, 10)) {
        log_this(SR_DATABASE, "PostgreSQL COMMIT execution time exceeded 10 seconds", LOG_LEVEL_ERROR, 0);
        if (res) PQclear_ptr(res);
        return false;
    }

    if (PQresultStatus_ptr(res) != PGRES_COMMAND_OK) {
        log_this(SR_DATABASE, "PostgreSQL COMMIT failed", LOG_LEVEL_ERROR, 0);
        PQclear_ptr(res);
        return false;
    }
    PQclear_ptr(res);

    pg_conn->in_transaction = false;
    transaction->active = false;
    connection->current_transaction = NULL;

    // log_this(SR_DATABASE, "PostgreSQL transaction committed", LOG_LEVEL_TRACE, 0);
    return true;
}

bool postgresql_rollback_transaction(DatabaseHandle* connection, Transaction* transaction) {
    if (!connection || !transaction || connection->engine_type != DB_ENGINE_POSTGRESQL) {
        return false;
    }

    PostgresConnection* pg_conn = (PostgresConnection*)connection->connection_handle;
    if (!pg_conn || !pg_conn->connection) {
        return false;
    }

    // Note: Timeout is now set once per connection in postgresql_connect()
    // No need to set it before each rollback operation

    time_t start_time = time(NULL);
    void* res = PQexec_ptr(pg_conn->connection, "ROLLBACK");

    // Check if rollback took too long
    if (check_timeout_expired(start_time, 10)) {
        log_this(SR_DATABASE, "PostgreSQL ROLLBACK execution time exceeded 10 seconds", LOG_LEVEL_ERROR, 0);
        if (res) PQclear_ptr(res);
        return false;
    }

    if (PQresultStatus_ptr(res) != PGRES_COMMAND_OK) {
        log_this(SR_DATABASE, "PostgreSQL ROLLBACK failed", LOG_LEVEL_ERROR, 0);
        PQclear_ptr(res);
        return false;
    }
    PQclear_ptr(res);

    pg_conn->in_transaction = false;
    transaction->active = false;
    connection->current_transaction = NULL;

    // log_this(SR_DATABASE, "PostgreSQL transaction rolled back", LOG_LEVEL_TRACE, 0);
    return true;
}