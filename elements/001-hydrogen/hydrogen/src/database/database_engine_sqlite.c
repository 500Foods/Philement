/*
 * SQLite Database Engine Implementation
 *
 * Implements the SQLite database engine for the Hydrogen database subsystem.
 * Uses dynamic loading (dlopen/dlsym) for libsqlite3 to avoid static linking dependencies.
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

// Function pointer types for libsqlite3 functions
typedef int (*sqlite3_open_t)(const char*, void**);
typedef int (*sqlite3_close_t)(void*);
// typedef int (*sqlite3_exec_t)(void*, const char*, int (*)(void*,int,char**,char**), void*, char**);
typedef int (*sqlite3_prepare_v2_t)(void*, const char*, int, void**, const char**);
typedef int (*sqlite3_step_t)(void*);
// typedef int (*sqlite3_finalize_t)(void*);
// typedef int (*sqlite3_column_count_t)(void*);
// typedef const char* (*sqlite3_column_name_t)(void*, int);
// typedef const char* (*sqlite3_column_text_t)(void*, int);
// typedef int (*sqlite3_column_type_t)(void*, int);
// typedef int (*sqlite3_changes_t)(void*);
// typedef int (*sqlite3_reset_t)(void*);
// typedef int (*sqlite3_clear_bindings_t)(void*);
// typedef int (*sqlite3_bind_text_t)(void*, int, const char*, int, void*);
// typedef int (*sqlite3_bind_int_t)(void*, int, int);
// typedef int (*sqlite3_bind_double_t)(void*, int, double);
// typedef int (*sqlite3_bind_null_t)(void*, int);
// typedef const char* (*sqlite3_errmsg_t)(void*);
// typedef int (*sqlite3_extended_result_codes_t)(void*, int);

// SQLite function pointers (loaded dynamically)
static sqlite3_open_t sqlite3_open_ptr = NULL;
static sqlite3_close_t sqlite3_close_ptr = NULL;
// static sqlite3_exec_t sqlite3_exec_ptr = NULL;
// static sqlite3_prepare_v2_t sqlite3_prepare_v2_ptr = NULL;
// static sqlite3_step_t sqlite3_step_ptr = NULL;
// static sqlite3_finalize_t sqlite3_finalize_ptr = NULL;
// static sqlite3_column_count_t sqlite3_column_count_ptr = NULL;
// static sqlite3_column_name_t sqlite3_column_name_ptr = NULL;
// static sqlite3_column_text_t sqlite3_column_text_ptr = NULL;
// static sqlite3_column_type_t sqlite3_column_type_ptr = NULL;
// static sqlite3_changes_t sqlite3_changes_ptr = NULL;
// static sqlite3_reset_t sqlite3_reset_ptr = NULL;
// static sqlite3_clear_bindings_t sqlite3_clear_bindings_ptr = NULL;
// static sqlite3_bind_text_t sqlite3_bind_text_ptr = NULL;
// static sqlite3_bind_int_t sqlite3_bind_int_ptr = NULL;
// static sqlite3_bind_double_t sqlite3_bind_double_ptr = NULL;
// static sqlite3_bind_null_t sqlite3_bind_null_ptr = NULL;
// static sqlite3_errmsg_t sqlite3_errmsg_ptr = NULL;
// static sqlite3_extended_result_codes_t sqlite3_extended_result_codes_ptr = NULL;

// Library handle
static void* libsqlite_handle = NULL;
static pthread_mutex_t libsqlite_mutex = PTHREAD_MUTEX_INITIALIZER;

// Constants (defined since we can't include sqlite3.h)
#define SQLITE_OK 0
#define SQLITE_ROW 100
#define SQLITE_DONE 101

// Prepared statement cache structure
typedef struct PreparedStatementCache {
    char** names;
    size_t count;
    size_t capacity;
    pthread_mutex_t lock;
} PreparedStatementCache;

// SQLite-specific connection structure
typedef struct SQLiteConnection {
    void* db;  // sqlite3* loaded dynamically
    char* db_path;
    PreparedStatementCache* prepared_statements;
} SQLiteConnection;

/*
 * Function Prototypes
 */

// Connection management
bool sqlite_connect(ConnectionConfig* config, DatabaseHandle** connection);
bool sqlite_disconnect(DatabaseHandle* connection);
bool sqlite_health_check(DatabaseHandle* connection);
bool sqlite_reset_connection(DatabaseHandle* connection);

// Query execution
bool sqlite_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result);
bool sqlite_execute_prepared(DatabaseHandle* connection, PreparedStatement* stmt, QueryRequest* request, QueryResult** result);

// Transaction management
bool sqlite_begin_transaction(DatabaseHandle* connection, DatabaseIsolationLevel level, Transaction** transaction);
bool sqlite_commit_transaction(DatabaseHandle* connection, Transaction* transaction);
bool sqlite_rollback_transaction(DatabaseHandle* connection, Transaction* transaction);

// Prepared statement management
bool sqlite_prepare_statement(DatabaseHandle* connection, const char* name, const char* sql, PreparedStatement** stmt);
bool sqlite_unprepare_statement(DatabaseHandle* connection, PreparedStatement* stmt);

// Utility functions
char* sqlite_get_connection_string(ConnectionConfig* config);
bool sqlite_validate_connection_string(const char* connection_string);
char* sqlite_escape_string(DatabaseHandle* connection, const char* input);

// Engine interface
DatabaseEngineInterface* database_engine_sqlite_get_interface(void);

/*
 * Library Loading Functions
 */

static bool load_libsqlite_functions(void) {
    if (libsqlite_handle) {
        return true; // Already loaded
    }

    pthread_mutex_lock(&libsqlite_mutex);

    if (libsqlite_handle) {
        pthread_mutex_unlock(&libsqlite_mutex);
        return true; // Another thread loaded it
    }

    // Try to load libsqlite3
    libsqlite_handle = dlopen("libsqlite3.so.0", RTLD_LAZY);
    if (!libsqlite_handle) {
        libsqlite_handle = dlopen("libsqlite3.so", RTLD_LAZY);
    }
    if (!libsqlite_handle) {
        log_this(SR_DATABASE, "Failed to load libsqlite3 library", LOG_LEVEL_ERROR, true, true, true);
        log_this(SR_DATABASE, dlerror(), LOG_LEVEL_ERROR, true, true, true);
        pthread_mutex_unlock(&libsqlite_mutex);
        return false;
    }

    // Load function pointers
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    sqlite3_open_ptr = (sqlite3_open_t)dlsym(libsqlite_handle, "sqlite3_open");
    sqlite3_close_ptr = (sqlite3_close_t)dlsym(libsqlite_handle, "sqlite3_close");
    // sqlite3_exec_ptr = (sqlite3_exec_t)dlsym(libsqlite_handle, "sqlite3_exec");
    // sqlite3_prepare_v2_ptr = (sqlite3_prepare_v2_t)dlsym(libsqlite_handle, "sqlite3_prepare_v2");
    // sqlite3_step_ptr = (sqlite3_step_t)dlsym(libsqlite_handle, "sqlite3_step");
    // sqlite3_finalize_ptr = (sqlite3_finalize_t)dlsym(libsqlite_handle, "sqlite3_finalize");
    // sqlite3_column_count_ptr = (sqlite3_column_count_t)dlsym(libsqlite_handle, "sqlite3_column_count");
    // sqlite3_column_name_ptr = (sqlite3_column_name_t)dlsym(libsqlite_handle, "sqlite3_column_name");
    // sqlite3_column_text_ptr = (sqlite3_column_text_t)dlsym(libsqlite_handle, "sqlite3_column_text");
    // sqlite3_column_type_ptr = (sqlite3_column_type_t)dlsym(libsqlite_handle, "sqlite3_column_type");
    // sqlite3_changes_ptr = (sqlite3_changes_t)dlsym(libsqlite_handle, "sqlite3_changes");
    // sqlite3_reset_ptr = (sqlite3_reset_t)dlsym(libsqlite_handle, "sqlite3_reset");
    // sqlite3_clear_bindings_ptr = (sqlite3_clear_bindings_t)dlsym(libsqlite_handle, "sqlite3_clear_bindings");
    // sqlite3_bind_text_ptr = (sqlite3_bind_text_t)dlsym(libsqlite_handle, "sqlite3_bind_text");
    // sqlite3_bind_int_ptr = (sqlite3_bind_int_t)dlsym(libsqlite_handle, "sqlite3_bind_int");
    // sqlite3_bind_double_ptr = (sqlite3_bind_double_t)dlsym(libsqlite_handle, "sqlite3_bind_double");
    // sqlite3_bind_null_ptr = (sqlite3_bind_null_t)dlsym(libsqlite_handle, "sqlite3_bind_null");
    // sqlite3_errmsg_ptr = (sqlite3_errmsg_t)dlsym(libsqlite_handle, "sqlite3_errmsg");
    // sqlite3_extended_result_codes_ptr = (sqlite3_extended_result_codes_t)dlsym(libsqlite_handle, "sqlite3_extended_result_codes");
#pragma GCC diagnostic pop

    // Check if all functions were loaded
    if (!sqlite3_open_ptr || !sqlite3_close_ptr) {
        log_this(SR_DATABASE, "Failed to load all required libsqlite3 functions", LOG_LEVEL_ERROR, true, true, true);
        dlclose(libsqlite_handle);
        libsqlite_handle = NULL;
        pthread_mutex_unlock(&libsqlite_mutex);
        return false;
    }

    pthread_mutex_unlock(&libsqlite_mutex);
    log_this(SR_DATABASE, "Successfully loaded libsqlite3 library", LOG_LEVEL_STATE, true, true, true);
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

bool sqlite_connect(ConnectionConfig* config, DatabaseHandle** connection) {
    if (!config || !connection) {
        log_this(SR_DATABASE, "Invalid parameters for SQLite connection", LOG_LEVEL_ERROR, true, true, true);
        return false;
    }

    // Load libsqlite3 library if not already loaded
    if (!load_libsqlite_functions()) {
        log_this(SR_DATABASE, "SQLite library not available", LOG_LEVEL_ERROR, true, true, true);
        return false;
    }

    // Determine database path
    const char* db_path;
    if (config->connection_string) {
        db_path = config->connection_string;
    } else if (config->database) {
        db_path = config->database;
    } else {
        db_path = ":memory:"; // In-memory database
    }

    // Open database
    void* sqlite_db = NULL;
    int rc = sqlite3_open_ptr(db_path, &sqlite_db);
    if (rc != SQLITE_OK) {
        log_this(SR_DATABASE, "SQLite database open failed", LOG_LEVEL_ERROR, true, true, true);
        return false;
    }

    // Create database handle
    DatabaseHandle* db_handle = calloc(1, sizeof(DatabaseHandle));
    if (!db_handle) {
        sqlite3_close_ptr(sqlite_db);
        return false;
    }

    // Create SQLite-specific connection wrapper
    SQLiteConnection* sqlite_wrapper = calloc(1, sizeof(SQLiteConnection));
    if (!sqlite_wrapper) {
        free(db_handle);
        sqlite3_close_ptr(sqlite_db);
        return false;
    }

    sqlite_wrapper->db = sqlite_db;
    sqlite_wrapper->db_path = strdup(db_path);
    sqlite_wrapper->prepared_statements = create_prepared_statement_cache();
    if (!sqlite_wrapper->prepared_statements) {
        free(sqlite_wrapper->db_path);
        free(sqlite_wrapper);
        free(db_handle);
        sqlite3_close_ptr(sqlite_db);
        return false;
    }

    // Initialize database handle
    db_handle->engine_type = DB_ENGINE_SQLITE;
    db_handle->connection_handle = sqlite_wrapper;
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

    log_this(SR_DATABASE, "SQLite connection established successfully", LOG_LEVEL_STATE, true, true, true);
    return true;
}

bool sqlite_disconnect(DatabaseHandle* connection) {
    if (!connection || connection->engine_type != DB_ENGINE_SQLITE) {
        return false;
    }

    SQLiteConnection* sqlite_conn = (SQLiteConnection*)connection->connection_handle;
    if (sqlite_conn) {
        if (sqlite_conn->db) {
            sqlite3_close_ptr(sqlite_conn->db);
        }
        destroy_prepared_statement_cache(sqlite_conn->prepared_statements);
        free(sqlite_conn->db_path);
        free(sqlite_conn);
    }

    connection->status = DB_CONNECTION_DISCONNECTED;
    log_this(SR_DATABASE, "SQLite connection closed", LOG_LEVEL_STATE, true, true, true);
    return true;
}

bool sqlite_health_check(DatabaseHandle* connection) {
    if (!connection || connection->engine_type != DB_ENGINE_SQLITE) {
        return false;
    }

    SQLiteConnection* sqlite_conn = (SQLiteConnection*)connection->connection_handle;
    if (!sqlite_conn || !sqlite_conn->db) {
        return false;
    }

    // SQLite is always healthy if connection exists
    connection->last_health_check = time(NULL);
    connection->consecutive_failures = 0;
    return true;
}

bool sqlite_reset_connection(DatabaseHandle* connection) {
    if (!connection || connection->engine_type != DB_ENGINE_SQLITE) {
        return false;
    }

    // SQLite connections don't need reset - they're always ready
    connection->status = DB_CONNECTION_CONNECTED;
    connection->connected_since = time(NULL);
    connection->consecutive_failures = 0;

    log_this(SR_DATABASE, "SQLite connection reset successfully", LOG_LEVEL_STATE, true, true, true);
    return true;
}

/*
 * Query Execution
 */

bool sqlite_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result) {
    if (!connection || !request || !result || connection->engine_type != DB_ENGINE_SQLITE) {
        return false;
    }

    SQLiteConnection* sqlite_conn = (SQLiteConnection*)connection->connection_handle;
    if (!sqlite_conn || !sqlite_conn->db) {
        return false;
    }

    // TODO: Implement SQLite query execution
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

    // log_this(SR_DATABASE, "SQLite query executed (placeholder implementation)", LOG_LEVEL_DEBUG, true, true, true);
    return true;
}

bool sqlite_execute_prepared(DatabaseHandle* connection, PreparedStatement* stmt, QueryRequest* request, QueryResult** result) {
    if (!connection || !stmt || !request || !result || connection->engine_type != DB_ENGINE_SQLITE) {
        return false;
    }

    // For simplicity, execute as regular query for now
    return sqlite_execute_query(connection, request, result);
}

/*
 * Transaction Management
 */

bool sqlite_begin_transaction(DatabaseHandle* connection, DatabaseIsolationLevel level, Transaction** transaction) {
    if (!connection || !transaction || connection->engine_type != DB_ENGINE_SQLITE) {
        return false;
    }

    SQLiteConnection* sqlite_conn = (SQLiteConnection*)connection->connection_handle;
    if (!sqlite_conn || !sqlite_conn->db) {
        return false;
    }

    // TODO: Implement SQLite transaction begin
    // For now, create a placeholder transaction
    Transaction* tx = calloc(1, sizeof(Transaction));
    if (!tx) {
        return false;
    }

    tx->transaction_id = strdup("sqlite_tx");
    tx->isolation_level = level;
    tx->started_at = time(NULL);
    tx->active = true;

    *transaction = tx;
    connection->current_transaction = tx;

    // log_this(SR_DATABASE, "SQLite transaction started (placeholder)", LOG_LEVEL_DEBUG, true, true, true);
    return true;
}

bool sqlite_commit_transaction(DatabaseHandle* connection, Transaction* transaction) {
    if (!connection || !transaction || connection->engine_type != DB_ENGINE_SQLITE) {
        return false;
    }

    // TODO: Implement SQLite commit
    transaction->active = false;
    connection->current_transaction = NULL;

    // log_this(SR_DATABASE, "SQLite transaction committed (placeholder)", LOG_LEVEL_DEBUG, true, true, true);
    return true;
}

bool sqlite_rollback_transaction(DatabaseHandle* connection, Transaction* transaction) {
    if (!connection || !transaction || connection->engine_type != DB_ENGINE_SQLITE) {
        return false;
    }

    // TODO: Implement SQLite rollback
    transaction->active = false;
    connection->current_transaction = NULL;

    // log_this(SR_DATABASE, "SQLite transaction rolled back (placeholder)", LOG_LEVEL_DEBUG, true, true, true);
    return true;
}

/*
 * Prepared Statement Management
 */

bool sqlite_prepare_statement(DatabaseHandle* connection, const char* name, const char* sql, PreparedStatement** stmt) {
    if (!connection || !name || !sql || !stmt || connection->engine_type != DB_ENGINE_SQLITE) {
        return false;
    }

    // TODO: Implement SQLite prepared statement preparation
    PreparedStatement* prepared_stmt = calloc(1, sizeof(PreparedStatement));
    if (!prepared_stmt) {
        return false;
    }

    prepared_stmt->name = strdup(name);
    prepared_stmt->sql_template = strdup(sql);
    prepared_stmt->created_at = time(NULL);
    prepared_stmt->usage_count = 0;

    *stmt = prepared_stmt;

    // log_this(SR_DATABASE, "SQLite prepared statement created (placeholder)", LOG_LEVEL_DEBUG, true, true, true);
    return true;
}

bool sqlite_unprepare_statement(DatabaseHandle* connection, PreparedStatement* stmt) {
    if (!connection || !stmt || connection->engine_type != DB_ENGINE_SQLITE) {
        return false;
    }

    // TODO: Implement SQLite prepared statement cleanup
    free(stmt->name);
    free(stmt->sql_template);
    free(stmt);

    // log_this(SR_DATABASE, "SQLite prepared statement removed (placeholder)", LOG_LEVEL_DEBUG, true, true, true);
    return true;
}

/*
 * Utility Functions
 */

char* sqlite_get_connection_string(ConnectionConfig* config) {
    if (!config) return NULL;

    char* conn_str = calloc(1, 256);
    if (!conn_str) return NULL;

    if (config->connection_string) {
        strcpy(conn_str, config->connection_string);
    } else if (config->database) {
        strcpy(conn_str, config->database);
    } else {
        strcpy(conn_str, ":memory:");
    }

    return conn_str;
}

bool sqlite_validate_connection_string(const char* connection_string) {
    if (!connection_string) return false;

    // SQLite accepts file paths or :memory:
    return strlen(connection_string) > 0;
}

char* sqlite_escape_string(DatabaseHandle* connection, const char* input) {
    if (!connection || !input || connection->engine_type != DB_ENGINE_SQLITE) {
        return NULL;
    }

    // SQLite string escaping - simple implementation
    size_t escaped_len = strlen(input) * 2 + 1;
    char* escaped = calloc(1, escaped_len);
    if (!escaped) return NULL;

    // Simple escaping - replace single quotes with double quotes
    const char* src = input;
    char* dst = escaped;
    while (*src) {
        if (*src == '\'') {
            *dst++ = '\'';
            *dst++ = '\'';
        } else {
            *dst++ = *src;
        }
        src++;
    }

    return escaped;
}

/*
 * Engine Interface Registration
 */

static DatabaseEngineInterface sqlite_engine_interface = {
    .engine_type = DB_ENGINE_SQLITE,
    .name = (char*)"sqlite",
    .connect = sqlite_connect,
    .disconnect = sqlite_disconnect,
    .health_check = sqlite_health_check,
    .reset_connection = sqlite_reset_connection,
    .execute_query = sqlite_execute_query,
    .execute_prepared = sqlite_execute_prepared,
    .begin_transaction = sqlite_begin_transaction,
    .commit_transaction = sqlite_commit_transaction,
    .rollback_transaction = sqlite_rollback_transaction,
    .prepare_statement = sqlite_prepare_statement,
    .unprepare_statement = sqlite_unprepare_statement,
    .get_connection_string = sqlite_get_connection_string,
    .validate_connection_string = sqlite_validate_connection_string,
    .escape_string = sqlite_escape_string
};

DatabaseEngineInterface* database_engine_sqlite_get_interface(void) {
    return &sqlite_engine_interface;
}