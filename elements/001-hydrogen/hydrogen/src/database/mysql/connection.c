/*
 * MySQL Database Engine - Connection Management Implementation
 *
 * Implements MySQL connection management functions.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/database/dbqueue/dbqueue.h>

// Local includes
#include "types.h"
#include "connection.h"

#ifdef USE_MOCK_LIBMYSQLCLIENT
#include <unity/mocks/mock_libmysqlclient.h>
#endif

// MySQL function pointers (loaded dynamically)
#ifdef USE_MOCK_LIBMYSQLCLIENT
// For mocking, assign all mock function pointers
mysql_init_t mysql_init_ptr = mock_mysql_init;
mysql_real_connect_t mysql_real_connect_ptr = mock_mysql_real_connect;
mysql_query_t mysql_query_ptr = mock_mysql_query;
mysql_store_result_t mysql_store_result_ptr = mock_mysql_store_result;
mysql_num_rows_t mysql_num_rows_ptr = NULL;
mysql_num_fields_t mysql_num_fields_ptr = NULL;
mysql_fetch_row_t mysql_fetch_row_ptr = NULL;
mysql_fetch_fields_t mysql_fetch_fields_ptr = NULL;
mysql_free_result_t mysql_free_result_ptr = mock_mysql_free_result;
mysql_error_t mysql_error_ptr = mock_mysql_error;
mysql_close_t mysql_close_ptr = mock_mysql_close;
mysql_options_t mysql_options_ptr = mock_mysql_options;
mysql_ping_t mysql_ping_ptr = mock_mysql_ping;
mysql_autocommit_t mysql_autocommit_ptr = mock_mysql_autocommit;
mysql_commit_t mysql_commit_ptr = mock_mysql_commit;
mysql_rollback_t mysql_rollback_ptr = mock_mysql_rollback;
mysql_stmt_init_t mysql_stmt_init_ptr = NULL;
mysql_stmt_prepare_t mysql_stmt_prepare_ptr = NULL;
mysql_stmt_execute_t mysql_stmt_execute_ptr = NULL;
mysql_stmt_close_t mysql_stmt_close_ptr = NULL;
#else
mysql_init_t mysql_init_ptr = NULL;
mysql_real_connect_t mysql_real_connect_ptr = NULL;
mysql_query_t mysql_query_ptr = NULL;
mysql_store_result_t mysql_store_result_ptr = NULL;
mysql_num_rows_t mysql_num_rows_ptr = NULL;
mysql_num_fields_t mysql_num_fields_ptr = NULL;
mysql_fetch_row_t mysql_fetch_row_ptr = NULL;
mysql_fetch_fields_t mysql_fetch_fields_ptr = NULL;
mysql_free_result_t mysql_free_result_ptr = NULL;
mysql_error_t mysql_error_ptr = NULL;
mysql_close_t mysql_close_ptr = NULL;
mysql_options_t mysql_options_ptr = NULL;
mysql_ping_t mysql_ping_ptr = NULL;
mysql_autocommit_t mysql_autocommit_ptr = NULL;
mysql_commit_t mysql_commit_ptr = NULL;
mysql_rollback_t mysql_rollback_ptr = NULL;
mysql_stmt_init_t mysql_stmt_init_ptr = NULL;
mysql_stmt_prepare_t mysql_stmt_prepare_ptr = NULL;
mysql_stmt_execute_t mysql_stmt_execute_ptr = NULL;
mysql_stmt_close_t mysql_stmt_close_ptr = NULL;
#endif

// Library handle
#ifndef USE_MOCK_LIBMYSQLCLIENT
static void* libmysql_handle = NULL;
static pthread_mutex_t libmysql_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

/*
 * Library Loading Functions
 */

bool load_libmysql_functions(const char* designator __attribute__((unused))) {
#ifdef USE_MOCK_LIBMYSQLCLIENT
    // For mocking, functions are already set
    return true;
#else
    if (libmysql_handle) {
        return true; // Already loaded
    }

    const char* log_subsystem = designator ? designator : SR_DATABASE;
    MUTEX_LOCK(&libmysql_mutex, log_subsystem);

    if (libmysql_handle) {
        MUTEX_UNLOCK(&libmysql_mutex, log_subsystem);
        return true; // Another thread loaded it
    }

    // Try to load libmysqlclient
    libmysql_handle = dlopen("libmysqlclient.so.21", RTLD_LAZY);
    if (!libmysql_handle) {
        libmysql_handle = dlopen("libmysqlclient.so.18", RTLD_LAZY);
    }
    if (!libmysql_handle) {
        libmysql_handle = dlopen("libmysqlclient.so.20", RTLD_LAZY);
    }
    if (!libmysql_handle) {
        libmysql_handle = dlopen("libmysqlclient.so", RTLD_LAZY);
    }
    if (!libmysql_handle) {
        log_this(log_subsystem, "Failed to load libmysqlclient library", LOG_LEVEL_ERROR, 0);
        log_this(log_subsystem, dlerror(), LOG_LEVEL_ERROR, 0);
        MUTEX_UNLOCK(&libmysql_mutex, log_subsystem);
        return false;
    }

    // Load function pointers
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    mysql_init_ptr = (mysql_init_t)dlsym(libmysql_handle, "mysql_init");
    mysql_real_connect_ptr = (mysql_real_connect_t)dlsym(libmysql_handle, "mysql_real_connect");
    mysql_query_ptr = (mysql_query_t)dlsym(libmysql_handle, "mysql_query");
    mysql_store_result_ptr = (mysql_store_result_t)dlsym(libmysql_handle, "mysql_store_result");
    mysql_num_rows_ptr = (mysql_num_rows_t)dlsym(libmysql_handle, "mysql_num_rows");
    mysql_num_fields_ptr = (mysql_num_fields_t)dlsym(libmysql_handle, "mysql_num_fields");
    mysql_fetch_row_ptr = (mysql_fetch_row_t)dlsym(libmysql_handle, "mysql_fetch_row");
    mysql_fetch_fields_ptr = (mysql_fetch_fields_t)dlsym(libmysql_handle, "mysql_fetch_fields");
    mysql_free_result_ptr = (mysql_free_result_t)dlsym(libmysql_handle, "mysql_free_result");
    mysql_error_ptr = (mysql_error_t)dlsym(libmysql_handle, "mysql_error");
    mysql_close_ptr = (mysql_close_t)dlsym(libmysql_handle, "mysql_close");
    mysql_options_ptr = (mysql_options_t)dlsym(libmysql_handle, "mysql_options");
    mysql_ping_ptr = (mysql_ping_t)dlsym(libmysql_handle, "mysql_ping");
    mysql_autocommit_ptr = (mysql_autocommit_t)dlsym(libmysql_handle, "mysql_autocommit");
    mysql_commit_ptr = (mysql_commit_t)dlsym(libmysql_handle, "mysql_commit");
    mysql_rollback_ptr = (mysql_rollback_t)dlsym(libmysql_handle, "mysql_rollback");
    mysql_stmt_init_ptr = (mysql_stmt_init_t)dlsym(libmysql_handle, "mysql_stmt_init");
    mysql_stmt_prepare_ptr = (mysql_stmt_prepare_t)dlsym(libmysql_handle, "mysql_stmt_prepare");
    mysql_stmt_execute_ptr = (mysql_stmt_execute_t)dlsym(libmysql_handle, "mysql_stmt_execute");
    mysql_stmt_close_ptr = (mysql_stmt_close_t)dlsym(libmysql_handle, "mysql_stmt_close");
#pragma GCC diagnostic pop

    // Check if all required functions were loaded
    if (!mysql_init_ptr || !mysql_real_connect_ptr || !mysql_query_ptr ||
        !mysql_store_result_ptr || !mysql_num_rows_ptr || !mysql_num_fields_ptr ||
        !mysql_fetch_row_ptr || !mysql_fetch_fields_ptr || !mysql_free_result_ptr ||
        !mysql_error_ptr || !mysql_close_ptr) {
        log_this(log_subsystem, "Failed to load all required libmysqlclient functions", LOG_LEVEL_ERROR, 0);
        dlclose(libmysql_handle);
        libmysql_handle = NULL;
        MUTEX_UNLOCK(&libmysql_mutex, log_subsystem);
        return false;
    }

    // Optional functions - log if not available
    if (!mysql_options_ptr) {
        log_this(log_subsystem, "mysql_options function not available - connection options will be limited", LOG_LEVEL_TRACE, 0);
    }
    if (!mysql_ping_ptr) {
        log_this(log_subsystem, "mysql_ping function not available - health check will use query method only", LOG_LEVEL_TRACE, 0);
    }
    if (!mysql_autocommit_ptr || !mysql_commit_ptr || !mysql_rollback_ptr) {
        log_this(log_subsystem, "Transaction functions not available - transactions will be limited", LOG_LEVEL_DEBUG, 0);
    }
    if (!mysql_stmt_init_ptr || !mysql_stmt_prepare_ptr || !mysql_stmt_execute_ptr || !mysql_stmt_close_ptr) {
        log_this(log_subsystem, "Prepared statement functions not available - prepared statements will be limited", LOG_LEVEL_TRACE, 0);
    }

    MUTEX_UNLOCK(&libmysql_mutex, log_subsystem);
    log_this(log_subsystem, "Successfully loaded libmysqlclient library", LOG_LEVEL_TRACE, 0);
    return true;
#endif
}

/*
 * Utility Functions
 */

PreparedStatementCache* mysql_create_prepared_statement_cache(void) {
    PreparedStatementCache* cache = calloc(1, sizeof(PreparedStatementCache));
    if (!cache) return NULL;

    cache->capacity = 16;
    cache->names = calloc(cache->capacity, sizeof(char*));
    if (!cache->names) {
        free(cache);
        return NULL;
    }

    pthread_mutex_init(&cache->lock, NULL);
    return cache;
}

void mysql_destroy_prepared_statement_cache(PreparedStatementCache* cache) {
    if (!cache) return;

    MUTEX_LOCK(&cache->lock, SR_DATABASE);
    for (size_t i = 0; i < cache->count; i++) {
        free(cache->names[i]);
    }
    free(cache->names);
    MUTEX_UNLOCK(&cache->lock, SR_DATABASE);
    pthread_mutex_destroy(&cache->lock);
    free(cache);
}

/*
 * Connection Management
 */

bool mysql_connect(ConnectionConfig* config, DatabaseHandle** connection, const char* designator) {
    if (!config || !connection) {
        log_this(SR_DATABASE, "Invalid parameters for MySQL connection", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Load libmysqlclient library if not already loaded
    if (!load_libmysql_functions(designator)) {
        const char* log_subsystem = designator ? designator : SR_DATABASE;
        log_this(log_subsystem, "MySQL library not available", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Initialize MySQL connection
    void* mysql_conn = mysql_init_ptr(NULL);
    if (!mysql_conn) {
        log_this(SR_DATABASE, "MySQL connection initialization failed", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Set auto-reconnect option
    if (mysql_options_ptr) {
        int reconnect = 1;
        mysql_options_ptr(mysql_conn, MYSQL_OPT_RECONNECT, &reconnect);
    }

    // Establish connection
    void* result = mysql_real_connect_ptr(
        mysql_conn,
        config->host ? config->host : "localhost",
        config->username ? config->username : "",
        config->password ? config->password : "",
        config->database ? config->database : "",
        (unsigned int)(config->port > 0 ? config->port : 3306),
        NULL,  // unix_socket
        0      // client_flag
    );

    if (!result) {
        log_this(SR_DATABASE, "MySQL connection failed", LOG_LEVEL_ERROR, 0);
        if (mysql_error_ptr) {
            const char* error_msg = mysql_error_ptr(mysql_conn);
            if (error_msg && strlen(error_msg) > 0) {
                log_this(SR_DATABASE, error_msg, LOG_LEVEL_ERROR, 0);
            }
        }
        if (mysql_close_ptr) {
            mysql_close_ptr(mysql_conn);
        }
        return false;
    }

    // Create database handle
    DatabaseHandle* db_handle = calloc(1, sizeof(DatabaseHandle));
    if (!db_handle) {
        // mysql_close_ptr(mysql_conn);
        return false;
    }

    // Create MySQL-specific connection wrapper
    MySQLConnection* mysql_wrapper = calloc(1, sizeof(MySQLConnection));
    if (!mysql_wrapper) {
        free(db_handle);
        // mysql_close_ptr(mysql_conn);
        return false;
    }

    mysql_wrapper->connection = mysql_conn;
    mysql_wrapper->reconnect = true;
    mysql_wrapper->prepared_statements = mysql_create_prepared_statement_cache();
    if (!mysql_wrapper->prepared_statements) {
        free(mysql_wrapper);
        free(db_handle);
        // mysql_close_ptr(mysql_conn);
        return false;
    }

    // Store designator for future use (disconnect, etc.)
    db_handle->designator = designator ? strdup(designator) : NULL;

    // Initialize database handle
    db_handle->engine_type = DB_ENGINE_MYSQL;
    db_handle->connection_handle = mysql_wrapper;
    db_handle->config = config;
    db_handle->status = DB_CONNECTION_CONNECTED;
    db_handle->connected_since = time(NULL);
    db_handle->current_transaction = NULL;
    db_handle->prepared_statements = NULL; // Will be managed by engine-specific code
    db_handle->prepared_statement_count = 0;
    pthread_mutex_init(&db_handle->connection_lock, NULL);
    db_handle->in_use = false;
    db_handle->last_health_check = time(NULL);
    db_handle->consecutive_failures = 0;

    *connection = db_handle;

    // Use designator for logging if provided, otherwise use generic Database subsystem
    const char* log_subsystem = designator ? designator : SR_DATABASE;
    log_this(log_subsystem, "MySQL connection established successfully", LOG_LEVEL_TRACE, 0);
    return true;
}

bool mysql_disconnect(DatabaseHandle* connection) {
    if (!connection || connection->engine_type != DB_ENGINE_MYSQL) {
        return false;
    }

    MySQLConnection* mysql_conn = (MySQLConnection*)connection->connection_handle;
    if (mysql_conn) {
        if (mysql_conn->connection && mysql_close_ptr) {
            mysql_close_ptr(mysql_conn->connection);
        }
        mysql_destroy_prepared_statement_cache(mysql_conn->prepared_statements);
        free(mysql_conn);
    }

    connection->status = DB_CONNECTION_DISCONNECTED;

    // Use stored designator for logging if available
    const char* log_subsystem = connection->designator ? connection->designator : SR_DATABASE;
    log_this(log_subsystem, "MySQL connection closed", LOG_LEVEL_TRACE, 0);
    return true;
}

bool mysql_health_check(DatabaseHandle* connection) {
    if (!connection) {
        const char* designator = SR_DATABASE;
        log_this(designator, "MySQL health check: connection is NULL", LOG_LEVEL_ERROR, 0);
        return false;
    }

    const char* designator = connection->designator ? connection->designator : SR_DATABASE;
    log_this(designator, "MySQL health check: Starting validation", LOG_LEVEL_TRACE, 0);

    if (connection->engine_type != DB_ENGINE_MYSQL) {
        log_this(designator, "MySQL health check: wrong engine type %d", LOG_LEVEL_ERROR, 1, connection->engine_type);
        return false;
    }

    MySQLConnection* mysql_conn = (MySQLConnection*)connection->connection_handle;
    if (!mysql_conn) {
        log_this(designator, "MySQL health check: mysql_conn is NULL", LOG_LEVEL_ERROR, 0);
        return false;
    }

    if (!mysql_conn->connection) {
        log_this(designator, "MySQL health check: mysql_conn->connection is NULL", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Function pointer validation
    if (!mysql_ping_ptr && !mysql_query_ptr) {
        log_this(designator, "MySQL health check: neither mysql_ping_ptr nor mysql_query_ptr available", LOG_LEVEL_ERROR, 0);
        return false;
    }

    log_this(designator, "MySQL health check: All validations passed, executing health check", LOG_LEVEL_TRACE, 0);

    // Try ping method first if available
    if (mysql_ping_ptr) {
        log_this(designator, "MySQL health check: Trying mysql_ping method", LOG_LEVEL_TRACE, 0);
        int ping_result = mysql_ping_ptr(mysql_conn->connection);
        log_this(designator, "MySQL health check: mysql_ping result: %d", LOG_LEVEL_TRACE, 1, ping_result);

        if (ping_result == 0) {
            log_this(designator, "MySQL health check passed via mysql_ping", LOG_LEVEL_TRACE, 0);
            connection->last_health_check = time(NULL);
            connection->consecutive_failures = 0;
            return true;
        } else {
            log_this(designator, "MySQL health check: mysql_ping failed, trying query method", LOG_LEVEL_TRACE, 0);
        }
    }

    // Fallback to query method
    if (mysql_query_ptr) {
        log_this(designator, "MySQL health check: Executing 'SELECT 1'", LOG_LEVEL_TRACE, 0);

        if (mysql_query_ptr(mysql_conn->connection, "SELECT 1") != 0) {
            log_this(designator, "MySQL health check: Query failed", LOG_LEVEL_ERROR, 0);
            if (mysql_error_ptr) {
                const char* error_msg = mysql_error_ptr(mysql_conn->connection);
                if (error_msg && strlen(error_msg) > 0) {
                    log_this(designator, "MySQL health check error: %s", LOG_LEVEL_ERROR, 1, error_msg);
                }
            }
            connection->consecutive_failures++;
            return false;
        }

        // Store and free result
        if (mysql_store_result_ptr) {
            void* result = mysql_store_result_ptr(mysql_conn->connection);
            if (result && mysql_free_result_ptr) {
                mysql_free_result_ptr(result);
            }
        }

        log_this(designator, "MySQL health check passed via query", LOG_LEVEL_TRACE, 0);
        connection->last_health_check = time(NULL);
        connection->consecutive_failures = 0;
        return true;
    }

    log_this(designator, "MySQL health check: No health check method available", LOG_LEVEL_ERROR, 0);
    return false;
}

bool mysql_reset_connection(DatabaseHandle* connection) {
    if (!connection || connection->engine_type != DB_ENGINE_MYSQL) {
        return false;
    }

    // MySQL auto-reconnect should handle this
    connection->status = DB_CONNECTION_CONNECTED;
    connection->connected_since = time(NULL);
    connection->consecutive_failures = 0;

    log_this(SR_DATABASE, "MySQL connection reset successfully", LOG_LEVEL_TRACE, 0);
    return true;
}