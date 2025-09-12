/*
 * PostgreSQL Database Engine Implementation
 *
 * Implements the PostgreSQL database engine for the Hydrogen database subsystem.
 * Uses dynamic loading (dlopen/dlsym) for libpq to avoid static linking dependencies.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dlfcn.h>
#include <pthread.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>

#include "../hydrogen.h"
#include "database.h"
#include "database_queue.h"

// Function pointer types for libpq functions
typedef void* (*PQconnectdb_t)(const char* conninfo);
typedef int (*PQstatus_t)(void* conn);
typedef char* (*PQerrorMessage_t)(void* conn);
typedef void (*PQfinish_t)(void* conn);
typedef void* (*PQexec_t)(void* conn, const char* query);
typedef int (*PQresultStatus_t)(void* res);
typedef void (*PQclear_t)(void* res);
typedef int (*PQntuples_t)(void* res);
typedef int (*PQnfields_t)(void* res);
typedef char* (*PQfname_t)(void* res, int column_number);
typedef char* (*PQgetvalue_t)(void* res, int row_number, int column_number);
typedef char* (*PQcmdTuples_t)(void* res);
typedef void (*PQreset_t)(void* conn);
typedef void* (*PQprepare_t)(void* conn, const char* stmtName, const char* query, int nParams, const char* const* paramTypes);
typedef size_t (*PQescapeStringConn_t)(void* conn, char* to, const char* from, size_t length, int* error);
typedef int (*PQping_t)(const char* conninfo);

// PostgreSQL function pointers (loaded dynamically)
static PQconnectdb_t PQconnectdb_ptr = NULL;
static PQstatus_t PQstatus_ptr = NULL;
static PQerrorMessage_t PQerrorMessage_ptr = NULL;
static PQfinish_t PQfinish_ptr = NULL;
static PQexec_t PQexec_ptr = NULL;
static PQresultStatus_t PQresultStatus_ptr = NULL;
static PQclear_t PQclear_ptr = NULL;
static PQntuples_t PQntuples_ptr = NULL;
static PQnfields_t PQnfields_ptr = NULL;
static PQfname_t PQfname_ptr = NULL;
static PQgetvalue_t PQgetvalue_ptr = NULL;
static PQcmdTuples_t PQcmdTuples_ptr = NULL;
static PQreset_t PQreset_ptr = NULL;
static PQprepare_t PQprepare_ptr = NULL;
static PQescapeStringConn_t PQescapeStringConn_ptr = NULL;
static PQping_t PQping_ptr = NULL;

// Library handle
static void* libpq_handle = NULL;
static pthread_mutex_t libpq_mutex = PTHREAD_MUTEX_INITIALIZER;

// Simple timeout mechanism without signals
static bool check_timeout_expired(time_t start_time, int timeout_seconds) {
    return (time(NULL) - start_time) >= timeout_seconds;
}

// Constants (defined since we can't include libpq-fe.h)
#define CONNECTION_OK 0
#define CONNECTION_BAD 1
#define PGRES_EMPTY_QUERY 0
#define PGRES_COMMAND_OK 1
#define PGRES_TUPLES_OK 2
#define PGRES_COPY_OUT 3
#define PGRES_COPY_IN 4
#define PGRES_BAD_RESPONSE 5
#define PGRES_NONFATAL_ERROR 6
#define PGRES_FATAL_ERROR 7

// Prepared statement cache structure
typedef struct PreparedStatementCache {
    char** names;
    size_t count;
    size_t capacity;
    pthread_mutex_t lock;
} PreparedStatementCache;

// PostgreSQL-specific connection structure
typedef struct PostgresConnection {
    void* connection;  // PGconn* loaded dynamically
    bool in_transaction;
    PreparedStatementCache* prepared_statements;
} PostgresConnection;

/*
 * Function Prototypes
 */

// Connection management
bool postgresql_connect(ConnectionConfig* config, DatabaseHandle** connection, const char* designator);
bool postgresql_disconnect(DatabaseHandle* connection);
bool postgresql_health_check(DatabaseHandle* connection);
bool postgresql_reset_connection(DatabaseHandle* connection);

// Query execution
bool postgresql_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result);
bool postgresql_execute_prepared(DatabaseHandle* connection, PreparedStatement* stmt, QueryRequest* request, QueryResult** result);

// Transaction management
bool postgresql_begin_transaction(DatabaseHandle* connection, DatabaseIsolationLevel level, Transaction** transaction);
bool postgresql_commit_transaction(DatabaseHandle* connection, Transaction* transaction);
bool postgresql_rollback_transaction(DatabaseHandle* connection, Transaction* transaction);

// Prepared statement management
bool postgresql_prepare_statement(DatabaseHandle* connection, const char* name, const char* sql, PreparedStatement** stmt);
bool postgresql_unprepare_statement(DatabaseHandle* connection, PreparedStatement* stmt);

// Utility functions
char* postgresql_get_connection_string(ConnectionConfig* config);
bool postgresql_validate_connection_string(const char* connection_string);
char* postgresql_escape_string(DatabaseHandle* connection, const char* input);

// Engine interface
DatabaseEngineInterface* database_engine_postgresql_get_interface(void);

/*
 * Library Loading Functions
 */

static bool load_libpq_functions(void) {
    if (libpq_handle) {
        return true; // Already loaded
    }

    pthread_mutex_lock(&libpq_mutex);

    if (libpq_handle) {
        pthread_mutex_unlock(&libpq_mutex);
        return true; // Another thread loaded it
    }

    // Try to load libpq
    libpq_handle = dlopen("libpq.so.5", RTLD_LAZY);
    if (!libpq_handle) {
        libpq_handle = dlopen("libpq.so", RTLD_LAZY);
    }
    if (!libpq_handle) {
        log_this(SR_DATABASE, "Failed to load libpq library", LOG_LEVEL_ERROR, 0);
        log_this(SR_DATABASE, dlerror(), LOG_LEVEL_ERROR, 0);
        pthread_mutex_unlock(&libpq_mutex);
        return false;
    }

    // Load function pointers
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    PQconnectdb_ptr = (PQconnectdb_t)dlsym(libpq_handle, "PQconnectdb");
    PQstatus_ptr = (PQstatus_t)dlsym(libpq_handle, "PQstatus");
    PQerrorMessage_ptr = (PQerrorMessage_t)dlsym(libpq_handle, "PQerrorMessage");
    PQfinish_ptr = (PQfinish_t)dlsym(libpq_handle, "PQfinish");
    PQexec_ptr = (PQexec_t)dlsym(libpq_handle, "PQexec");
    PQresultStatus_ptr = (PQresultStatus_t)dlsym(libpq_handle, "PQresultStatus");
    PQclear_ptr = (PQclear_t)dlsym(libpq_handle, "PQclear");
    PQntuples_ptr = (PQntuples_t)dlsym(libpq_handle, "PQntuples");
    PQnfields_ptr = (PQnfields_t)dlsym(libpq_handle, "PQnfields");
    PQfname_ptr = (PQfname_t)dlsym(libpq_handle, "PQfname");
    PQgetvalue_ptr = (PQgetvalue_t)dlsym(libpq_handle, "PQgetvalue");
    PQcmdTuples_ptr = (PQcmdTuples_t)dlsym(libpq_handle, "PQcmdTuples");
    PQreset_ptr = (PQreset_t)dlsym(libpq_handle, "PQreset");
    PQprepare_ptr = (PQprepare_t)dlsym(libpq_handle, "PQprepare");
    PQescapeStringConn_ptr = (PQescapeStringConn_t)dlsym(libpq_handle, "PQescapeStringConn");
    PQping_ptr = (PQping_t)dlsym(libpq_handle, "PQping");
#pragma GCC diagnostic pop

    // Check if all functions were loaded
    if (!PQconnectdb_ptr || !PQstatus_ptr || !PQerrorMessage_ptr || !PQfinish_ptr ||
        !PQexec_ptr || !PQresultStatus_ptr || !PQclear_ptr || !PQntuples_ptr ||
        !PQnfields_ptr || !PQfname_ptr || !PQgetvalue_ptr || !PQcmdTuples_ptr) {
        log_this(SR_DATABASE, "Failed to load all required libpq functions", LOG_LEVEL_ERROR, 0);
        dlclose(libpq_handle);
        libpq_handle = NULL;
        pthread_mutex_unlock(&libpq_mutex);
        return false;
    }

    // PQping is optional - log if not available
    if (!PQping_ptr) {
        log_this(SR_DATABASE, "PQping function not available - health check will use query method only", LOG_LEVEL_DEBUG, 0);
    }

    pthread_mutex_unlock(&libpq_mutex);
    log_this(SR_DATABASE, "Successfully loaded libpq library", LOG_LEVEL_STATE, 0);
    return true;
}

/*
 * Utility Functions
 */

static PreparedStatementCache* create_prepared_statement_cache(void) {
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

static void destroy_prepared_statement_cache(PreparedStatementCache* cache) {
    if (!cache) return;

    pthread_mutex_lock(&cache->lock);
    for (size_t i = 0; i < cache->count; i++) {
        free(cache->names[i]);
    }
    free(cache->names);
    pthread_mutex_unlock(&cache->lock);
    pthread_mutex_destroy(&cache->lock);
    free(cache);
}

static bool add_prepared_statement(PreparedStatementCache* cache, const char* name) {
    if (!cache || !name) return false;

    pthread_mutex_lock(&cache->lock);

    // Check if already exists
    for (size_t i = 0; i < cache->count; i++) {
        if (strcmp(cache->names[i], name) == 0) {
            pthread_mutex_unlock(&cache->lock);
            return true; // Already exists
        }
    }

    // Expand capacity if needed
    if (cache->count >= cache->capacity) {
        size_t new_capacity = cache->capacity * 2;
        char** new_names = realloc(cache->names, new_capacity * sizeof(char*));
        if (!new_names) {
            pthread_mutex_unlock(&cache->lock);
            return false;
        }
        cache->names = new_names;
        cache->capacity = new_capacity;
    }

    cache->names[cache->count] = strdup(name);
    if (!cache->names[cache->count]) {
        pthread_mutex_unlock(&cache->lock);
        return false;
    }

    cache->count++;
    pthread_mutex_unlock(&cache->lock);
    return true;
}

static bool remove_prepared_statement(PreparedStatementCache* cache, const char* name) {
    if (!cache || !name) return false;

    pthread_mutex_lock(&cache->lock);

    for (size_t i = 0; i < cache->count; i++) {
        if (strcmp(cache->names[i], name) == 0) {
            free(cache->names[i]);
            // Shift remaining elements
            for (size_t j = i; j < cache->count - 1; j++) {
                cache->names[j] = cache->names[j + 1];
            }
            cache->count--;
            pthread_mutex_unlock(&cache->lock);
            return true;
        }
    }

    pthread_mutex_unlock(&cache->lock);
    return false;
}

/*
 * Connection Management
 */

bool postgresql_connect(ConnectionConfig* config, DatabaseHandle** connection, const char* designator) {
    if (!config || !connection) {
        log_this(SR_DATABASE, "Invalid parameters for PostgreSQL connection", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Load libpq library if not already loaded
    if (!load_libpq_functions()) {
        log_this(SR_DATABASE, "PostgreSQL library not available", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Build connection string
    char conninfo[1024];
    if (config->connection_string) {
        snprintf(conninfo, sizeof(conninfo), "%s", config->connection_string);
    } else {
        snprintf(conninfo, sizeof(conninfo),
                 "host=%s port=%d dbname=%s user=%s password=%s connect_timeout=%d",
                 config->host ? config->host : "localhost",
                 config->port ? config->port : 5432,
                 config->database ? config->database : "postgres",
                 config->username ? config->username : "",
                 config->password ? config->password : "",
                 config->timeout_seconds ? config->timeout_seconds : 30);
    }

    // Establish connection
    void* pg_conn = PQconnectdb_ptr(conninfo);
    if (PQstatus_ptr(pg_conn) != CONNECTION_OK) {
        log_this(SR_DATABASE, "PostgreSQL connection failed", LOG_LEVEL_ERROR, 0);
        log_this(SR_DATABASE, PQerrorMessage_ptr(pg_conn), LOG_LEVEL_ERROR, 0);
        PQfinish_ptr(pg_conn);
        return false;
    }

    // Create database handle
    DatabaseHandle* db_handle = calloc(1, sizeof(DatabaseHandle));
    if (!db_handle) {
        PQfinish_ptr(pg_conn);
        return false;
    }

    // Create PostgreSQL-specific connection wrapper
    PostgresConnection* pg_wrapper = calloc(1, sizeof(PostgresConnection));
    if (!pg_wrapper) {
        free(db_handle);
        PQfinish_ptr(pg_conn);
        return false;
    }

    pg_wrapper->connection = pg_conn;
    pg_wrapper->in_transaction = false;
    pg_wrapper->prepared_statements = create_prepared_statement_cache();
    if (!pg_wrapper->prepared_statements) {
        free(pg_wrapper);
        free(db_handle);
        PQfinish_ptr(pg_conn);
        return false;
    }

    // Store designator for future use (disconnect, etc.)
    db_handle->designator = designator ? strdup(designator) : NULL;

    // Initialize database handle
    db_handle->engine_type = DB_ENGINE_POSTGRESQL;
    db_handle->connection_handle = pg_wrapper;
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
    log_this(log_subsystem, "PostgreSQL connection established successfully", LOG_LEVEL_STATE, 0);
    return true;
}

bool postgresql_disconnect(DatabaseHandle* connection) {
    if (!connection || connection->engine_type != DB_ENGINE_POSTGRESQL) {
        return false;
    }

    PostgresConnection* pg_conn = (PostgresConnection*)connection->connection_handle;
    if (pg_conn) {
        if (pg_conn->connection) {
            PQfinish_ptr(pg_conn->connection);
        }
        destroy_prepared_statement_cache(pg_conn->prepared_statements);
        free(pg_conn);
    }

    connection->status = DB_CONNECTION_DISCONNECTED;

    // Use stored designator for logging if available
    const char* log_subsystem = connection->designator ? connection->designator : SR_DATABASE;
    log_this(log_subsystem, "PostgreSQL connection closed", LOG_LEVEL_STATE, 0);
    return true;
}

bool postgresql_health_check(DatabaseHandle* connection) {
    const char* designator = connection->designator ? connection->designator : SR_DATABASE;

    // Early validation logging
    log_this(designator, "PostgreSQL health check: Starting validation", LOG_LEVEL_DEBUG, 0);

    if (!connection) {
        log_this(designator, "PostgreSQL health check: connection is NULL", LOG_LEVEL_ERROR, 0);
        return false;
    }

    if (connection->engine_type != DB_ENGINE_POSTGRESQL) {
        log_this(designator, "PostgreSQL health check: wrong engine type %d", LOG_LEVEL_ERROR, 1, connection->engine_type);
        return false;
    }

    // Function pointer validation
    if (!PQexec_ptr) {
        log_this(designator, "PostgreSQL health check: PQexec_ptr is NULL", LOG_LEVEL_ERROR, 0);
        return false;
    }
    if (!PQresultStatus_ptr) {
        log_this(designator, "PostgreSQL health check: PQresultStatus_ptr is NULL", LOG_LEVEL_ERROR, 0);
        return false;
    }
    if (!PQclear_ptr) {
        log_this(designator, "PostgreSQL health check: PQclear_ptr is NULL", LOG_LEVEL_ERROR, 0);
        return false;
    }
    if (!PQerrorMessage_ptr) {
        log_this(designator, "PostgreSQL health check: PQerrorMessage_ptr is NULL", LOG_LEVEL_ERROR, 0);
        return false;
    }

    PostgresConnection* pg_conn = (PostgresConnection*)connection->connection_handle;
    if (!pg_conn) {
        log_this(designator, "PostgreSQL health check: pg_conn is NULL", LOG_LEVEL_ERROR, 0);
        return false;
    }

    if (!pg_conn->connection) {
        log_this(designator, "PostgreSQL health check: pg_conn->connection is NULL", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Check connection status
    if (PQstatus_ptr && PQstatus_ptr(pg_conn->connection) != CONNECTION_OK) {
        int conn_status = PQstatus_ptr ? PQstatus_ptr(pg_conn->connection) : -1;
        log_this(designator, "PostgreSQL health check: connection status is not OK: %d", LOG_LEVEL_ERROR, 1, conn_status);
        return false;
    }

    // Check transaction state
    if (pg_conn->in_transaction) {
        log_this(designator, "PostgreSQL health check: connection is in transaction state", LOG_LEVEL_DEBUG, 0);
        // This might be okay, but let's log it for debugging
    }

    log_this(designator, "PostgreSQL health check: All validations passed, executing query", LOG_LEVEL_DEBUG, 0);

    // Try alternative health check method first - PQping if available
    if (PQping_ptr && connection->config && connection->config->connection_string) {
        log_this(designator, "PostgreSQL health check: Trying PQping method", LOG_LEVEL_DEBUG, 0);
        int ping_result = PQping_ptr(connection->config->connection_string);
        log_this(designator, "PostgreSQL health check: PQping result: %d", LOG_LEVEL_DEBUG, 1, ping_result);

        // PING_OK = 0, PING_REJECT = 1, PING_NO_RESPONSE = 2, PING_NO_ATTEMPT = 3
        if (ping_result == 0) {
            log_this(designator, "PostgreSQL health check passed via PQping", LOG_LEVEL_STATE, 0);
            connection->last_health_check = time(NULL);
            connection->consecutive_failures = 0;
            return true;
        } else {
            log_this(designator, "PostgreSQL health check: PQping failed, trying query method", LOG_LEVEL_DEBUG, 0);
        }
    }

    // Execute a simple query to check connectivity with timeout protection
    log_this(designator, "PostgreSQL health check: Executing 'SELECT 1'", LOG_LEVEL_DEBUG, 0);

    // Set a short timeout for health check
    void* timeout_result = PQexec_ptr(pg_conn->connection, "SET statement_timeout = 5000"); // 5 seconds
    if (timeout_result) {
        PQclear_ptr(timeout_result);
    }

    time_t start_time = time(NULL);
    void* res = PQexec_ptr(pg_conn->connection, "SELECT 1");

    // Check if query took too long (approximate check)
    if (check_timeout_expired(start_time, 5)) {
        log_this(designator, "PostgreSQL health check: Query execution time exceeded 5 seconds", LOG_LEVEL_ERROR, 0);
        if (res) PQclear_ptr(res);
        return false;
    }

    if (!res) {
        log_this(designator, "PostgreSQL health check: PQexec returned NULL", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Debug: Log the actual result status
    int result_status = PQresultStatus_ptr(res);
    log_this(designator, "PostgreSQL health check: Query executed, result status: %d", LOG_LEVEL_DEBUG, 1, result_status);

    // Log additional result information for debugging
    if (PQntuples_ptr && PQnfields_ptr) {
        int ntuples = PQntuples_ptr(res);
        int nfields = PQnfields_ptr(res);
        log_this(designator, "PostgreSQL health check: Result has %d rows, %d columns", LOG_LEVEL_DEBUG, 2, ntuples, nfields);
    }

    // Check for successful result statuses
    if (result_status == PGRES_TUPLES_OK || result_status == PGRES_COMMAND_OK) {
        log_this(designator, "PostgreSQL health check: Query succeeded with status %d", LOG_LEVEL_DEBUG, 1, result_status);
    } else {
        // Handle different error conditions
        const char* status_desc = "unknown";
        switch (result_status) {
            case PGRES_EMPTY_QUERY: status_desc = "empty query"; break;
            case PGRES_BAD_RESPONSE: status_desc = "bad response"; break;
            case PGRES_NONFATAL_ERROR: status_desc = "non-fatal error"; break;
            case PGRES_FATAL_ERROR: status_desc = "fatal error"; break;
            case PGRES_COPY_OUT: status_desc = "copy out"; break;
            case PGRES_COPY_IN: status_desc = "copy in"; break;
            default: status_desc = "unknown"; break;
        }

        log_this(designator, "PostgreSQL health check failed - status: %d (%s)", LOG_LEVEL_ERROR, 2, result_status, status_desc);

        // Also log any error message
        if (PQerrorMessage_ptr) {
            char* error_msg = PQerrorMessage_ptr(pg_conn->connection);
            if (error_msg && strlen(error_msg) > 0) {
                log_this(designator, "PostgreSQL health check error: %s", LOG_LEVEL_ERROR, 1, error_msg);
            }
        }

        PQclear_ptr(res);
        connection->consecutive_failures++;
        return false;
    }

    // Success - log it
    log_this(designator, "PostgreSQL health check passed", LOG_LEVEL_STATE, 0);

    PQclear_ptr(res);
    connection->last_health_check = time(NULL);
    connection->consecutive_failures = 0;
    return true;
}

bool postgresql_reset_connection(DatabaseHandle* connection) {
    if (!connection || connection->engine_type != DB_ENGINE_POSTGRESQL) {
        return false;
    }

    PostgresConnection* pg_conn = (PostgresConnection*)connection->connection_handle;
    if (!pg_conn || !pg_conn->connection) {
        return false;
    }

    // Attempt to reset the connection
    PQreset_ptr(pg_conn->connection);
    if (PQstatus_ptr(pg_conn->connection) != CONNECTION_OK) {
        log_this(SR_DATABASE, "PostgreSQL connection reset failed", LOG_LEVEL_ERROR, 0);
        return false;
    }

    connection->status = DB_CONNECTION_CONNECTED;
    connection->connected_since = time(NULL);
    connection->consecutive_failures = 0;

    log_this(SR_DATABASE, "PostgreSQL connection reset successfully", LOG_LEVEL_STATE, 0);
    return true;
}

/*
 * Query Execution
 */

bool postgresql_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result) {
    const char* designator = connection->designator ? connection->designator : SR_DATABASE;

    log_this(designator, "postgresql_execute_query: ENTER - connection=%p, request=%p, result=%p", LOG_LEVEL_DEBUG, 3, (void*)connection, (void*)request, (void*)result);

    if (!connection || !request || !result || connection->engine_type != DB_ENGINE_POSTGRESQL) {
        log_this(designator, "PostgreSQL execute_query: Invalid parameters", LOG_LEVEL_ERROR, 0);
        return false;
    }

    log_this(designator, "postgresql_execute_query: Parameters validated, proceeding", LOG_LEVEL_DEBUG, 0);

    PostgresConnection* pg_conn = (PostgresConnection*)connection->connection_handle;
    if (!pg_conn || !pg_conn->connection) {
        log_this(designator, "PostgreSQL execute_query: Invalid connection handle", LOG_LEVEL_ERROR, 0);
        return false;
    }

    log_this(designator, "PostgreSQL execute_query: Executing query: %s", LOG_LEVEL_DEBUG, 1, request->sql_template);
    log_this(designator, "PostgreSQL execute_query: Query timeout: %d seconds", LOG_LEVEL_DEBUG, 1, request->timeout_seconds);

    // Set PostgreSQL statement timeout before executing query
    int query_timeout = request->timeout_seconds > 0 ? request->timeout_seconds : 30;
    char timeout_sql[256];
    snprintf(timeout_sql, sizeof(timeout_sql), "SET statement_timeout = %d", query_timeout * 1000); // Convert to milliseconds

    log_this(designator, "PostgreSQL execute_query: Setting statement timeout to %d seconds", LOG_LEVEL_DEBUG, 1, query_timeout);

    // Set the timeout
    void* timeout_result = PQexec_ptr(pg_conn->connection, timeout_sql);
    if (timeout_result) {
        int timeout_status = PQresultStatus_ptr(timeout_result);
        log_this(designator, "PostgreSQL execute_query: Timeout setting result status: %d", LOG_LEVEL_DEBUG, 1, timeout_status);
        PQclear_ptr(timeout_result);
    } else {
        log_this(designator, "PostgreSQL execute_query: Failed to set statement timeout", LOG_LEVEL_ERROR, 0);
    }

    time_t start_time = time(NULL);
    log_this(designator, "PostgreSQL execute_query: Starting query execution at %ld", LOG_LEVEL_DEBUG, 1, start_time);

    // CRITICAL DEBUG: Log right before PQexec call
    log_this(designator, "CRITICAL DEBUG: About to call PQexec_ptr - if hang occurs, it's here", LOG_LEVEL_ERROR, 0);
    log_this(designator, "CRITICAL DEBUG: connection=%p, query='%s'", LOG_LEVEL_ERROR, 2, pg_conn->connection, request->sql_template);

    // Force flush all logging before the potentially hanging call
    fflush(stdout);
    fflush(stderr);

    log_this(designator, "PQEXEC_CALL: Calling PQexec_ptr now...", LOG_LEVEL_ERROR, 0);
    // Execute the query (now with PostgreSQL-level timeout protection)
    void* pg_result = PQexec_ptr(pg_conn->connection, request->sql_template);
    log_this(designator, "PQEXEC_RETURN: PQexec_ptr returned %p", LOG_LEVEL_ERROR, 1, pg_result);

    // CRITICAL DEBUG: Log immediately after PQexec call
    log_this(designator, "CRITICAL DEBUG: PQexec_ptr returned - result=%p", LOG_LEVEL_ERROR, 1, pg_result);

    time_t end_time = time(NULL);
    time_t execution_time = end_time - start_time;

    log_this(designator, "PostgreSQL execute_query: Query execution completed in %ld seconds", LOG_LEVEL_DEBUG, 1, execution_time);

    // Check if query took too long (approximate check)
    if (check_timeout_expired(start_time, query_timeout)) {
        log_this(designator, "PostgreSQL execute_query: Query execution time exceeded %d seconds (actual: %ld)", LOG_LEVEL_ERROR, 2, query_timeout, execution_time);
        if (pg_result) {
            log_this(designator, "PostgreSQL execute_query: Cleaning up failed query result", LOG_LEVEL_DEBUG, 0);
            PQclear_ptr(pg_result);
        }
        return false;
    }

    log_this(designator, "PostgreSQL execute_query: Query execution within timeout limits", LOG_LEVEL_DEBUG, 0);

    if (!pg_result) {
        log_this(designator, "PostgreSQL execute_query: PQexec returned NULL", LOG_LEVEL_ERROR, 0);
        return false;
    }

    int result_status = PQresultStatus_ptr(pg_result);
    log_this(designator, "PostgreSQL execute_query: Result status: %d", LOG_LEVEL_DEBUG, 1, result_status);

    if (result_status != PGRES_TUPLES_OK && result_status != PGRES_COMMAND_OK) {
        log_this(designator, "PostgreSQL query execution failed - status: %d", LOG_LEVEL_ERROR, 1, result_status);
        char* error_msg = PQerrorMessage_ptr(pg_conn->connection);
        if (error_msg && strlen(error_msg) > 0) {
            log_this(designator, "PostgreSQL query error: %s", LOG_LEVEL_ERROR, 1, error_msg);
        }
        PQclear_ptr(pg_result);
        return false;
    }

    // Create result structure
    QueryResult* db_result = calloc(1, sizeof(QueryResult));
    if (!db_result) {
        PQclear_ptr(pg_result);
        return false;
    }

    db_result->success = true;
    db_result->row_count = (size_t)PQntuples_ptr(pg_result);
    db_result->column_count = (size_t)PQnfields_ptr(pg_result);
    db_result->execution_time_ms = 0; // TODO: Implement timing
    db_result->affected_rows = atoi(PQcmdTuples_ptr(pg_result));

    log_this(designator, "PostgreSQL execute_query: Query returned %zu rows, %zu columns, affected %d rows", LOG_LEVEL_DEBUG, 3, 
        db_result->row_count, 
        db_result->column_count, 
        db_result->affected_rows);

    // Extract column names
    if (db_result->column_count > 0) {
        db_result->column_names = calloc(db_result->column_count, sizeof(char*));
        if (!db_result->column_names) {
            PQclear_ptr(pg_result);
            free(db_result);
            return false;
        }

        for (size_t i = 0; i < db_result->column_count; i++) {
            db_result->column_names[i] = strdup(PQfname_ptr(pg_result, (int)i));
            if (!db_result->column_names[i]) {
                // Cleanup on failure
                for (size_t j = 0; j < i; j++) {
                    free(db_result->column_names[j]);
                }
                free(db_result->column_names);
                PQclear_ptr(pg_result);
                free(db_result);
                return false;
            }
        }
    }

    // Convert result to JSON
    if (db_result->row_count > 0 && db_result->column_count > 0) {
        // Simple JSON array of objects
        size_t json_size = 1024 * db_result->row_count; // Estimate size
        db_result->data_json = calloc(1, json_size);
        if (!db_result->data_json) {
            // Cleanup
            for (size_t i = 0; i < db_result->column_count; i++) {
                free(db_result->column_names[i]);
            }
            free(db_result->column_names);
            PQclear_ptr(pg_result);
            free(db_result);
            return false;
        }

        strcpy(db_result->data_json, "[");
        for (size_t row = 0; row < db_result->row_count; row++) {
            if (row > 0) strcat(db_result->data_json, ",");
            strcat(db_result->data_json, "{");

            for (size_t col = 0; col < db_result->column_count; col++) {
                if (col > 0) strcat(db_result->data_json, ",");
                char* value = PQgetvalue_ptr(pg_result, (int)row, (int)col);
                char buffer[256];
                snprintf(buffer, sizeof(buffer), "\"%s\":\"%s\"",
                        db_result->column_names[col], value ? value : "");
                strcat(db_result->data_json, buffer);

                // Log the actual data values for debugging
                log_this(designator, "PostgreSQL execute_query: Row %zu, Column %zu (%s,4,3,2,1,0): %s", LOG_LEVEL_DEBUG, 4,
                    row, 
                    col, 
                    db_result->column_names[col], 
                    value ? value : "NULL");
            }
            strcat(db_result->data_json, "}");
        }
        strcat(db_result->data_json, "]");

        log_this(designator, "PostgreSQL execute_query: Complete result JSON: %s", LOG_LEVEL_DEBUG, 1, db_result->data_json);
    } else {
        log_this(designator, "PostgreSQL execute_query: Query returned no data (0 rows or 0 columns)", LOG_LEVEL_DEBUG, 0);
    }

    PQclear_ptr(pg_result);
    *result = db_result;

    // log_this(SR_DATABASE, "PostgreSQL query executed successfully", LOG_LEVEL_DEBUG, 0);
    return true;
}

bool postgresql_execute_prepared(DatabaseHandle* connection, PreparedStatement* stmt, QueryRequest* request, QueryResult** result) {
    if (!connection || !stmt || !request || !result || connection->engine_type != DB_ENGINE_POSTGRESQL) {
        return false;
    }

    PostgresConnection* pg_conn = (PostgresConnection*)connection->connection_handle;
    if (!pg_conn || !pg_conn->connection) {
        return false;
    }

    // For simplicity, execute as regular query for now
    // TODO: Implement proper prepared statement execution with parameters
    return postgresql_execute_query(connection, request, result);
}

/*
 * Transaction Management
 */

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

    // log_this(SR_DATABASE, "PostgreSQL transaction started", LOG_LEVEL_DEBUG, 0);
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

    // log_this(SR_DATABASE, "PostgreSQL transaction committed", LOG_LEVEL_DEBUG, 0);
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

    // log_this(SR_DATABASE, "PostgreSQL transaction rolled back", LOG_LEVEL_DEBUG, 0);
    return true;
}

/*
 * Prepared Statement Management
 */

bool postgresql_prepare_statement(DatabaseHandle* connection, const char* name, const char* sql, PreparedStatement** stmt) {
    if (!connection || !name || !sql || !stmt || connection->engine_type != DB_ENGINE_POSTGRESQL) {
        return false;
    }

    PostgresConnection* pg_conn = (PostgresConnection*)connection->connection_handle;
    if (!pg_conn || !pg_conn->connection) {
        return false;
    }

    // Set timeout for prepare operation
    void* timeout_result = PQexec_ptr(pg_conn->connection, "SET statement_timeout = 15000"); // 15 seconds
    if (timeout_result) {
        PQclear_ptr(timeout_result);
    }

    // Prepare the statement with timeout protection
    time_t start_time = time(NULL);
    void* res = PQprepare_ptr(pg_conn->connection, name, sql, 0, NULL);

    // Check if prepare took too long
    if (check_timeout_expired(start_time, 15)) {
        log_this(SR_DATABASE, "PostgreSQL PREPARE execution time exceeded 15 seconds", LOG_LEVEL_ERROR, 0);
        if (res) PQclear_ptr(res);
        return false;
    }

    if (PQresultStatus_ptr(res) != PGRES_COMMAND_OK) {
        log_this(SR_DATABASE, "PostgreSQL PREPARE failed", LOG_LEVEL_ERROR, 0);
        log_this(SR_DATABASE, PQerrorMessage_ptr(pg_conn->connection), LOG_LEVEL_ERROR, 0);
        PQclear_ptr(res);
        return false;
    }
    PQclear_ptr(res);

    // Add to cache
    if (!add_prepared_statement(pg_conn->prepared_statements, name)) {
        // Deallocate the prepared statement with timeout protection
        char dealloc_query[256];
        snprintf(dealloc_query, sizeof(dealloc_query), "DEALLOCATE %s", name);

        {
            // Set timeout for deallocate operation
            void* dealloc_timeout_result = PQexec_ptr(pg_conn->connection, "SET statement_timeout = 5000"); // 5 seconds
            if (dealloc_timeout_result) {
                PQclear_ptr(dealloc_timeout_result);
            }

            time_t dealloc_start = time(NULL);
            void* dealloc_res = PQexec_ptr(pg_conn->connection, dealloc_query);

            // Check if dealloc took too long
            if (check_timeout_expired(dealloc_start, 5)) {
                log_this(SR_DATABASE, "PostgreSQL DEALLOCATE on failure execution time exceeded 5 seconds", LOG_LEVEL_ERROR, 0);
            }

            if (dealloc_res) PQclear_ptr(dealloc_res);
        }
        return false;
    }

    // Create prepared statement structure
    PreparedStatement* prepared_stmt = calloc(1, sizeof(PreparedStatement));
    if (!prepared_stmt) {
        // Cleanup with timeout protection
        remove_prepared_statement(pg_conn->prepared_statements, name);
        char dealloc_query[256];
        snprintf(dealloc_query, sizeof(dealloc_query), "DEALLOCATE %s", name);

        {
            // Set timeout for deallocate operation
            void* stmt_timeout_result = PQexec_ptr(pg_conn->connection, "SET statement_timeout = 5000"); // 5 seconds
            if (stmt_timeout_result) {
                PQclear_ptr(stmt_timeout_result);
            }

            time_t dealloc_start = time(NULL);
            void* dealloc_res = PQexec_ptr(pg_conn->connection, dealloc_query);

            // Check if dealloc took too long
            if (check_timeout_expired(dealloc_start, 5)) {
                log_this(SR_DATABASE, "PostgreSQL DEALLOCATE on prepared statement failure execution time exceeded 5 seconds", LOG_LEVEL_ERROR, 0);
            }

            if (dealloc_res) PQclear_ptr(dealloc_res);
        }
        return false;
    }

    prepared_stmt->name = strdup(name);
    prepared_stmt->sql_template = strdup(sql);
    prepared_stmt->created_at = time(NULL);
    prepared_stmt->usage_count = 0;

    *stmt = prepared_stmt;

    // log_this(SR_DATABASE, "PostgreSQL prepared statement created", LOG_LEVEL_DEBUG, 0);
    return true;
}

bool postgresql_unprepare_statement(DatabaseHandle* connection, PreparedStatement* stmt) {
    if (!connection || !stmt || connection->engine_type != DB_ENGINE_POSTGRESQL) {
        return false;
    }

    PostgresConnection* pg_conn = (PostgresConnection*)connection->connection_handle;
    if (!pg_conn || !pg_conn->connection) {
        return false;
    }

    // Deallocate from database with timeout protection
    char dealloc_query[256];
    snprintf(dealloc_query, sizeof(dealloc_query), "DEALLOCATE %s", stmt->name);

    // Set timeout for deallocate operation
    void* timeout_result = PQexec_ptr(pg_conn->connection, "SET statement_timeout = 10000"); // 10 seconds
    if (timeout_result) {
        PQclear_ptr(timeout_result);
    }

    time_t start_time = time(NULL);
    void* res = PQexec_ptr(pg_conn->connection, dealloc_query);

    // Check if dealloc took too long
    if (check_timeout_expired(start_time, 10)) {
        log_this(SR_DATABASE, "PostgreSQL DEALLOCATE execution time exceeded 10 seconds", LOG_LEVEL_ERROR, 0);
        if (res) PQclear_ptr(res);
        return false;
    }

    if (PQresultStatus_ptr(res) != PGRES_COMMAND_OK) {
        log_this(SR_DATABASE, "PostgreSQL DEALLOCATE failed", LOG_LEVEL_ERROR, 0);
        PQclear_ptr(res);
        return false;
    }
    PQclear_ptr(res);

    // Remove from cache
    remove_prepared_statement(pg_conn->prepared_statements, stmt->name);

    // Free statement structure
    free(stmt->name);
    free(stmt->sql_template);
    free(stmt);

    // log_this(SR_DATABASE, "PostgreSQL prepared statement removed", LOG_LEVEL_DEBUG, 0);
    return true;
}

/*
 * Utility Functions
 */

char* postgresql_get_connection_string(ConnectionConfig* config) {
    if (!config) return NULL;

    char* conn_str = calloc(1, 1024);
    if (!conn_str) return NULL;

    if (config->connection_string) {
        strcpy(conn_str, config->connection_string);
    } else {
        snprintf(conn_str, 1024,
                 "postgresql://%s:%s@%s:%d/%s",
                 config->username ? config->username : "",
                 config->password ? config->password : "",
                 config->host ? config->host : "localhost",
                 config->port ? config->port : 5432,
                 config->database ? config->database : "postgres");
    }

    return conn_str;
}

bool postgresql_validate_connection_string(const char* connection_string) {
    if (!connection_string) return false;

    // Basic validation - check for postgresql:// prefix
    return strncmp(connection_string, "postgresql://", 13) == 0;
}

char* postgresql_escape_string(DatabaseHandle* connection, const char* input) {
    if (!connection || !input || connection->engine_type != DB_ENGINE_POSTGRESQL) {
        return NULL;
    }

    PostgresConnection* pg_conn = (PostgresConnection*)connection->connection_handle;
    if (!pg_conn || !pg_conn->connection) {
        return NULL;
    }

    // PostgreSQL escaping
    size_t escaped_len = strlen(input) * 2 + 1; // Worst case
    char* escaped = calloc(1, escaped_len);
    if (!escaped) return NULL;

    PQescapeStringConn_ptr(pg_conn->connection, escaped, input, strlen(input), NULL);
    return escaped;
}

/*
 * Engine Interface Registration
 */

static DatabaseEngineInterface postgresql_engine_interface = {
    .engine_type = DB_ENGINE_POSTGRESQL,
    .name = (char*)"postgresql",
    .connect = postgresql_connect,
    .disconnect = postgresql_disconnect,
    .health_check = postgresql_health_check,
    .reset_connection = postgresql_reset_connection,
    .execute_query = postgresql_execute_query,
    .execute_prepared = postgresql_execute_prepared,
    .begin_transaction = postgresql_begin_transaction,
    .commit_transaction = postgresql_commit_transaction,
    .rollback_transaction = postgresql_rollback_transaction,
    .prepare_statement = postgresql_prepare_statement,
    .unprepare_statement = postgresql_unprepare_statement,
    .get_connection_string = postgresql_get_connection_string,
    .validate_connection_string = postgresql_validate_connection_string,
    .escape_string = postgresql_escape_string
};

DatabaseEngineInterface* database_engine_postgresql_get_interface(void) {
    // Validate the interface structure before returning
    if (!postgresql_engine_interface.execute_query) {
        log_this(SR_DATABASE, "CRITICAL ERROR: PostgreSQL engine interface execute_query is NULL!", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    if (!postgresql_engine_interface.name) {
        log_this(SR_DATABASE, "CRITICAL ERROR: PostgreSQL engine interface name is NULL!", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    log_this(SR_DATABASE, "PostgreSQL engine interface validated: name=%s, execute_query=%p", LOG_LEVEL_DEBUG, 2,
        postgresql_engine_interface.name, 
        (void*)(uintptr_t)postgresql_engine_interface.execute_query);

    return &postgresql_engine_interface;
}