/*
 * SQLite Database Engine - Connection Management Implementation
 *
 * Implements SQLite connection management functions.
 */
#include "../../hydrogen.h"
#include "../database.h"
#include "../queue/database_queue.h"
#include "types.h"
#include "connection.h"

#ifdef USE_MOCK_LIBSQLITE3
#include "../../../tests/unity/mocks/mock_libsqlite3.h"
#endif

// SQLite function pointers (loaded dynamically)
#ifdef USE_MOCK_LIBSQLITE3
// For mocking, assign all mock function pointers
sqlite3_open_t sqlite3_open_ptr = mock_sqlite3_open;
sqlite3_close_t sqlite3_close_ptr = mock_sqlite3_close;
sqlite3_exec_t sqlite3_exec_ptr = mock_sqlite3_exec;
sqlite3_prepare_v2_t sqlite3_prepare_v2_ptr = NULL;
sqlite3_step_t sqlite3_step_ptr = NULL;
sqlite3_finalize_t sqlite3_finalize_ptr = NULL;
sqlite3_column_count_t sqlite3_column_count_ptr = NULL;
sqlite3_column_name_t sqlite3_column_name_ptr = NULL;
sqlite3_column_text_t sqlite3_column_text_ptr = NULL;
sqlite3_column_type_t sqlite3_column_type_ptr = NULL;
sqlite3_changes_t sqlite3_changes_ptr = NULL;
sqlite3_reset_t sqlite3_reset_ptr = NULL;
sqlite3_clear_bindings_t sqlite3_clear_bindings_ptr = NULL;
sqlite3_bind_text_t sqlite3_bind_text_ptr = NULL;
sqlite3_bind_int_t sqlite3_bind_int_ptr = NULL;
sqlite3_bind_double_t sqlite3_bind_double_ptr = NULL;
sqlite3_bind_null_t sqlite3_bind_null_ptr = NULL;
sqlite3_errmsg_t sqlite3_errmsg_ptr = mock_sqlite3_errmsg;
sqlite3_extended_result_codes_t sqlite3_extended_result_codes_ptr = mock_sqlite3_extended_result_codes;
sqlite3_free_t sqlite3_free_ptr = mock_sqlite3_free;
#else
sqlite3_open_t sqlite3_open_ptr = NULL;
sqlite3_close_t sqlite3_close_ptr = NULL;
sqlite3_exec_t sqlite3_exec_ptr = NULL;
sqlite3_prepare_v2_t sqlite3_prepare_v2_ptr = NULL;
sqlite3_step_t sqlite3_step_ptr = NULL;
sqlite3_finalize_t sqlite3_finalize_ptr = NULL;
sqlite3_column_count_t sqlite3_column_count_ptr = NULL;
sqlite3_column_name_t sqlite3_column_name_ptr = NULL;
sqlite3_column_text_t sqlite3_column_text_ptr = NULL;
sqlite3_column_type_t sqlite3_column_type_ptr = NULL;
sqlite3_changes_t sqlite3_changes_ptr = NULL;
sqlite3_reset_t sqlite3_reset_ptr = NULL;
sqlite3_clear_bindings_t sqlite3_clear_bindings_ptr = NULL;
sqlite3_bind_text_t sqlite3_bind_text_ptr = NULL;
sqlite3_bind_int_t sqlite3_bind_int_ptr = NULL;
sqlite3_bind_double_t sqlite3_bind_double_ptr = NULL;
sqlite3_bind_null_t sqlite3_bind_null_ptr = NULL;
sqlite3_errmsg_t sqlite3_errmsg_ptr = NULL;
sqlite3_extended_result_codes_t sqlite3_extended_result_codes_ptr = NULL;
sqlite3_free_t sqlite3_free_ptr = NULL;
#endif

// Library handle
#ifndef USE_MOCK_LIBSQLITE3
static void* libsqlite_handle = NULL;
static pthread_mutex_t libsqlite_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

// Library Loading Functions
bool load_libsqlite_functions(const char* designator __attribute__((unused))) {
#ifdef USE_MOCK_LIBSQLITE3
    // For mocking, functions are already set
    return true;
#else
    if (libsqlite_handle) {
        return true; // Already loaded
    }

    const char* log_subsystem = designator ? designator : SR_DATABASE;
    MUTEX_LOCK(&libsqlite_mutex, log_subsystem);

    if (libsqlite_handle) {
        MUTEX_UNLOCK(&libsqlite_mutex, log_subsystem);
        return true; // Another thread loaded it
    }

    // Try to load libsqlite3
    libsqlite_handle = dlopen("libsqlite3.so.0", RTLD_LAZY);
    if (!libsqlite_handle) {
        libsqlite_handle = dlopen("libsqlite3.so", RTLD_LAZY);
    }
    if (!libsqlite_handle) {
        log_this(log_subsystem, "Failed to load libsqlite3 library", LOG_LEVEL_ERROR, 0);
        log_this(log_subsystem, dlerror(), LOG_LEVEL_ERROR, 0);
        MUTEX_UNLOCK(&libsqlite_mutex, log_subsystem);
        return false;
    }

    // Load function pointers
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    sqlite3_open_ptr = (sqlite3_open_t)dlsym(libsqlite_handle, "sqlite3_open");
    sqlite3_close_ptr = (sqlite3_close_t)dlsym(libsqlite_handle, "sqlite3_close");
    sqlite3_exec_ptr = (sqlite3_exec_t)dlsym(libsqlite_handle, "sqlite3_exec");
    sqlite3_prepare_v2_ptr = (sqlite3_prepare_v2_t)dlsym(libsqlite_handle, "sqlite3_prepare_v2");
    sqlite3_step_ptr = (sqlite3_step_t)dlsym(libsqlite_handle, "sqlite3_step");
    sqlite3_finalize_ptr = (sqlite3_finalize_t)dlsym(libsqlite_handle, "sqlite3_finalize");
    sqlite3_column_count_ptr = (sqlite3_column_count_t)dlsym(libsqlite_handle, "sqlite3_column_count");
    sqlite3_column_name_ptr = (sqlite3_column_name_t)dlsym(libsqlite_handle, "sqlite3_column_name");
    sqlite3_column_text_ptr = (sqlite3_column_text_t)dlsym(libsqlite_handle, "sqlite3_column_text");
    sqlite3_column_type_ptr = (sqlite3_column_type_t)dlsym(libsqlite_handle, "sqlite3_column_type");
    sqlite3_changes_ptr = (sqlite3_changes_t)dlsym(libsqlite_handle, "sqlite3_changes");
    sqlite3_reset_ptr = (sqlite3_reset_t)dlsym(libsqlite_handle, "sqlite3_reset");
    sqlite3_clear_bindings_ptr = (sqlite3_clear_bindings_t)dlsym(libsqlite_handle, "sqlite3_clear_bindings");
    sqlite3_bind_text_ptr = (sqlite3_bind_text_t)dlsym(libsqlite_handle, "sqlite3_bind_text");
    sqlite3_bind_int_ptr = (sqlite3_bind_int_t)dlsym(libsqlite_handle, "sqlite3_bind_int");
    sqlite3_bind_double_ptr = (sqlite3_bind_double_t)dlsym(libsqlite_handle, "sqlite3_bind_double");
    sqlite3_bind_null_ptr = (sqlite3_bind_null_t)dlsym(libsqlite_handle, "sqlite3_bind_null");
    sqlite3_errmsg_ptr = (sqlite3_errmsg_t)dlsym(libsqlite_handle, "sqlite3_errmsg");
    sqlite3_extended_result_codes_ptr = (sqlite3_extended_result_codes_t)dlsym(libsqlite_handle, "sqlite3_extended_result_codes");
    sqlite3_free_ptr = (sqlite3_free_t)dlsym(libsqlite_handle, "sqlite3_free");
#pragma GCC diagnostic pop

    // Check if all required functions were loaded
    if (!sqlite3_open_ptr || !sqlite3_close_ptr || !sqlite3_exec_ptr ||
        !sqlite3_prepare_v2_ptr || !sqlite3_step_ptr || !sqlite3_finalize_ptr ||
        !sqlite3_column_count_ptr || !sqlite3_column_name_ptr || !sqlite3_column_text_ptr ||
        !sqlite3_errmsg_ptr) {
        log_this(log_subsystem, "Failed to load all required libsqlite3 functions", LOG_LEVEL_ERROR, 0);
        dlclose(libsqlite_handle);
        libsqlite_handle = NULL;
        MUTEX_UNLOCK(&libsqlite_mutex, log_subsystem);
        return false;
    }

    // Optional functions - log if not available
    if (!sqlite3_extended_result_codes_ptr) {
        log_this(log_subsystem, "sqlite3_extended_result_codes function not available - extended error codes disabled", LOG_LEVEL_TRACE, 0);
    }
    if (!sqlite3_changes_ptr) {
        log_this(log_subsystem, "sqlite3_changes function not available - affected rows may not be accurate", LOG_LEVEL_TRACE, 0);
    }

    MUTEX_UNLOCK(&libsqlite_mutex, log_subsystem);
    log_this(log_subsystem, "Successfully loaded libsqlite3 library", LOG_LEVEL_TRACE, 0);
    return true;
#endif
}

// Utility Functions for Prepared Statement Cache
PreparedStatementCache* sqlite_create_prepared_statement_cache(void) {
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

void sqlite_destroy_prepared_statement_cache(PreparedStatementCache* cache) {
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

// Connection Management Functions
bool sqlite_connect(ConnectionConfig* config, DatabaseHandle** connection, const char* designator) {
    if (!config || !connection) {
        log_this(SR_DATABASE, "Invalid parameters for SQLite connection", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Load libsqlite3 library if not already loaded
    if (!load_libsqlite_functions(designator)) {
        const char* log_subsystem = designator ? designator : SR_DATABASE;
        log_this(log_subsystem, "SQLite library not available", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Determine database file path
    const char* db_path = NULL;
    if (config->database && strlen(config->database) > 0) {
        db_path = config->database;
        log_this(designator, "SQLite connection: Using database field: %s", LOG_LEVEL_TRACE, 1, db_path);
    } else if (config->connection_string && strncmp(config->connection_string, "sqlite://", 9) == 0) {
        db_path = config->connection_string + 9; // Skip "sqlite://"
        log_this(designator, "SQLite connection: Using connection string: %s", LOG_LEVEL_TRACE, 1, db_path);
    } else {
        db_path = ":memory:"; // Default to in-memory database
        log_this(designator, "SQLite connection: WARNING - Using in-memory database! config->database='%s', connection_string='%s'",
                 LOG_LEVEL_ERROR, 2, config->database ? config->database : "NULL", config->connection_string ? config->connection_string : "NULL");
    }

    // Log current working directory and resolved path for debugging
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        log_this(designator, "SQLite connection: Current working directory: %s", LOG_LEVEL_TRACE, 1, cwd);
    }
    log_this(designator, "SQLite connection: Attempting to open database: %s", LOG_LEVEL_TRACE, 1, db_path);

    // Open database
    void* sqlite_db = NULL;
    int result = sqlite3_open_ptr(db_path, &sqlite_db);
    if (result != SQLITE_OK) {
        log_this(SR_DATABASE, "SQLite database open failed", LOG_LEVEL_ERROR, 0);
        if (sqlite3_errmsg_ptr && sqlite_db) {
            const char* error_msg = sqlite3_errmsg_ptr(sqlite_db);
            if (error_msg) {
                log_this(SR_DATABASE, error_msg, LOG_LEVEL_ERROR, 0);
            }
        }
        if (sqlite_db) {
            sqlite3_close_ptr(sqlite_db);
        }
        return false;
    }

    // Enable extended result codes if available
    if (sqlite3_extended_result_codes_ptr) {
        sqlite3_extended_result_codes_ptr(sqlite_db, 1);
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
    sqlite_wrapper->prepared_statements = sqlite_create_prepared_statement_cache();
    if (!sqlite_wrapper->prepared_statements) {
        free(sqlite_wrapper->db_path);
        free(sqlite_wrapper);
        free(db_handle);
        sqlite3_close_ptr(sqlite_db);
        return false;
    }

    // Store designator for future use (disconnect, etc.)
    db_handle->designator = designator ? strdup(designator) : NULL;

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

    // Use designator for logging if provided, otherwise use generic Database subsystem
    const char* log_subsystem = designator ? designator : SR_DATABASE;
    log_this(log_subsystem, "SQLite connection established successfully", LOG_LEVEL_TRACE, 0);
    return true;
}

bool sqlite_disconnect(DatabaseHandle* connection) {
    if (!connection || connection->engine_type != DB_ENGINE_SQLITE) {
        return false;
    }

    SQLiteConnection* sqlite_conn = (SQLiteConnection*)connection->connection_handle;
    if (sqlite_conn) {
        if (sqlite_conn->db && sqlite3_close_ptr) {
            sqlite3_close_ptr(sqlite_conn->db);
        }
        sqlite_destroy_prepared_statement_cache(sqlite_conn->prepared_statements);
        free(sqlite_conn->db_path);
        free(sqlite_conn);
    }

    connection->status = DB_CONNECTION_DISCONNECTED;

    // Use stored designator for logging if available
    const char* log_subsystem = connection->designator ? connection->designator : SR_DATABASE;
    log_this(log_subsystem, "SQLite connection closed", LOG_LEVEL_TRACE, 0);
    return true;
}

bool sqlite_health_check(DatabaseHandle* connection) {
    if (!connection) {
        const char* designator = SR_DATABASE;
        log_this(designator, "SQLite health check: connection is NULL", LOG_LEVEL_ERROR, 0);
        return false;
    }

    const char* designator = connection->designator ? connection->designator : SR_DATABASE;
    log_this(designator, "SQLite health check: Starting validation", LOG_LEVEL_TRACE, 0);

    if (connection->engine_type != DB_ENGINE_SQLITE) {
        log_this(designator, "SQLite health check: wrong engine type %d", LOG_LEVEL_ERROR, 1, connection->engine_type);
        return false;
    }

    SQLiteConnection* sqlite_conn = (SQLiteConnection*)connection->connection_handle;
    if (!sqlite_conn) {
        log_this(designator, "SQLite health check: sqlite_conn is NULL", LOG_LEVEL_ERROR, 0);
        return false;
    }

    if (!sqlite_conn->db) {
        log_this(designator, "SQLite health check: sqlite_conn->db is NULL", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Function pointer validation
    if (!sqlite3_exec_ptr) {
        log_this(designator, "SQLite health check: sqlite3_exec_ptr is NULL", LOG_LEVEL_ERROR, 0);
        return false;
    }

    log_this(designator, "SQLite health check: All validations passed, executing health check", LOG_LEVEL_TRACE, 0);

    // Execute a simple query to check connectivity
    char* error_msg = NULL;
    int result = sqlite3_exec_ptr(sqlite_conn->db, "SELECT 1;", NULL, NULL, &error_msg);

    if (result != SQLITE_OK) {
        log_this(designator, "SQLite health check failed - result: %d", LOG_LEVEL_ERROR, 1, result);
        if (error_msg) {
            log_this(designator, "SQLite health check error: %s", LOG_LEVEL_ERROR, 1, error_msg);
            if (sqlite3_free_ptr) {
                sqlite3_free_ptr(error_msg);
            } else {
                free(error_msg);
            }
        }
        connection->consecutive_failures++;
        return false;
    }

    if (error_msg) {
        // Note: sqlite3_free is a macro that calls free() in most implementations
        free(error_msg);
    }

    log_this(designator, "SQLite health check passed", LOG_LEVEL_TRACE, 0);
    connection->last_health_check = time(NULL);
    connection->consecutive_failures = 0;
    return true;
}

bool sqlite_reset_connection(DatabaseHandle* connection) {
    if (!connection || connection->engine_type != DB_ENGINE_SQLITE) {
        return false;
    }

    // SQLite connections are typically long-lived and don't need reset
    // Just update the status
    connection->status = DB_CONNECTION_CONNECTED;
    connection->connected_since = time(NULL);
    connection->consecutive_failures = 0;

    log_this(SR_DATABASE, "SQLite connection reset successfully", LOG_LEVEL_TRACE, 0);
    return true;
}