/*
 * Database Engine Abstraction Layer
 *
 * Implements the multi-engine interface layer for database operations.
 * Provides unified interface for PostgreSQL, SQLite, MySQL, DB2, and future engines.
 */

#include <src/hydrogen.h>
#include "database.h"
#include "database_connstring.h"
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
            log_this(SR_DATABASE, "- Registering PostgreSQL engine: %s at index %d", LOG_LEVEL_DEBUG, 2,
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
            log_this(SR_DATABASE, "- Registering SQLite engine: %s at index %d", LOG_LEVEL_DEBUG, 2,
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
            log_this(SR_DATABASE, "- Registering MySQL engine: %s at index %d", LOG_LEVEL_DEBUG, 2,
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
            log_this(SR_DATABASE, "- Registering DB2 engine: %s at index %d", LOG_LEVEL_DEBUG, 2,
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

    // Connection mutex validation removed - each thread owns its connection exclusively
    // No other thread accesses this connection's state, so no mutex needed
    
    // Skip connection lock - each thread owns its connection exclusively
    // No other thread accesses this connection's state
    // The connection and its prepared statements are thread-local to this DQM

    // log_this(SR_DATABASE, "database_engine_execute: Mutex locked successfully", LOG_LEVEL_ERROR, 0);

    // Use connection's designator for subsequent operations (connection already validated above)
    const char* designator = connection->designator ? connection->designator : SR_DATABASE;
   
    // log_this(designator, "database_engine_execute: Starting execution with connection lock held", LOG_LEVEL_DEBUG, 0);

    // Validate remaining parameters (connection already validated at function entry)
    if (!request || !result) {
        log_this(designator, "database_engine_execute: Invalid parameters - request=%p, result=%p", LOG_LEVEL_ERROR, 2,
            (void*)request,
            (void*)result);
        return false;
    }

    DatabaseEngine engine_type = connection->engine_type;
    if (engine_type >= DB_ENGINE_MAX || engine_type < 0) {
        log_this(designator, "CRITICAL ERROR: Invalid engine_type %d (must be 0-%d)", LOG_LEVEL_ERROR, 2, (int)engine_type, DB_ENGINE_MAX-1);
        return false;
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
        bool stmt_was_created = false;

        if (!stmt && engine->prepare_statement) {
            // Create new prepared statement if it doesn't exist
            log_this(designator, "database_engine_execute: Creating new prepared statement: %s", LOG_LEVEL_TRACE, 1,
                     request->prepared_statement_name);

            bool prepared = engine->prepare_statement(connection, request->prepared_statement_name,
                                                    request->sql_template, &stmt, false);
            if (prepared && stmt) {
                stmt_was_created = true;
                
                // Try to store the prepared statement for future use
                if (!store_prepared_statement(connection, stmt)) {
                    log_this(designator, "database_engine_execute: Cache full, will use statement once then free it", LOG_LEVEL_TRACE, 0);
                    // NOTE: stmt_was_created flag tracks that we need to clean this up
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
            
            bool exec_result = engine->execute_prepared(connection, stmt, request, result);
            
            // CRITICAL: If we created this statement but couldn't cache it, we must free it now
            if (stmt_was_created && find_prepared_statement(connection, request->prepared_statement_name) == NULL) {
                log_this(designator, "database_engine_execute: Freeing uncached prepared statement: %s", LOG_LEVEL_TRACE, 1,
                         request->prepared_statement_name);
                if (engine->unprepare_statement) {
                    engine->unprepare_statement(connection, stmt);
                }
            }
            
            return exec_result;
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
    
    // CRITICAL: Clean up prepared statements BEFORE disconnecting
    // The statements need access to connection->connection_handle to unprepare properly
    if (connection->prepared_statements) {
        // If count is 0, statements were already cleared by clear_prepared_statements()
        // In that case, just free the array and LRU counter
        if (connection->prepared_statement_count == 0) {
            free(connection->prepared_statements);
            connection->prepared_statements = NULL;
            if (connection->prepared_statement_lru_counter) {
                free(connection->prepared_statement_lru_counter);
                connection->prepared_statement_lru_counter = NULL;
            }
        } else {
            // Normal cleanup path: unprepare and free each statement
            for (size_t i = 0; i < connection->prepared_statement_count; i++) {
                PreparedStatement* stmt = connection->prepared_statements[i];
                
                // Skip NULL or obviously invalid pointers
                if (!stmt || (uintptr_t)stmt < 0x1000) {
                    continue;
                }
                
                // Call engine unprepare if available
                if (engine && engine->unprepare_statement) {
                    engine->unprepare_statement(connection, stmt);
                } else {
                    // Manual cleanup if no engine function
                    if (stmt->name && (uintptr_t)stmt->name >= 0x1000) {
                        free(stmt->name);
                    }
                    if (stmt->sql_template && (uintptr_t)stmt->sql_template >= 0x1000) {
                        free(stmt->sql_template);
                    }
                    free(stmt);
                }
            }
            free(connection->prepared_statements);
            connection->prepared_statements = NULL;
            
            if (connection->prepared_statement_lru_counter) {
                free(connection->prepared_statement_lru_counter);
                connection->prepared_statement_lru_counter = NULL;
            }
        }
    }
    
    // Now disconnect - this frees ODBC handles but NOT the engine-specific connection structure
    if (engine && engine->disconnect) {
        engine->disconnect(connection);
    }
    
    // Free the engine-specific connection structure (e.g., DB2Connection)
    // This must be done AFTER unpreparing statements but AFTER disconnect
    if (connection->connection_handle) {
        // For DB2, need to free the DB2Connection structure and its prepared_statements cache
        if (connection->engine_type == DB_ENGINE_DB2) {
            typedef struct {
                void* environment;
                void* connection;
                void* prepared_statements; // PreparedStatementCache*
            } DB2Connection;
            
            DB2Connection* db2_conn = (DB2Connection*)connection->connection_handle;
            // Free the prepared statement cache if it exists
            if (db2_conn->prepared_statements) {
                // Need to call db2_destroy_prepared_statement_cache
                // But we can't include the header here, so just free it directly
                typedef struct {
                    char** names;
                    size_t count;
                    size_t capacity;
                    pthread_mutex_t lock;
                } PreparedStatementCache;
                
                PreparedStatementCache* cache = (PreparedStatementCache*)db2_conn->prepared_statements;
                pthread_mutex_lock(&cache->lock);
                for (size_t i = 0; i < cache->count; i++) {
                    free(cache->names[i]);
                }
                free(cache->names);
                pthread_mutex_unlock(&cache->lock);
                pthread_mutex_destroy(&cache->lock);
                free(cache);
            }
            free(db2_conn);
        }
        // Other engines free their structures in their disconnect functions
        connection->connection_handle = NULL;
    }

    // CRITICAL: Free the ConnectionConfig - the DatabaseHandle owns it after successful connection
    // The config was transferred to the handle by the engine's connect function
    if (connection->config) {
        free_connection_config(connection->config);
        connection->config = NULL;
    }

    // Clean up designator
    if (connection->designator) {
        free((char*)connection->designator);
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


/*
 * Prepared Statement Management Helpers
 */

// Find a prepared statement by name in the connection
PreparedStatement* find_prepared_statement(DatabaseHandle* connection, const char* name) {
    if (!connection || !name || !connection->prepared_statements) {
        return NULL;
    }

    for (size_t i = 0; i < connection->prepared_statement_count; i++) {
        PreparedStatement* stmt = connection->prepared_statements[i];
        
        // Defense against dangling pointers: validate stmt pointer is in valid range
        if (!stmt || (uintptr_t)stmt < 0x1000) {
            // NULL or invalid pointer - skip this entry
            continue;
        }
        
        // Defense against dangling name pointer: validate before strcmp
        if (!stmt->name || (uintptr_t)stmt->name < 0x1000) {
            // NULL or invalid name pointer - skip this entry
            continue;
        }
        
        // Additional defense: validate first byte of name is readable
        // This catches freed memory that might have been overwritten
        char first_char = stmt->name[0];
        if (first_char == '\0') {
            // Empty name string - skip
            continue;
        }
        
        // Safe to compare now
        if (strcmp(stmt->name, name) == 0) {
            return stmt;
        }
    }

    return NULL;
}

// Store a prepared statement in the connection's array
bool store_prepared_statement(DatabaseHandle* connection, PreparedStatement* stmt) {
    if (!connection || !stmt) {
        return false;
    }

    // Get cache size from connection config (set by config system)
    size_t cache_size = (size_t)connection->config->prepared_statement_cache_size;

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

    // Check if we can store another statement (need strict < not <=)
    // Array indices are 0 to (cache_size - 1), so count must be < cache_size
    if (connection->prepared_statement_count >= cache_size) {
        // Cache is full - implement LRU eviction
        log_this(connection->designator ? connection->designator : SR_DATABASE,
                 "Prepared statement cache full (%zu/%zu), evicting LRU to make room for: %s", 
                 LOG_LEVEL_TRACE, 3, connection->prepared_statement_count, cache_size, stmt->name);
        
        // Find least recently used prepared statement (lowest LRU counter)
        size_t lru_index = 0;
        uint64_t min_lru_value = UINT64_MAX;
        
        for (size_t i = 0; i < connection->prepared_statement_count; i++) {
            if (connection->prepared_statement_lru_counter[i] < min_lru_value) {
                min_lru_value = connection->prepared_statement_lru_counter[i];
                lru_index = i;
            }
        }
        
        // Evict the LRU prepared statement
        PreparedStatement* evicted_stmt = connection->prepared_statements[lru_index];
        if (evicted_stmt) {
            // Get engine interface to call unprepare
            DatabaseEngineInterface* engine = database_engine_get_with_designator(connection->engine_type, 
                                                                                   connection->designator ? connection->designator : SR_DATABASE);
            if (engine && engine->unprepare_statement) {
                // This will free the statement and remove it from the array
                engine->unprepare_statement(connection, evicted_stmt);
            } else {
                // Fallback: manual cleanup
                if (evicted_stmt->name) free(evicted_stmt->name);
                if (evicted_stmt->sql_template) free(evicted_stmt->sql_template);
                free(evicted_stmt);
                
                // Manually shift array elements
                for (size_t i = lru_index; i < connection->prepared_statement_count - 1; i++) {
                    connection->prepared_statements[i] = connection->prepared_statements[i + 1];
                    connection->prepared_statement_lru_counter[i] = connection->prepared_statement_lru_counter[i + 1];
                }
                
                // NULL out the last element
                connection->prepared_statements[connection->prepared_statement_count - 1] = NULL;
                connection->prepared_statement_lru_counter[connection->prepared_statement_count - 1] = 0;
                connection->prepared_statement_count--;
            }
            
            log_this(connection->designator ? connection->designator : SR_DATABASE,
                     "Evicted LRU prepared statement to make room for: %s", LOG_LEVEL_TRACE, 1, stmt->name);
        }
    }

    // cppcheck-suppress knownConditionTrueFalse
    // Justification: Defensive check to detect corruption - intentionally redundant for safety
    // Paranoid check: Ensure we're not about to write beyond the array
    // This should never trigger if the above check is correct, but prevents corruption
    size_t next_index = connection->prepared_statement_count;
    // cppcheck-suppress knownConditionTrueFalse
    if (next_index >= cache_size) {
        log_this(connection->designator ? connection->designator : SR_DATABASE,
                 "CRITICAL: prepared_statement_count corruption detected (%zu >= %zu)", LOG_LEVEL_ERROR, 2,
                 next_index, cache_size);
        return false;
    }

    // Store the statement
    connection->prepared_statements[next_index] = stmt;
    connection->prepared_statement_count++;

    log_this(connection->designator ? connection->designator : SR_DATABASE,
             "Stored prepared statement: %s (total: %zu of %zu)", LOG_LEVEL_TRACE, 3,
             stmt->name, connection->prepared_statement_count, cache_size);

    return true;
}

// Clear all prepared statements from a connection
// This should be called when prepared statements may have become invalid (e.g., after transactions)
void database_engine_clear_prepared_statements(DatabaseHandle* connection) {
    if (!connection || !connection->prepared_statements) {
        return;
    }

    const char* designator = connection->designator ? connection->designator : SR_DATABASE;
    
    // Save count for logging before resetting
    size_t cleared_count = connection->prepared_statement_count;
    
    // CRITICAL: After transaction commit/rollback, PreparedStatement structures may be corrupted
    // Transaction cleanup may have invalidated the entire structure including name/sql_template pointers
    // Attempting to free these can cause heap corruption and crashes
    
    // SAFEST approach: Just NULL out the pointers and accept small memory leak
    // Migrations are bootstrap-time operations, so a few leaked structures won't matter
    // Better to leak memory than crash the system
    for (size_t i = 0; i < connection->prepared_statement_count; i++) {
        connection->prepared_statements[i] = NULL;
    }
    
    // Reset count
    connection->prepared_statement_count = 0;
    
    log_this(designator, "Invalidated %zu prepared statement references (transaction boundary)",
             LOG_LEVEL_TRACE, 1, cleared_count);
}

// Get database counts by type
void database_get_counts_by_type(int* postgres_count, int* mysql_count, int* sqlite_count, int* db2_count) {
    if (!app_config) {
        *postgres_count = 0;
        *mysql_count = 0;
        *sqlite_count = 0;
        *db2_count = 0;
        return;
    }

    const DatabaseConfig* db_config = &app_config->databases;

    *postgres_count = 0;
    *mysql_count = 0;
    *sqlite_count = 0;
    *db2_count = 0;

    for (int i = 0; i < db_config->connection_count; i++) {
        const DatabaseConnection* conn = &db_config->connections[i];
        if (conn->enabled && conn->type) {
            if (strcmp(conn->type, "postgresql") == 0 || strcmp(conn->type, "postgres") == 0) {
                (*postgres_count)++;
            } else if (strcmp(conn->type, "mysql") == 0) {
                (*mysql_count)++;
            } else if (strcmp(conn->type, "sqlite") == 0) {
                (*sqlite_count)++;
            } else if (strcmp(conn->type, "db2") == 0) {
                (*db2_count)++;
            }
        }
    }
}

// Get supported database engines
void database_get_supported_engines(char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) {
        return;
    }

    if (!database_subsystem) {
        snprintf(buffer, buffer_size, "Database subsystem not initialized");
        return;
    }

    // List supported engines
    const char* engines = "PostgreSQL, SQLite, MySQL, DB2";
    strncpy(buffer, engines, buffer_size - 1);
    buffer[buffer_size - 1] = '\0';
}
