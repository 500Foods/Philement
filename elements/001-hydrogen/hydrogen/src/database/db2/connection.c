/*
 * DB2 Database Engine - Connection Management Implementation
 *
 * Implements DB2 connection management functions.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/database/dbqueue/dbqueue.h>

// Local includes
#include "types.h"
#include "connection.h"
#include "utils.h"

// Include mock functions when testing
#ifdef USE_MOCK_LIBDB2
#include <unity/mocks/mock_libdb2.h>
#endif

// ODBC type definitions for DB2
typedef signed short SQLSMALLINT;
typedef long SQLINTEGER;
typedef unsigned char SQLCHAR;
typedef short SQLRETURN;
typedef void* SQLHANDLE;
typedef void* SQLHWND;
typedef unsigned short SQLUSMALLINT;

// ODBC constants
#define SQL_SUCCESS 0
#define SQL_SUCCESS_WITH_INFO 1
#define SQL_HANDLE_DBC 2
#define SQL_HANDLE_ENV 1
#define SQL_NTS -3

// DB2 function pointers (loaded dynamically or mocked)
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
SQLDescribeCol_t SQLDescribeCol_ptr = NULL;
// Transaction control function
SQLSetConnectAttr_t SQLSetConnectAttr_ptr = NULL;
SQLDriverConnect_t SQLDriverConnect_ptr = NULL;
SQLGetDiagRec_t SQLGetDiagRec_ptr = NULL;

// Library handle
#ifndef USE_MOCK_LIBDB2
static void* libdb2_handle = NULL;
static pthread_mutex_t libdb2_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

/*
 * Library Loading Functions
 */

bool load_libdb2_functions(const char* designator __attribute__((unused))) {
#ifdef USE_MOCK_LIBDB2
    // For mocking, directly assign mock function pointers
    SQLAllocHandle_ptr = mock_SQLAllocHandle;
    SQLConnect_ptr = mock_SQLConnect;
    SQLDriverConnect_ptr = mock_SQLDriverConnect;
    SQLExecDirect_ptr = mock_SQLExecDirect;
    SQLFetch_ptr = mock_SQLFetch;
    SQLGetData_ptr = mock_SQLGetData;
    SQLNumResultCols_ptr = mock_SQLNumResultCols;
    SQLRowCount_ptr = mock_SQLRowCount;
    SQLFreeHandle_ptr = mock_SQLFreeHandle;
    SQLDisconnect_ptr = mock_SQLDisconnect;
    SQLEndTran_ptr = mock_SQLEndTran;
    SQLPrepare_ptr = mock_SQLPrepare;
    SQLExecute_ptr = mock_SQLExecute;
    SQLFreeStmt_ptr = mock_SQLFreeStmt;
    SQLDescribeCol_ptr = mock_SQLDescribeCol;
    SQLGetDiagRec_ptr = mock_SQLGetDiagRec;
    SQLSetConnectAttr_ptr = mock_SQLSetConnectAttr;
    return true;
#else
    if (libdb2_handle) {
        return true; // Already loaded
    }

    const char* log_subsystem = designator ? designator : SR_DATABASE;
    MUTEX_LOCK(&libdb2_mutex, log_subsystem);

    if (libdb2_handle) {
        MUTEX_UNLOCK(&libdb2_mutex, log_subsystem);
        return true; // Another thread loaded it
    }

    // Try to load libdb2
    libdb2_handle = dlopen("libdb2.so", RTLD_LAZY);
    if (!libdb2_handle) {
        libdb2_handle = dlopen("libdb2.so.1", RTLD_LAZY);
    }
    if (!libdb2_handle) {
        log_this(log_subsystem, "Failed to load libdb2 library", LOG_LEVEL_ERROR, 0);
        log_this(log_subsystem, dlerror(), LOG_LEVEL_ERROR, 0);
        MUTEX_UNLOCK(&libdb2_mutex, log_subsystem);
        return false;
    }

    // Load function pointers
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    SQLAllocHandle_ptr = (SQLAllocHandle_t)dlsym(libdb2_handle, "SQLAllocHandle");
    SQLConnect_ptr = (SQLConnect_t)dlsym(libdb2_handle, "SQLConnect");
    SQLDriverConnect_ptr = (SQLDriverConnect_t)dlsym(libdb2_handle, "SQLDriverConnect");
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
    SQLDescribeCol_ptr = (SQLDescribeCol_t)dlsym(libdb2_handle, "SQLDescribeCol");
    SQLGetDiagRec_ptr = (SQLGetDiagRec_t)dlsym(libdb2_handle, "SQLGetDiagRec");
    SQLSetConnectAttr_ptr = (SQLSetConnectAttr_t)dlsym(libdb2_handle, "SQLSetConnectAttr");
#pragma GCC diagnostic pop

    // Check if all required functions were loaded
    if (!SQLAllocHandle_ptr || !SQLConnect_ptr || !SQLDriverConnect_ptr || !SQLExecDirect_ptr ||
        !SQLFetch_ptr || !SQLGetData_ptr || !SQLNumResultCols_ptr ||
        !SQLFreeHandle_ptr || !SQLDisconnect_ptr) {
        log_this(log_subsystem, "Failed to load all required libdb2 functions", LOG_LEVEL_ERROR, 0);
        dlclose(libdb2_handle);
        libdb2_handle = NULL;
        MUTEX_UNLOCK(&libdb2_mutex, log_subsystem);
        return false;
    }

    // Optional functions - log if not available
    if (!SQLEndTran_ptr) {
        log_this(log_subsystem, "SQLEndTran function not available - transactions may be limited", LOG_LEVEL_TRACE, 0);
    }
    if (!SQLPrepare_ptr || !SQLExecute_ptr || !SQLFreeStmt_ptr) {
        log_this(log_subsystem, "Prepared statement functions not available - prepared statements will be limited", LOG_LEVEL_TRACE, 0);
    }

    MUTEX_UNLOCK(&libdb2_mutex, log_subsystem);
    log_this(log_subsystem, "Successfully loaded libdb2 library", LOG_LEVEL_TRACE, 0);
    return true;
#endif
}

/*
 * Utility Functions
 */

// Simple timeout mechanism without signals
bool db2_check_timeout_expired(time_t start_time, int timeout_seconds) {
    return (time(NULL) - start_time) >= timeout_seconds;
}

PreparedStatementCache* db2_create_prepared_statement_cache(void) {
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

void db2_destroy_prepared_statement_cache(PreparedStatementCache* cache) {
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

bool db2_connect(ConnectionConfig* config, DatabaseHandle** connection, const char* designator) {
    const char* log_subsystem = designator ? designator : SR_DATABASE;

    if (!config || !connection) {
        log_this(log_subsystem, "Invalid parameters for DB2 connection", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Load libdb2 library if not already loaded
    if (!load_libdb2_functions(designator)) {
        log_this(log_subsystem, "DB2 connection failed: DB2 library not available", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Allocate environment handle
    void* env_handle = NULL;
    if (SQLAllocHandle_ptr(SQL_HANDLE_ENV, NULL, &env_handle) != SQL_SUCCESS) {
        log_this(log_subsystem, "DB2 connection failed: Environment handle allocation failed", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Allocate connection handle
    void* conn_handle = NULL;
    if (SQLAllocHandle_ptr(SQL_HANDLE_DBC, env_handle, &conn_handle) != SQL_SUCCESS) {
        log_this(log_subsystem, "DB2 connection failed: Connection handle allocation failed", LOG_LEVEL_ERROR, 0);
        SQLFreeHandle_ptr(SQL_HANDLE_ENV, env_handle);
        return false;
    }

    // Connect to database using SQLDriverConnect for full connection string support
    char* conn_string = NULL;
    if (config->connection_string) {
        conn_string = strdup(config->connection_string);
        // log_this(log_subsystem, "DB2 connecting using provided connection string", LOG_LEVEL_TRACE, 0);
    } else {
        // Build full connection string from config
        conn_string = db2_get_connection_string(config);
        // log_this(log_subsystem, "DB2 connecting using built connection string", LOG_LEVEL_TRACE, 0);
    }

    if (!conn_string) {
        log_this(log_subsystem, "DB2 connection failed: Unable to get connection string", LOG_LEVEL_ERROR, 0);
        SQLFreeHandle_ptr(SQL_HANDLE_DBC, conn_handle);
        SQLFreeHandle_ptr(SQL_HANDLE_ENV, env_handle);
        return false;
    }

    // Mask password in logs for security
    char safe_conn_str[1024];
    strcpy(safe_conn_str, conn_string);
    char* pwd_pos = strstr(safe_conn_str, "PWD=");
    if (pwd_pos) {
        const char* end_pos = strchr(pwd_pos, ';');
        if (end_pos) {
            memset(pwd_pos + 4, '*', (size_t)(end_pos - (pwd_pos + 4)));
        } else {
            memset(pwd_pos + 4, '*', strlen(pwd_pos + 4));
        }
    }

    log_this(log_subsystem, "%s", LOG_LEVEL_TRACE, 1, safe_conn_str);

    // Use SQLDriverConnect for full connection string support
    char out_conn_string[1024] = {0};
    SQLSMALLINT out_conn_string_len = 0;
    SQLUSMALLINT driver_completion = 0; // SQL_DRIVER_NOPROMPT

    int result = SQLDriverConnect_ptr(
        conn_handle,
        NULL,  // No window handle
        (SQLCHAR*)conn_string, SQL_NTS,
        (SQLCHAR*)out_conn_string, sizeof(out_conn_string),
        &out_conn_string_len,
        driver_completion
    );

    // Clean up connection string
    free(conn_string);

    if (result != SQL_SUCCESS) {
        log_this(log_subsystem, "DB2 connection failed: SQLDriverConnect returned %d", LOG_LEVEL_ERROR, 1, result);
        log_this(log_subsystem, "DB2 connection details: %s", LOG_LEVEL_ERROR, 1, safe_conn_str);

        // Try to get detailed DB2 error information
        log_this(log_subsystem, "DB2 diagnostic: Connection handle is %p", LOG_LEVEL_TRACE, 1, (void*)conn_handle);
        if (conn_handle) {
            log_this(log_subsystem, "DB2 attempting to retrieve diagnostic information", LOG_LEVEL_TRACE, 0);

            // Check if SQLGetDiagRec is available
#ifdef USE_MOCK_LIBDB2
            log_this(log_subsystem, "DB2 diagnostic: SQLGetDiagRec is available (mocked)", LOG_LEVEL_TRACE, 0);
            // Always available in mock
            {
                char sql_state[6] = {0};
                char error_msg[1024] = {0};
                SQLINTEGER native_error = 0;
                SQLSMALLINT msg_len = 0;

                int diag_result = SQLGetDiagRec_ptr(SQL_HANDLE_DBC, conn_handle, 1,
                                                   (SQLCHAR*)sql_state, &native_error,
                                                   (SQLCHAR*)error_msg, sizeof(error_msg), &msg_len);
                if (diag_result == SQL_SUCCESS || diag_result == SQL_SUCCESS_WITH_INFO) {
                    log_this(log_subsystem, "DB2 diagnostic: SQLSTATE='%s', Native Error=%d, Message='%s'",
                             LOG_LEVEL_ERROR, 3, sql_state, (int)native_error, error_msg);
                } else {
                    log_this(log_subsystem, "DB2 diagnostic: SQLGetDiagRec returned %d (unable to retrieve error details)", LOG_LEVEL_ERROR, 1, (int)diag_result);
                }
            }
#else
            log_this(log_subsystem, "DB2 diagnostic: SQLGetDiagRec_ptr is %s", LOG_LEVEL_TRACE, 1, SQLGetDiagRec_ptr ? "available" : "NULL");
            if (!SQLGetDiagRec_ptr) {
                log_this(log_subsystem, "DB2 diagnostic: SQLGetDiagRec function not available", LOG_LEVEL_ERROR, 0);
            } else {
                char sql_state[6] = {0};
                char error_msg[1024] = {0};
                SQLINTEGER native_error = 0;
                SQLSMALLINT msg_len = 0;

                int diag_result = SQLGetDiagRec_ptr(SQL_HANDLE_DBC, conn_handle, 1,
                                                   (SQLCHAR*)sql_state, &native_error,
                                                   (SQLCHAR*)error_msg, sizeof(error_msg), &msg_len);
                if (diag_result == SQL_SUCCESS || diag_result == SQL_SUCCESS_WITH_INFO) {
                    log_this(log_subsystem, "DB2 diagnostic: SQLSTATE='%s', Native Error=%d, Message='%s'",
                             LOG_LEVEL_ERROR, 3, sql_state, (int)native_error, error_msg);
                } else {
                    log_this(log_subsystem, "DB2 diagnostic: SQLGetDiagRec returned %d (unable to retrieve error details)", LOG_LEVEL_ERROR, 1, (int)diag_result);
                }
            }
#endif
        } else {
            log_this(log_subsystem, "DB2 diagnostic: No connection handle available for error retrieval", LOG_LEVEL_ERROR, 0);
        }

        // Clean up ODBC handles before returning
        SQLFreeHandle_ptr(SQL_HANDLE_DBC, conn_handle);
        SQLFreeHandle_ptr(SQL_HANDLE_ENV, env_handle);
        return false;
    }

    // Create database handle
    DatabaseHandle* db_handle = calloc(1, sizeof(DatabaseHandle));
    if (!db_handle) {
        SQLFreeHandle_ptr(SQL_HANDLE_DBC, conn_handle);
        SQLFreeHandle_ptr(SQL_HANDLE_ENV, env_handle);
        return false;
    }

    // Create DB2-specific connection wrapper
    DB2Connection* db2_wrapper = calloc(1, sizeof(DB2Connection));
    if (!db2_wrapper) {
        free(db_handle);
        SQLFreeHandle_ptr(SQL_HANDLE_DBC, conn_handle);
        SQLFreeHandle_ptr(SQL_HANDLE_ENV, env_handle);
        return false;
    }

    db2_wrapper->environment = env_handle;
    db2_wrapper->connection = conn_handle;
    db2_wrapper->prepared_statements = db2_create_prepared_statement_cache();
    if (!db2_wrapper->prepared_statements) {
        free(db2_wrapper);
        free(db_handle);
        SQLFreeHandle_ptr(SQL_HANDLE_DBC, conn_handle);
        SQLFreeHandle_ptr(SQL_HANDLE_ENV, env_handle);
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
    db_handle->prepared_statement_lru_counter = NULL; // LRU tracking for prepared statements
    pthread_mutex_init(&db_handle->connection_lock, NULL);
    db_handle->in_use = false;
    db_handle->last_health_check = time(NULL);
    db_handle->consecutive_failures = 0;

    *connection = db_handle;

    // Use designator for logging if provided, otherwise use generic Database subsystem
    // log_this(log_subsystem, "DB2 connection established successfully", LOG_LEVEL_TRACE, 0);
    return true;
}

bool db2_disconnect(DatabaseHandle* connection) {
    if (!connection || connection->engine_type != DB_ENGINE_DB2) {
        return false;
    }

    DB2Connection* db2_conn = (DB2Connection*)connection->connection_handle;
    if (db2_conn) {
        // Disconnect from database and free ODBC handles
        if (db2_conn->connection) {
            if (SQLDisconnect_ptr) {
                SQLDisconnect_ptr(db2_conn->connection);
            }
            SQLFreeHandle_ptr(SQL_HANDLE_DBC, db2_conn->connection);
            db2_conn->connection = NULL; // Mark as disconnected
        }
        if (db2_conn->environment) {
            SQLFreeHandle_ptr(SQL_HANDLE_ENV, db2_conn->environment);
            db2_conn->environment = NULL; // Mark as freed
        }
        
        // NOTE: Do NOT free db2_conn here - it's needed by database_engine_cleanup_connection()
        // to unprepare statements. The DB2Connection structure will be freed there.
    }

    connection->status = DB_CONNECTION_DISCONNECTED;

    // Use stored designator for logging if available
    const char* log_subsystem = connection->designator ? connection->designator : SR_DATABASE;
    log_this(log_subsystem, "DB2 connection closed", LOG_LEVEL_TRACE, 0);
    
    // NOTE: DatabaseHandle cleanup (designator, mutex, free) is done by database_engine_cleanup_connection()
    // This function only handles DB2-specific ODBC cleanup (disconnect, free ODBC handles)
    
    return true;
}

bool db2_health_check(DatabaseHandle* connection) {
    if (!connection || connection->engine_type != DB_ENGINE_DB2) {
        return false;
    }

    const DB2Connection* db2_conn = (const DB2Connection*)connection->connection_handle;
    if (!db2_conn || !db2_conn->connection) {
        return false;
    }

    // Implement basic DB2 health check with a simple query
    void* stmt_handle = NULL;
    bool health_check_passed = false;

    // Allocate statement handle for health check
    if (SQLAllocHandle_ptr(SQL_HANDLE_STMT, db2_conn->connection, &stmt_handle) == SQL_SUCCESS) {
        // Execute a simple health check query
        const char* health_query = "SELECT 1 FROM SYSIBM.SYSDUMMY1";
        int exec_result = SQLExecDirect_ptr(stmt_handle, (char*)health_query, SQL_NTS);

        if (exec_result == SQL_SUCCESS || exec_result == SQL_SUCCESS_WITH_INFO) {
            health_check_passed = true;
        }

        // Clean up statement handle
        SQLFreeHandle_ptr(SQL_HANDLE_STMT, stmt_handle);
    }

    if (health_check_passed) {
        connection->last_health_check = time(NULL);
        connection->consecutive_failures = 0;
        return true;
    } else {
        connection->consecutive_failures++;
        const char* log_subsystem = connection->designator ? connection->designator : SR_DATABASE;
        log_this(log_subsystem, "DB2 health check failed", LOG_LEVEL_ERROR, 0);
        return false;
    }
}

bool db2_reset_connection(DatabaseHandle* connection) {
    if (!connection || connection->engine_type != DB_ENGINE_DB2) {
        return false;
    }

    // DB2 connections are persistent
    connection->status = DB_CONNECTION_CONNECTED;
    connection->connected_since = time(NULL);
    connection->consecutive_failures = 0;

    const char* log_subsystem = connection->designator ? connection->designator : SR_DATABASE;
    log_this(log_subsystem, "DB2 connection reset successfully", LOG_LEVEL_TRACE, 0);
    return true;
}