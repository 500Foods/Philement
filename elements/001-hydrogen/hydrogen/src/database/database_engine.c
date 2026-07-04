/*
 * Database Engine Abstraction Layer
 *
 * Implements the multi-engine interface layer for database operations.
 * Provides unified interface for PostgreSQL, SQLite, MySQL, DB2, and future engines.
 *
 * Module split:
 *   - database_engine_registry.c    : engine registry lifecycle
 *   - database_engine_transaction.c : transaction wrappers
 *   - database_engine_prepared.c    : prepared-statement cache helpers
 *   - database_engine_metrics.c     : global counters and metrics
 *   - database_engine.c (this file) : execute, connect/health wrappers, cleanup
 */

#include <src/hydrogen.h>
#include "database.h"
#include "database_connstring.h"
#include "database_params.h"
#include "database_watchdog.h"
#include "dbqueue/dbqueue.h"
#include "database_engine_metrics_internal.h"

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
            // Create new prepared statement if it doesn't exist - cache miss
            __sync_add_and_fetch(&db_prepared_statement_cache_misses, 1);
            
            log_this(designator, "database_engine_execute: Creating new prepared statement: %s", LOG_LEVEL_TRACE, 1,
                     request->prepared_statement_name);

            // Convert named parameters to positional format for prepared statements
            // This ensures :paramName syntax is converted to $1, $2, etc. before PREPARE
            char* sql_for_prepare = (char*)request->sql_template;
            char* converted_sql = NULL;
            
            // Always attempt to convert named parameters, even with empty parameter list
            // This handles migrations where SQL has :paramName but no JSON parameters are provided
            ParameterList* param_list = NULL;
            if (request->parameters_json && strlen(request->parameters_json) > 0) {
                param_list = parse_typed_parameters(request->parameters_json, designator);
            }
            
            // Create empty parameter list if none provided (so convert_named_to_positional can still work)
            if (!param_list) {
                param_list = (ParameterList*)malloc(sizeof(ParameterList));
                if (param_list) {
                    param_list->count = 0;
                    param_list->params = NULL;
                }
            }
            
            if (param_list) {
                TypedParameter** ordered_params = NULL;
                size_t ordered_param_count = 0;
                
                converted_sql = convert_named_to_positional(
                    request->sql_template,
                    param_list,
                    connection->engine_type,
                    &ordered_params,
                    &ordered_param_count,
                    designator
                );
                
                if (converted_sql) {
                    sql_for_prepare = converted_sql;
                    log_this(designator, "database_engine_execute: Converted SQL for prepared statement (%zu params)", LOG_LEVEL_TRACE, 1, ordered_param_count);
                }
                
                // Clean up temporary parameter data
                if (ordered_params) {
                    free(ordered_params);
                }
                free_parameter_list(param_list);
            }

            bool prepared = engine->prepare_statement(connection, request->prepared_statement_name,
                                                    sql_for_prepare, &stmt, false);
            
            // Clean up converted SQL if it was allocated
            if (converted_sql) {
                free(converted_sql);
            }
            if (prepared && stmt) {
                stmt_was_created = true;
                __sync_add_and_fetch(&db_prepared_statements_cached, 1);
                
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
        } else if (stmt) {
            // Found existing prepared statement - cache hit
            __sync_add_and_fetch(&db_prepared_statement_cache_hits, 1);
        }

        if (stmt) {
            log_this(designator, "database_engine_execute: Executing prepared statement: %s", LOG_LEVEL_TRACE, 1,
                     request->prepared_statement_name);
            
            // Increment prepared statement execution counter
            __sync_add_and_fetch(&db_queries_executed_total, 1);
            __sync_add_and_fetch(&db_queries_prepared_executed, 1);
            
            bool exec_result = engine->execute_prepared(connection, stmt, request, result);
            
            if (exec_result) {
                __sync_add_and_fetch(&db_queries_successful, 1);
            } else {
                __sync_add_and_fetch(&db_queries_failed, 1);
            }
            
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

    /*
     * Register the entire operation with the watchdog so a hang on
     * any engine call (or on the retries between them) is detected
     * and the engine-specific cancel hook is invoked. One
     * registration covers the whole retry loop - the watchdog's
     * effective_timeout_seconds becomes the total operation budget
     * across all attempts, not per-attempt. The cancel fires once on
     * the first transition to expired; the entry's cancelled flag
     * prevents repeated cancels on subsequent attempts, which is
     * fine - if the retry also hangs, the server-side
     * statement_timeout will eventually catch it.
     *
     * Register is a no-op (returns NULL) if the watchdog subsystem
     * is not initialized, e.g. in early startup before the database
     * subsystem brings it up, or in unit tests that don't init it.
     */
    DatabaseWatchdogHandle* watchdog_handle =
        database_watchdog_register(connection, request);

    /*
     * Retry loop for transient failures. The engine is called up to
     * (1 + request->max_retries) times. Between attempts the thread
     * sleeps with exponential backoff (1s, 2s, 4s, ..., capped at
     * 30s). Each attempt has the full request->timeout_seconds to
     * complete independently. Only DB_ERR_TRANSPORT and DB_ERR_TIMEOUT
     * are retried - syntax, schema, constraint, and other "the user
     * did something wrong" errors fail immediately because retrying
     * them just wastes time and clutters the log.
     */
    int max_retries = (request && request->max_retries > 0) ? request->max_retries : 0;
    int total_attempts = max_retries + 1;
    bool result_success = false;
    QueryResult* attempt_result = NULL;

    for (int attempt = 1; attempt <= total_attempts; attempt++) {
        // Free any leftover result from a prior attempt before retrying.
        if (attempt_result) {
            database_engine_cleanup_result(attempt_result);
            attempt_result = NULL;
        }
        *result = NULL;

        struct timespec query_start_time, query_end_time;
        clock_gettime(CLOCK_MONOTONIC, &query_start_time);

        __sync_add_and_fetch(&db_queries_executed_total, 1);
        __sync_add_and_fetch(&db_queries_direct_executed, 1);

        result_success = engine->execute_query(connection, request, &attempt_result);

        if (result_success) {
            __sync_add_and_fetch(&db_queries_successful, 1);
        } else {
            __sync_add_and_fetch(&db_queries_failed, 1);
        }

        clock_gettime(CLOCK_MONOTONIC, &query_end_time);

        if (result_success && attempt_result) {
            time_t execution_time_us = (query_end_time.tv_sec - query_start_time.tv_sec) * 1000000 +
                                       (query_end_time.tv_nsec - query_start_time.tv_nsec) / 1000;
            attempt_result->execution_time_ms = execution_time_us;
        }

        if (result_success) {
            if (attempt > 1) {
                log_this(designator,
                         "database_engine_execute: query_id=%s succeeded on attempt %d/%d",
                         LOG_LEVEL_STATE, 2,
                         request->query_id ? request->query_id : "?",
                         attempt, total_attempts);
            }
            break;
        }

        /*
         * Failure path: inspect the result's error_class to decide
         * whether to retry. Default to DB_ERR_OTHER if the engine
         * did not set it (calloc-zeroes the field to DB_ERR_NONE,
         * which is the wrong default for a failure - bump to OTHER
         * so we never accidentally retry an unclassified error).
         */
        DatabaseErrorClass err_class = (attempt_result ? attempt_result->error_class
                                                        : DB_ERR_OTHER);
        if (!attempt_result) {
            // No result struct was returned at all - treat as transport
            // (the engine simply bailed before allocating a result).
            err_class = DB_ERR_TRANSPORT;
        } else if (err_class == DB_ERR_NONE) {
            err_class = DB_ERR_OTHER;
        }

        const char* err_class_name = (err_class == DB_ERR_TRANSPORT) ? "transport"
                                    : (err_class == DB_ERR_TIMEOUT) ? "timeout"
                                    : "other";
        const char* err_msg = (attempt_result && attempt_result->error_message)
                                  ? attempt_result->error_message
                                  : "(no error message)";

        bool retryable = (err_class == DB_ERR_TRANSPORT || err_class == DB_ERR_TIMEOUT)
                         && (attempt < total_attempts);

        if (retryable) {
            int backoff_seconds = 1 << (attempt - 1); // 1, 2, 4, 8, 16, 32...
            if (backoff_seconds > 30) {
                backoff_seconds = 30;
            }
            log_this(designator,
                     "database_engine_execute: query_id=%s attempt %d/%d failed (%s): %s - retrying in %ds",
                     LOG_LEVEL_ALERT, 4,
                     request->query_id ? request->query_id : "?",
                     attempt, total_attempts, err_class_name, err_msg, backoff_seconds);
            sleep((unsigned int)backoff_seconds);
            continue;
        }

        if (attempt < total_attempts) {
            log_this(designator,
                     "database_engine_execute: query_id=%s attempt %d/%d failed (%s, not retryable): %s",
                     LOG_LEVEL_DEBUG, 4,
                     request->query_id ? request->query_id : "?",
                     attempt, total_attempts, err_class_name, err_msg);
        } else {
            log_this(designator,
                     "database_engine_execute: query_id=%s failed after %d attempt(s) (%s): %s",
                     LOG_LEVEL_ERROR, 4,
                     request->query_id ? request->query_id : "?",
                     total_attempts, err_class_name, err_msg);
        }
        break;
    }

    *result = attempt_result;

    // Always deregister the watchdog entry, even on failure, so the
    // registry doesn't accumulate stale entries. Deregister is safe
    // with a NULL handle (no-op) for callers that didn't init the
    // watchdog.
    database_watchdog_deregister(watchdog_handle);

    return result_success;
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

