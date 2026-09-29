/*
 * MSSQL Database Engine - Connection Management Implementation
 *
 * Implements MSSQL connection management functions.
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
#ifdef USE_MOCK_LIBODBC
#include <unity/mocks/mock_libodbc.h>
#endif

// ODBC type definitions for MSSQL
typedef signed short SQLSMALLINT;
typedef long SQLINTEGER;
typedef unsigned long SQLUINTEGER;
typedef unsigned char SQLCHAR;
typedef short SQLRETURN;
typedef void* SQLHANDLE;
typedef void* SQLHWND;
typedef void* SQLPOINTER;
typedef unsigned short SQLUSMALLINT;

// ODBC constants
#define SQL_SUCCESS 0
#define SQL_SUCCESS_WITH_INFO 1
#define SQL_HANDLE_DBC 2
#define SQL_HANDLE_ENV 1
#define SQL_NTS -3
#define SQL_ATTR_QUERY_TIMEOUT 0     // Statement query timeout
#define SQL_ATTR_AUTOCOMMIT 102      // Auto-commit mode
#define SQL_COMMIT_TYPE_DEFAULT 0    // Default commit behavior
#define SQL_IS_UINTEGER -5           // SQLUINTEGER type indicator

// MSSQL function pointers (loaded dynamically or mocked)
SQLAllocHandle_t mssql_SQLAllocHandle_ptr = NULL;
SQLConnect_t mssql_SQLConnect_ptr = NULL;
SQLExecDirect_t mssql_SQLExecDirect_ptr = NULL;
SQLFetch_t mssql_SQLFetch_ptr = NULL;
SQLGetData_t mssql_SQLGetData_ptr = NULL;
SQLNumResultCols_t mssql_SQLNumResultCols_ptr = NULL;
SQLRowCount_t mssql_SQLRowCount_ptr = NULL;
SQLFreeHandle_t mssql_SQLFreeHandle_ptr = NULL;
SQLDisconnect_t mssql_SQLDisconnect_ptr = NULL;
SQLEndTran_t mssql_SQLEndTran_ptr = NULL;
SQLPrepare_t mssql_SQLPrepare_ptr = NULL;
SQLExecute_t mssql_SQLExecute_ptr = NULL;
SQLFreeStmt_t mssql_SQLFreeStmt_ptr = NULL;
SQLDescribeCol_t mssql_SQLDescribeCol_ptr = NULL;
SQLBindParameter_t mssql_SQLBindParameter_ptr = NULL;
// Transaction control function
SQLSetConnectAttr_t mssql_SQLSetConnectAttr_ptr = NULL;
SQLDriverConnect_t mssql_SQLDriverConnect_ptr = NULL;
SQLGetDiagRec_t mssql_SQLGetDiagRec_ptr = NULL;
// Watchdog cancel hook
SQLCancel_t mssql_SQLCancel_ptr = NULL;

// Library handle
#ifndef USE_MOCK_LIBODBC
static void* libodbc_handle = NULL;
static pthread_mutex_t libodbc_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

/*
 * Library Loading Functions
 */

bool load_msobdc_functions(const char* designator __attribute__((unused))) {
#ifdef USE_MOCK_LIBODBC
    // For mocking, directly assign mock function pointers
    mssql_SQLAllocHandle_ptr = mssql_mock_SQLAllocHandle;
    mssql_SQLConnect_ptr = mssql_mock_SQLConnect;
    mssql_SQLDriverConnect_ptr = mssql_mock_SQLDriverConnect;
    mssql_SQLExecDirect_ptr = mssql_mock_SQLExecDirect;
    mssql_SQLFetch_ptr = mssql_mock_SQLFetch;
    mssql_SQLGetData_ptr = mssql_mock_SQLGetData;
    mssql_SQLNumResultCols_ptr = mssql_mock_SQLNumResultCols;
    mssql_SQLRowCount_ptr = mssql_mock_SQLRowCount;
    mssql_SQLFreeHandle_ptr = mssql_mock_SQLFreeHandle;
    mssql_SQLDisconnect_ptr = mssql_mock_SQLDisconnect;
    mssql_SQLEndTran_ptr = mssql_mock_SQLEndTran;
    mssql_SQLPrepare_ptr = mssql_mock_SQLPrepare;
    mssql_SQLExecute_ptr = mssql_mock_SQLExecute;
    mssql_SQLFreeStmt_ptr = mssql_mock_SQLFreeStmt;
    mssql_SQLDescribeCol_ptr = mssql_mock_SQLDescribeCol;
    mssql_SQLGetDiagRec_ptr = mssql_mock_SQLGetDiagRec;
    mssql_SQLSetConnectAttr_ptr = mssql_mock_SQLSetConnectAttr;
    mssql_SQLBindParameter_ptr = mssql_mock_SQLBindParameter;
    mssql_SQLCancel_ptr = mssql_mock_SQLCancel;
    return true;
#else
    const char* log_subsystem = designator ? designator : SR_DATABASE;
    MUTEX_LOCK(&libodbc_mutex, log_subsystem);

    if (libodbc_handle) {
        MUTEX_UNLOCK(&libodbc_mutex, log_subsystem);
        return true; // Another thread loaded it
    }

    // Try to load libodbc (unixODBC)
    libodbc_handle = dlopen("libodbc.so", RTLD_LAZY);
    if (!libodbc_handle) {
        libodbc_handle = dlopen("libodbc.so.2", RTLD_LAZY);
    }
    if (!libodbc_handle) {
        log_this(log_subsystem, "Failed to load libodbc library", LOG_LEVEL_ERROR, 0);
        log_this(log_subsystem, dlerror(), LOG_LEVEL_ERROR, 0);
        MUTEX_UNLOCK(&libodbc_mutex, log_subsystem);
        return false;
    }

    // Load function pointers
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    mssql_SQLAllocHandle_ptr = (SQLAllocHandle_t)dlsym(libodbc_handle, "SQLAllocHandle");
    mssql_SQLConnect_ptr = (SQLConnect_t)dlsym(libodbc_handle, "SQLConnect");
    mssql_SQLDriverConnect_ptr = (SQLDriverConnect_t)dlsym(libodbc_handle, "SQLDriverConnect");
    mssql_SQLExecDirect_ptr = (SQLExecDirect_t)dlsym(libodbc_handle, "SQLExecDirect");
    mssql_SQLFetch_ptr = (SQLFetch_t)dlsym(libodbc_handle, "SQLFetch");
    mssql_SQLGetData_ptr = (SQLGetData_t)dlsym(libodbc_handle, "SQLGetData");
    mssql_SQLNumResultCols_ptr = (SQLNumResultCols_t)dlsym(libodbc_handle, "SQLNumResultCols");
    mssql_SQLRowCount_ptr = (SQLRowCount_t)dlsym(libodbc_handle, "SQLRowCount");
    mssql_SQLFreeHandle_ptr = (SQLFreeHandle_t)dlsym(libodbc_handle, "SQLFreeHandle");
    mssql_SQLDisconnect_ptr = (SQLDisconnect_t)dlsym(libodbc_handle, "SQLDisconnect");
    mssql_SQLEndTran_ptr = (SQLEndTran_t)dlsym(libodbc_handle, "SQLEndTran");
    mssql_SQLPrepare_ptr = (SQLPrepare_t)dlsym(libodbc_handle, "SQLPrepare");
    mssql_SQLExecute_ptr = (SQLExecute_t)dlsym(libodbc_handle, "SQLExecute");
    mssql_SQLFreeStmt_ptr = (SQLFreeStmt_t)dlsym(libodbc_handle, "SQLFreeStmt");
    mssql_SQLDescribeCol_ptr = (SQLDescribeCol_t)dlsym(libodbc_handle, "SQLDescribeCol");
    mssql_SQLGetDiagRec_ptr = (SQLGetDiagRec_t)dlsym(libodbc_handle, "SQLGetDiagRec");
    mssql_SQLSetConnectAttr_ptr = (SQLSetConnectAttr_t)(void*)dlsym(libodbc_handle, "SQLSetConnectAttr");
    mssql_SQLBindParameter_ptr = (SQLBindParameter_t)dlsym(libodbc_handle, "SQLBindParameter");
    mssql_SQLCancel_ptr = (SQLCancel_t)dlsym(libodbc_handle, "SQLCancel");
#pragma GCC diagnostic pop

    // Check if all required functions were loaded
    if (!mssql_SQLAllocHandle_ptr || !mssql_SQLConnect_ptr || !mssql_SQLDriverConnect_ptr || !mssql_SQLExecDirect_ptr ||
        !mssql_SQLFetch_ptr || !mssql_SQLGetData_ptr || !mssql_SQLNumResultCols_ptr ||
        !mssql_SQLFreeHandle_ptr || !mssql_SQLDisconnect_ptr) {
        log_this(log_subsystem, "Failed to load all required libodbc functions", LOG_LEVEL_ERROR, 0);
        dlclose(libodbc_handle);
        libodbc_handle = NULL;
        MUTEX_UNLOCK(&libodbc_mutex, log_subsystem);
        return false;
    }

    // Optional functions - log if not available
    if (!mssql_SQLEndTran_ptr) {
        log_this(log_subsystem, "SQLEndTran function not available - transactions may be limited", LOG_LEVEL_TRACE, 0);
    }
    if (!mssql_SQLPrepare_ptr || !mssql_SQLExecute_ptr || !mssql_SQLFreeStmt_ptr) {
        log_this(log_subsystem, "Prepared statement functions not available - prepared statements will be limited", LOG_LEVEL_TRACE, 0);
    }
    if (!mssql_SQLCancel_ptr) {
        log_this(log_subsystem, "SQLCancel function not available - watchdog cancel will be a no-op for MSSQL", LOG_LEVEL_ALERT, 0);
    }

    MUTEX_UNLOCK(&libodbc_mutex, log_subsystem);
    log_this(log_subsystem, "Successfully loaded libodbc library", LOG_LEVEL_TRACE, 0);
    return true;
#endif
}

/*
 * Utility Functions
 */

// Simple timeout mechanism without signals
bool mssql_check_timeout_expired(time_t start_time, int timeout_seconds) {
    return (time(NULL) - start_time) >= timeout_seconds;
}

PreparedStatementCache* mssql_create_prepared_statement_cache(void) {
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

void mssql_destroy_prepared_statement_cache(PreparedStatementCache* cache) {
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

bool mssql_connect(ConnectionConfig* config, DatabaseHandle** connection, const char* designator) {
    const char* log_subsystem = designator ? designator : SR_DATABASE;

    if (!config || !connection) {
        log_this(log_subsystem, "Invalid parameters for MSSQL connection", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Load libodbc library if not already loaded
    if (!load_msobdc_functions(designator)) {
        log_this(log_subsystem, "MSSQL connection failed: ODBC library not available", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Allocate environment handle
    void* env_handle = NULL;
    if (mssql_SQLAllocHandle_ptr(SQL_HANDLE_ENV, NULL, &env_handle) != SQL_SUCCESS) {
        log_this(log_subsystem, "MSSQL connection failed: Environment handle allocation failed", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Allocate connection handle
    void* conn_handle = NULL;
    if (mssql_SQLAllocHandle_ptr(SQL_HANDLE_DBC, env_handle, &conn_handle) != SQL_SUCCESS) {
        log_this(log_subsystem, "MSSQL connection failed: Connection handle allocation failed", LOG_LEVEL_ERROR, 0);
        mssql_SQLFreeHandle_ptr(SQL_HANDLE_ENV, env_handle);
        return false;
    }

    // Connect to database using ODBC connection string
    char* conn_string = NULL;
    if (config->connection_string) {
        conn_string = strdup(config->connection_string);
    } else {
        // Build ODBC connection string from config
        conn_string = mssql_get_connection_string(config);
    }

    if (!conn_string) {
        log_this(log_subsystem, "MSSQL connection failed: Unable to get connection string", LOG_LEVEL_ERROR, 0);
        mssql_SQLFreeHandle_ptr(SQL_HANDLE_DBC, conn_handle);
        mssql_SQLFreeHandle_ptr(SQL_HANDLE_ENV, env_handle);
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

    log_this(log_subsystem, "MSSQL connecting with: %s", LOG_LEVEL_TRACE, 1, safe_conn_str);

    // Use SQLDriverConnect for ODBC connection with Driver 18 settings
    char out_conn_string[1024] = {0};
    SQLSMALLINT out_conn_string_len = 0;
    SQLUSMALLINT driver_completion = 0; // SQL_DRIVER_NOPROMPT

    int result = mssql_SQLDriverConnect_ptr(
        conn_handle,
        NULL,  // No window handle
        (SQLCHAR*)conn_string, SQL_NTS,
        (SQLCHAR*)out_conn_string, sizeof(out_conn_string),
        &out_conn_string_len,
        driver_completion
    );

    // Clean up connection string
    free(conn_string);

    if (result == SQL_SUCCESS) {
        // Set connection attributes for optimal MSSQL performance
        int rc;

        // 1. Query timeout
        if (mssql_SQLSetConnectAttr_ptr) {
            rc = mssql_SQLSetConnectAttr_ptr(conn_handle, SQL_ATTR_QUERY_TIMEOUT, (long)30, 0);
            if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
                log_this(log_subsystem, "Failed to set query timeout", LOG_LEVEL_ALERT, 0);
            }
        }

        // 2. AUTOCOMMIT OFF - required so multi-statement LOAD/APPLY can roll back
        if (mssql_SQLSetConnectAttr_ptr) {
            rc = mssql_SQLSetConnectAttr_ptr(conn_handle, SQL_ATTR_AUTOCOMMIT, (long)SQL_AUTOCOMMIT_OFF, SQL_IS_UINTEGER);
            if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
                log_this(log_subsystem, "Failed to disable autocommit", LOG_LEVEL_ALERT, 0);
            }
        }

        // 3. Row array size - optional, but nice for bulk fetches later
        if (mssql_SQLSetConnectAttr_ptr) {
            SQLUINTEGER rows = 100;
            rc = mssql_SQLSetConnectAttr_ptr(conn_handle, SQL_ATTR_ROW_ARRAY_SIZE, (long)rows, SQL_IS_UINTEGER);
            if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
                log_this(log_subsystem, "Failed to set row array size", LOG_LEVEL_ALERT, 0);
            }
        }

        log_this(log_subsystem, "MSSQL ODBC: optimizations applied", LOG_LEVEL_TRACE, 0);
    }

    if (result != SQL_SUCCESS) {
        log_this(log_subsystem, "MSSQL connection failed: SQLDriverConnect returned %d", LOG_LEVEL_ERROR, 1, result);
        log_this(log_subsystem, "MSSQL connection details: %s", LOG_LEVEL_ERROR, 1, safe_conn_str);

        // Try to get detailed MSSQL error information
        log_this(log_subsystem, "MSSQL diagnostic: Connection handle is %p", LOG_LEVEL_TRACE, 1, (void*)conn_handle);
        if (conn_handle) {
            log_this(log_subsystem, "MSSQL attempting to retrieve diagnostic information", LOG_LEVEL_TRACE, 0);

#ifdef USE_MOCK_LIBODBC
            log_this(log_subsystem, "MSSQL diagnostic: SQLGetDiagRec is available (mocked)", LOG_LEVEL_TRACE, 0);
            // Always available in mock
            {
                char sql_state[6] = {0};
                char error_msg[1024] = {0};
                SQLINTEGER native_error = 0;
                SQLSMALLINT msg_len = 0;

                int diag_result = mssql_SQLGetDiagRec_ptr(SQL_HANDLE_DBC, conn_handle, 1,
                                                   (SQLCHAR*)sql_state, &native_error,
                                                   (SQLCHAR*)error_msg, sizeof(error_msg), &msg_len);
                if (diag_result == SQL_SUCCESS || diag_result == SQL_SUCCESS_WITH_INFO) {
                    log_this(log_subsystem, "MSSQL diagnostic: SQLSTATE='%s', Native Error=%d, Message='%s'",
                             LOG_LEVEL_ERROR, 3, sql_state, (int)native_error, error_msg);
                } else {
                    log_this(log_subsystem, "MSSQL diagnostic: SQLGetDiagRec returned %d (unable to retrieve error details)", LOG_LEVEL_ERROR, 1, (int)diag_result);
                }
            }
#else
            log_this(log_subsystem, "MSSQL diagnostic: mssql_SQLGetDiagRec_ptr is %s", LOG_LEVEL_TRACE, 1, mssql_SQLGetDiagRec_ptr ? "available" : "NULL");
            if (!mssql_SQLGetDiagRec_ptr) {
                log_this(log_subsystem, "MSSQL diagnostic: SQLGetDiagRec function not available", LOG_LEVEL_ERROR, 0);
            } else {
                char sql_state[6] = {0};
                char error_msg[1024] = {0};
                SQLINTEGER native_error = 0;
                SQLSMALLINT msg_len = 0;

                int diag_result = mssql_SQLGetDiagRec_ptr(SQL_HANDLE_DBC, conn_handle, 1,
                                                   (SQLCHAR*)sql_state, &native_error,
                                                   (SQLCHAR*)error_msg, sizeof(error_msg), &msg_len);
                if (diag_result == SQL_SUCCESS || diag_result == SQL_SUCCESS_WITH_INFO) {
                    log_this(log_subsystem, "MSSQL diagnostic: SQLSTATE='%s', Native Error=%d, Message='%s'",
                             LOG_LEVEL_ERROR, 3, sql_state, (int)native_error, error_msg);
                } else {
                    log_this(log_subsystem, "MSSQL diagnostic: SQLGetDiagRec returned %d (unable to retrieve error details)", LOG_LEVEL_ERROR, 1, (int)diag_result);
                }
            }
#endif
        } else {
            log_this(log_subsystem, "MSSQL diagnostic: No connection handle available for error retrieval", LOG_LEVEL_ERROR, 0);
        }

        // Clean up ODBC handles before returning
        mssql_SQLFreeHandle_ptr(SQL_HANDLE_DBC, conn_handle);
        mssql_SQLFreeHandle_ptr(SQL_HANDLE_ENV, env_handle);
        return false;
    }

    // Create database handle
    DatabaseHandle* db_handle = calloc(1, sizeof(DatabaseHandle));
    if (!db_handle) {
        mssql_SQLFreeHandle_ptr(SQL_HANDLE_DBC, conn_handle);
        mssql_SQLFreeHandle_ptr(SQL_HANDLE_ENV, env_handle);
        return false;
    }

    // Create MSSQL-specific connection wrapper
    MSSQLConnection* mssql_wrapper = calloc(1, sizeof(MSSQLConnection));
    if (!mssql_wrapper) {
        free(db_handle);
        mssql_SQLFreeHandle_ptr(SQL_HANDLE_DBC, conn_handle);
        mssql_SQLFreeHandle_ptr(SQL_HANDLE_ENV, env_handle);
        return false;
    }

    mssql_wrapper->environment = env_handle;
    mssql_wrapper->connection = conn_handle;
    mssql_wrapper->prepared_statements = mssql_create_prepared_statement_cache();
    mssql_wrapper->active_stmt = NULL;
    pthread_mutex_init(&mssql_wrapper->active_stmt_lock, NULL);
    if (!mssql_wrapper->prepared_statements) {
        pthread_mutex_destroy(&mssql_wrapper->active_stmt_lock);
        free(mssql_wrapper);
        free(db_handle);
        mssql_SQLFreeHandle_ptr(SQL_HANDLE_DBC, conn_handle);
        mssql_SQLFreeHandle_ptr(SQL_HANDLE_ENV, env_handle);
        return false;
    }

    // Store designator for future use (disconnect, etc.)
    db_handle->designator = designator ? strdup(designator) : NULL;

    // Initialize database handle
    db_handle->engine_type = DB_ENGINE_MSSQL;
    db_handle->connection_handle = mssql_wrapper;
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

    log_this(log_subsystem, "MSSQL connection established successfully", LOG_LEVEL_TRACE, 0);
    return true;
}

bool mssql_disconnect(DatabaseHandle* connection) {
    if (!connection || connection->engine_type != DB_ENGINE_MSSQL) {
        return false;
    }

    MSSQLConnection* mssql_conn = (MSSQLConnection*)connection->connection_handle;
    if (mssql_conn) {
        // Disconnect from database and free ODBC handles
        if (mssql_conn->connection) {
            if (mssql_SQLDisconnect_ptr) {
                mssql_SQLDisconnect_ptr(mssql_conn->connection);
            }
            mssql_SQLFreeHandle_ptr(SQL_HANDLE_DBC, mssql_conn->connection);
            mssql_conn->connection = NULL; // Mark as disconnected
        }
        if (mssql_conn->environment) {
            mssql_SQLFreeHandle_ptr(SQL_HANDLE_ENV, mssql_conn->environment);
            mssql_conn->environment = NULL; // Mark as freed
        }
        pthread_mutex_destroy(&mssql_conn->active_stmt_lock);
        mssql_conn->active_stmt = NULL;

        // NOTE: Do NOT free mssql_conn here - it's needed by database_engine_cleanup_connection()
        // to unprepare statements. The MSSQLConnection structure will be freed there.
    }

    connection->status = DB_CONNECTION_DISCONNECTED;

    // Use stored designator for logging if available
    const char* log_subsystem = connection->designator ? connection->designator : SR_DATABASE;
    log_this(log_subsystem, "MSSQL connection closed", LOG_LEVEL_TRACE, 0);

    // NOTE: DatabaseHandle cleanup (designator, mutex, free) is done by database_engine_cleanup_connection()
    // This function only handles MSSQL-specific ODBC cleanup (disconnect, free ODBC handles)

    return true;
}

bool mssql_health_check(DatabaseHandle* connection) {
    if (!connection || connection->engine_type != DB_ENGINE_MSSQL) {
        return false;
    }

    const MSSQLConnection* mssql_conn = (const MSSQLConnection*)connection->connection_handle;
    if (!mssql_conn || !mssql_conn->connection) {
        return false;
    }

    // Implement basic MSSQL health check with a simple query
    void* stmt_handle = NULL;
    bool health_check_passed = false;

    // Allocate statement handle for health check
    if (mssql_SQLAllocHandle_ptr(SQL_HANDLE_STMT, mssql_conn->connection, &stmt_handle) == SQL_SUCCESS) {
        // Execute a simple health check query (no FROM needed in SQL Server)
        const char* health_query = "SELECT 1";
        int exec_result = mssql_SQLExecDirect_ptr(stmt_handle, (char*)health_query, SQL_NTS);

        if (exec_result == SQL_SUCCESS || exec_result == SQL_SUCCESS_WITH_INFO) {
            health_check_passed = true;
        }

        // Clean up statement handle
        mssql_SQLFreeHandle_ptr(SQL_HANDLE_STMT, stmt_handle);
    }

    if (health_check_passed) {
        connection->last_health_check = time(NULL);
        connection->consecutive_failures = 0;
        return true;
    } else {
        connection->consecutive_failures++;
        const char* log_subsystem = connection->designator ? connection->designator : SR_DATABASE;
        log_this(log_subsystem, "MSSQL health check failed", LOG_LEVEL_ERROR, 0);
        return false;
    }
}

bool mssql_reset_connection(DatabaseHandle* connection) {
    if (!connection || connection->engine_type != DB_ENGINE_MSSQL) {
        return false;
    }

    // MSSQL connections are persistent
    connection->status = DB_CONNECTION_CONNECTED;
    connection->connected_since = time(NULL);
    connection->consecutive_failures = 0;

    const char* log_subsystem = connection->designator ? connection->designator : SR_DATABASE;
    log_this(log_subsystem, "MSSQL connection reset successfully", LOG_LEVEL_TRACE, 0);
    return true;
}

/*
 * Cancel any in-flight query on this MSSQL connection.
 *
 * The MSSQL query path stores the active statement handle in
 * MSSQLConnection::active_stmt while the query is in flight and
 * clears it on completion. We read the handle under a small mutex
 * and call SQLCancel on it, which the ODBC spec documents as
 * safe to call from a different thread than the one executing
 * the statement. SQLCancel causes SQLExecDirect / SQLExecute on
 * the original thread to return SQL_ERROR with a "statement was
 * cancelled" diagnostic.
 */
void mssql_cancel_inflight(DatabaseHandle* connection) {
    if (!connection || connection->engine_type != DB_ENGINE_MSSQL) {
        return;
    }
    if (!mssql_SQLCancel_ptr) {
        return;
    }

    MSSQLConnection* mssql_conn = (MSSQLConnection*)connection->connection_handle;
    if (!mssql_conn) {
        return;
    }

    pthread_mutex_lock(&mssql_conn->active_stmt_lock);
    void* active_stmt = mssql_conn->active_stmt;
    pthread_mutex_unlock(&mssql_conn->active_stmt_lock);

    if (!active_stmt) {
        return; // No in-flight query
    }

    int rc = mssql_SQLCancel_ptr(active_stmt);
    const char* designator = connection->designator ? connection->designator : SR_DATABASE;
    if (rc != 0 && rc != 1) {  // SQL_SUCCESS=0, SQL_SUCCESS_WITH_INFO=1
        log_this(designator, "MSSQL: SQLCancel returned %d", LOG_LEVEL_ERROR, 1, rc);
    } else {
        log_this(designator, "MSSQL: requested cancel of in-flight query", LOG_LEVEL_ALERT, 0);
    }
}

/*
 * Set the in-flight statement handle so the watchdog can find it.
 * Called by mssql_execute_query / mssql_execute_prepared after
 * SQLAllocHandle returns. Holds active_stmt_lock briefly.
 */
void mssql_active_stmt_set(DatabaseHandle* connection, void* stmt_handle) {
    if (!connection || connection->engine_type != DB_ENGINE_MSSQL || !stmt_handle) {
        return;
    }
    MSSQLConnection* mssql_conn = (MSSQLConnection*)connection->connection_handle;
    if (!mssql_conn) {
        return;
    }
    pthread_mutex_lock(&mssql_conn->active_stmt_lock);
    mssql_conn->active_stmt = stmt_handle;
    pthread_mutex_unlock(&mssql_conn->active_stmt_lock);
}

/*
 * Clear the in-flight statement handle. Called by mssql_execute_query
 * / mssql_execute_prepared immediately before SQLFreeHandle so the
 * watchdog does not see a freed handle. Only clears if the
 * currently stored handle matches the supplied one - protects
 * against a stale clear from a different query.
 */
void mssql_active_stmt_clear(DatabaseHandle* connection, const void* stmt_handle) {
    if (!connection || connection->engine_type != DB_ENGINE_MSSQL) {
        return;
    }
    MSSQLConnection* mssql_conn = (MSSQLConnection*)connection->connection_handle;
    if (!mssql_conn) {
        return;
    }
    pthread_mutex_lock(&mssql_conn->active_stmt_lock);
    if (mssql_conn->active_stmt == stmt_handle) {
        mssql_conn->active_stmt = NULL;
    }
    pthread_mutex_unlock(&mssql_conn->active_stmt_lock);
}
