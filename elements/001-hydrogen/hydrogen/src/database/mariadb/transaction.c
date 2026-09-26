/*
 * MariaDB Database Engine - Transaction Management Implementation
 *
 * Implements MariaDB transaction management functions.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/database/dbqueue/dbqueue.h>
#include <src/utils/utils_uuid.h>

// Local includes
#include "types.h"
#include "connection.h"
#include "transaction.h"

// External declarations for timeout checking (defined in connection.c)
extern bool mariadb_check_timeout_expired(time_t start_time, int timeout_seconds);

// External declarations for libmariadb function pointers (defined in connection.c)
extern mariadb_query_t mariadb_query_ptr;
extern mariadb_autocommit_t mariadb_autocommit_ptr;
extern mariadb_commit_t mariadb_commit_ptr;
extern mariadb_rollback_t mariadb_rollback_ptr;
extern mariadb_error_t mariadb_error_ptr;

/*
 * Transaction Management
 */

bool mariadb_begin_transaction(DatabaseHandle* connection, DatabaseIsolationLevel level, Transaction** transaction) {
    if (!connection || !transaction || connection->engine_type != DB_ENGINE_MARIADB) {
        return false;
    }

    MariadbConnection* mariadb_conn = (MariadbConnection*)connection->connection_handle;
    if (!mariadb_conn || !mariadb_conn->connection) {
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
            isolation_str = "REPEATABLE READ"; // MariaDB default
    }

    // Set isolation level with timeout protection
    if (mariadb_query_ptr) {
        char query[256];
        snprintf(query, sizeof(query), "SET SESSION TRANSACTION ISOLATION LEVEL %s", isolation_str);

        time_t start_time = time(NULL);
        int result = mariadb_query_ptr(mariadb_conn->connection, query);

        // Check if operation took too long
        if (mariadb_check_timeout_expired(start_time, 10)) {
            log_this(SR_DATABASE, "MariaDB SET ISOLATION LEVEL execution time exceeded 10 seconds", LOG_LEVEL_ERROR, 0);
            return false;
        }

        if (result != 0) {
            log_this(SR_DATABASE, "MariaDB SET ISOLATION LEVEL failed", LOG_LEVEL_ERROR, 0);
            if (mariadb_error_ptr) {
                const char* error_msg = mariadb_error_ptr(mariadb_conn->connection);
                if (error_msg && strlen(error_msg) > 0) {
                    log_this(SR_DATABASE, error_msg, LOG_LEVEL_ERROR, 0);
                }
            }
            return false;
        }
    }

    // Begin transaction with timeout protection
    time_t begin_time = time(NULL);
    if (mariadb_autocommit_ptr) {
        if (mariadb_autocommit_ptr(mariadb_conn->connection, 0) != 0) {
            log_this(SR_DATABASE, "MariaDB BEGIN TRANSACTION failed", LOG_LEVEL_ERROR, 0);
            if (mariadb_error_ptr) {
                const char* error_msg = mariadb_error_ptr(mariadb_conn->connection);
                if (error_msg && strlen(error_msg) > 0) {
                    log_this(SR_DATABASE, error_msg, LOG_LEVEL_ERROR, 0);
                }
            }
            return false;
        }
    } else if (mariadb_query_ptr) {
        if (mariadb_query_ptr(mariadb_conn->connection, "START TRANSACTION") != 0) {
            log_this(SR_DATABASE, "MariaDB START TRANSACTION failed", LOG_LEVEL_ERROR, 0);
            if (mariadb_error_ptr) {
                const char* error_msg = mariadb_error_ptr(mariadb_conn->connection);
                if (error_msg && strlen(error_msg) > 0) {
                    log_this(SR_DATABASE, error_msg, LOG_LEVEL_ERROR, 0);
                }
            }
            return false;
        }
    }

    // Check if begin transaction took too long
    if (mariadb_check_timeout_expired(begin_time, 10)) {
        log_this(SR_DATABASE, "MariaDB BEGIN TRANSACTION execution time exceeded 10 seconds", LOG_LEVEL_ERROR, 0);
        // Try to restore autocommit
        if (mariadb_autocommit_ptr) {
            mariadb_autocommit_ptr(mariadb_conn->connection, 1);
        }
        return false;
    }

    // Create transaction structure
    Transaction* tx = calloc(1, sizeof(Transaction));
    if (!tx) {
        // Rollback on failure
        if (mariadb_autocommit_ptr) {
            mariadb_autocommit_ptr(mariadb_conn->connection, 1);
        } else if (mariadb_query_ptr) {
            mariadb_query_ptr(mariadb_conn->connection, "ROLLBACK");
        }
        return false;
    }

    {
        char uuid_buf[UUID_STR_LEN];
        generate_uuid(uuid_buf);
        tx->transaction_id = strdup(uuid_buf);
    }
    if (!tx->transaction_id) {
        if (mariadb_autocommit_ptr) {
            mariadb_autocommit_ptr(mariadb_conn->connection, 1);
        } else if (mariadb_query_ptr) {
            mariadb_query_ptr(mariadb_conn->connection, "ROLLBACK");
        }
        free(tx);
        return false;
    }
    tx->isolation_level = level;
    tx->started_at = time(NULL);
    tx->active = true;

    *transaction = tx;
    connection->current_transaction = tx;

    log_this(SR_DATABASE, "MariaDB transaction started", LOG_LEVEL_TRACE, 0);
    return true;
}

bool mariadb_commit_transaction(DatabaseHandle* connection, Transaction* transaction) {
    if (!connection || !transaction || connection->engine_type != DB_ENGINE_MARIADB) {
        return false;
    }

    MariadbConnection* mariadb_conn = (MariadbConnection*)connection->connection_handle;
    if (!mariadb_conn || !mariadb_conn->connection) {
        return false;
    }

    // Commit transaction with timeout protection
    time_t start_time = time(NULL);
    if (mariadb_commit_ptr) {
        if (mariadb_commit_ptr(mariadb_conn->connection) != 0) {
            log_this(SR_DATABASE, "MariaDB COMMIT failed", LOG_LEVEL_ERROR, 0);
            if (mariadb_error_ptr) {
                const char* error_msg = mariadb_error_ptr(mariadb_conn->connection);
                if (error_msg && strlen(error_msg) > 0) {
                    log_this(SR_DATABASE, error_msg, LOG_LEVEL_ERROR, 0);
                }
            }
            return false;
        }
    } else if (mariadb_query_ptr) {
        if (mariadb_query_ptr(mariadb_conn->connection, "COMMIT") != 0) {
            log_this(SR_DATABASE, "MariaDB COMMIT failed", LOG_LEVEL_ERROR, 0);
            if (mariadb_error_ptr) {
                const char* error_msg = mariadb_error_ptr(mariadb_conn->connection);
                if (error_msg && strlen(error_msg) > 0) {
                    log_this(SR_DATABASE, error_msg, LOG_LEVEL_ERROR, 0);
                }
            }
            return false;
        }
    }

    // Check if commit took too long
    if (mariadb_check_timeout_expired(start_time, 10)) {
        log_this(SR_DATABASE, "MariaDB COMMIT execution time exceeded 10 seconds", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Re-enable autocommit
    if (mariadb_autocommit_ptr) {
        mariadb_autocommit_ptr(mariadb_conn->connection, 1);
    }

    transaction->active = false;
    connection->current_transaction = NULL;

    log_this(SR_DATABASE, "MariaDB transaction committed", LOG_LEVEL_TRACE, 0);
    return true;
}

bool mariadb_rollback_transaction(DatabaseHandle* connection, Transaction* transaction) {
    if (!connection || !transaction || connection->engine_type != DB_ENGINE_MARIADB) {
        return false;
    }

    MariadbConnection* mariadb_conn = (MariadbConnection*)connection->connection_handle;
    if (!mariadb_conn || !mariadb_conn->connection) {
        return false;
    }

    // Rollback transaction with timeout protection
    time_t start_time = time(NULL);
    if (mariadb_rollback_ptr) {
        if (mariadb_rollback_ptr(mariadb_conn->connection) != 0) {
            log_this(SR_DATABASE, "MariaDB ROLLBACK failed", LOG_LEVEL_ERROR, 0);
            if (mariadb_error_ptr) {
                const char* error_msg = mariadb_error_ptr(mariadb_conn->connection);
                if (error_msg && strlen(error_msg) > 0) {
                    log_this(SR_DATABASE, error_msg, LOG_LEVEL_ERROR, 0);
                }
            }
            return false;
        }
    } else if (mariadb_query_ptr) {
        if (mariadb_query_ptr(mariadb_conn->connection, "ROLLBACK") != 0) {
            log_this(SR_DATABASE, "MariaDB ROLLBACK failed", LOG_LEVEL_ERROR, 0);
            if (mariadb_error_ptr) {
                const char* error_msg = mariadb_error_ptr(mariadb_conn->connection);
                if (error_msg && strlen(error_msg) > 0) {
                    log_this(SR_DATABASE, error_msg, LOG_LEVEL_ERROR, 0);
                }
            }
            return false;
        }
    }

    // Check if rollback took too long
    if (mariadb_check_timeout_expired(start_time, 10)) {
        log_this(SR_DATABASE, "MariaDB ROLLBACK execution time exceeded 10 seconds", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Re-enable autocommit
    if (mariadb_autocommit_ptr) {
        mariadb_autocommit_ptr(mariadb_conn->connection, 1);
    }

    transaction->active = false;
    connection->current_transaction = NULL;

    log_this(SR_DATABASE, "MariaDB transaction rolled back", LOG_LEVEL_TRACE, 0);
    return true;
}
