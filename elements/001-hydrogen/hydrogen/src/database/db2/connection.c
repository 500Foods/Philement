/*
 * DB2 Database Engine - Connection Management Implementation
 *
 * Implements DB2 connection management functions.
 */
#include "../../hydrogen.h"
#include "../database.h"
#include "../database_queue.h"
#include "types.h"
#include "connection.h"

// DB2 function pointers (loaded dynamically)
SQLAllocHandle_t SQLAllocHandle_ptr = NULL;
SQLConnect_t SQLConnect_ptr = NULL;
SQLExecDirect_t SQLExecDirect_ptr = NULL;
SQLFetch_t SQLFetch_ptr = NULL;
SQLGetData_t SQLGetData_ptr = NULL;
SQLNumResultCols_t SQLNumResultCols_ptr = NULL;
SQLRowCount_t SQLRowCount_ptr = NULL;
SQLFreeHandle_t SQLFreeHandle_ptr = NULL;
SQLDisconnect_t SQLDisconnect_ptr = NULL;
SQLEndTran_t SQLEndTran_ptr = NULL;
SQLPrepare_t SQLPrepare_ptr = NULL;
SQLExecute_t SQLExecute_ptr = NULL;
SQLFreeStmt_t SQLFreeStmt_ptr = NULL;

// Library handle
void* libdb2_handle = NULL;
pthread_mutex_t libdb2_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * Library Loading Functions
 */

bool load_libdb2_functions(void) {
    if (libdb2_handle) {
        return true; // Already loaded
    }

    pthread_mutex_lock(&libdb2_mutex);

    if (libdb2_handle) {
        pthread_mutex_unlock(&libdb2_mutex);
        return true; // Another thread loaded it
    }

    // Try to load libdb2
    libdb2_handle = dlopen("libdb2.so", RTLD_LAZY);
    if (!libdb2_handle) {
        libdb2_handle = dlopen("libdb2.so.1", RTLD_LAZY);
    }
    if (!libdb2_handle) {
        log_this(SR_DATABASE, "Failed to load libdb2 library", LOG_LEVEL_ERROR, 0);
        log_this(SR_DATABASE, dlerror(), LOG_LEVEL_ERROR, 0);
        pthread_mutex_unlock(&libdb2_mutex);
        return false;
    }

    // Load function pointers
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    SQLAllocHandle_ptr = (SQLAllocHandle_t)dlsym(libdb2_handle, "SQLAllocHandle");
    SQLConnect_ptr = (SQLConnect_t)dlsym(libdb2_handle, "SQLConnect");
    SQLExecDirect_ptr = (SQLExecDirect_t)dlsym(libdb2_handle, "SQLExecDirect");
    SQLFetch_ptr = (SQLFetch_t)dlsym(libdb2_handle, "SQLFetch");
    SQLGetData_ptr = (SQLGetData_t)dlsym(libdb2_handle, "SQLGetData");
    SQLNumResultCols_ptr = (SQLNumResultCols_t)dlsym(libdb2_handle, "SQLNumResultCols");
    SQLRowCount_ptr = (SQLRowCount_t)dlsym(libdb2_handle, "SQLRowCount");
    SQLFreeHandle_ptr = (SQLFreeHandle_t)dlsym(libdb2_handle, "SQLFreeHandle");
    SQLDisconnect_ptr = (SQLDisconnect_t)dlsym(libdb2_handle, "SQLDisconnect");
    SQLEndTran_ptr = (SQLEndTran_t)dlsym(libdb2_handle, "SQLEndTran");
    SQLPrepare_ptr = (SQLPrepare_t)dlsym(libdb2_handle, "SQLPrepare");
    SQLExecute_ptr = (SQLExecute_t)dlsym(libdb2_handle, "SQLExecute");
    SQLFreeStmt_ptr = (SQLFreeStmt_t)dlsym(libdb2_handle, "SQLFreeStmt");
#pragma GCC diagnostic pop

    // Check if all required functions were loaded
    if (!SQLAllocHandle_ptr || !SQLConnect_ptr || !SQLExecDirect_ptr ||
        !SQLFetch_ptr || !SQLGetData_ptr || !SQLNumResultCols_ptr ||
        !SQLFreeHandle_ptr || !SQLDisconnect_ptr) {
        log_this(SR_DATABASE, "Failed to load all required libdb2 functions", LOG_LEVEL_ERROR, 0);
        dlclose(libdb2_handle);
        libdb2_handle = NULL;
        pthread_mutex_unlock(&libdb2_mutex);
        return false;
    }

    // Optional functions - log if not available
    if (!SQLEndTran_ptr) {
        log_this(SR_DATABASE, "SQLEndTran function not available - transactions may be limited", LOG_LEVEL_DEBUG, 0);
    }
    if (!SQLPrepare_ptr || !SQLExecute_ptr || !SQLFreeStmt_ptr) {
        log_this(SR_DATABASE, "Prepared statement functions not available - prepared statements will be limited", LOG_LEVEL_DEBUG, 0);
    }

    pthread_mutex_unlock(&libdb2_mutex);
    log_this(SR_DATABASE, "Successfully loaded libdb2 library", LOG_LEVEL_STATE, 0);
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

/*
 * Connection Management
 */

bool db2_connect(ConnectionConfig* config, DatabaseHandle** connection, const char* designator) {
    if (!config || !connection) {
        log_this(SR_DATABASE, "Invalid parameters for DB2 connection", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Load libdb2 library if not already loaded
    if (!load_libdb2_functions()) {
        log_this(SR_DATABASE, "DB2 library not available", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Allocate environment handle
    void* env_handle = NULL;
    if (SQLAllocHandle_ptr(SQL_HANDLE_ENV, NULL, &env_handle) != SQL_SUCCESS) {
        log_this(SR_DATABASE, "DB2 environment allocation failed", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Allocate connection handle
    void* conn_handle = NULL;
    if (SQLAllocHandle_ptr(SQL_HANDLE_DBC, env_handle, &conn_handle) != SQL_SUCCESS) {
        log_this(SR_DATABASE, "DB2 connection allocation failed", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Connect to database
    char dsn[256];
    if (config->connection_string) {
        strcpy(dsn, config->connection_string);
    } else {
        // Build DSN from config
        snprintf(dsn, sizeof(dsn), "%s", config->database ? config->database : "SAMPLE");
    }

    int result = SQLConnect_ptr(
        conn_handle,
        (char*)dsn, SQL_NTS,
        config->username ? (char*)config->username : NULL, SQL_NTS,
        config->password ? (char*)config->password : NULL, SQL_NTS
    );

    if (result != SQL_SUCCESS) {
        log_this(SR_DATABASE, "DB2 connection failed", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Create database handle
    DatabaseHandle* db_handle = calloc(1, sizeof(DatabaseHandle));
    if (!db_handle) {
        return false;
    }

    // Create DB2-specific connection wrapper
    DB2Connection* db2_wrapper = calloc(1, sizeof(DB2Connection));
    if (!db2_wrapper) {
        free(db_handle);
        return false;
    }

    db2_wrapper->environment = env_handle;
    db2_wrapper->connection = conn_handle;
    db2_wrapper->prepared_statements = create_prepared_statement_cache();
    if (!db2_wrapper->prepared_statements) {
        free(db2_wrapper);
        free(db_handle);
        return false;
    }

    // Store designator for future use (disconnect, etc.)
    db_handle->designator = designator ? strdup(designator) : NULL;

    // Initialize database handle
    db_handle->engine_type = DB_ENGINE_DB2;
    db_handle->connection_handle = db2_wrapper;
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
    log_this(log_subsystem, "DB2 connection established successfully", LOG_LEVEL_STATE, 0);
    return true;
}

bool db2_disconnect(DatabaseHandle* connection) {
    if (!connection || connection->engine_type != DB_ENGINE_DB2) {
        return false;
    }

    DB2Connection* db2_conn = (DB2Connection*)connection->connection_handle;
    if (db2_conn) {
        destroy_prepared_statement_cache(db2_conn->prepared_statements);
        free(db2_conn);
    }

    connection->status = DB_CONNECTION_DISCONNECTED;

    // Use stored designator for logging if available
    const char* log_subsystem = connection->designator ? connection->designator : SR_DATABASE;
    log_this(log_subsystem, "DB2 connection closed", LOG_LEVEL_STATE, 0);
    return true;
}

bool db2_health_check(DatabaseHandle* connection) {
    if (!connection || connection->engine_type != DB_ENGINE_DB2) {
        return false;
    }

    DB2Connection* db2_conn = (DB2Connection*)connection->connection_handle;
    if (!db2_conn || !db2_conn->connection) {
        return false;
    }

    // TODO: Implement DB2 health check query
    // For now, assume healthy if connection exists
    connection->last_health_check = time(NULL);
    connection->consecutive_failures = 0;
    return true;
}

bool db2_reset_connection(DatabaseHandle* connection) {
    if (!connection || connection->engine_type != DB_ENGINE_DB2) {
        return false;
    }

    // DB2 connections are persistent
    connection->status = DB_CONNECTION_CONNECTED;
    connection->connected_since = time(NULL);
    connection->consecutive_failures = 0;

    log_this(SR_DATABASE, "DB2 connection reset successfully", LOG_LEVEL_STATE, 0);
    return true;
}