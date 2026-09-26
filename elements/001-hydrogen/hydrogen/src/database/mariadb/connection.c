/*
 * MariaDB Database Engine - Connection Management Implementation
 *
 * Implements MariaDB connection management functions.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/database/dbqueue/dbqueue.h>

// Local includes
#include "types.h"
#include "connection.h"

#ifdef USE_MOCK_LIBMARIADB
#include <unity/mocks/mock_libmariadb.h>
#endif

// MariaDB function pointers (loaded dynamically)
#ifdef USE_MOCK_LIBMARIADB
// For mocking, assign all mock function pointers
mariadb_init_t mariadb_init_ptr = mock_mariadb_init;
mariadb_real_connect_t mariadb_real_connect_ptr = mock_mariadb_real_connect;
mariadb_query_t mariadb_query_ptr = mock_mariadb_query;
mariadb_store_result_t mariadb_store_result_ptr = mock_mariadb_store_result;
mariadb_num_rows_t mariadb_num_rows_ptr = mock_mariadb_num_rows;
mariadb_num_fields_t mariadb_num_fields_ptr = mock_mariadb_num_fields;
mariadb_fetch_row_t mariadb_fetch_row_ptr = mock_mariadb_fetch_row;
mariadb_fetch_fields_t mariadb_fetch_fields_ptr = mock_mariadb_fetch_fields;
mariadb_free_result_t mariadb_free_result_ptr = mock_mariadb_free_result;
mariadb_error_t mariadb_error_ptr = mock_mariadb_error;
mariadb_close_t mariadb_close_ptr = mock_mariadb_close;
mariadb_options_t mariadb_options_ptr = mock_mariadb_options;
mariadb_ping_t mariadb_ping_ptr = mock_mariadb_ping;
mariadb_autocommit_t mariadb_autocommit_ptr = mock_mariadb_autocommit;
mariadb_commit_t mariadb_commit_ptr = mock_mariadb_commit;
mariadb_rollback_t mariadb_rollback_ptr = mock_mariadb_rollback;
mariadb_affected_rows_t mariadb_affected_rows_ptr = mock_mariadb_affected_rows;
mariadb_stmt_init_t mariadb_stmt_init_ptr = mock_mariadb_stmt_init;
mariadb_stmt_prepare_t mariadb_stmt_prepare_ptr = mock_mariadb_stmt_prepare;
mariadb_stmt_execute_t mariadb_stmt_execute_ptr = mock_mariadb_stmt_execute;
mariadb_stmt_close_t mariadb_stmt_close_ptr = mock_mariadb_stmt_close;
mariadb_stmt_result_metadata_t mariadb_stmt_result_metadata_ptr = mock_mariadb_stmt_result_metadata;
mariadb_stmt_fetch_t mariadb_stmt_fetch_ptr = mock_mariadb_stmt_fetch;
mariadb_stmt_bind_param_t mariadb_stmt_bind_param_ptr = mock_mariadb_stmt_bind_param;
mariadb_stmt_bind_result_t mariadb_stmt_bind_result_ptr = mock_mariadb_stmt_bind_result;
mariadb_stmt_error_t mariadb_stmt_error_ptr = mock_mariadb_stmt_error;
mariadb_stmt_affected_rows_t mariadb_stmt_affected_rows_ptr = mock_mariadb_stmt_affected_rows;
mariadb_stmt_store_result_t mariadb_stmt_store_result_ptr = mock_mariadb_stmt_store_result;
mariadb_stmt_free_result_t mariadb_stmt_free_result_ptr = mock_mariadb_stmt_free_result;
mariadb_stmt_field_count_t mariadb_stmt_field_count_ptr = mock_mariadb_stmt_field_count;
mariadb_kill_t mariadb_kill_ptr = mock_mariadb_kill;
mariadb_thread_id_t mariadb_thread_id_ptr = mock_mariadb_thread_id;
#else
mariadb_init_t mariadb_init_ptr = NULL;
mariadb_real_connect_t mariadb_real_connect_ptr = NULL;
mariadb_query_t mariadb_query_ptr = NULL;
mariadb_store_result_t mariadb_store_result_ptr = NULL;
mariadb_num_rows_t mariadb_num_rows_ptr = NULL;
mariadb_num_fields_t mariadb_num_fields_ptr = NULL;
mariadb_fetch_row_t mariadb_fetch_row_ptr = NULL;
mariadb_fetch_fields_t mariadb_fetch_fields_ptr = NULL;
mariadb_free_result_t mariadb_free_result_ptr = NULL;
mariadb_error_t mariadb_error_ptr = NULL;
mariadb_close_t mariadb_close_ptr = NULL;
mariadb_options_t mariadb_options_ptr = NULL;
mariadb_ping_t mariadb_ping_ptr = NULL;
mariadb_autocommit_t mariadb_autocommit_ptr = NULL;
mariadb_commit_t mariadb_commit_ptr = NULL;
mariadb_rollback_t mariadb_rollback_ptr = NULL;
mariadb_affected_rows_t mariadb_affected_rows_ptr = NULL;
mariadb_stmt_init_t mariadb_stmt_init_ptr = NULL;
mariadb_stmt_prepare_t mariadb_stmt_prepare_ptr = NULL;
mariadb_stmt_execute_t mariadb_stmt_execute_ptr = NULL;
mariadb_stmt_close_t mariadb_stmt_close_ptr = NULL;
mariadb_stmt_result_metadata_t mariadb_stmt_result_metadata_ptr = NULL;
mariadb_stmt_fetch_t mariadb_stmt_fetch_ptr = NULL;
mariadb_stmt_bind_param_t mariadb_stmt_bind_param_ptr = NULL;
mariadb_stmt_bind_result_t mariadb_stmt_bind_result_ptr = NULL;
mariadb_stmt_error_t mariadb_stmt_error_ptr = NULL;
mariadb_stmt_affected_rows_t mariadb_stmt_affected_rows_ptr = NULL;
mariadb_stmt_store_result_t mariadb_stmt_store_result_ptr = NULL;
mariadb_stmt_free_result_t mariadb_stmt_free_result_ptr = NULL;
mariadb_stmt_field_count_t mariadb_stmt_field_count_ptr = NULL;
mariadb_kill_t mariadb_kill_ptr = NULL;
mariadb_thread_id_t mariadb_thread_id_ptr = NULL;
#endif

// Library handle and mutex (only in non-mock build)
#ifndef USE_MOCK_LIBMARIADB
static void* libmariadb_handle = NULL;
static pthread_mutex_t libmariadb_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

/*
 * Library Loading Functions
 */

bool load_libmariadb_functions(const char* designator __attribute__((unused))) {
#ifdef USE_MOCK_LIBMARIADB
    // For mocking, functions are already set
    return true;
#else
    const char* log_subsystem = designator ? designator : SR_DATABASE;
    MUTEX_LOCK(&libmariadb_mutex, log_subsystem);

    if (libmariadb_handle) {
        MUTEX_UNLOCK(&libmariadb_mutex, log_subsystem);
        return true; // Another thread loaded it
    }

    // Try to load libmariadb
    libmariadb_handle = dlopen("libmariadb.so.3", RTLD_LAZY);
    if (!libmariadb_handle) {
        libmariadb_handle = dlopen("libmariadb.so", RTLD_LAZY);
    }
    if (!libmariadb_handle) {
        log_this(log_subsystem, "Failed to load libmariadb library", LOG_LEVEL_ERROR, 0);
        log_this(log_subsystem, dlerror(), LOG_LEVEL_ERROR, 0);
        MUTEX_UNLOCK(&libmariadb_mutex, log_subsystem);
        return false;
    }

    // Load function pointers
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    mariadb_init_ptr = (mariadb_init_t)dlsym(libmariadb_handle, "mysql_init");
    mariadb_real_connect_ptr = (mariadb_real_connect_t)dlsym(libmariadb_handle, "mysql_real_connect");
    mariadb_query_ptr = (mariadb_query_t)dlsym(libmariadb_handle, "mysql_query");
    mariadb_store_result_ptr = (mariadb_store_result_t)dlsym(libmariadb_handle, "mysql_store_result");
    mariadb_num_rows_ptr = (mariadb_num_rows_t)dlsym(libmariadb_handle, "mysql_num_rows");
    mariadb_num_fields_ptr = (mariadb_num_fields_t)dlsym(libmariadb_handle, "mysql_num_fields");
    mariadb_fetch_row_ptr = (mariadb_fetch_row_t)dlsym(libmariadb_handle, "mysql_fetch_row");
    mariadb_fetch_fields_ptr = (mariadb_fetch_fields_t)dlsym(libmariadb_handle, "mysql_fetch_fields");
    mariadb_free_result_ptr = (mariadb_free_result_t)dlsym(libmariadb_handle, "mysql_free_result");
    mariadb_error_ptr = (mariadb_error_t)dlsym(libmariadb_handle, "mysql_error");
    mariadb_close_ptr = (mariadb_close_t)dlsym(libmariadb_handle, "mysql_close");
    mariadb_options_ptr = (mariadb_options_t)dlsym(libmariadb_handle, "mysql_options");
    mariadb_ping_ptr = (mariadb_ping_t)dlsym(libmariadb_handle, "mysql_ping");
    mariadb_autocommit_ptr = (mariadb_autocommit_t)dlsym(libmariadb_handle, "mysql_autocommit");
    mariadb_commit_ptr = (mariadb_commit_t)dlsym(libmariadb_handle, "mysql_commit");
    mariadb_rollback_ptr = (mariadb_rollback_t)dlsym(libmariadb_handle, "mysql_rollback");
    mariadb_affected_rows_ptr = (mariadb_affected_rows_t)dlsym(libmariadb_handle, "mysql_affected_rows");
    mariadb_stmt_init_ptr = (mariadb_stmt_init_t)dlsym(libmariadb_handle, "mysql_stmt_init");
    mariadb_stmt_prepare_ptr = (mariadb_stmt_prepare_t)dlsym(libmariadb_handle, "mysql_stmt_prepare");
    mariadb_stmt_execute_ptr = (mariadb_stmt_execute_t)dlsym(libmariadb_handle, "mysql_stmt_execute");
    mariadb_stmt_close_ptr = (mariadb_stmt_close_t)dlsym(libmariadb_handle, "mysql_stmt_close");
    mariadb_stmt_result_metadata_ptr = (mariadb_stmt_result_metadata_t)dlsym(libmariadb_handle, "mysql_stmt_result_metadata");
    mariadb_stmt_fetch_ptr = (mariadb_stmt_fetch_t)dlsym(libmariadb_handle, "mysql_stmt_fetch");
    mariadb_stmt_bind_param_ptr = (mariadb_stmt_bind_param_t)dlsym(libmariadb_handle, "mysql_stmt_bind_param");
    mariadb_stmt_bind_result_ptr = (mariadb_stmt_bind_result_t)dlsym(libmariadb_handle, "mysql_stmt_bind_result");
    mariadb_stmt_error_ptr = (mariadb_stmt_error_t)dlsym(libmariadb_handle, "mysql_stmt_error");
    mariadb_stmt_affected_rows_ptr = (mariadb_stmt_affected_rows_t)dlsym(libmariadb_handle, "mysql_stmt_affected_rows");
    mariadb_stmt_store_result_ptr = (mariadb_stmt_store_result_t)dlsym(libmariadb_handle, "mysql_stmt_store_result");
    mariadb_stmt_free_result_ptr = (mariadb_stmt_free_result_t)dlsym(libmariadb_handle, "mysql_stmt_free_result");
    mariadb_stmt_field_count_ptr = (mariadb_stmt_field_count_t)dlsym(libmariadb_handle, "mysql_stmt_field_count");
    mariadb_kill_ptr = (mariadb_kill_t)dlsym(libmariadb_handle, "mysql_kill");
    mariadb_thread_id_ptr = (mariadb_thread_id_t)dlsym(libmariadb_handle, "mysql_thread_id");
#pragma GCC diagnostic pop

    // Check if all required functions were loaded
    if (!mariadb_init_ptr || !mariadb_real_connect_ptr || !mariadb_query_ptr ||
        !mariadb_store_result_ptr || !mariadb_num_rows_ptr || !mariadb_num_fields_ptr ||
        !mariadb_fetch_row_ptr || !mariadb_fetch_fields_ptr || !mariadb_free_result_ptr ||
        !mariadb_error_ptr || !mariadb_close_ptr) {
        log_this(log_subsystem, "Failed to load all required libmariadb functions", LOG_LEVEL_ERROR, 0);
        dlclose(libmariadb_handle);
        libmariadb_handle = NULL;
        MUTEX_UNLOCK(&libmariadb_mutex, log_subsystem);
        return false;
    }

    // Optional functions - log if not available
    if (!mariadb_options_ptr) {
        log_this(log_subsystem, "mariadb_options function not available - connection options will be limited", LOG_LEVEL_TRACE, 0);
    }
    if (!mariadb_ping_ptr) {
        log_this(log_subsystem, "mariadb_ping function not available - health check will use query method only", LOG_LEVEL_TRACE, 0);
    }
    if (!mariadb_autocommit_ptr || !mariadb_commit_ptr || !mariadb_rollback_ptr) {
        log_this(log_subsystem, "Transaction functions not available - transactions will be limited", LOG_LEVEL_DEBUG, 0);
    }
    if (!mariadb_stmt_init_ptr || !mariadb_stmt_prepare_ptr || !mariadb_stmt_execute_ptr || !mariadb_stmt_close_ptr) {
        log_this(log_subsystem, "Prepared statement functions not available - prepared statements will be limited", LOG_LEVEL_TRACE, 0);
    }
    if (!mariadb_kill_ptr || !mariadb_thread_id_ptr) {
        log_this(log_subsystem, "mariadb_kill / mariadb_thread_id not available - watchdog cancel will be a no-op for MariaDB", LOG_LEVEL_ALERT, 0);
    }

    MUTEX_UNLOCK(&libmariadb_mutex, log_subsystem);
    log_this(log_subsystem, "Successfully loaded libmariadb library", LOG_LEVEL_TRACE, 0);
    return true;
#endif
}

/*
 * Utility Functions
 */

// Simple timeout mechanism without signals
bool mariadb_check_timeout_expired(time_t start_time, int timeout_seconds) {
    return (time(NULL) - start_time) >= timeout_seconds;
}

PreparedStatementCache* mariadb_create_prepared_statement_cache(void) {
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

void mariadb_destroy_prepared_statement_cache(PreparedStatementCache* cache) {
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

bool mariadb_connect(ConnectionConfig* config, DatabaseHandle** connection, const char* designator) {
    if (!config || !connection) {
        log_this(SR_DATABASE, "Invalid parameters for MariaDB connection", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Load libmariadb library if not already loaded
    if (!load_libmariadb_functions(designator)) {
        const char* log_subsystem = designator ? designator : SR_DATABASE;
        log_this(log_subsystem, "MariaDB library not available", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Initialize MariaDB connection
    void* mariadb_conn = mariadb_init_ptr(NULL);
    if (!mariadb_conn) {
        log_this(SR_DATABASE, "MariaDB connection initialization failed", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Set auto-reconnect option
    if (mariadb_options_ptr) {
        int reconnect = 1;
        mariadb_options_ptr(mariadb_conn, MYSQL_OPT_RECONNECT, &reconnect);
    }

    // Establish connection
    void* result = mariadb_real_connect_ptr(
        mariadb_conn,
        config->host ? config->host : "localhost",
        config->username ? config->username : "",
        config->password ? config->password : "",
        config->database ? config->database : "",
        (unsigned int)(config->port > 0 ? config->port : 3306),
        NULL,  // unix_socket
        0      // client_flag
    );

    if (!result) {
        log_this(SR_DATABASE, "MariaDB connection failed", LOG_LEVEL_ERROR, 0);
        if (mariadb_error_ptr) {
            const char* error_msg = mariadb_error_ptr(mariadb_conn);
            if (error_msg && strlen(error_msg) > 0) {
                log_this(SR_DATABASE, error_msg, LOG_LEVEL_ERROR, 0);
            }
        }
        if (mariadb_close_ptr) {
            mariadb_close_ptr(mariadb_conn);
        }
        return false;
    }

    // Create database handle
    DatabaseHandle* db_handle = calloc(1, sizeof(DatabaseHandle));
    if (!db_handle) {
        return false;
    }

    // Create MariaDB-specific connection wrapper
    MariadbConnection* mariadb_wrapper = calloc(1, sizeof(MariadbConnection));
    if (!mariadb_wrapper) {
        free(db_handle);
        return false;
    }

    mariadb_wrapper->connection = mariadb_conn;
    mariadb_wrapper->reconnect = true;
    mariadb_wrapper->prepared_statements = mariadb_create_prepared_statement_cache();
    if (!mariadb_wrapper->prepared_statements) {
        free(mariadb_wrapper);
        free(db_handle);
        return false;
    }

    // Store designator for future use (disconnect, etc.)
    db_handle->designator = designator ? strdup(designator) : NULL;

    // Initialize database handle
    db_handle->engine_type = DB_ENGINE_MARIADB;
    db_handle->connection_handle = mariadb_wrapper;
    db_handle->config = config;
    db_handle->status = DB_CONNECTION_CONNECTED;
    db_handle->connected_since = time(NULL);
    db_handle->current_transaction = NULL;
    db_handle->prepared_statements = NULL; // Will be managed by engine-specific code
    db_handle->prepared_statement_count = 0;
    db_handle->prepared_statement_lru_counter = NULL; // LRU tracking for prepared statements
    pthread_mutex_init(&db_handle->connection_lock, NULL);
    db_handle->in_use = false;
    db_handle->last_health_check = time(NULL);
    db_handle->consecutive_failures = 0;

    *connection = db_handle;

    // Use designator for logging if provided, otherwise use generic Database subsystem
    const char* log_subsystem = designator ? designator : SR_DATABASE;
    log_this(log_subsystem, "MariaDB connection established successfully", LOG_LEVEL_TRACE, 0);
    return true;
}

bool mariadb_disconnect(DatabaseHandle* connection) {
    if (!connection || connection->engine_type != DB_ENGINE_MARIADB) {
        return false;
    }

    MariadbConnection* mariadb_conn = (MariadbConnection*)connection->connection_handle;
    if (mariadb_conn) {
        if (mariadb_conn->connection && mariadb_close_ptr) {
            mariadb_close_ptr(mariadb_conn->connection);
        }
        mariadb_destroy_prepared_statement_cache(mariadb_conn->prepared_statements);
        free(mariadb_conn);
    }

    connection->status = DB_CONNECTION_DISCONNECTED;

    // Use stored designator for logging if available
    const char* log_subsystem = connection->designator ? connection->designator : SR_DATABASE;
    log_this(log_subsystem, "MariaDB connection closed", LOG_LEVEL_TRACE, 0);
    return true;
}

bool mariadb_health_check(DatabaseHandle* connection) {
    if (!connection) {
        const char* designator = SR_DATABASE;
        log_this(designator, "MariaDB health check: connection is NULL", LOG_LEVEL_ERROR, 0);
        return false;
    }

    const char* designator = connection->designator ? connection->designator : SR_DATABASE;
    log_this(designator, "MariaDB health check: Starting validation", LOG_LEVEL_TRACE, 0);

    if (connection->engine_type != DB_ENGINE_MARIADB) {
        log_this(designator, "MariaDB health check: wrong engine type %d", LOG_LEVEL_ERROR, 1, connection->engine_type);
        return false;
    }

    MariadbConnection* mariadb_conn = (MariadbConnection*)connection->connection_handle;
    if (!mariadb_conn) {
        log_this(designator, "MariaDB health check: mariadb_conn is NULL", LOG_LEVEL_ERROR, 0);
        return false;
    }

    if (!mariadb_conn->connection) {
        log_this(designator, "MariaDB health check: mariadb_conn->connection is NULL", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Function pointer validation
    if (!mariadb_ping_ptr && !mariadb_query_ptr) {
        log_this(designator, "MariaDB health check: neither mariadb_ping_ptr nor mariadb_query_ptr available", LOG_LEVEL_ERROR, 0);
        return false;
    }

    log_this(designator, "MariaDB health check: All validations passed, executing health check", LOG_LEVEL_TRACE, 0);

    // Try ping method first if available
    if (mariadb_ping_ptr) {
        log_this(designator, "MariaDB health check: Trying mariadb_ping method", LOG_LEVEL_TRACE, 0);
        int ping_result = mariadb_ping_ptr(mariadb_conn->connection);
        log_this(designator, "MariaDB health check: mariadb_ping result: %d", LOG_LEVEL_TRACE, 1, ping_result);

        if (ping_result == 0) {
            log_this(designator, "MariaDB health check passed via mariadb_ping", LOG_LEVEL_TRACE, 0);
            connection->last_health_check = time(NULL);
            connection->consecutive_failures = 0;
            return true;
        } else {
            log_this(designator, "MariaDB health check: mariadb_ping failed, trying query method", LOG_LEVEL_TRACE, 0);
        }
    }

    // Fallback to query method
    if (mariadb_query_ptr) {
        log_this(designator, "MariaDB health check: Executing 'SELECT 1'", LOG_LEVEL_TRACE, 0);

        if (mariadb_query_ptr(mariadb_conn->connection, "SELECT 1") != 0) {
            log_this(designator, "MariaDB health check: Query failed", LOG_LEVEL_ERROR, 0);
            if (mariadb_error_ptr) {
                const char* error_msg = mariadb_error_ptr(mariadb_conn->connection);
                if (error_msg && strlen(error_msg) > 0) {
                    log_this(designator, "MariaDB health check error: %s", LOG_LEVEL_ERROR, 1, error_msg);
                }
            }
            connection->consecutive_failures++;
            return false;
        }

        // Store and free result
        if (mariadb_store_result_ptr) {
            void* result = mariadb_store_result_ptr(mariadb_conn->connection);
            if (result && mariadb_free_result_ptr) {
                mariadb_free_result_ptr(result);
            }
        }

        log_this(designator, "MariaDB health check passed via query", LOG_LEVEL_TRACE, 0);
        connection->last_health_check = time(NULL);
        connection->consecutive_failures = 0;
        return true;
    }

    log_this(designator, "MariaDB health check: No health check method available", LOG_LEVEL_ERROR, 0);
    return false;
}

bool mariadb_h_reset_connection(DatabaseHandle* connection) {
    if (!connection || connection->engine_type != DB_ENGINE_MARIADB) {
        return false;
    }

    // MariaDB auto-reconnect should handle this
    connection->status = DB_CONNECTION_CONNECTED;
    connection->connected_since = time(NULL);
    connection->consecutive_failures = 0;

    log_this(SR_DATABASE, "MariaDB connection reset successfully", LOG_LEVEL_TRACE, 0);
    return true;
}

/*
 * Cancel any in-flight query on this MariaDB connection.
 *
 * Sends a KILL for the connection's own thread_id. The MariaDB server
 * terminates the running query and the blocked mariadb_real_query on
 * the caller side eventually returns. mariadb_kill acquires the
 * connection's internal mutex (libmariadb is thread-safe per
 * connection), so it is safe to call from a different thread than
 * the one stuck in the query.
 *
 * LIMITATION: if the TCP socket is dead, mariadb_kill can block the
 * caller (watchdog) writing KILL. Prefer a dedicated admin connection
 * for cancel under full fault-tolerance work (docs/H/TODO.md). Best-effort
 * when the server is slow but reachable.
 */
void mariadb_cancel_inflight(DatabaseHandle* connection) {
    if (!connection || connection->engine_type != DB_ENGINE_MARIADB) {
        return;
    }
    if (!mariadb_kill_ptr || !mariadb_thread_id_ptr) {
        return;
    }

    MariadbConnection* mariadb_conn = (MariadbConnection*)connection->connection_handle;
    if (!mariadb_conn || !mariadb_conn->connection) {
        return;
    }

    unsigned long thread_id = mariadb_thread_id_ptr(mariadb_conn->connection);
    int rc = mariadb_kill_ptr(mariadb_conn->connection, thread_id);
    const char* designator = connection->designator ? connection->designator : SR_DATABASE;
    if (rc != 0) {
        const char* error_msg = mariadb_error_ptr ? mariadb_error_ptr(mariadb_conn->connection) : NULL;
        log_this(designator, "MariaDB: mariadb_kill(thread_id=%lu) failed: %s",
                 LOG_LEVEL_ERROR, 2, thread_id, error_msg ? error_msg : "unknown");
    } else {
        log_this(designator, "MariaDB: requested cancel of in-flight query (thread_id=%lu)",
                 LOG_LEVEL_ALERT, 1, thread_id);
    }
}
