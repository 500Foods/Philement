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

// Library handle
static void* libpq_handle = NULL;
static pthread_mutex_t libpq_mutex = PTHREAD_MUTEX_INITIALIZER;

// Constants (defined since we can't include libpq-fe.h)
#define CONNECTION_OK 0
#define PGRES_TUPLES_OK 2
#define PGRES_COMMAND_OK 1

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
bool postgresql_connect(ConnectionConfig* config, DatabaseHandle** connection);
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
        log_this(SR_DATABASE, "Failed to load libpq library", LOG_LEVEL_ERROR, true, true, true);
        log_this(SR_DATABASE, dlerror(), LOG_LEVEL_ERROR, true, true, true);
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
#pragma GCC diagnostic pop

    // Check if all functions were loaded
    if (!PQconnectdb_ptr || !PQstatus_ptr || !PQerrorMessage_ptr || !PQfinish_ptr ||
        !PQexec_ptr || !PQresultStatus_ptr || !PQclear_ptr || !PQntuples_ptr ||
        !PQnfields_ptr || !PQfname_ptr || !PQgetvalue_ptr || !PQcmdTuples_ptr) {
        log_this(SR_DATABASE, "Failed to load all required libpq functions", LOG_LEVEL_ERROR, true, true, true);
        dlclose(libpq_handle);
        libpq_handle = NULL;
        pthread_mutex_unlock(&libpq_mutex);
        return false;
    }

    pthread_mutex_unlock(&libpq_mutex);
    log_this(SR_DATABASE, "Successfully loaded libpq library", LOG_LEVEL_STATE, true, true, true);
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

bool postgresql_connect(ConnectionConfig* config, DatabaseHandle** connection) {
    if (!config || !connection) {
        log_this(SR_DATABASE, "Invalid parameters for PostgreSQL connection", LOG_LEVEL_ERROR, true, true, true);
        return false;
    }

    // Load libpq library if not already loaded
    if (!load_libpq_functions()) {
        log_this(SR_DATABASE, "PostgreSQL library not available", LOG_LEVEL_ERROR, true, true, true);
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
        log_this(SR_DATABASE, "PostgreSQL connection failed", LOG_LEVEL_ERROR, true, true, true);
        log_this(SR_DATABASE, PQerrorMessage_ptr(pg_conn), LOG_LEVEL_ERROR, true, true, true);
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

    log_this(SR_DATABASE, "PostgreSQL connection established successfully", LOG_LEVEL_STATE, true, true, true);
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
    log_this(SR_DATABASE, "PostgreSQL connection closed", LOG_LEVEL_STATE, true, true, true);
    return true;
}

bool postgresql_health_check(DatabaseHandle* connection) {
    if (!connection || connection->engine_type != DB_ENGINE_POSTGRESQL) {
        return false;
    }

    PostgresConnection* pg_conn = (PostgresConnection*)connection->connection_handle;
    if (!pg_conn || !pg_conn->connection) {
        return false;
    }

    // Execute a simple query to check connectivity
    void* res = PQexec_ptr(pg_conn->connection, "SELECT 1");
    if (PQresultStatus_ptr(res) != PGRES_TUPLES_OK) {
        PQclear_ptr(res);
        connection->consecutive_failures++;
        return false;
    }

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
        log_this(SR_DATABASE, "PostgreSQL connection reset failed", LOG_LEVEL_ERROR, true, true, true);
        return false;
    }

    connection->status = DB_CONNECTION_CONNECTED;
    connection->connected_since = time(NULL);
    connection->consecutive_failures = 0;

    log_this(SR_DATABASE, "PostgreSQL connection reset successfully", LOG_LEVEL_STATE, true, true, true);
    return true;
}

/*
 * Query Execution
 */

bool postgresql_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result) {
    if (!connection || !request || !result || connection->engine_type != DB_ENGINE_POSTGRESQL) {
        return false;
    }

    PostgresConnection* pg_conn = (PostgresConnection*)connection->connection_handle;
    if (!pg_conn || !pg_conn->connection) {
        return false;
    }

    // Execute the query
    void* pg_result = PQexec_ptr(pg_conn->connection, request->sql_template);
    if (PQresultStatus_ptr(pg_result) != PGRES_TUPLES_OK && PQresultStatus_ptr(pg_result) != PGRES_COMMAND_OK) {
        log_this(SR_DATABASE, "PostgreSQL query execution failed", LOG_LEVEL_ERROR, true, true, true);
        log_this(SR_DATABASE, PQerrorMessage_ptr(pg_conn->connection), LOG_LEVEL_ERROR, true, true, true);
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
            }
            strcat(db_result->data_json, "}");
        }
        strcat(db_result->data_json, "]");
    }

    PQclear_ptr(pg_result);
    *result = db_result;

    log_this(SR_DATABASE, "PostgreSQL query executed successfully", LOG_LEVEL_DEBUG, true, true, true);
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

    // Begin transaction
    char query[256];
    snprintf(query, sizeof(query), "BEGIN ISOLATION LEVEL %s", isolation_str);

    void* res = PQexec_ptr(pg_conn->connection, query);
    if (PQresultStatus_ptr(res) != PGRES_COMMAND_OK) {
        log_this(SR_DATABASE, "PostgreSQL BEGIN TRANSACTION failed", LOG_LEVEL_ERROR, true, true, true);
        PQclear_ptr(res);
        return false;
    }
    PQclear_ptr(res);

    pg_conn->in_transaction = true;

    // Create transaction structure
    Transaction* tx = calloc(1, sizeof(Transaction));
    if (!tx) {
        // Rollback on failure
        void* rollback_res = PQexec_ptr(pg_conn->connection, "ROLLBACK");
        PQclear_ptr(rollback_res);
        pg_conn->in_transaction = false;
        return false;
    }

    tx->transaction_id = strdup("postgresql_tx"); // TODO: Generate unique ID
    tx->isolation_level = level;
    tx->started_at = time(NULL);
    tx->active = true;

    *transaction = tx;
    connection->current_transaction = tx;

    log_this(SR_DATABASE, "PostgreSQL transaction started", LOG_LEVEL_DEBUG, true, true, true);
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

    void* res = PQexec_ptr(pg_conn->connection, "COMMIT");
    if (PQresultStatus_ptr(res) != PGRES_COMMAND_OK) {
        log_this(SR_DATABASE, "PostgreSQL COMMIT failed", LOG_LEVEL_ERROR, true, true, true);
        PQclear_ptr(res);
        return false;
    }
    PQclear_ptr(res);

    pg_conn->in_transaction = false;
    transaction->active = false;
    connection->current_transaction = NULL;

    log_this(SR_DATABASE, "PostgreSQL transaction committed", LOG_LEVEL_DEBUG, true, true, true);
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

    void* res = PQexec_ptr(pg_conn->connection, "ROLLBACK");
    if (PQresultStatus_ptr(res) != PGRES_COMMAND_OK) {
        log_this(SR_DATABASE, "PostgreSQL ROLLBACK failed", LOG_LEVEL_ERROR, true, true, true);
        PQclear_ptr(res);
        return false;
    }
    PQclear_ptr(res);

    pg_conn->in_transaction = false;
    transaction->active = false;
    connection->current_transaction = NULL;

    log_this(SR_DATABASE, "PostgreSQL transaction rolled back", LOG_LEVEL_DEBUG, true, true, true);
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

    // Prepare the statement
    void* res = PQprepare_ptr(pg_conn->connection, name, sql, 0, NULL);
    if (PQresultStatus_ptr(res) != PGRES_COMMAND_OK) {
        log_this(SR_DATABASE, "PostgreSQL PREPARE failed", LOG_LEVEL_ERROR, true, true, true);
        log_this(SR_DATABASE, PQerrorMessage_ptr(pg_conn->connection), LOG_LEVEL_ERROR, true, true, true);
        PQclear_ptr(res);
        return false;
    }
    PQclear_ptr(res);

    // Add to cache
    if (!add_prepared_statement(pg_conn->prepared_statements, name)) {
        // Deallocate the prepared statement
        char dealloc_query[256];
        snprintf(dealloc_query, sizeof(dealloc_query), "DEALLOCATE %s", name);
        void* dealloc_res = PQexec_ptr(pg_conn->connection, dealloc_query);
        PQclear_ptr(dealloc_res);
        return false;
    }

    // Create prepared statement structure
    PreparedStatement* prepared_stmt = calloc(1, sizeof(PreparedStatement));
    if (!prepared_stmt) {
        // Cleanup
        remove_prepared_statement(pg_conn->prepared_statements, name);
        char dealloc_query[256];
        snprintf(dealloc_query, sizeof(dealloc_query), "DEALLOCATE %s", name);
        void* dealloc_res = PQexec_ptr(pg_conn->connection, dealloc_query);
        PQclear_ptr(dealloc_res);
        return false;
    }

    prepared_stmt->name = strdup(name);
    prepared_stmt->sql_template = strdup(sql);
    prepared_stmt->created_at = time(NULL);
    prepared_stmt->usage_count = 0;

    *stmt = prepared_stmt;

    log_this(SR_DATABASE, "PostgreSQL prepared statement created", LOG_LEVEL_DEBUG, true, true, true);
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

    // Deallocate from database
    char dealloc_query[256];
    snprintf(dealloc_query, sizeof(dealloc_query), "DEALLOCATE %s", stmt->name);

    void* res = PQexec_ptr(pg_conn->connection, dealloc_query);
    if (PQresultStatus_ptr(res) != PGRES_COMMAND_OK) {
        log_this(SR_DATABASE, "PostgreSQL DEALLOCATE failed", LOG_LEVEL_ERROR, true, true, true);
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

    log_this(SR_DATABASE, "PostgreSQL prepared statement removed", LOG_LEVEL_DEBUG, true, true, true);
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
    return &postgresql_engine_interface;
}