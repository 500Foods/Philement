/*
 * Database Engine Abstraction Layer
 *
 * Implements the multi-engine interface layer for database operations.
 * Provides unified interface for PostgreSQL, SQLite, MySQL, DB2, and future engines.
 */

#include <src/hydrogen.h>
#include "database.h"
#include "dbqueue/dbqueue.h"

// Forward declarations for database engines
DatabaseEngineInterface* postgresql_get_interface(void);
DatabaseEngineInterface* sqlite_get_interface(void);
DatabaseEngineInterface* mysql_get_interface(void);
DatabaseEngineInterface* db2_get_interface(void);

// Global engine registry
static DatabaseEngineInterface* engine_registry[DB_ENGINE_MAX] = {NULL};
static pthread_mutex_t engine_registry_lock = PTHREAD_MUTEX_INITIALIZER;
static bool engine_system_initialized = false;

/*
 * Engine Registry Management
 */

bool database_engine_init(void) {
    // log_this(SR_DATABASE, "Starting database engine registry initialization", LOG_LEVEL_TRACE, 0);

    if (engine_system_initialized) {
        return true;
    }

    MutexResult result = MUTEX_LOCK(&engine_registry_lock, SR_DATABASE);
    if (result != MUTEX_SUCCESS) {
        return false;
    }

    // Initialize registry to NULL
    memset(engine_registry, 0, sizeof(engine_registry));

    // Count databases by engine type to determine which engines to register
    int postgres_count = 0, mysql_count = 0, sqlite_count = 0, db2_count = 0;

    if (app_config && app_config->databases.connection_count > 0) {
        for (int i = 0; i < app_config->databases.connection_count; i++) {
            const DatabaseConnection* conn = &app_config->databases.connections[i];
            if (conn->enabled && conn->type) {
                const char* engine_type = conn->type;
                if (strcmp(engine_type, "postgresql") == 0 || strcmp(engine_type, "postgres") == 0) {
                    postgres_count++;
                } else if (strcmp(engine_type, "mysql") == 0) {
                    mysql_count++;
                } else if (strcmp(engine_type, "sqlite") == 0) {
                    sqlite_count++;
                } else if (strcmp(engine_type, "db2") == 0) {
                    db2_count++;
                }
            }
        }
    }

    // Register only engines that are being used
    if (postgres_count > 0) {
        DatabaseEngineInterface* postgresql_engine = postgresql_get_interface();
        if (postgresql_engine) {
            log_this(SR_DATABASE, "- Registering PostgreSQL engine: %s at index %d", LOG_LEVEL_TRACE, 2,
                postgresql_engine->name ? postgresql_engine->name : "NULL",
                DB_ENGINE_POSTGRESQL);
            engine_registry[DB_ENGINE_POSTGRESQL] = postgresql_engine;
        } else {
            log_this(SR_DATABASE, "CRITICAL ERROR: Failed to get PostgreSQL engine interface!", LOG_LEVEL_ERROR, 0);
        }
    } else {
        log_this(SR_DATABASE, "- Skipping PostgreSQL engine", LOG_LEVEL_TRACE, 0);
    }

    if (sqlite_count > 0) {
        DatabaseEngineInterface* sqlite_engine = sqlite_get_interface();
        if (sqlite_engine) {
            log_this(SR_DATABASE, "- Registering SQLite engine: %s at index %d", LOG_LEVEL_TRACE, 2,
                sqlite_engine->name ? sqlite_engine->name : "NULL",
                DB_ENGINE_SQLITE);
            engine_registry[DB_ENGINE_SQLITE] = sqlite_engine;
        }
    } else {
        log_this(SR_DATABASE, "- Skipping SQLite engine", LOG_LEVEL_TRACE, 0);
    }

    if (mysql_count > 0) {
        DatabaseEngineInterface* mysql_engine = mysql_get_interface();
        if (mysql_engine) {
            log_this(SR_DATABASE, "- Registering MySQL engine: %s at index %d", LOG_LEVEL_TRACE, 2,
                mysql_engine->name ? mysql_engine->name : "NULL",
                DB_ENGINE_MYSQL);
            engine_registry[DB_ENGINE_MYSQL] = mysql_engine;
        }
    } else {
        log_this(SR_DATABASE, "- Skipping MySQL engine", LOG_LEVEL_TRACE, 0);
    }

    if (db2_count > 0) {
        DatabaseEngineInterface* db2_engine = db2_get_interface();
        if (db2_engine) {
            log_this(SR_DATABASE, "- Registering DB2 engine: %s at index %d", LOG_LEVEL_TRACE, 2,
                db2_engine->name ? db2_engine->name : "NULL",
                DB_ENGINE_DB2);
            engine_registry[DB_ENGINE_DB2] = db2_engine;
        }
    } else {
        log_this(SR_DATABASE, "- Skipping DB2 engine", LOG_LEVEL_TRACE, 0);
    }

    engine_system_initialized = true;
    MUTEX_UNLOCK(&engine_registry_lock, SR_DATABASE);

    // log_this(SR_DATABASE, "Database engine registry initialization completed", LOG_LEVEL_TRACE, 0);
    return true;
}

bool database_engine_register(DatabaseEngineInterface* engine) {
    if (!engine || !engine_system_initialized) {
        return false;
    }

    if (engine->engine_type >= DB_ENGINE_MAX) {
        log_this(SR_DATABASE, "Invalid engine type for registration", LOG_LEVEL_ERROR, 0);
        return false;
    }

    MutexResult result = MUTEX_LOCK(&engine_registry_lock, SR_DATABASE);
    if (result != MUTEX_SUCCESS) {
        return false;
    }

    if (engine_registry[engine->engine_type] != NULL) {
        log_this(SR_DATABASE, "Engine already registered for this type", LOG_LEVEL_ERROR, 0);
        mutex_unlock(&engine_registry_lock);
        return false;
    }

    engine_registry[engine->engine_type] = engine;

    MUTEX_UNLOCK(&engine_registry_lock, SR_DATABASE);

    // log_this(SR_DATABASE, "Database engine registered successfully", LOG_LEVEL_TRACE, 0);
    return true;
}

DatabaseEngineInterface* database_engine_get(DatabaseEngine engine_type) {
    return database_engine_get_with_designator(engine_type, SR_DATABASE);
}

DatabaseEngineInterface* database_engine_get_with_designator(DatabaseEngine engine_type, const char* designator) {
    // log_this(SR_DATABASE, "database_engine_get called with engine_type=%d", LOG_LEVEL_TRACE, 1, (int)engine_type);

    if (engine_type >= DB_ENGINE_MAX || !engine_system_initialized) {
        log_this(designator, "database_engine_get: Invalid engine_type or system not initialized", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // log_this(designator, "database_engine_get: About to lock engine_registry_lock", LOG_LEVEL_TRACE, 0);
    MutexResult result = MUTEX_LOCK(&engine_registry_lock, designator);
    if (result != MUTEX_SUCCESS) {
        log_this(designator, "database_engine_get: Failed to lock engine_registry_lock, result=%s", LOG_LEVEL_ERROR, 1, mutex_result_to_string(result));
        return NULL;
    }

    // log_this(designator, "database_engine_get: Successfully locked, getting engine from registry", LOG_LEVEL_TRACE, 0);
    DatabaseEngineInterface* engine = engine_registry[engine_type];
    // log_this(designator, "database_engine_get: Engine retrieved: %p", LOG_LEVEL_TRACE, 1, (void*)engine);

    // log_this(designator, "database_engine_get: About to unlock engine_registry_lock", LOG_LEVEL_TRACE, 0);
    MUTEX_UNLOCK(&engine_registry_lock, designator);
    // log_this(SR_DATABASE, "database_engine_get: Successfully unlocked, returning", LOG_LEVEL_TRACE, 0);

    return engine;
}

DatabaseEngineInterface* database_engine_get_by_name(const char* name) {
    if (!name || !engine_system_initialized) {
        return NULL;
    }

    MutexResult result = MUTEX_LOCK(&engine_registry_lock, SR_DATABASE);
    if (result != MUTEX_SUCCESS) {
        return NULL;
    }

    for (int i = 0; i < DB_ENGINE_MAX; i++) {
        if (engine_registry[i] && strcmp(engine_registry[i]->name, name) == 0) {
            DatabaseEngineInterface* engine = engine_registry[i];
            MUTEX_UNLOCK(&engine_registry_lock, SR_DATABASE);
            return engine;
        }
    }

    MUTEX_UNLOCK(&engine_registry_lock, SR_DATABASE);
    return NULL;
}

/*
 * Connection Management
 */

bool database_engine_connect(DatabaseEngine engine_type, ConnectionConfig* config, DatabaseHandle** connection) {
    return database_engine_connect_with_designator(engine_type, config, connection, NULL);
}

bool database_engine_connect_with_designator(DatabaseEngine engine_type, ConnectionConfig* config, DatabaseHandle** connection, const char* designator) {
    DatabaseEngineInterface* engine = database_engine_get_with_designator(engine_type, designator ? designator : SR_DATABASE);
    if (!engine || !engine->connect || !config || !connection) {
        return false;
    }

    return engine->connect(config, connection, designator);
}

bool database_engine_health_check(DatabaseHandle* connection) {
    const char* designator = connection && connection->designator ? connection->designator : SR_DATABASE;
    log_this(designator, "database_engine_health_check: Function called with connection=%p", LOG_LEVEL_TRACE, 1, (void*)connection);

    if (!connection) {
        log_this(designator, "database_engine_health_check: connection is NULL", LOG_LEVEL_ERROR, 0);
        return false;
    }

    log_this(designator, "database_engine_health_check: connection->engine_type = %d", LOG_LEVEL_TRACE, 1, connection->engine_type);
    log_this(designator, "database_engine_health_check: DB_ENGINE_MAX = %d", LOG_LEVEL_TRACE, 1, DB_ENGINE_MAX);

    if (connection->engine_type >= DB_ENGINE_MAX) {
        log_this(designator, "database_engine_health_check: Invalid engine_type %d (must be < %d)", LOG_LEVEL_ERROR, 2, connection->engine_type, DB_ENGINE_MAX);
        return false;
    }

    log_this(designator, "database_engine_health_check: Engine type validation passed", LOG_LEVEL_TRACE, 0);

    DatabaseEngineInterface* engine = database_engine_get_with_designator(connection->engine_type, designator);
    log_this(designator, "database_engine_health_check: database_engine_get returned %p", LOG_LEVEL_TRACE, 1, (void*)engine);

    if (!engine) {
        log_this(designator, "database_engine_health_check: No engine found for type %d", LOG_LEVEL_ERROR, 1, connection->engine_type);
        return false;
    }

    log_this(designator, "database_engine_health_check: Engine found, checking health_check function", LOG_LEVEL_TRACE, 0);
    if (!engine->health_check) {
        log_this(designator, "database_engine_health_check: Engine has no health_check function", LOG_LEVEL_ERROR, 0);
        return false;
    }

    log_this(designator, "database_engine_health_check: Calling engine health_check function", LOG_LEVEL_TRACE, 0);
    return engine->health_check(connection);
}

/*
 * Prepared Statement Management Helpers
 */

// Find a prepared statement by name in the connection
PreparedStatement* find_prepared_statement(DatabaseHandle* connection, const char* name) {
    if (!connection || !name || !connection->prepared_statements) {
        return NULL;
    }

    for (size_t i = 0; i < connection->prepared_statement_count; i++) {
        if (connection->prepared_statements[i] &&
            strcmp(connection->prepared_statements[i]->name, name) == 0) {
            return connection->prepared_statements[i];
        }
    }

    return NULL;
}

// Store a prepared statement in the connection's array
bool store_prepared_statement(DatabaseHandle* connection, PreparedStatement* stmt) {
    if (!connection || !stmt) {
        return false;
    }

    // Get cache size from connection config (default to 1000 if not set)
    size_t cache_size = 1000; // Default value
    if (connection->config && connection->config->prepared_statement_cache_size > 0) {
        cache_size = (size_t)connection->config->prepared_statement_cache_size;
    }

    // Initialize array if needed
    if (!connection->prepared_statements) {
        connection->prepared_statements = calloc(cache_size, sizeof(PreparedStatement*));
        if (!connection->prepared_statements) {
            return false;
        }
        connection->prepared_statement_count = 0;
        // Initialize LRU counter
        if (!connection->prepared_statement_lru_counter) {
            connection->prepared_statement_lru_counter = calloc(cache_size, sizeof(uint64_t));
        }
    }

    // Check if we need to expand the array
    if (connection->prepared_statement_count >= cache_size) {
        // For now, just don't store if we exceed capacity
        // TODO: Implement dynamic resizing
        log_this(connection->designator ? connection->designator : SR_DATABASE,
                 "Prepared statement cache full, not storing: %s", LOG_LEVEL_ALERT, 1, stmt->name);
        return false;
    }

    // Store the statement
    connection->prepared_statements[connection->prepared_statement_count] = stmt;
    connection->prepared_statement_count++;

    log_this(connection->designator ? connection->designator : SR_DATABASE,
             "Stored prepared statement: %s (total: %zu)", LOG_LEVEL_TRACE, 2,
             stmt->name, connection->prepared_statement_count);

    return true;
}

/*
 * Query Execution
 */

bool database_engine_execute(DatabaseHandle* connection, QueryRequest* request, QueryResult** result) {

    // CRITICAL: Use same debug function as caller - FIRST LINE OF FUNCTION
    // debug_dump_connection("FUNCTION_ENTRY", connection, SR_DATABASE);
    
    // CRITICAL: Validate parameters IMMEDIATELY upon function entry
    // log_this(SR_DATABASE, "database_engine_execute: Function entry - validating parameters", LOG_LEVEL_ERROR, 0);
    
    if (!connection) {
        log_this(SR_DATABASE, "database_engine_execute: CONNECTION IS NULL", LOG_LEVEL_ERROR, 0);
        return false;
    }
    
    if ((uintptr_t)connection < 0x1000) {
        log_this(SR_DATABASE, "database_engine_execute: CONNECTION IS INVALID POINTER", LOG_LEVEL_ERROR, 0);
        return false;
    }
    
    // CRITICAL: Validate mutex before attempting to lock
    // log_this(SR_DATABASE, "database_engine_execute: Validating connection mutex", LOG_LEVEL_ERROR, 0);

    // Check if the mutex pointer looks valid
    pthread_mutex_t* mutex_ptr = &connection->connection_lock;
    uintptr_t mutex_addr = (uintptr_t)mutex_ptr;
    // uintptr_t conn_addr = (uintptr_t)connection;

    // Log with consistent format and same log level to compare
    // log_this(SR_DATABASE, "MUTEX_CHECK: mutex_ptr=%p, connection=%p, engine_type=%d", LOG_LEVEL_ERROR, 3,
    //     (void*)mutex_ptr, 
    //     (void*)connection, 
    //     connection->engine_type);

    // Store values to check for changes
    uintptr_t saved_mutex_addr = mutex_addr;
    // uintptr_t saved_conn_addr = conn_addr;

    // Check if values changed between logging calls
    uintptr_t current_mutex_addr = (uintptr_t)&connection->connection_lock;
    // uintptr_t current_conn_addr = (uintptr_t)connection;

    if (saved_mutex_addr != current_mutex_addr) {
        log_this(SR_DATABASE, "MUTEX_ADDRESS_CHANGED: Was %p, now %p - this explains the discrepancy!", LOG_LEVEL_ERROR, 2, 
            (void*)saved_mutex_addr, 
            (void*)current_mutex_addr);
    }

    // CRITICAL: Check for memory corruption in mutex structure
    if ((uintptr_t)mutex_ptr < 0x1000) {
        log_this(SR_DATABASE, "CRITICAL ERROR: Connection mutex pointer is corrupted! Address: %p", LOG_LEVEL_ERROR, 1, (void*)mutex_ptr);
        log_this(SR_DATABASE, "This indicates memory corruption in the DatabaseHandle structure", LOG_LEVEL_ERROR, 0);
        log_this(SR_DATABASE, "The connection object itself may be corrupted or double-freed", LOG_LEVEL_ERROR, 0);
        return false;
    }
    
    // Skip connection lock - each thread owns its connection exclusively
    // No other thread accesses this connection's state

    // log_this(SR_DATABASE, "database_engine_execute: Mutex locked successfully", LOG_LEVEL_ERROR, 0);

    // Use connection's designator for subsequent operations
    const char* designator = connection && connection->designator ? connection->designator : SR_DATABASE;
   
    // log_this(designator, "database_engine_execute: Starting execution with connection lock held", LOG_LEVEL_DEBUG, 0);

    if (!connection || !request || !result) {
        log_this(designator, "database_engine_execute: Invalid parameters - connection=%p, request=%p, result=%p", LOG_LEVEL_ERROR, 3,
            (void*)connection,
            (void*)request,
            (void*)result);
        return false;
    }

    // Check if pointer is valid before dereferencing
    if ((uintptr_t)connection == 0x1) {
        log_this(designator, "CONNECTION IS THE CORRUPTED 0x1 VALUE - ABORTING", LOG_LEVEL_ERROR, 0);
        mutex_unlock(&connection->connection_lock);
        return false;
    }
    
    // Dump connection contents safely with memory barriers
    // log_this(designator, "About to read engine_type field", LOG_LEVEL_ERROR, 0);
    
    // Force compiler to not optimize this access
    const volatile DatabaseEngine* volatile_engine_ptr = &connection->engine_type;
    DatabaseEngine captured_engine_type = *volatile_engine_ptr;

    // log_this(designator, "Successfully read engine_type: %d", LOG_LEVEL_ERROR, 1, (int)captured_engine_type);

    // Also try direct access without volatile
    DatabaseEngine direct_engine_type = connection->engine_type;
    // log_this(designator, "Direct engine_type read: %d", LOG_LEVEL_ERROR, 1, (int)direct_engine_type);

    // CRITICAL: Check for engine type corruption
    if (captured_engine_type != direct_engine_type) {
        log_this(designator, "CRITICAL ERROR: Engine type corruption detected! Volatile read: %d, Direct read: %d", LOG_LEVEL_ERROR, 2,
            (int)captured_engine_type, 
            (int)direct_engine_type);
        log_this(designator, "This indicates severe memory corruption in the connection structure", LOG_LEVEL_ERROR, 0);
    }

    // Log engine type for debugging (no longer assume PostgreSQL is expected)
    if (captured_engine_type != direct_engine_type) {
        log_this(designator, "DEBUG: Engine type mismatch detected: volatile=%d, direct=%d", LOG_LEVEL_DEBUG, 2,
            (int)captured_engine_type,
            (int)direct_engine_type);
    }
    
    // Compare the connection pointer values
    // log_this(designator, "Connection address in engine_type access: %p", LOG_LEVEL_ERROR, 1, (void*)connection);

    // CRITICAL: Check for connection pointer corruption
    if ((uintptr_t)connection < 0x1000) {
        log_this(designator, "CRITICAL ERROR: Connection pointer itself is corrupted! Value: %p", LOG_LEVEL_ERROR, 1, (void*)connection);
        log_this(designator, "This indicates the entire connection structure has been corrupted", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Validate expected engine type for this connection
    if (captured_engine_type >= DB_ENGINE_MAX || captured_engine_type < 0) {
        log_this(designator, "CRITICAL ERROR: Connection engine_type corrupted! Invalid value %d (must be 0-%d)", LOG_LEVEL_ERROR, 2, (int)captured_engine_type, DB_ENGINE_MAX-1);
        log_this(designator, "This should have returned false but continuing for debugging", LOG_LEVEL_ERROR, 0);
        // Don't return false yet - let's see what happens
    }
    
    // log_this(designator, "About to call database_engine_get with engine_type=%d", LOG_LEVEL_TRACE, 1, connection->engine_type);
    DatabaseEngineInterface* engine = database_engine_get_with_designator(connection->engine_type, designator);
    // log_this(designator, "database_engine_get returned: %p", LOG_LEVEL_TRACE, 1, (void*)engine);

    if (!engine) {
        log_this(designator, "database_engine_execute: No engine found for type %d", LOG_LEVEL_ERROR, 1, connection->engine_type);
        return false;
    }

    // CRITICAL: Check engine object integrity right before accessing it
    // debug_dump_engine("ENGINE_CHECK", engine, SR_DATABASE);

    // CRITICAL: Test the exact log call that's hanging
    // log_this(designator, "PRE_ENGINE_FOUND_LOG: About to log 'Engine found' message", LOG_LEVEL_ERROR, 0);

    // FIXED: Remove the extra boolean parameters that were causing the hang
    // log_this(designator, "database_engine_execute: Engine found - %s", LOG_LEVEL_TRACE, 1, engine->name);

    // log_this(designator, "POST_ENGINE_FOUND_LOG: 'Engine found' message logged successfully", LOG_LEVEL_ERROR, 0);

    // CRITICAL: Validate engine interface before calling
    // log_this(designator, "database_engine_execute: Validating engine interface", LOG_LEVEL_TRACE, 0);
    // log_this(designator, "database_engine_execute: engine=%p, execute_query=%p", LOG_LEVEL_TRACE, 2, (void*)engine, (void*)(uintptr_t)engine->execute_query);

    if (!engine->execute_query) {
        log_this(designator, "CRITICAL ERROR: Engine execute_query function pointer is NULL!", LOG_LEVEL_ERROR, 0);
        mutex_unlock(&connection->connection_lock);
        return false;
    }

    // CRITICAL: Test engine->name access before the main log call
    // log_this(designator, "PRE_NAME_ACCESS: About to access engine->name", LOG_LEVEL_ERROR, 0);
    // const char* engine_name = engine->name;
    // log_this(designator, "POST_NAME_ACCESS: engine->name = '%s'", LOG_LEVEL_ERROR, 1, engine_name ? engine_name : "NULL");

    // Use prepared statement if requested and available
    if (request->use_prepared_statement && request->prepared_statement_name && engine->execute_prepared) {
        log_this(designator, "database_engine_execute: Using prepared statement path", LOG_LEVEL_TRACE, 0);

        // Find existing prepared statement
        PreparedStatement* stmt = find_prepared_statement(connection, request->prepared_statement_name);

        if (!stmt && engine->prepare_statement) {
            // Create new prepared statement if it doesn't exist
            log_this(designator, "database_engine_execute: Creating new prepared statement: %s", LOG_LEVEL_TRACE, 1,
                     request->prepared_statement_name);

            bool prepared = engine->prepare_statement(connection, request->prepared_statement_name,
                                                    request->sql_template, &stmt);
            if (prepared && stmt) {
                // Store the prepared statement for future use
                if (!store_prepared_statement(connection, stmt)) {
                    log_this(designator, "database_engine_execute: Failed to store prepared statement, but prepared statement is still usable", LOG_LEVEL_TRACE, 0);
                    // Note: The prepared statement is still valid for execution
                    // We just couldn't cache it due to capacity limits
                }
            } else {
                log_this(designator, "database_engine_execute: Failed to prepare statement: %s", LOG_LEVEL_ERROR, 1,
                         request->prepared_statement_name);
                stmt = NULL;
            }
        }

        if (stmt) {
            log_this(designator, "database_engine_execute: Executing prepared statement: %s", LOG_LEVEL_TRACE, 1,
                     request->prepared_statement_name);
            // log_this(designator, "database_engine_execute: Calling execute_prepared", LOG_LEVEL_TRACE, 0);
            mutex_unlock(&connection->connection_lock);
            return engine->execute_prepared(connection, stmt, request, result);
        } else {
            log_this(designator, "database_engine_execute: Prepared statement not available, falling back to direct execution", LOG_LEVEL_TRACE, 0);
        }
    }

    // debug_dump_connection("H", connection, SR_DATABASE);

    // Fall back to regular query execution
    // log_this(designator, "database_engine_execute: Using regular query execution path", LOG_LEVEL_TRACE, 0);

    if (!engine->execute_query) {
        log_this(designator, "database_engine_execute: Engine has no execute_query function", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // log_this(designator, "database_engine_execute: Calling engine->execute_query", LOG_LEVEL_TRACE, 0);

    // Execute query with timing measurement - no connection lock needed since thread owns connection exclusively
    time_t query_start_time = time(NULL);
    bool result_success = engine->execute_query(connection, request, result);
    time_t query_end_time = time(NULL);

    // Calculate execution time in milliseconds and store in result if successful
    if (result_success && *result) {
        time_t execution_time_seconds = query_end_time - query_start_time;
        (*result)->execution_time_ms = (time_t)(execution_time_seconds * 1000); // Convert to milliseconds

        // log_this(designator, "database_engine_execute: Query executed in %ld ms", LOG_LEVEL_TRACE, 1, (*result)->execution_time_ms);
    }

    // log_this(designator, "database_engine_execute: Engine execute_query returned %s", LOG_LEVEL_TRACE, 1, result_success ? "SUCCESS" : "FAILURE");

    return result_success;
}

/*
 * Transaction Management
 */

bool database_engine_begin_transaction(DatabaseHandle* connection, DatabaseIsolationLevel level, Transaction** transaction) {
    if (!connection || !transaction) {
        return false;
    }

    const char* designator = connection->designator ? connection->designator : SR_DATABASE;
    DatabaseEngineInterface* engine = database_engine_get_with_designator(connection->engine_type, designator);
    if (!engine || !engine->begin_transaction) {
        return false;
    }

    return engine->begin_transaction(connection, level, transaction);
}

bool database_engine_commit_transaction(DatabaseHandle* connection, Transaction* transaction) {
    if (!connection || !transaction) {
        return false;
    }

    const char* designator = connection->designator ? connection->designator : SR_DATABASE;
    DatabaseEngineInterface* engine = database_engine_get_with_designator(connection->engine_type, designator);
    if (!engine || !engine->commit_transaction) {
        return false;
    }

    return engine->commit_transaction(connection, transaction);
}

bool database_engine_rollback_transaction(DatabaseHandle* connection, Transaction* transaction) {
    if (!connection || !transaction) {
        return false;
    }

    const char* designator = connection->designator ? connection->designator : SR_DATABASE;
    DatabaseEngineInterface* engine = database_engine_get_with_designator(connection->engine_type, designator);
    if (!engine || !engine->rollback_transaction) {
        return false;
    }

    return engine->rollback_transaction(connection, transaction);
}

/*
 * Connection String Utilities
 */

char* database_engine_build_connection_string(DatabaseEngine engine_type, ConnectionConfig* config) {
    if (!config) {
        return NULL;
    }

    DatabaseEngineInterface* engine = database_engine_get_with_designator(engine_type, SR_DATABASE);
    if (!engine || !engine->get_connection_string) {
        return NULL;
    }

    return engine->get_connection_string(config);
}

bool database_engine_validate_connection_string(DatabaseEngine engine_type, const char* connection_string) {
    if (!connection_string) {
        return false;
    }

    DatabaseEngineInterface* engine = database_engine_get_with_designator(engine_type, SR_DATABASE);
    if (!engine || !engine->validate_connection_string) {
        return false;
    }

    return engine->validate_connection_string(connection_string);
}

/*
 * Cleanup Functions
 */

void database_engine_cleanup_connection(DatabaseHandle* connection) {
    if (!connection) {
        return;
    }

    const char* designator = connection->designator ? connection->designator : SR_DATABASE;
    DatabaseEngineInterface* engine = database_engine_get_with_designator(connection->engine_type, designator);
    if (engine && engine->disconnect) {
        engine->disconnect(connection);
    }

    // Clean up prepared statements
    if (connection->prepared_statements) {
        for (size_t i = 0; i < connection->prepared_statement_count; i++) {
            if (connection->prepared_statements[i]) {
                if (engine && engine->unprepare_statement) {
                    engine->unprepare_statement(connection, connection->prepared_statements[i]);
                }
                free(connection->prepared_statements[i]);
            }
        }
        free(connection->prepared_statements);
    }

    // Note: ConnectionConfig is owned by the caller, not by DatabaseHandle
    // Do NOT free connection->config here to avoid double-free
    // The caller (e.g., database_queue_check_connection) is responsible for freeing it
    if (connection->config) {
        log_this(SR_DATABASE, "database_engine_cleanup_connection: NOT freeing connection config (owned by caller)", LOG_LEVEL_TRACE, 0);
    }

    // Clean up designator
    if (connection->designator) {
        free(connection->designator);
    }

    pthread_mutex_destroy(&connection->connection_lock);
    free(connection);
}

void database_engine_cleanup_result(QueryResult* result) {
    if (!result) {
        return;
    }

    if (result->data_json) {
        free(result->data_json);
    }

    if (result->error_message) {
        free(result->error_message);
    }

    if (result->column_names) {
        for (size_t i = 0; i < result->column_count; i++) {
            if (result->column_names[i]) {
                free(result->column_names[i]);
            }
        }
        free(result->column_names);
    }

    free(result);
}

void database_engine_cleanup_transaction(Transaction* transaction) {
    if (!transaction) {
        return;
    }

    if (transaction->transaction_id) {
        free(transaction->transaction_id);
    }

    free(transaction);
}