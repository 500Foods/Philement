/*
 * Database Engine Abstraction Layer
 *
 * Implements the multi-engine interface layer for database operations.
 * Provides unified interface for PostgreSQL, SQLite, MySQL, DB2, and future engines.
 */

#include "../hydrogen.h"
#include "database.h"
#include "database_queue.h"

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
    log_this(SR_DATABASE, "Starting database engine registry initialization", LOG_LEVEL_STATE, 0);

    if (engine_system_initialized) {
        return true;
    }

    MutexResult result = MUTEX_LOCK(&engine_registry_lock, SR_DATABASE);
    if (result != MUTEX_SUCCESS) {
        return false;
    }

    // Initialize registry to NULL
    memset(engine_registry, 0, sizeof(engine_registry));

    // Register all engines
    DatabaseEngineInterface* postgresql_engine = postgresql_get_interface();
    if (postgresql_engine) {
        log_this(SR_DATABASE, "Registering PostgreSQL engine: %s at index %d", LOG_LEVEL_DEBUG, 2,
                 postgresql_engine->name ? postgresql_engine->name : "NULL",
                 DB_ENGINE_POSTGRESQL);
        engine_registry[DB_ENGINE_POSTGRESQL] = postgresql_engine;
    } else {
        log_this(SR_DATABASE, "CRITICAL ERROR: Failed to get PostgreSQL engine interface!", LOG_LEVEL_ERROR, 0);
    }

    DatabaseEngineInterface* sqlite_engine = sqlite_get_interface();
    if (sqlite_engine) {
        engine_registry[DB_ENGINE_SQLITE] = sqlite_engine;
    }

    DatabaseEngineInterface* mysql_engine = mysql_get_interface();
    if (mysql_engine) {
        engine_registry[DB_ENGINE_MYSQL] = mysql_engine;
    }

    DatabaseEngineInterface* db2_engine = db2_get_interface();
    if (db2_engine) {
        engine_registry[DB_ENGINE_DB2] = db2_engine;
    }

    engine_system_initialized = true;
    mutex_unlock(&engine_registry_lock);

    log_this(SR_DATABASE, "Database engine registry initialization completed", LOG_LEVEL_STATE, 0);
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

    mutex_unlock(&engine_registry_lock);

    log_this(SR_DATABASE, "Database engine registered successfully", LOG_LEVEL_STATE, 0);
    return true;
}

DatabaseEngineInterface* database_engine_get(DatabaseEngine engine_type) {
    log_this(SR_DATABASE, "WE BE HERE NOW", LOG_LEVEL_DEBUG, 0);
    log_this(SR_DATABASE, "database_engine_get called with engine_type=%d", LOG_LEVEL_DEBUG, 1, (int)engine_type);

    if (engine_type >= DB_ENGINE_MAX || !engine_system_initialized) {
        log_this(SR_DATABASE, "database_engine_get: Invalid engine_type or system not initialized", LOG_LEVEL_DEBUG, 0);
        return NULL;
    }

    log_this(SR_DATABASE, "database_engine_get: About to lock engine_registry_lock", LOG_LEVEL_DEBUG, 0);
    MutexResult result = MUTEX_LOCK(&engine_registry_lock, SR_DATABASE);
    if (result != MUTEX_SUCCESS) {
        log_this(SR_DATABASE, "database_engine_get: Failed to lock engine_registry_lock, result=%s", LOG_LEVEL_ERROR, 1, mutex_result_to_string(result));
        return NULL;
    }

    log_this(SR_DATABASE, "database_engine_get: Successfully locked, getting engine from registry", LOG_LEVEL_DEBUG, 0);
    DatabaseEngineInterface* engine = engine_registry[engine_type];
    log_this(SR_DATABASE, "database_engine_get: Engine retrieved: %p", LOG_LEVEL_DEBUG, 1, (void*)engine);

    log_this(SR_DATABASE, "database_engine_get: About to unlock engine_registry_lock", LOG_LEVEL_DEBUG, 0);
    mutex_unlock(&engine_registry_lock);
    log_this(SR_DATABASE, "database_engine_get: Successfully unlocked, returning", LOG_LEVEL_DEBUG, 0);

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
            mutex_unlock(&engine_registry_lock);
            return engine;
        }
    }

    mutex_unlock(&engine_registry_lock);
    return NULL;
}

/*
 * Connection Management
 */

bool database_engine_connect(DatabaseEngine engine_type, ConnectionConfig* config, DatabaseHandle** connection) {
    return database_engine_connect_with_designator(engine_type, config, connection, NULL);
}

bool database_engine_connect_with_designator(DatabaseEngine engine_type, ConnectionConfig* config, DatabaseHandle** connection, const char* designator) {
    DatabaseEngineInterface* engine = database_engine_get(engine_type);
    if (!engine || !engine->connect || !config || !connection) {
        return false;
    }

    return engine->connect(config, connection, designator);
}

bool database_engine_health_check(DatabaseHandle* connection) {
    if (!connection || !connection->engine_type) {
        return false;
    }

    DatabaseEngineInterface* engine = database_engine_get(connection->engine_type);
    if (!engine || !engine->health_check) {
        return false;
    }

    return engine->health_check(connection);
}

/*
 * Query Execution
 */

bool database_engine_execute(DatabaseHandle* connection, QueryRequest* request, QueryResult** result) {

    // CRITICAL: Use same debug function as caller - FIRST LINE OF FUNCTION
    debug_dump_connection("FUNCTION_ENTRY", connection, SR_DATABASE);
    
    // CRITICAL: Validate parameters IMMEDIATELY upon function entry
    log_this(SR_DATABASE, "database_engine_execute: Function entry - validating parameters", LOG_LEVEL_ERROR, 0);
    
    if (!connection) {
        log_this(SR_DATABASE, "database_engine_execute: CONNECTION IS NULL", LOG_LEVEL_ERROR, 0);
        return false;
    }
    
    if ((uintptr_t)connection < 0x1000) {
        log_this(SR_DATABASE, "database_engine_execute: CONNECTION IS INVALID POINTER", LOG_LEVEL_ERROR, 0);
        return false;
    }
    
    // CRITICAL: Validate mutex before attempting to lock
    log_this(SR_DATABASE, "database_engine_execute: Validating connection mutex", LOG_LEVEL_ERROR, 0);

    // Check if the mutex pointer looks valid
    pthread_mutex_t* mutex_ptr = &connection->connection_lock;
    uintptr_t mutex_addr = (uintptr_t)mutex_ptr;
    uintptr_t conn_addr = (uintptr_t)connection;

    // Log with consistent format and same log level to compare
    log_this(SR_DATABASE, "MUTEX_CHECK: mutex_ptr=%p, connection=%p, engine_type=%d", LOG_LEVEL_ERROR, 2,
        (void*)mutex_ptr, 
        (void*)connection, connection->engine_type);

    // Store values to check for changes
    uintptr_t saved_mutex_addr = mutex_addr;
    uintptr_t saved_conn_addr = conn_addr;
    DatabaseEngine saved_engine_type = connection->engine_type;

    // Check if values changed between logging calls
    uintptr_t current_mutex_addr = (uintptr_t)&connection->connection_lock;
    uintptr_t current_conn_addr = (uintptr_t)connection;
    DatabaseEngine current_engine_type = connection->engine_type;

    if (saved_mutex_addr != current_mutex_addr) {
        log_this(SR_DATABASE, "MUTEX_ADDRESS_CHANGED: Was %p, now %p - this explains the discrepancy!", LOG_LEVEL_ERROR, 2, 
            (void*)saved_mutex_addr, 
            (void*)current_mutex_addr);
    }

    if (saved_conn_addr != current_conn_addr) {
        log_this(SR_DATABASE, "CONNECTION_ADDRESS_CHANGED: Was %p, now %p", LOG_LEVEL_ERROR, 2,
            (void*)saved_conn_addr, 
            (void*)current_conn_addr);
    }

    if (saved_engine_type != current_engine_type) {
        log_this(SR_DATABASE, "ENGINE_TYPE_CHANGED: Was %d, now %d", LOG_LEVEL_ERROR, 2,
            (int)saved_engine_type, 
            (int)current_engine_type);
    }

    // CRITICAL: Check for memory corruption in mutex structure
    if ((uintptr_t)mutex_ptr == 0x1 || (uintptr_t)mutex_ptr < 0x1000) {
        log_this(SR_DATABASE, "CRITICAL ERROR: Connection mutex pointer is corrupted! Address: %p", LOG_LEVEL_ERROR, 1, (void*)mutex_ptr);
        log_this(SR_DATABASE, "This indicates memory corruption in the DatabaseHandle structure", LOG_LEVEL_ERROR, 0);
        log_this(SR_DATABASE, "The connection object itself may be corrupted or double-freed", LOG_LEVEL_ERROR, 0);
        return false;
    }
    
    // Try to lock with timeout using the new mutex library
    log_this(SR_DATABASE, "database_engine_execute: Attempting timed mutex lock", LOG_LEVEL_ERROR, 0);
    MutexResult lock_result = MUTEX_LOCK(&connection->connection_lock, SR_DATABASE);

    if (lock_result != MUTEX_SUCCESS) {
        log_this(SR_DATABASE, "MUTEX LOCK FAILED with result %s - this explains the deadlock!", LOG_LEVEL_ERROR, 1, mutex_result_to_string(lock_result));
        return false;
    }

    log_this(SR_DATABASE, "database_engine_execute: Mutex locked successfully", LOG_LEVEL_ERROR, 0);
    
    // Use static designator to avoid accessing corrupted connection
    const char* designator = SR_DATABASE;
   
    log_this(designator, "database_engine_execute: Starting execution with connection lock held", LOG_LEVEL_DEBUG, 0);

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
        return false;
    }
    
    // Dump connection contents safely with memory barriers
    log_this(designator, "About to read engine_type field", LOG_LEVEL_ERROR, 0);
    
    // Force compiler to not optimize this access
    const volatile DatabaseEngine* volatile_engine_ptr = &connection->engine_type;
    DatabaseEngine captured_engine_type = *volatile_engine_ptr;

    log_this(designator, "Successfully read engine_type: %d", LOG_LEVEL_ERROR, 1, (int)captured_engine_type);

    // Also try direct access without volatile
    DatabaseEngine direct_engine_type = connection->engine_type;
    log_this(designator, "Direct engine_type read: %d", LOG_LEVEL_ERROR, 1, (int)direct_engine_type);

    // CRITICAL: Check for engine type corruption
    if (captured_engine_type != direct_engine_type) {
        log_this(designator, "CRITICAL ERROR: Engine type corruption detected! Volatile read: %d, Direct read: %d", LOG_LEVEL_ERROR, 2,
            (int)captured_engine_type, 
            (int)direct_engine_type);
        log_this(designator, "This indicates severe memory corruption in the connection structure", LOG_LEVEL_ERROR, 0);
    }

    if (captured_engine_type != DB_ENGINE_POSTGRESQL) {
        log_this(designator, "WARNING: Unexpected engine type %d (expected %d for PostgreSQL)", LOG_LEVEL_ERROR, 2,
            (int)captured_engine_type, 
            DB_ENGINE_POSTGRESQL);
    }
    
    // Compare the connection pointer values
    log_this(designator, "Connection address in engine_type access: %p", LOG_LEVEL_ERROR, 1, (void*)connection);

    // CRITICAL: Check for connection pointer corruption
    if ((uintptr_t)connection == 0x1 || (uintptr_t)connection < 0x1000) {
        log_this(designator, "CRITICAL ERROR: Connection pointer itself is corrupted! Value: %p", LOG_LEVEL_ERROR, 1, (void*)connection);
        log_this(designator, "This indicates the entire connection structure has been corrupted", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Validate expected engine type for this connection
    if (captured_engine_type != DB_ENGINE_POSTGRESQL) {
        log_this(designator, "CRITICAL ERROR: Connection engine_type corrupted! Expected PostgreSQL(0), got %d", LOG_LEVEL_ERROR, 1, (int)captured_engine_type);
        log_this(designator, "This should have returned false but continuing for debugging", LOG_LEVEL_ERROR, 0);
        // Don't return false yet - let's see what happens
    }
    
    log_this(designator, "About to call database_engine_get with engine_type=%d", LOG_LEVEL_DEBUG, 1, connection->engine_type);
    DatabaseEngineInterface* engine = database_engine_get(connection->engine_type);
    log_this(designator, "database_engine_get returned: %p", LOG_LEVEL_DEBUG, 1, (void*)engine);

    if (!engine) {
        log_this(designator, "database_engine_execute: No engine found for type %d", LOG_LEVEL_ERROR, 1, connection->engine_type);
        return false;
    }

    // CRITICAL: Check engine object integrity right before accessing it
    debug_dump_engine("ENGINE_CHECK", engine, SR_DATABASE);

    // CRITICAL: Test the exact log call that's hanging
    log_this(designator, "PRE_ENGINE_FOUND_LOG: About to log 'Engine found' message", LOG_LEVEL_ERROR, 0);

    // FIXED: Remove the extra boolean parameters that were causing the hang
    log_this(designator, "database_engine_execute: Engine found - %s", LOG_LEVEL_DEBUG, 1, engine->name);

    log_this(designator, "POST_ENGINE_FOUND_LOG: 'Engine found' message logged successfully", LOG_LEVEL_ERROR, 0);

    // CRITICAL: Validate engine interface before calling
    log_this(designator, "database_engine_execute: Validating engine interface", LOG_LEVEL_DEBUG, 0);
    log_this(designator, "database_engine_execute: engine=%p, execute_query=%p", LOG_LEVEL_DEBUG, 2, (void*)engine, (void*)(uintptr_t)engine->execute_query);

    if (!engine->execute_query) {
        log_this(designator, "CRITICAL ERROR: Engine execute_query function pointer is NULL!", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // CRITICAL: Test engine->name access before the main log call
    log_this(designator, "PRE_NAME_ACCESS: About to access engine->name", LOG_LEVEL_ERROR, 0);
    const char* engine_name = engine->name;
    log_this(designator, "POST_NAME_ACCESS: engine->name = '%s'", LOG_LEVEL_ERROR, 1, engine_name ? engine_name : "NULL");

    // Use prepared statement if requested and available
    if (request->use_prepared_statement && request->prepared_statement_name && engine->execute_prepared) {
        log_this(designator, "database_engine_execute: Using prepared statement path", LOG_LEVEL_DEBUG, 0);
        
        // Find prepared statement
        PreparedStatement* stmt = NULL;
        for (size_t i = 0; i < connection->prepared_statement_count; i++) {
            if (connection->prepared_statements[i] &&
                strcmp(connection->prepared_statements[i]->name, request->prepared_statement_name) == 0) {
                stmt = connection->prepared_statements[i];
                break;
            }
        }

        if (stmt) {
            log_this(designator, "database_engine_execute: Calling execute_prepared", LOG_LEVEL_DEBUG, 0);
            return engine->execute_prepared(connection, stmt, request, result);
        }
    }

    debug_dump_connection("H", connection, SR_DATABASE);

    // Fall back to regular query execution
    log_this(designator, "database_engine_execute: Using regular query execution path", LOG_LEVEL_DEBUG, 0);
    
    if (!engine->execute_query) {
        log_this(designator, "database_engine_execute: Engine has no execute_query function", LOG_LEVEL_ERROR, 0);
        return false;
    }

    log_this(designator, "database_engine_execute: Calling engine->execute_query", LOG_LEVEL_DEBUG, 0);

    log_this(designator, "FUNCTION_CALL_DEBUG: About to call engine->execute_query", LOG_LEVEL_ERROR, 0);
    log_this(designator, "FUNCTION_CALL_DEBUG: engine=%p, execute_query=%p", LOG_LEVEL_ERROR, 2, (void*)engine, (void*)(uintptr_t)engine->execute_query);
    log_this(designator, "FUNCTION_CALL_DEBUG: connection=%p, request=%p, result=%p", LOG_LEVEL_ERROR, 2, (void*)connection, (void*)request, (void*)result);

    bool result_success = engine->execute_query(connection, request, result);

    log_this(designator, "FUNCTION_CALL_DEBUG: engine->execute_query returned %d", LOG_LEVEL_ERROR, 1, result_success);
    
    log_this(designator, "database_engine_execute: Engine execute_query returned %s", LOG_LEVEL_DEBUG, 1, result_success ? "SUCCESS" : "FAILURE");
    
    // Release connection object mutex
    log_this(SR_DATABASE, "MUTEX_UNLOCK_ATTEMPT: About to unlock connection mutex", LOG_LEVEL_DEBUG, 0);
    MutexResult unlock_result = mutex_unlock(&connection->connection_lock);
    log_this(SR_DATABASE, "MUTEX_UNLOCK_RESULT: Unlock completed with result %s", LOG_LEVEL_DEBUG, 1, mutex_result_to_string(unlock_result));
    
    return result_success;
}

/*
 * Transaction Management
 */

bool database_engine_begin_transaction(DatabaseHandle* connection, DatabaseIsolationLevel level, Transaction** transaction) {
    if (!connection || !transaction) {
        return false;
    }

    DatabaseEngineInterface* engine = database_engine_get(connection->engine_type);
    if (!engine || !engine->begin_transaction) {
        return false;
    }

    return engine->begin_transaction(connection, level, transaction);
}

bool database_engine_commit_transaction(DatabaseHandle* connection, Transaction* transaction) {
    if (!connection || !transaction) {
        return false;
    }

    DatabaseEngineInterface* engine = database_engine_get(connection->engine_type);
    if (!engine || !engine->commit_transaction) {
        return false;
    }

    return engine->commit_transaction(connection, transaction);
}

bool database_engine_rollback_transaction(DatabaseHandle* connection, Transaction* transaction) {
    if (!connection || !transaction) {
        return false;
    }

    DatabaseEngineInterface* engine = database_engine_get(connection->engine_type);
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

    DatabaseEngineInterface* engine = database_engine_get(engine_type);
    if (!engine || !engine->get_connection_string) {
        return NULL;
    }

    return engine->get_connection_string(config);
}

bool database_engine_validate_connection_string(DatabaseEngine engine_type, const char* connection_string) {
    if (!connection_string) {
        return false;
    }

    DatabaseEngineInterface* engine = database_engine_get(engine_type);
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

    DatabaseEngineInterface* engine = database_engine_get(connection->engine_type);
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
        log_this(SR_DATABASE, "database_engine_cleanup_connection: NOT freeing connection config (owned by caller)", LOG_LEVEL_DEBUG, 0);
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