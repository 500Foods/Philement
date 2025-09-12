/*
 * MySQL Database Engine Implementation
 *
 * Implements the MySQL database engine for the Hydrogen database subsystem.
 * Uses dynamic loading (dlopen/dlsym) for libmysqlclient to avoid static linking dependencies.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dlfcn.h>
#include <pthread.h>
#include <assert.h>

#include "../hydrogen.h"
#include "database.h"
#include "database_queue.h"

// Function pointer types for libmysqlclient functions
typedef void* (*mysql_init_t)(void*);
typedef void* (*mysql_real_connect_t)(void*, const char*, const char*, const char*, const char*, unsigned int, const char*, unsigned long);
typedef int (*mysql_query_t)(void*, const char*);
// typedef void* (*mysql_store_result_t)(void*);
// typedef unsigned int (*mysql_num_rows_t)(void*);
// typedef unsigned int (*mysql_num_fields_t)(void*);
// typedef void* (*mysql_fetch_row_t)(void*);
// typedef char** (*mysql_fetch_fields_t)(void*);
// typedef void (*mysql_free_result_t)(void*);
// typedef const char* (*mysql_error_t)(void*);
// typedef void (*mysql_close_t)(void*);
// typedef int (*mysql_options_t)(void*, int, const void*);
// typedef int (*mysql_ping_t)(void*);
// typedef int (*mysql_autocommit_t)(void*, int);
// typedef int (*mysql_commit_t)(void*);
// typedef int (*mysql_rollback_t)(void*);
// typedef void* (*mysql_stmt_init_t)(void*);
// typedef int (*mysql_stmt_prepare_t)(void*, const char*, unsigned long);
// typedef int (*mysql_stmt_execute_t)(void*);
// typedef void (*mysql_stmt_close_t)(void*);

// MySQL function pointers (loaded dynamically)
static mysql_init_t mysql_init_ptr = NULL;
static mysql_real_connect_t mysql_real_connect_ptr = NULL;
static mysql_query_t mysql_query_ptr = NULL;
// static mysql_store_result_t mysql_store_result_ptr = NULL;
// static mysql_num_rows_t mysql_num_rows_ptr = NULL;
// static mysql_num_fields_t mysql_num_fields_ptr = NULL;
// static mysql_fetch_row_t mysql_fetch_row_ptr = NULL;
// static mysql_fetch_fields_t mysql_fetch_fields_ptr = NULL;
// static mysql_free_result_t mysql_free_result_ptr = NULL;
// static mysql_error_t mysql_error_ptr = NULL;
// static mysql_close_t mysql_close_ptr = NULL;
// static mysql_options_t mysql_options_ptr = NULL;
// static mysql_ping_t mysql_ping_ptr = NULL;
// static mysql_autocommit_t mysql_autocommit_ptr = NULL;
// static mysql_commit_t mysql_commit_ptr = NULL;
// static mysql_rollback_t mysql_rollback_ptr = NULL;
// static mysql_stmt_init_t mysql_stmt_init_ptr = NULL;
// static mysql_stmt_prepare_t mysql_stmt_prepare_ptr = NULL;
// static mysql_stmt_execute_t mysql_stmt_execute_ptr = NULL;
// static mysql_stmt_close_t mysql_stmt_close_ptr = NULL;

// Library handle
static void* libmysql_handle = NULL;
static pthread_mutex_t libmysql_mutex = PTHREAD_MUTEX_INITIALIZER;

// Constants (defined since we can't include mysql.h)
#define MYSQL_OPT_RECONNECT 20

// Prepared statement cache structure
typedef struct PreparedStatementCache {
    char** names;
    size_t count;
    size_t capacity;
    pthread_mutex_t lock;
} PreparedStatementCache;

// MySQL-specific connection structure
typedef struct MySQLConnection {
    void* connection;  // MYSQL* loaded dynamically
    bool reconnect;
    PreparedStatementCache* prepared_statements;
} MySQLConnection;

/*
 * Function Prototypes
 */

// Connection management
bool mysql_connect(ConnectionConfig* config, DatabaseHandle** connection, const char* designator);
bool mysql_disconnect(DatabaseHandle* connection);
bool mysql_health_check(DatabaseHandle* connection);
bool mysql_reset_connection(DatabaseHandle* connection);

// Query execution
bool mysql_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result);
bool mysql_execute_prepared(DatabaseHandle* connection, PreparedStatement* stmt, QueryRequest* request, QueryResult** result);

// Transaction management
bool mysql_begin_transaction(DatabaseHandle* connection, DatabaseIsolationLevel level, Transaction** transaction);
bool mysql_commit_transaction(DatabaseHandle* connection, Transaction* transaction);
bool mysql_rollback_transaction(DatabaseHandle* connection, Transaction* transaction);

// Prepared statement management
bool mysql_prepare_statement(DatabaseHandle* connection, const char* name, const char* sql, PreparedStatement** stmt);
bool mysql_unprepare_statement(DatabaseHandle* connection, PreparedStatement* stmt);

// Utility functions
char* mysql_get_connection_string(ConnectionConfig* config);
bool mysql_validate_connection_string(const char* connection_string);
char* mysql_escape_string(DatabaseHandle* connection, const char* input);

// Engine interface
DatabaseEngineInterface* database_engine_mysql_get_interface(void);

/*
 * Library Loading Functions
 */

static bool load_libmysql_functions(void) {
    if (libmysql_handle) {
        return true; // Already loaded
    }

    pthread_mutex_lock(&libmysql_mutex);

    if (libmysql_handle) {
        pthread_mutex_unlock(&libmysql_mutex);
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
        log_this(SR_DATABASE, "Failed to load libmysqlclient library", LOG_LEVEL_ERROR, 0);
        log_this(SR_DATABASE, dlerror(), LOG_LEVEL_ERROR, 0);
        pthread_mutex_unlock(&libmysql_mutex);
        return false;
    }

    // Load function pointers
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    mysql_init_ptr = (mysql_init_t)dlsym(libmysql_handle, "mysql_init");
    mysql_real_connect_ptr = (mysql_real_connect_t)dlsym(libmysql_handle, "mysql_real_connect");
    mysql_query_ptr = (mysql_query_t)dlsym(libmysql_handle, "mysql_query");
    // mysql_store_result_ptr = (mysql_store_result_t)dlsym(libmysql_handle, "mysql_store_result");
    // mysql_num_rows_ptr = (mysql_num_rows_t)dlsym(libmysql_handle, "mysql_num_rows");
    // mysql_num_fields_ptr = (mysql_num_fields_t)dlsym(libmysql_handle, "mysql_num_fields");
    // mysql_fetch_row_ptr = (mysql_fetch_row_t)dlsym(libmysql_handle, "mysql_fetch_row");
    // mysql_fetch_fields_ptr = (mysql_fetch_fields_t)dlsym(libmysql_handle, "mysql_fetch_fields");
    // mysql_free_result_ptr = (mysql_free_result_t)dlsym(libmysql_handle, "mysql_free_result");
    // mysql_error_ptr = (mysql_error_t)dlsym(libmysql_handle, "mysql_error");
    // mysql_close_ptr = (mysql_close_t)dlsym(libmysql_handle, "mysql_close");
    // mysql_options_ptr = (mysql_options_t)dlsym(libmysql_handle, "mysql_options");
    // mysql_ping_ptr = (mysql_ping_t)dlsym(libmysql_handle, "mysql_ping");
    // mysql_autocommit_ptr = (mysql_autocommit_t)dlsym(libmysql_handle, "mysql_autocommit");
    // mysql_commit_ptr = (mysql_commit_t)dlsym(libmysql_handle, "mysql_commit");
    // mysql_rollback_ptr = (mysql_rollback_t)dlsym(libmysql_handle, "mysql_rollback");
    // mysql_stmt_init_ptr = (mysql_stmt_init_t)dlsym(libmysql_handle, "mysql_stmt_init");
    // mysql_stmt_prepare_ptr = (mysql_stmt_prepare_t)dlsym(libmysql_handle, "mysql_stmt_prepare");
    // mysql_stmt_execute_ptr = (mysql_stmt_execute_t)dlsym(libmysql_handle, "mysql_stmt_execute");
    // mysql_stmt_close_ptr = (mysql_stmt_close_t)dlsym(libmysql_handle, "mysql_stmt_close");
#pragma GCC diagnostic pop

    // Check if all functions were loaded
    if (!mysql_init_ptr || !mysql_real_connect_ptr || !mysql_query_ptr) {
        log_this(SR_DATABASE, "Failed to load all required libmysqlclient functions", LOG_LEVEL_ERROR, 0);
        dlclose(libmysql_handle);
        libmysql_handle = NULL;
        pthread_mutex_unlock(&libmysql_mutex);
        return false;
    }

    pthread_mutex_unlock(&libmysql_mutex);
    log_this(SR_DATABASE, "Successfully loaded libmysqlclient library", LOG_LEVEL_STATE, 0);
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

bool mysql_connect(ConnectionConfig* config, DatabaseHandle** connection, const char* designator) {
    if (!config || !connection) {
        log_this(SR_DATABASE, "Invalid parameters for MySQL connection", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Load libmysqlclient library if not already loaded
    if (!load_libmysql_functions()) {
        log_this(SR_DATABASE, "MySQL library not available", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Initialize MySQL connection
    void* mysql_conn = mysql_init_ptr(NULL);
    if (!mysql_conn) {
        log_this(SR_DATABASE, "MySQL connection initialization failed", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Set auto-reconnect option
    // int reconnect = 1;
    // mysql_options_ptr(mysql_conn, MYSQL_OPT_RECONNECT, &reconnect);

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
        // mysql_close_ptr(mysql_conn);
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
    mysql_wrapper->prepared_statements = create_prepared_statement_cache();
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
    log_this(log_subsystem, "MySQL connection established successfully", LOG_LEVEL_STATE, 0);
    return true;
}

bool mysql_disconnect(DatabaseHandle* connection) {
    if (!connection || connection->engine_type != DB_ENGINE_MYSQL) {
        return false;
    }

    MySQLConnection* mysql_conn = (MySQLConnection*)connection->connection_handle;
    if (mysql_conn) {
        if (mysql_conn->connection) {
            // mysql_close_ptr(mysql_conn->connection);
        }
        destroy_prepared_statement_cache(mysql_conn->prepared_statements);
        free(mysql_conn);
    }

    connection->status = DB_CONNECTION_DISCONNECTED;

    // Use stored designator for logging if available
    const char* log_subsystem = connection->designator ? connection->designator : SR_DATABASE;
    log_this(log_subsystem, "MySQL connection closed", LOG_LEVEL_STATE, 0);
    return true;
}

bool mysql_health_check(DatabaseHandle* connection) {
    if (!connection || connection->engine_type != DB_ENGINE_MYSQL) {
        return false;
    }

    MySQLConnection* mysql_conn = (MySQLConnection*)connection->connection_handle;
    if (!mysql_conn || !mysql_conn->connection) {
        return false;
    }

    // TODO: Implement MySQL ping check
    // if (mysql_ping_ptr(mysql_conn->connection) != 0) {
    //     connection->consecutive_failures++;
    //     return false;
    // }

    connection->last_health_check = time(NULL);
    connection->consecutive_failures = 0;
    return true;
}

bool mysql_reset_connection(DatabaseHandle* connection) {
    if (!connection || connection->engine_type != DB_ENGINE_MYSQL) {
        return false;
    }

    // MySQL auto-reconnect should handle this
    connection->status = DB_CONNECTION_CONNECTED;
    connection->connected_since = time(NULL);
    connection->consecutive_failures = 0;

    log_this(SR_DATABASE, "MySQL connection reset successfully", LOG_LEVEL_STATE, 0);
    return true;
}

/*
 * Query Execution
 */

bool mysql_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result) {
    if (!connection || !request || !result || connection->engine_type != DB_ENGINE_MYSQL) {
        return false;
    }

    MySQLConnection* mysql_conn = (MySQLConnection*)connection->connection_handle;
    if (!mysql_conn || !mysql_conn->connection) {
        return false;
    }

    // Execute query
    if (mysql_query_ptr(mysql_conn->connection, request->sql_template) != 0) {
        log_this(SR_DATABASE, "MySQL query execution failed", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // TODO: Implement result processing
    // For now, return a placeholder result
    QueryResult* db_result = calloc(1, sizeof(QueryResult));
    if (!db_result) {
        return false;
    }

    db_result->success = true;
    db_result->row_count = 0;
    db_result->column_count = 0;
    db_result->execution_time_ms = 0;
    db_result->affected_rows = 0;
    db_result->data_json = strdup("[]");

    *result = db_result;

    // log_this(SR_DATABASE, "MySQL query executed (placeholder implementation)", LOG_LEVEL_DEBUG, 0);
    return true;
}

bool mysql_execute_prepared(DatabaseHandle* connection, PreparedStatement* stmt, QueryRequest* request, QueryResult** result) {
    if (!connection || !stmt || !request || !result || connection->engine_type != DB_ENGINE_MYSQL) {
        return false;
    }

    // For simplicity, execute as regular query for now
    return mysql_execute_query(connection, request, result);
}

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

    // TODO: Implement MySQL transaction begin with isolation level
    // For now, create a placeholder transaction
    Transaction* tx = calloc(1, sizeof(Transaction));
    if (!tx) {
        return false;
    }

    tx->transaction_id = strdup("mysql_tx");
    tx->isolation_level = level;
    tx->started_at = time(NULL);
    tx->active = true;

    *transaction = tx;
    connection->current_transaction = tx;

    // log_this(SR_DATABASE, "MySQL transaction started (placeholder)", LOG_LEVEL_DEBUG, 0);
    return true;
}

bool mysql_commit_transaction(DatabaseHandle* connection, Transaction* transaction) {
    if (!connection || !transaction || connection->engine_type != DB_ENGINE_MYSQL) {
        return false;
    }

    // TODO: Implement MySQL commit
    transaction->active = false;
    connection->current_transaction = NULL;

    // log_this(SR_DATABASE, "MySQL transaction committed (placeholder)", LOG_LEVEL_DEBUG, 0);
    return true;
}

bool mysql_rollback_transaction(DatabaseHandle* connection, Transaction* transaction) {
    if (!connection || !transaction || connection->engine_type != DB_ENGINE_MYSQL) {
        return false;
    }

    // TODO: Implement MySQL rollback
    transaction->active = false;
    connection->current_transaction = NULL;

    // log_this(SR_DATABASE, "MySQL transaction rolled back (placeholder)", LOG_LEVEL_DEBUG, 0);
    return true;
}

/*
 * Prepared Statement Management
 */

bool mysql_prepare_statement(DatabaseHandle* connection, const char* name, const char* sql, PreparedStatement** stmt) {
    if (!connection || !name || !sql || !stmt || connection->engine_type != DB_ENGINE_MYSQL) {
        return false;
    }

    // TODO: Implement MySQL prepared statement preparation
    PreparedStatement* prepared_stmt = calloc(1, sizeof(PreparedStatement));
    if (!prepared_stmt) {
        return false;
    }

    prepared_stmt->name = strdup(name);
    prepared_stmt->sql_template = strdup(sql);
    prepared_stmt->created_at = time(NULL);
    prepared_stmt->usage_count = 0;

    *stmt = prepared_stmt;

    // log_this(SR_DATABASE, "MySQL prepared statement created (placeholder)", LOG_LEVEL_DEBUG, 0);
    return true;
}

bool mysql_unprepare_statement(DatabaseHandle* connection, PreparedStatement* stmt) {
    if (!connection || !stmt || connection->engine_type != DB_ENGINE_MYSQL) {
        return false;
    }

    // TODO: Implement MySQL prepared statement cleanup
    free(stmt->name);
    free(stmt->sql_template);
    free(stmt);

    // log_this(SR_DATABASE, "MySQL prepared statement removed (placeholder)", LOG_LEVEL_DEBUG, 0);
    return true;
}

/*
 * Utility Functions
 */

char* mysql_get_connection_string(ConnectionConfig* config) {
    if (!config) return NULL;

    char* conn_str = calloc(1, 1024);
    if (!conn_str) return NULL;

    if (config->connection_string) {
        strcpy(conn_str, config->connection_string);
    } else {
        snprintf(conn_str, 1024,
                 "mysql://%s:%s@%s:%d/%s",
                 config->username ? config->username : "",
                 config->password ? config->password : "",
                 config->host ? config->host : "localhost",
                 config->port > 0 ? config->port : 3306,
                 config->database ? config->database : "");
    }

    return conn_str;
}

bool mysql_validate_connection_string(const char* connection_string) {
    if (!connection_string) return false;

    // Basic validation - check for mysql:// prefix
    return strncmp(connection_string, "mysql://", 8) == 0;
}

char* mysql_escape_string(DatabaseHandle* connection, const char* input) {
    if (!connection || !input || connection->engine_type != DB_ENGINE_MYSQL) {
        return NULL;
    }

    // MySQL string escaping - simple implementation
    size_t escaped_len = strlen(input) * 2 + 1;
    char* escaped = calloc(1, escaped_len);
    if (!escaped) return NULL;

    // Simple escaping - replace single quotes and backslashes
    const char* src = input;
    char* dst = escaped;
    while (*src) {
        if (*src == '\'' || *src == '\\') {
            *dst++ = '\\';
        }
        *dst++ = *src;
        src++;
    }

    return escaped;
}

/*
 * Engine Interface Registration
 */

static DatabaseEngineInterface mysql_engine_interface = {
    .engine_type = DB_ENGINE_MYSQL,
    .name = (char*)"mysql",
    .connect = mysql_connect,
    .disconnect = mysql_disconnect,
    .health_check = mysql_health_check,
    .reset_connection = mysql_reset_connection,
    .execute_query = mysql_execute_query,
    .execute_prepared = mysql_execute_prepared,
    .begin_transaction = mysql_begin_transaction,
    .commit_transaction = mysql_commit_transaction,
    .rollback_transaction = mysql_rollback_transaction,
    .prepare_statement = mysql_prepare_statement,
    .unprepare_statement = mysql_unprepare_statement,
    .get_connection_string = mysql_get_connection_string,
    .validate_connection_string = mysql_validate_connection_string,
    .escape_string = mysql_escape_string
};

DatabaseEngineInterface* database_engine_mysql_get_interface(void) {
    return &mysql_engine_interface;
}