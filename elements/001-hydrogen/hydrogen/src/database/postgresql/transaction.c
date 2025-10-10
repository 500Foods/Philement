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

    // Determine isolation level string
    const char* isolation_str;
    switch (level) {
        case DB_ISOLATION_READ_UNCOMMITTED:
            isolation_str = "READ UNCOMMITTED";
            break;
        case DB_ISOLATION_READ_COMMITTED:
            isolation_str = "READ COMMITTED";
            break;
        case DB_ISOLATION_REPEATABLE_READ:
            isolation_str = "REPEATABLE READ";
            break;
        case DB_ISOLATION_SERIALIZABLE:
            isolation_str = "SERIALIZABLE";
            break;
        default:
            isolation_str = "READ COMMITTED";
    }

    // Begin transaction with timeout protection
    char query[256];
    snprintf(query, sizeof(query), "BEGIN ISOLATION LEVEL %s", isolation_str);

    // Set timeout for transaction operations
    void* timeout_result = PQexec_ptr(pg_conn->connection, "SET statement_timeout = 10000"); // 10 seconds
    if (timeout_result) {
        PQclear_ptr(timeout_result);
    }

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
        // Rollback on failure with timeout protection
        time_t rollback_start = time(NULL);
        rollback_res = PQexec_ptr(pg_conn->connection, "ROLLBACK");

        // Check if rollback took too long
        if (check_timeout_expired(rollback_start, 5)) {
            log_this(SR_DATABASE, "PostgreSQL ROLLBACK on failure execution time exceeded 5 seconds", LOG_LEVEL_ERROR, 0);
        }

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

    // Set timeout for commit operation
    void* timeout_result = PQexec_ptr(pg_conn->connection, "SET statement_timeout = 10000"); // 10 seconds
    if (timeout_result) {
        PQclear_ptr(timeout_result);
    }

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

    // Set timeout for rollback operation
    void* timeout_result = PQexec_ptr(pg_conn->connection, "SET statement_timeout = 10000"); // 10 seconds
    if (timeout_result) {
        PQclear_ptr(timeout_result);
    }

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