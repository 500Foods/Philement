/*
  * MySQL Database Engine - Transaction Management Implementation
  *
  * Implements MySQL transaction management functions.
  */
#include "../../hydrogen.h"
#include "../database.h"
#include "../database_queue.h"
#include "types.h"
#include "connection.h"
#include "transaction.h"

// External declarations for libmysqlclient function pointers (defined in connection.c)
extern mysql_query_t mysql_query_ptr;
extern mysql_autocommit_t mysql_autocommit_ptr;
extern mysql_commit_t mysql_commit_ptr;
extern mysql_rollback_t mysql_rollback_ptr;
extern mysql_error_t mysql_error_ptr;

/*
 * Transaction Management
 */

bool mysql_begin_transaction(DatabaseHandle* connection, DatabaseIsolationLevel level, Transaction** transaction) {
    if (!connection || !transaction || connection->engine_type != DB_ENGINE_MYSQL) {
        return false;
    }

    MySQLConnection* mysql_conn = (MySQLConnection*)connection->connection_handle;
    if (!mysql_conn || !mysql_conn->connection) {
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
            isolation_str = "REPEATABLE READ"; // MySQL default
    }

    // Set isolation level
    if (mysql_query_ptr) {
        char query[256];
        snprintf(query, sizeof(query), "SET SESSION TRANSACTION ISOLATION LEVEL %s", isolation_str);
        if (mysql_query_ptr(mysql_conn->connection, query) != 0) {
            log_this(SR_DATABASE, "MySQL SET ISOLATION LEVEL failed", LOG_LEVEL_ERROR, 0);
            if (mysql_error_ptr) {
                const char* error_msg = mysql_error_ptr(mysql_conn->connection);
                if (error_msg && strlen(error_msg) > 0) {
                    log_this(SR_DATABASE, error_msg, LOG_LEVEL_ERROR, 0);
                }
            }
            return false;
        }
    }

    // Begin transaction
    if (mysql_autocommit_ptr) {
        if (mysql_autocommit_ptr(mysql_conn->connection, 0) != 0) {
            log_this(SR_DATABASE, "MySQL BEGIN TRANSACTION failed", LOG_LEVEL_ERROR, 0);
            if (mysql_error_ptr) {
                const char* error_msg = mysql_error_ptr(mysql_conn->connection);
                if (error_msg && strlen(error_msg) > 0) {
                    log_this(SR_DATABASE, error_msg, LOG_LEVEL_ERROR, 0);
                }
            }
            return false;
        }
    } else if (mysql_query_ptr) {
        if (mysql_query_ptr(mysql_conn->connection, "START TRANSACTION") != 0) {
            log_this(SR_DATABASE, "MySQL START TRANSACTION failed", LOG_LEVEL_ERROR, 0);
            if (mysql_error_ptr) {
                const char* error_msg = mysql_error_ptr(mysql_conn->connection);
                if (error_msg && strlen(error_msg) > 0) {
                    log_this(SR_DATABASE, error_msg, LOG_LEVEL_ERROR, 0);
                }
            }
            return false;
        }
    }

    // Create transaction structure
    Transaction* tx = calloc(1, sizeof(Transaction));
    if (!tx) {
        // Rollback on failure
        if (mysql_autocommit_ptr) {
            mysql_autocommit_ptr(mysql_conn->connection, 1);
        } else if (mysql_query_ptr) {
            mysql_query_ptr(mysql_conn->connection, "ROLLBACK");
        }
        return false;
    }

    tx->transaction_id = strdup("mysql_tx");
    tx->isolation_level = level;
    tx->started_at = time(NULL);
    tx->active = true;

    *transaction = tx;
    connection->current_transaction = tx;

    log_this(SR_DATABASE, "MySQL transaction started", LOG_LEVEL_TRACE, 0);
    return true;
}

bool mysql_commit_transaction(DatabaseHandle* connection, Transaction* transaction) {
    if (!connection || !transaction || connection->engine_type != DB_ENGINE_MYSQL) {
        return false;
    }

    MySQLConnection* mysql_conn = (MySQLConnection*)connection->connection_handle;
    if (!mysql_conn || !mysql_conn->connection) {
        return false;
    }

    // Commit transaction
    if (mysql_commit_ptr) {
        if (mysql_commit_ptr(mysql_conn->connection) != 0) {
            log_this(SR_DATABASE, "MySQL COMMIT failed", LOG_LEVEL_ERROR, 0);
            if (mysql_error_ptr) {
                const char* error_msg = mysql_error_ptr(mysql_conn->connection);
                if (error_msg && strlen(error_msg) > 0) {
                    log_this(SR_DATABASE, error_msg, LOG_LEVEL_ERROR, 0);
                }
            }
            return false;
        }
    } else if (mysql_query_ptr) {
        if (mysql_query_ptr(mysql_conn->connection, "COMMIT") != 0) {
            log_this(SR_DATABASE, "MySQL COMMIT failed", LOG_LEVEL_ERROR, 0);
            if (mysql_error_ptr) {
                const char* error_msg = mysql_error_ptr(mysql_conn->connection);
                if (error_msg && strlen(error_msg) > 0) {
                    log_this(SR_DATABASE, error_msg, LOG_LEVEL_ERROR, 0);
                }
            }
            return false;
        }
    }

    // Re-enable autocommit
    if (mysql_autocommit_ptr) {
        mysql_autocommit_ptr(mysql_conn->connection, 1);
    }

    transaction->active = false;
    connection->current_transaction = NULL;

    log_this(SR_DATABASE, "MySQL transaction committed", LOG_LEVEL_TRACE, 0);
    return true;
}

bool mysql_rollback_transaction(DatabaseHandle* connection, Transaction* transaction) {
    if (!connection || !transaction || connection->engine_type != DB_ENGINE_MYSQL) {
        return false;
    }

    MySQLConnection* mysql_conn = (MySQLConnection*)connection->connection_handle;
    if (!mysql_conn || !mysql_conn->connection) {
        return false;
    }

    // Rollback transaction
    if (mysql_rollback_ptr) {
        if (mysql_rollback_ptr(mysql_conn->connection) != 0) {
            log_this(SR_DATABASE, "MySQL ROLLBACK failed", LOG_LEVEL_ERROR, 0);
            if (mysql_error_ptr) {
                const char* error_msg = mysql_error_ptr(mysql_conn->connection);
                if (error_msg && strlen(error_msg) > 0) {
                    log_this(SR_DATABASE, error_msg, LOG_LEVEL_ERROR, 0);
                }
            }
            return false;
        }
    } else if (mysql_query_ptr) {
        if (mysql_query_ptr(mysql_conn->connection, "ROLLBACK") != 0) {
            log_this(SR_DATABASE, "MySQL ROLLBACK failed", LOG_LEVEL_ERROR, 0);
            if (mysql_error_ptr) {
                const char* error_msg = mysql_error_ptr(mysql_conn->connection);
                if (error_msg && strlen(error_msg) > 0) {
                    log_this(SR_DATABASE, error_msg, LOG_LEVEL_ERROR, 0);
                }
            }
            return false;
        }
    }

    // Re-enable autocommit
    if (mysql_autocommit_ptr) {
        mysql_autocommit_ptr(mysql_conn->connection, 1);
    }

    transaction->active = false;
    connection->current_transaction = NULL;

    log_this(SR_DATABASE, "MySQL transaction rolled back", LOG_LEVEL_TRACE, 0);
    return true;
}