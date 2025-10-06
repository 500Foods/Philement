/*
 * PostgreSQL Database Engine - Connection Management Implementation
 *
 * Implements PostgreSQL connection management functions.
 */
#include "../../hydrogen.h"
#include "../database.h"
#include "../database_queue.h"
#include "connection.h"

#ifdef USE_MOCK_LIBPQ
#include "../../../tests/unity/mocks/mock_libpq.h"
#endif

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

// PostgreSQL function pointers (loaded dynamically or mocked)
#ifdef USE_MOCK_LIBPQ
// For mocking, assign all mock function pointers
PQconnectdb_t PQconnectdb_ptr = mock_PQconnectdb;
PQstatus_t PQstatus_ptr = mock_PQstatus;
PQerrorMessage_t PQerrorMessage_ptr = mock_PQerrorMessage;
PQfinish_t PQfinish_ptr = mock_PQfinish;
PQexec_t PQexec_ptr = mock_PQexec;
PQresultStatus_t PQresultStatus_ptr = mock_PQresultStatus;
PQclear_t PQclear_ptr = mock_PQclear;
PQntuples_t PQntuples_ptr = mock_PQntuples;
PQnfields_t PQnfields_ptr = mock_PQnfields;
PQfname_t PQfname_ptr = mock_PQfname;
PQgetvalue_t PQgetvalue_ptr = mock_PQgetvalue;
PQcmdTuples_t PQcmdTuples_ptr = mock_PQcmdTuples;
PQreset_t PQreset_ptr = mock_PQreset;
PQprepare_t PQprepare_ptr = mock_PQprepare;
PQescapeStringConn_t PQescapeStringConn_ptr = mock_PQescapeStringConn;
PQping_t PQping_ptr = mock_PQping;
#else
PQconnectdb_t PQconnectdb_ptr = NULL;
PQstatus_t PQstatus_ptr = NULL;
PQerrorMessage_t PQerrorMessage_ptr = NULL;
PQfinish_t PQfinish_ptr = NULL;
PQexec_t PQexec_ptr = NULL;
PQresultStatus_t PQresultStatus_ptr = NULL;
PQclear_t PQclear_ptr = NULL;
PQntuples_t PQntuples_ptr = NULL;
PQnfields_t PQnfields_ptr = NULL;
PQfname_t PQfname_ptr = NULL;
PQgetvalue_t PQgetvalue_ptr = NULL;
PQcmdTuples_t PQcmdTuples_ptr = NULL;
PQreset_t PQreset_ptr = NULL;
PQprepare_t PQprepare_ptr = NULL;
PQescapeStringConn_t PQescapeStringConn_ptr = NULL;
PQping_t PQping_ptr = NULL;
#endif

// Library handle
#ifndef USE_MOCK_LIBPQ
static void* libpq_handle = NULL;
static pthread_mutex_t libpq_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

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

// Simple timeout mechanism without signals
bool check_timeout_expired(time_t start_time, int timeout_seconds) {
#ifdef USE_MOCK_LIBPQ
    return mock_check_timeout_expired(start_time, timeout_seconds);
#else
    return (time(NULL) - start_time) >= timeout_seconds;
#endif
}

// Library Loading Functions
bool load_libpq_functions(const char* designator __attribute__((unused))) {
#ifdef USE_MOCK_LIBPQ
    // For mocking, assign all mock function pointers
    PQconnectdb_ptr = mock_PQconnectdb;
    PQstatus_ptr = mock_PQstatus;
    PQerrorMessage_ptr = mock_PQerrorMessage;
    PQfinish_ptr = mock_PQfinish;
    PQexec_ptr = mock_PQexec;
    PQresultStatus_ptr = mock_PQresultStatus;
    PQclear_ptr = mock_PQclear;
    PQntuples_ptr = mock_PQntuples;
    PQnfields_ptr = mock_PQnfields;
    PQfname_ptr = mock_PQfname;
    PQgetvalue_ptr = mock_PQgetvalue;
    PQcmdTuples_ptr = mock_PQcmdTuples;
    PQreset_ptr = mock_PQreset;
    PQprepare_ptr = mock_PQprepare;
    PQescapeStringConn_ptr = mock_PQescapeStringConn;
    PQping_ptr = mock_PQping;
    return true;
#else
    if (libpq_handle) {
        return true; // Already loaded
    }

    const char* log_subsystem = designator ? designator : SR_DATABASE;
    MUTEX_LOCK(&libpq_mutex, log_subsystem);

    if (libpq_handle) {
        MUTEX_UNLOCK(&libpq_mutex, log_subsystem);
        return true; // Another thread loaded it
    }

    // Try to load libpq
    libpq_handle = dlopen("libpq.so.5", RTLD_LAZY);
    if (!libpq_handle) {
        libpq_handle = dlopen("libpq.so", RTLD_LAZY);
    }
    if (!libpq_handle) {
        log_this(log_subsystem, "Failed to load libpq library", LOG_LEVEL_ERROR, 0);
        log_this(log_subsystem, dlerror(), LOG_LEVEL_ERROR, 0);
        MUTEX_UNLOCK(&libpq_mutex, log_subsystem);
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
        log_this(log_subsystem, "Failed to load all required libpq functions", LOG_LEVEL_ERROR, 0);
        dlclose(libpq_handle);
        libpq_handle = NULL;
        MUTEX_UNLOCK(&libpq_mutex, log_subsystem);
        return false;
    }

    // PQping is optional - log if not available
    if (!PQping_ptr) {
        log_this(log_subsystem, "PQping function not available - health check will use query method only", LOG_LEVEL_DEBUG, 0);
    }

    MUTEX_UNLOCK(&libpq_mutex, log_subsystem);
    // log_this(log_subsystem, "Successfully loaded libpq library", LOG_LEVEL_STATE, 0);
    return true;
#endif
}

// Utility Functions for Prepared Statement Cache
PreparedStatementCache* create_prepared_statement_cache(void) {
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

void destroy_prepared_statement_cache(PreparedStatementCache* cache) {
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

bool add_prepared_statement(PreparedStatementCache* cache, const char* name) {
    if (!cache || !name) return false;

    MUTEX_LOCK(&cache->lock, SR_DATABASE);

    // Check if already exists
    for (size_t i = 0; i < cache->count; i++) {
        if (strcmp(cache->names[i], name) == 0) {
            MUTEX_UNLOCK(&cache->lock, SR_DATABASE);
            return true; // Already exists
        }
    }

    // Expand capacity if needed
    if (cache->count >= cache->capacity) {
        size_t new_capacity = cache->capacity * 2;
        char** new_names = realloc(cache->names, new_capacity * sizeof(char*));
        if (!new_names) {
            MUTEX_UNLOCK(&cache->lock, SR_DATABASE);
            return false;
        }
        cache->names = new_names;
        cache->capacity = new_capacity;
    }

    cache->names[cache->count] = strdup(name);
    if (!cache->names[cache->count]) {
        MUTEX_UNLOCK(&cache->lock, SR_DATABASE);
        return false;
    }

    cache->count++;
    MUTEX_UNLOCK(&cache->lock, SR_DATABASE);
    return true;
}

bool remove_prepared_statement(PreparedStatementCache* cache, const char* name) {
    if (!cache || !name) return false;

    MUTEX_LOCK(&cache->lock, SR_DATABASE);

    for (size_t i = 0; i < cache->count; i++) {
        if (strcmp(cache->names[i], name) == 0) {
            free(cache->names[i]);
            // Shift remaining elements
            for (size_t j = i; j < cache->count - 1; j++) {
                cache->names[j] = cache->names[j + 1];
            }
            cache->count--;
            MUTEX_UNLOCK(&cache->lock, SR_DATABASE);
            return true;
        }
    }

    MUTEX_UNLOCK(&cache->lock, SR_DATABASE);
    return false;
}

// Connection Management Functions
bool postgresql_connect(ConnectionConfig* config, DatabaseHandle** connection, const char* designator) {
    if (!config || !connection) {
        log_this(SR_DATABASE, "Invalid parameters for PostgreSQL connection", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Load libpq library if not already loaded
    if (!load_libpq_functions(designator)) {
        const char* log_subsystem = designator ? designator : SR_DATABASE;
        log_this(log_subsystem, "PostgreSQL library not available", LOG_LEVEL_ERROR, 0);
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
    if (!connection) {
        const char* designator = SR_DATABASE;
        log_this(designator, "PostgreSQL health check: connection is NULL", LOG_LEVEL_ERROR, 0);
        return false;
    }

    const char* designator = connection->designator ? connection->designator : SR_DATABASE;
    // Early validation logging - add simple log to verify function is called
    log_this(designator, "PostgreSQL health check: FUNCTION ENTRY - connection=%p, designator=%s", LOG_LEVEL_ERROR, 2, (void*)connection, designator);
    log_this(designator, "PostgreSQL health check: Starting validation", LOG_LEVEL_DEBUG, 0);

    // Add immediate return check to isolate the failure point
    log_this(designator, "PostgreSQL health check: About to check connection pointer", LOG_LEVEL_DEBUG, 0);
    log_this(designator, "PostgreSQL health check: Connection pointer: %p", LOG_LEVEL_DEBUG, 1, (void*)connection);
    log_this(designator, "PostgreSQL health check: Connection engine_type: %d", LOG_LEVEL_DEBUG, 1, connection->engine_type);

    if (connection->engine_type != DB_ENGINE_POSTGRESQL) {
        log_this(designator, "PostgreSQL health check: wrong engine type %d", LOG_LEVEL_ERROR, 1, connection->engine_type);
        return false;
    }

    // Function pointer validation
    log_this(designator, "PostgreSQL health check: Validating function pointers", LOG_LEVEL_DEBUG, 0);
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
    log_this(designator, "PostgreSQL health check: All function pointers validated", LOG_LEVEL_DEBUG, 0);

    log_this(designator, "PostgreSQL health check: Validating connection wrapper", LOG_LEVEL_DEBUG, 0);
    PostgresConnection* pg_conn = (PostgresConnection*)connection->connection_handle;
    log_this(designator, "PostgreSQL health check: connection_handle: %p", LOG_LEVEL_DEBUG, 1, connection->connection_handle);
    if (!pg_conn) {
        log_this(designator, "PostgreSQL health check: pg_conn is NULL", LOG_LEVEL_ERROR, 0);
        return false;
    }
    log_this(designator, "PostgreSQL health check: pg_conn validated", LOG_LEVEL_DEBUG, 0);

    if (!pg_conn->connection) {
        log_this(designator, "PostgreSQL health check: pg_conn->connection is NULL", LOG_LEVEL_ERROR, 0);
        return false;
    }
    log_this(designator, "PostgreSQL health check: pg_conn->connection validated", LOG_LEVEL_DEBUG, 0);

    // Check connection status
    if (PQstatus_ptr && PQstatus_ptr(pg_conn->connection) != CONNECTION_OK) {
        int conn_status = PQstatus_ptr(pg_conn->connection);
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