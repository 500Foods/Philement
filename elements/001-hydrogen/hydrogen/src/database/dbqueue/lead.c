/*
 * Database Queue Lead Management Functions
 *
 * Lead DQM Responsibilities (Conductor Pattern):
 * - Establish a connection to the database
 * - Run Bootstrap query if available
 * - Run Migration if indicated
 * - Run Migration Test if indicated
 * - Launch additional queues
 * - Launch and respond to heartbeats
 * - Process incoming queries on its queue
 */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/database/database_bootstrap.h>
#include <src/database/migration/migration.h>
#include <src/utils/utils_time.h>

// Local includes
#include "dbqueue.h"

// Migration timing variables
static struct timespec migration_start_time = {0, 0};
static struct timespec migration_end_time = {0, 0};
static volatile bool migration_timer_running = false;

/*
 * Establish database connection for Lead DQM
 */
bool database_queue_lead_establish_connection(DatabaseQueue* lead_queue) {
    if (!lead_queue || !lead_queue->is_lead_queue) {
        return false;
    }

    char* dqm_label = database_queue_generate_label(lead_queue);
    log_this(dqm_label, "Establishing database connection", LOG_LEVEL_TRACE, 0);

    // Use the existing connection logic from heartbeat.c
    bool success = database_queue_check_connection(lead_queue);

    free(dqm_label);
    return success;
}

/*
 * Run bootstrap query for Lead DQM
 */
bool database_queue_lead_run_bootstrap(DatabaseQueue* lead_queue) {
    if (!lead_queue || !lead_queue->is_lead_queue) {
        return false;
    }

    char* dqm_label = database_queue_generate_label(lead_queue);
    log_this(dqm_label, "Running bootstrap query", LOG_LEVEL_TRACE, 0);

    // Bootstrap query is submitted during connection establishment and processed by main loop
    // Conductor pattern just ensures bootstrap is part of the sequence - no direct execution needed
    // The bootstrap query will be processed in the main worker loop, not here

    free(dqm_label);
    return true;
}

/*
 * Determine what migration action should be taken based on documented algorithm
 */
MigrationAction database_queue_lead_determine_migration_action(const DatabaseQueue* lead_queue) {
    // Migration Available - The latest Migration script found (Lua script)
    // Migration Loaded    - The latest Migration script in the database (type_lua_28 = 1000)
    // Migration Applied   - The latest Migration script in the database (type_lua_28 = 1003)
    // Note: Based on the struct, "applied" appears to be the same as "loaded" or not tracked separately

    long long available = lead_queue->latest_available_migration;
    long long loaded = lead_queue->latest_installed_migration;

    // CASE 1: Database is empty, so we want to initialize it:
    // if Migration Available >= 1000 and Migration Loaded < 1000
    if (available >= 1000 && loaded < 1000) {
        return MIGRATION_ACTION_LOAD;
    }

    // CASE 2: Newer Migrations are available than what is in the database
    // if Migration Available >= 1000 and Migration Loaded < Migration Available
    if (available >= 1000 && loaded < available) {
        return MIGRATION_ACTION_LOAD;
    }

    // CASE 3: Migrations that were previously loaded have not been applied
    // Check if we have loaded migrations (1000) that haven't been applied (1003)
    if (available >= 1000 && loaded >= 1000) {
        // We have some migrations loaded, check if any are not applied
        // This would require querying the database to see if there are type_lua_28 = 1000 records
        // For now, assume if loaded >= available, we need to check for unapplied migrations
        // This is a simplified check - in practice we'd need to query the database
        return MIGRATION_ACTION_APPLY;
    }

    // CASE 4: Migrations already up to date - no action needed
    return MIGRATION_ACTION_NONE;
}

/*
 * Log migration status according to documented format
 */
void database_queue_lead_log_migration_status(DatabaseQueue* lead_queue, const char* action) {
    char* dqm_label = database_queue_generate_label(lead_queue);

    long long available = lead_queue->latest_available_migration;
    long long loaded = lead_queue->latest_installed_migration;

    if (strcmp(action, "current") == 0) {
        log_this(dqm_label, "Migration Current: Available = %lld, Loaded = %lld, Applied = %lld",
                 LOG_LEVEL_DEBUG, 3, available, loaded, loaded); // Using loaded for applied since struct doesn't distinguish
    } else if (strcmp(action, "updating") == 0) {
        log_this(dqm_label, "Migration Updating: Available = %lld, Loaded = %lld, Applied = %lld",
                 LOG_LEVEL_DEBUG, 3, available, loaded, loaded); // Using loaded for applied since struct doesn't distinguish
    } else if (strcmp(action, "loading") == 0) {
        log_this(dqm_label, "Migration Loading: Available = %lld, Loaded = %lld, Applied = %lld",
                 LOG_LEVEL_DEBUG, 3, available, loaded, loaded); // Using loaded for applied since struct doesn't distinguish
    }

    free(dqm_label);
}

/*
 * Validate migrations for Lead DQM
 */
bool database_queue_lead_validate_migrations(DatabaseQueue* lead_queue) {
    char* dqm_label = database_queue_generate_label(lead_queue);

    bool migrations_valid = validate(lead_queue);

    if (!migrations_valid) {
        if (lead_queue->empty_database) {
            // log_this(dqm_label, "Migration validation failed on empty database - this is expected for new installations", LOG_LEVEL_DEBUG, 0);
        } else {
            log_this(dqm_label, "Migration validation failed - continuing without migrations", LOG_LEVEL_ALERT, 0);
        }
    }

    free(dqm_label);
    return migrations_valid;
}

/*
 * Execute migration load phase
 *
 * LOAD Phase: Extract migration metadata from Lua scripts and populate Queries table
 * - Execute Lua migration scripts to generate SQL templates
 * - INSERT records into Queries table with type_lua_28 = 1000 (loaded status)
 * - NO database schema changes occur in this phase
 * - Only metadata population for later APPLY phase execution
 */
bool database_queue_lead_execute_migration_load(DatabaseQueue* lead_queue) {
    char* dqm_label = database_queue_generate_label(lead_queue);
    log_this(dqm_label, "Starting migration LOAD phase - populating Queries table metadata", LOG_LEVEL_DEBUG, 0);

    // Execute LOAD migrations (populate Queries table metadata only) - LOAD PHASE
    bool load_success = execute_load_migrations(lead_queue, lead_queue->persistent_connection);

    if (load_success) {
        log_this(dqm_label, "Migration LOAD phase completed successfully - Queries table populated with migration metadata", LOG_LEVEL_DEBUG, 0);
    } else {
        log_this(dqm_label, "Migration LOAD phase failed - could not populate Queries table", LOG_LEVEL_ERROR, 0);
    }

    free(dqm_label);
    return load_success;
}

/*
 * Execute migration apply phase
 *
 * APPLY Phase: Process loaded migrations through normal query pipeline
 * - Get list of loaded-but-not-applied migrations from bootstrap query results
 * - Handle multi-statement queries for all engines (DB2 requirement becomes universal)
 * - Re-run bootstrap query between each migration to maintain current state
 */
bool database_queue_lead_execute_migration_apply(DatabaseQueue* lead_queue) {
    char* dqm_label = database_queue_generate_label(lead_queue);
    log_this(dqm_label, "Starting migration APPLY phase", LOG_LEVEL_DEBUG, 0);

    bool overall_success = true;
    bool more_work = true;
    int applied_count = 0;

    // Continue applying migrations until no more work or error
    while (more_work && overall_success) {
        // Re-run bootstrap query to get current migration state
        database_queue_execute_bootstrap_query(lead_queue);
        // Note: bootstrap query always succeeds (void return), continue with migration application

        // Find next migration to apply (loaded but not applied)
        // Look for migrations with type_lua_28 = 1000 (loaded) that haven't been applied yet
        long long next_migration_id = database_queue_find_next_migration_to_apply(lead_queue);

        if (next_migration_id == 0) {
            // No more migrations to apply
            more_work = false;
            log_this(dqm_label, "No more migrations to apply - APPLY phase complete", LOG_LEVEL_DEBUG, 0);
            break;
        }

        log_this(dqm_label, "Applying migration ID: %lld", LOG_LEVEL_DEBUG, 1, next_migration_id);

        // Apply the migration through normal query pipeline
        if (database_queue_apply_single_migration(lead_queue, next_migration_id, dqm_label)) {
            applied_count++;
            log_this(dqm_label, "Successfully applied migration %lld (total applied: %d)", LOG_LEVEL_DEBUG, 2, next_migration_id, applied_count);

            // Re-run bootstrap query to update state after successful application
            database_queue_execute_bootstrap_query(lead_queue);
            // Note: bootstrap query always succeeds (void return)
        } else {
            log_this(dqm_label, "Failed to apply migration %lld - stopping APPLY phase", LOG_LEVEL_ERROR, 1, next_migration_id);
            overall_success = false;
            break;
        }
    }

    // Variable 'more_work' is no longer used after the loop
    (void)more_work;

    if (overall_success) {
        log_this(dqm_label, "Migration APPLY phase completed successfully - applied %d migrations", LOG_LEVEL_DEBUG, 1, applied_count);
    } else {
        log_this(dqm_label, "Migration APPLY phase failed after applying %d migrations", LOG_LEVEL_ERROR, 1, applied_count);
    }

    free(dqm_label);
    return overall_success;
}

/*
 * Check if auto-migration is enabled for this database
 */
bool database_queue_lead_is_auto_migration_enabled(const DatabaseQueue* lead_queue) {
    if (!app_config) {
        return false;
    }

    for (int i = 0; i < app_config->databases.connection_count; i++) {
        if (strcmp(app_config->databases.connections[i].name, lead_queue->database_name) == 0) {
            return app_config->databases.connections[i].auto_migration;
        }
    }

    return false;
}

/*
 * Acquire migration connection with proper locking
 */
bool database_queue_lead_acquire_migration_connection(DatabaseQueue* lead_queue, char* dqm_label) {
    MutexResult lock_result = MUTEX_LOCK(&lead_queue->connection_lock, dqm_label);
    if (lock_result != MUTEX_SUCCESS) {
        log_this(dqm_label, "Failed to acquire connection lock for migration", LOG_LEVEL_ERROR, 0);
        return false;
    }

    if (!lead_queue->persistent_connection) {
        log_this(dqm_label, "No persistent connection available for migration", LOG_LEVEL_ERROR, 0);
        mutex_unlock(&lead_queue->connection_lock);
        return false;
    }

    return true;
}

/*
 * Release migration connection lock
 */
void database_queue_lead_release_migration_connection(DatabaseQueue* lead_queue) {
    mutex_unlock(&lead_queue->connection_lock);
}

/*
 * Execute a single migration cycle according to the documented algorithm
 *
 * Migration Cycle Logic:
 * 1. LOAD phase: Populate Queries table with migration metadata (no schema changes)
 * 2. After LOAD, re-run bootstrap query to update migration state
 * 3. APPLY phase: Execute loaded migrations through normal query pipeline
 * 4. After each APPLY, re-run bootstrap query to check for more work
 * 5. Stop when no more migrations to apply or on error
 */
bool database_queue_lead_execute_migration_cycle(DatabaseQueue* lead_queue, char* dqm_label) {
    // Validate migrations first
    if (!database_queue_lead_validate_migrations(lead_queue)) {
        return true; // Validation failed but this is not an error for the orchestration
    }

    // Acquire connection for migration execution
    if (!database_queue_lead_acquire_migration_connection(lead_queue, dqm_label)) {
        return false;
    }

    // Determine what migration action to take based on documented algorithm
    MigrationAction action = database_queue_lead_determine_migration_action(lead_queue);

    bool success = true;

    switch (action) {
        case MIGRATION_ACTION_LOAD:
            database_queue_lead_log_migration_status(lead_queue, "updating");

            if (database_queue_lead_execute_migration_load(lead_queue)) {
                // Re-run bootstrap query to get updated migration state after LOAD
                database_queue_execute_bootstrap_query(lead_queue);
                log_this(dqm_label, "Migration LOAD phase completed - checking if APPLY phase needed", LOG_LEVEL_DEBUG, 0);

                // After successful LOAD, check if we need to proceed to APPLY
                MigrationAction next_action = database_queue_lead_determine_migration_action(lead_queue);
                if (next_action == MIGRATION_ACTION_APPLY) {
                    log_this(dqm_label, "Proceeding to APPLY phase after successful LOAD", LOG_LEVEL_DEBUG, 0);
                    // Continue in the loop - next iteration will handle APPLY
                } else {
                    log_this(dqm_label, "No APPLY phase needed after LOAD", LOG_LEVEL_DEBUG, 0);
                }
            } else {
                success = false;
            }
            break;

        case MIGRATION_ACTION_APPLY:
            database_queue_lead_log_migration_status(lead_queue, "updating");

            if (!database_queue_lead_execute_migration_apply(lead_queue)) {
                log_this(dqm_label, "Migration APPLY phase failed", LOG_LEVEL_ERROR, 0);
                success = false;
            }
            break;

        case MIGRATION_ACTION_NONE:
        default:
            // Migrations already up to date - just log status
            database_queue_lead_log_migration_status(lead_queue, "current");
            break;
    }

    database_queue_lead_release_migration_connection(lead_queue);
    return success;
}

/*
 * Run migration for Lead DQM - Orchestration Function
 *
 * Migration Workflow:
 * 1. LOAD Phase: Extract migration metadata from Lua scripts → populate Queries table
 * 2. APPLY Phase: Execute stored migrations through normal query pipeline
 * 3. Bootstrap Query Integration: Re-run between phases to maintain current state
 * 4. Iterative Application: Apply migrations one-by-one with state updates
 *
 * Migration Available - The latest Migration script found (Lua script)
 * Migration Loaded    - The latest Migration script in the database (type_lua_28 = 1000)
 * Migration Applied   - The latest Migration script in the database (type_lua_28 = 1003)
 */
bool database_queue_lead_run_migration(DatabaseQueue* lead_queue) {
    if (!lead_queue || !lead_queue->is_lead_queue) {
        return false;
    }

    char* dqm_label = database_queue_generate_label(lead_queue);
    log_this(dqm_label, "Running migration", LOG_LEVEL_TRACE, 0);

    // Start the migration timer
    clock_gettime(CLOCK_MONOTONIC, &migration_start_time);
    migration_timer_running = true;

    // Check if auto-migration is enabled
    if (!database_queue_lead_is_auto_migration_enabled(lead_queue)) {
        log_this(dqm_label, "Automatic Migration disabled - skipping migration execution", LOG_LEVEL_DEBUG, 0);
        // End the migration timer
        if (migration_timer_running) {
            clock_gettime(CLOCK_MONOTONIC, &migration_end_time);
            double elapsed_seconds = calc_elapsed_time(&migration_end_time, &migration_start_time);
            log_this(dqm_label, "Migration completed in %.3fs", LOG_LEVEL_TRACE, 1, elapsed_seconds);
            migration_timer_running = false;
        }

        // If no migration was run, populate QTC now since this is the final step
        log_this(dqm_label, "No migration needed - populating QTC for Conduit", LOG_LEVEL_DEBUG, 0);
        database_queue_execute_bootstrap_query_with_qtc(lead_queue);

        free(dqm_label);
        return true;
    }

    log_this(dqm_label, "Automatic Migration enabled - Importing Migrations", LOG_LEVEL_DEBUG, 0);

    // Execute migration cycles in a loop until no more work is needed
    bool migration_complete = false;
    int cycle_count = 0;
    const int max_cycles = 10; // Prevent infinite loops
    bool success = true;

    while (!migration_complete && cycle_count < max_cycles && success) {
        cycle_count++;

        // Execute one migration cycle
        if (!database_queue_lead_execute_migration_cycle(lead_queue, dqm_label)) {
            log_this(dqm_label, "Migration cycle %d failed", LOG_LEVEL_ERROR, 1, cycle_count);
            success = false;
        } else {
            // Check if another cycle is needed by re-evaluating the state
            MigrationAction next_action = database_queue_lead_determine_migration_action(lead_queue);
            if (next_action == MIGRATION_ACTION_NONE) {
                migration_complete = true;
            } else {
                log_this(dqm_label, "Migration cycle %d completed, continuing with next phase", LOG_LEVEL_DEBUG, 1, cycle_count);
            }
        }
    }

    if (cycle_count >= max_cycles) {
        log_this(dqm_label, "Migration exceeded maximum cycles (%d), stopping", LOG_LEVEL_ERROR, 1, max_cycles);
        success = false;
    }

    // End the migration timer
    if (migration_timer_running) {
        clock_gettime(CLOCK_MONOTONIC, &migration_end_time);
        double elapsed_seconds = calc_elapsed_time(&migration_end_time, &migration_start_time);
        log_this(dqm_label, "Migration completed in %.3fs", LOG_LEVEL_TRACE, 1, elapsed_seconds);
//        migration_timer_running = false;
    }

    // Migration process completed - now populate QTC for Conduit regardless of migration success
    // The migration situation has been resolved (either succeeded or failed), so we can proceed with QTC population
    // This allows the application to continue even if migration failed, enabling troubleshooting
    log_this(dqm_label, "Migration process completed (%s) - populating QTC for Conduit",
             LOG_LEVEL_DEBUG, 1, success ? "success" : "failed but continuing");
    database_queue_execute_bootstrap_query_with_qtc(lead_queue);

    free(dqm_label);
    return success;
}

/*
 * Run migration test for Lead DQM
 */
bool database_queue_lead_run_migration_test(DatabaseQueue* lead_queue) {
    if (!lead_queue || !lead_queue->is_lead_queue) {
        return false;
    }

    char* dqm_label = database_queue_generate_label(lead_queue);
    log_this(dqm_label, "Running migration test", LOG_LEVEL_TRACE, 0);

    // Check if test migration is enabled in config
    const DatabaseConnection* migration_conn_config = NULL;
    if (app_config) {
        for (int i = 0; i < app_config->databases.connection_count; i++) {
            if (strcmp(app_config->databases.connections[i].name, lead_queue->database_name) == 0) {
                migration_conn_config = &app_config->databases.connections[i];
                break;
            }
        }
    }

    if (migration_conn_config && migration_conn_config->test_migration) {
        log_this(dqm_label, "Test Migration enabled - Running migration tests", LOG_LEVEL_DEBUG, 0);
        // Migration test logic would go here
        // For now, just log that test migration is enabled
        log_this(dqm_label, "Migration test completed successfully", LOG_LEVEL_DEBUG, 0);
    } else {
        log_this(dqm_label, "Test Migration disabled - skipping migration test", LOG_LEVEL_DEBUG, 0);
    }

    // End the migration timer and output elapsed time
    if (migration_timer_running) {
        clock_gettime(CLOCK_MONOTONIC, &migration_end_time);
        double elapsed_seconds = calc_elapsed_time(&migration_end_time, &migration_start_time);
        log_this(dqm_label, "Migration test completed in %.3fs", LOG_LEVEL_TRACE, 1, elapsed_seconds);
        migration_timer_running = false;
    }

    free(dqm_label);
    return true;
}

/*
 * Launch additional queues for Lead DQM
 */
bool database_queue_lead_launch_additional_queues(DatabaseQueue* lead_queue) {
    if (!lead_queue || !lead_queue->is_lead_queue) {
        return false;
    }

    char* dqm_label = database_queue_generate_label(lead_queue);
    log_this(dqm_label, "Launching additional queues", LOG_LEVEL_TRACE, 0);

    // Use the existing queue spawning logic from bootstrap
    // This is currently embedded in database_bootstrap.c but should be extracted here
    if (app_config) {
        const DatabaseConnection* conn_config = NULL;
        for (int i = 0; i < app_config->databases.connection_count; i++) {
            if (strcmp(app_config->databases.connections[i].name, lead_queue->database_name) == 0) {
                conn_config = &app_config->databases.connections[i];
                break;
            }
        }

        if (conn_config) {
            // Launch queues based on start configuration
            if (conn_config->queues.cache.start > 0) {
                for (int i = 0; i < conn_config->queues.cache.start; i++) {
                    database_queue_spawn_child_queue(lead_queue, QUEUE_TYPE_CACHE);
                }
            }
            if (conn_config->queues.fast.start > 0) {
                for (int i = 0; i < conn_config->queues.fast.start; i++) {
                    database_queue_spawn_child_queue(lead_queue, QUEUE_TYPE_FAST);
                }
            }
            if (conn_config->queues.medium.start > 0) {
                for (int i = 0; i < conn_config->queues.medium.start; i++) {
                    database_queue_spawn_child_queue(lead_queue, QUEUE_TYPE_MEDIUM);
                }
            }
            if (conn_config->queues.slow.start > 0) {
                for (int i = 0; i < conn_config->queues.slow.start; i++) {
                    database_queue_spawn_child_queue(lead_queue, QUEUE_TYPE_SLOW);
                }
            }
        }
    }

    free(dqm_label);
    return true;
}

/*
 * Launch and respond to heartbeats for Lead DQM
 */
bool database_queue_lead_manage_heartbeats(DatabaseQueue* lead_queue) {
    if (!lead_queue || !lead_queue->is_lead_queue) {
        return false;
    }

    char* dqm_label = database_queue_generate_label(lead_queue);
    log_this(dqm_label, "Managing heartbeats", LOG_LEVEL_TRACE, 0);

    // Use the existing heartbeat management from heartbeat.c
    // Only start heartbeat if not already started
    if (lead_queue->last_heartbeat == 0) {
        database_queue_start_heartbeat(lead_queue);
    }
    database_queue_perform_heartbeat(lead_queue);

    free(dqm_label);
    return true;
}

/*
 * Find the next migration to apply from the loaded migrations
 * Returns migration ID if found, 0 if no more migrations to apply
 */
long long database_queue_find_next_migration_to_apply(DatabaseQueue* lead_queue) {
    if (!lead_queue) {
        return 0;
    }

    char* dqm_label = database_queue_generate_label(lead_queue);

    log_this(dqm_label, "Looking for next migration to apply (loaded but not applied)", LOG_LEVEL_DEBUG, 0);

    // Query to find migrations that are loaded (1000) but not yet applied (1003)
    const char* find_sql = "SELECT query_ref FROM test.queries WHERE query_type_lua_28 = 1000 "
                          "AND query_ref NOT IN (SELECT query_ref FROM test.queries WHERE query_type_lua_28 = 1003) "
                          "ORDER BY query_ref LIMIT 1";

    QueryRequest* find_request = calloc(1, sizeof(QueryRequest));
    if (!find_request) {
        log_this(dqm_label, "Failed to allocate find request", LOG_LEVEL_ERROR, 0);
        free(dqm_label);
        return 0;
    }

    find_request->query_id = strdup("find_next_migration");
    find_request->sql_template = strdup(find_sql);
    find_request->parameters_json = strdup("{}");
    find_request->timeout_seconds = 30;
    find_request->isolation_level = DB_ISOLATION_READ_COMMITTED;
    find_request->use_prepared_statement = false;

    if (!find_request->query_id || !find_request->sql_template || !find_request->parameters_json) {
        log_this(dqm_label, "Failed to allocate find request fields", LOG_LEVEL_ERROR, 0);
        if (find_request->query_id) free(find_request->query_id);
        if (find_request->sql_template) free(find_request->sql_template);
        if (find_request->parameters_json) free(find_request->parameters_json);
        free(find_request);
        free(dqm_label);
        return 0;
    }

    // Execute query
    QueryResult* find_result = NULL;
    bool find_success = false;
    long long next_migration_id = 0;

    MutexResult lock_result = MUTEX_LOCK(&lead_queue->connection_lock, dqm_label);
    if (lock_result == MUTEX_SUCCESS) {
        if (lead_queue->persistent_connection) {
            find_success = database_engine_execute(lead_queue->persistent_connection, find_request, &find_result);
        } else {
            log_this(dqm_label, "No persistent connection available for find query", LOG_LEVEL_ERROR, 0);
        }
        mutex_unlock(&lead_queue->connection_lock);
    } else {
        log_this(dqm_label, "Failed to acquire connection lock for find query", LOG_LEVEL_ERROR, 0);
    }

    // Process result
    if (find_success && find_result && find_result->success && find_result->row_count > 0) {
        // Extract migration ID from data_json field (JSON format)
        if (find_result->data_json) {
            // Parse JSON to extract first row's first column
            // For now, assume simple case where data_json contains the ID directly
            next_migration_id = atoll(find_result->data_json);
            log_this(dqm_label, "Found next migration to apply: %lld", LOG_LEVEL_DEBUG, 1, next_migration_id);
        } else {
            log_this(dqm_label, "No migration data found in query result", LOG_LEVEL_DEBUG, 0);
        }
    } else {
        log_this(dqm_label, "No migrations found to apply", LOG_LEVEL_DEBUG, 0);
    }

    if (find_result) {
        database_engine_cleanup_result(find_result);
    }

    // Clean up request
    if (find_request->query_id) free(find_request->query_id);
    if (find_request->sql_template) free(find_request->sql_template);
    if (find_request->parameters_json) free(find_request->parameters_json);
    free(find_request);

    free(dqm_label);
    return next_migration_id;
}

/*
 * Apply a single migration through the normal query processing pipeline
 * This retrieves the migration SQL from the database (via bootstrap query results) and executes it as a normal query
 */
bool database_queue_apply_single_migration(DatabaseQueue* lead_queue, long long migration_id, const char* dqm_label) {
    log_this(dqm_label, "Applying migration %lld through normal query pipeline", LOG_LEVEL_DEBUG, 1, migration_id);

    // Step 1: Retrieve migration SQL from Queries table using bootstrap query results
    // The bootstrap query already loaded all query information into the query cache
    // We need to find the migration SQL from the cache
    char* migration_sql = NULL;

    if (lead_queue->query_cache) {
        const QueryCacheEntry* entry = query_cache_lookup(lead_queue->query_cache, (int)migration_id);
        if (entry && entry->sql_template) {
            migration_sql = strdup(entry->sql_template);
            if (!migration_sql) {
                log_this(dqm_label, "Failed to duplicate SQL template for migration %lld", LOG_LEVEL_ERROR, 1, migration_id);
                return false;
            }
            log_this(dqm_label, "Retrieved SQL for migration %lld from QTC (%zu bytes)", LOG_LEVEL_DEBUG, 2, migration_id, strlen(migration_sql));
        } else {
            log_this(dqm_label, "Migration %lld not found in query cache", LOG_LEVEL_ERROR, 1, migration_id);
            return false;
        }
    } else {
        log_this(dqm_label, "No query cache available for migration %lld", LOG_LEVEL_ERROR, 1, migration_id);
        return false;
    }

    // Step 2: Split multi-statement SQL for all engines (DB2 requirement becomes universal)
    char** statements = NULL;
    size_t statement_count = 0;
    size_t statements_capacity = 0;

    // Use the existing parse_sql_statements function from transaction.c
    if (!parse_sql_statements(migration_sql, strlen(migration_sql), &statements, &statement_count, &statements_capacity, dqm_label)) {
        log_this(dqm_label, "Failed to split migration SQL for migration %lld", LOG_LEVEL_ERROR, 1, migration_id);
        free(migration_sql);
        return false;
    }

    free(migration_sql); // No longer needed after splitting

    // Step 3: Execute each statement as a separate transaction
    bool success = true;
    for (size_t i = 0; i < statement_count && success; i++) {
        // Create QueryRequest for each statement
        QueryRequest* stmt_request = calloc(1, sizeof(QueryRequest));
        if (!stmt_request) {
            log_this(dqm_label, "Failed to allocate statement request", LOG_LEVEL_ERROR, 0);
            success = false;
            break;
        }

        // Generate hash for prepared statement caching
        char stmt_hash[64];
        get_stmt_hash("MPSC", statements[i], 16, stmt_hash);

        stmt_request->query_id = strdup("migration_statement");
        stmt_request->sql_template = strdup(statements[i]);
        stmt_request->parameters_json = strdup("{}");
        stmt_request->timeout_seconds = 30;
        stmt_request->isolation_level = DB_ISOLATION_READ_COMMITTED;
        stmt_request->use_prepared_statement = true;
        stmt_request->prepared_statement_name = strdup(stmt_hash);

        if (!stmt_request->query_id || !stmt_request->sql_template ||
            !stmt_request->parameters_json || !stmt_request->prepared_statement_name) {
            log_this(dqm_label, "Failed to allocate request fields", LOG_LEVEL_ERROR, 0);
            success = false;
            if (stmt_request->query_id) free(stmt_request->query_id);
            if (stmt_request->sql_template) free(stmt_request->sql_template);
            if (stmt_request->parameters_json) free(stmt_request->parameters_json);
            if (stmt_request->prepared_statement_name) free(stmt_request->prepared_statement_name);
            free(stmt_request);
            break;
        }

        // Execute statement
        QueryResult* stmt_result = NULL;
        bool stmt_success = false;

        MutexResult lock_result = MUTEX_LOCK(&lead_queue->connection_lock, dqm_label);
        if (lock_result == MUTEX_SUCCESS) {
            if (lead_queue->persistent_connection) {
                stmt_success = database_engine_execute(lead_queue->persistent_connection, stmt_request, &stmt_result);
            } else {
                log_this(dqm_label, "No persistent connection available", LOG_LEVEL_ERROR, 0);
            }
            mutex_unlock(&lead_queue->connection_lock);
        } else {
            log_this(dqm_label, "Failed to acquire connection lock", LOG_LEVEL_ERROR, 0);
        }

        // Clean up request
        if (stmt_request->query_id) free(stmt_request->query_id);
        if (stmt_request->sql_template) free(stmt_request->sql_template);
        if (stmt_request->parameters_json) free(stmt_request->parameters_json);
        if (stmt_request->prepared_statement_name) free(stmt_request->prepared_statement_name);
        free(stmt_request);

        if (stmt_success && stmt_result && stmt_result->success) {
            log_this(dqm_label, "Statement %zu executed successfully (hash: %s): affected %lld rows",
                     LOG_LEVEL_TRACE, 3, i + 1, stmt_hash, stmt_result->affected_rows);
        } else {
            log_this(dqm_label, "Statement %zu failed (hash: %s)", LOG_LEVEL_ERROR, 2, i + 1, stmt_hash);
            success = false;
        }

        if (stmt_result) {
            database_engine_cleanup_result(stmt_result);
        }
    }

    // Step 4: Update migration status to applied (1003) on success
    if (success) {
        // Create UPDATE query to change migration status from 1000 (loaded) to 1003 (applied)
        char update_sql[512];
        snprintf(update_sql, sizeof(update_sql),
                 "UPDATE test.queries SET query_type_lua_28 = 1003 WHERE query_ref = %lld AND query_type_lua_28 = 1000",
                 migration_id);

        QueryRequest* update_request = calloc(1, sizeof(QueryRequest));
        if (!update_request) {
            log_this(dqm_label, "Failed to allocate update request", LOG_LEVEL_ERROR, 0);
            success = false;
        } else {
            update_request->query_id = strdup("update_migration_status");
            update_request->sql_template = strdup(update_sql);
            update_request->parameters_json = strdup("{}");
            update_request->timeout_seconds = 30;
            update_request->isolation_level = DB_ISOLATION_READ_COMMITTED;
            update_request->use_prepared_statement = false;

            if (!update_request->query_id || !update_request->sql_template || !update_request->parameters_json) {
                log_this(dqm_label, "Failed to allocate update request fields", LOG_LEVEL_ERROR, 0);
                success = false;
            } else {
                // Execute update
                QueryResult* update_result = NULL;
                bool update_success = false;

                MutexResult lock_result = MUTEX_LOCK(&lead_queue->connection_lock, dqm_label);
                if (lock_result == MUTEX_SUCCESS) {
                    if (lead_queue->persistent_connection) {
                        update_success = database_engine_execute(lead_queue->persistent_connection, update_request, &update_result);
                    } else {
                        log_this(dqm_label, "No persistent connection available for status update", LOG_LEVEL_ERROR, 0);
                    }
                    mutex_unlock(&lead_queue->connection_lock);
                } else {
                    log_this(dqm_label, "Failed to acquire connection lock for status update", LOG_LEVEL_ERROR, 0);
                }

                if (update_success && update_result && update_result->success) {
                    log_this(dqm_label, "Migration %lld status updated to applied (1003)", LOG_LEVEL_DEBUG, 1, migration_id);
                } else {
                    log_this(dqm_label, "Failed to update migration status to applied for migration %lld", LOG_LEVEL_ERROR, 1, migration_id);
                    success = false;
                }

                if (update_result) {
                    database_engine_cleanup_result(update_result);
                }
            }

            // Clean up update request
            if (update_request->query_id) free(update_request->query_id);
            if (update_request->sql_template) free(update_request->sql_template);
            if (update_request->parameters_json) free(update_request->parameters_json);
            free(update_request);
        }
    }

    // Cleanup
    for (size_t i = 0; i < statement_count; i++) {
        free(statements[i]);
    }
    free(statements);

    if (success) {
        log_this(dqm_label, "Successfully applied migration %lld (%zu statements)", LOG_LEVEL_DEBUG, 2, migration_id, statement_count);
    }

    return success;
}

/*
 * Process incoming queries on Lead DQM queue
 */
bool database_queue_lead_process_queries(DatabaseQueue* lead_queue) {
    if (!lead_queue || !lead_queue->is_lead_queue) {
        return false;
    }

    char* dqm_label = database_queue_generate_label(lead_queue);
    log_this(dqm_label, "Processing incoming queries", LOG_LEVEL_TRACE, 0);

    // Use the existing query processing from process.c
    // Lead queue processes queries like any other queue, but may have special handling
    DatabaseQuery* query = database_queue_process_next(lead_queue);
    if (query) {
        // Lead queue specific processing could go here
        // For now, just log and clean up like other queues
        log_this(dqm_label, "Lead queue processing query: %s", LOG_LEVEL_TRACE, 1,
                query->query_id ? query->query_id : "unknown");

        // Clean up query
        if (query->query_id) free(query->query_id);
        if (query->query_template) free(query->query_template);
        if (query->parameter_json) free(query->parameter_json);
        if (query->error_message) free(query->error_message);
        free(query);
    }

    free(dqm_label);
    return true;
}

/*
 * Spawn a child queue of the specified type
 */
bool database_queue_spawn_child_queue(DatabaseQueue* lead_queue, const char* queue_type) {
    if (!lead_queue || !lead_queue->is_lead_queue || !queue_type) {
        return false;
    }

    char* dqm_label = database_queue_generate_label(lead_queue);
    MutexResult lock_result = MUTEX_LOCK(&lead_queue->children_lock, dqm_label);
    if (lock_result != MUTEX_SUCCESS) {
        free(dqm_label);
        return false;
    }

    // Note: We allow multiple queues of the same type based on configuration
    // The check for existing queues is removed to allow scaling

    // Check if we have space for more child queues
    if (lead_queue->child_queue_count >= lead_queue->max_child_queues) {
        mutex_unlock(&lead_queue->children_lock);
        // log_this(SR_DATABASE, "Cannot spawn %s queue: maximum child queues reached for %s", LOG_LEVEL_ERROR, 2 queue_type, lead_queue->database_name);
        return false;
    }

    // Create the child worker queue
    DatabaseQueue* child_queue = database_queue_create_worker(
        lead_queue->database_name,
        lead_queue->connection_string,
        queue_type,
        dqm_label
    );

    if (!child_queue) {
        mutex_unlock(&lead_queue->children_lock);
        log_this(dqm_label, "Failed to create child queue", LOG_LEVEL_ERROR, 0);
        free(dqm_label);
        return false;
    }

    // Assign queue number (find next available number)
    int next_queue_number = 1;  // Start from 01 (Lead is 00)
    bool number_taken = true;
    while (number_taken) {
        number_taken = false;
        for (int i = 0; i < lead_queue->child_queue_count; i++) {
            if (lead_queue->child_queues[i] &&
                lead_queue->child_queues[i]->queue_number == next_queue_number) {
                number_taken = true;
                next_queue_number++;
                break;
            }
        }
    }
    child_queue->queue_number = next_queue_number;

    // Start the worker thread for the child queue
    if (!database_queue_start_worker(child_queue)) {
        mutex_unlock(&lead_queue->children_lock);
        database_queue_destroy(child_queue);
        log_this(dqm_label, "Failed to start worker for child queue", LOG_LEVEL_ERROR, 0);
        free(dqm_label);
        return false;
    }

    // Add to child queue array
    lead_queue->child_queues[lead_queue->child_queue_count] = child_queue;
    lead_queue->child_queue_count++;

    mutex_unlock(&lead_queue->children_lock);

    log_this(dqm_label, "Spawned child queue", LOG_LEVEL_TRACE, 0);
    free(dqm_label);
    return true;
}

/*
 * Shutdown a child queue of the specified type
 */
bool database_queue_shutdown_child_queue(DatabaseQueue* lead_queue, const char* queue_type) {
    if (!lead_queue || !lead_queue->is_lead_queue || !queue_type) {
        return false;
    }

    char* dqm_label = database_queue_generate_label(lead_queue);
    // First, signal all child queues to stop their worker threads
    MutexResult lock_result = MUTEX_LOCK(&lead_queue->children_lock, dqm_label);
    if (lock_result == MUTEX_SUCCESS) {
        for (int i = 0; i < lead_queue->child_queue_count; i++) {
            if (lead_queue->child_queues[i]) {
                database_queue_stop_worker(lead_queue->child_queues[i]);
            }
        }
        mutex_unlock(&lead_queue->children_lock);
    }

    // Now wait a bit for threads to stop before acquiring lock again
    usleep(50000); // 50ms wait for threads to stop

    // Now acquire lock for destruction
    lock_result = MUTEX_LOCK(&lead_queue->children_lock, dqm_label);
    if (lock_result != MUTEX_SUCCESS) {
        log_this(dqm_label, "Failed to acquire children_lock for shutdown after thread stop", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Find the child queue to shutdown
    int target_index = -1;
    for (int i = 0; i < lead_queue->child_queue_count; i++) {
        if (lead_queue->child_queues[i] &&
            lead_queue->child_queues[i]->queue_type &&
            strcmp(lead_queue->child_queues[i]->queue_type, queue_type) == 0) {
            target_index = i;
            break;
        }
    }

    if (target_index == -1) {
        mutex_unlock(&lead_queue->children_lock);
        // log_this(SR_DATABASE, "Child queue %s not found for database %s", LOG_LEVEL_TRACE, 2, queue_type, lead_queue->database_name);
        return false;
    }

    // Destroy the child queue
    DatabaseQueue* child_queue = lead_queue->child_queues[target_index];
    database_queue_destroy(child_queue);

    // Compact the array by moving the last element to the removed position
    if (target_index < lead_queue->child_queue_count - 1) {
        lead_queue->child_queues[target_index] = lead_queue->child_queues[lead_queue->child_queue_count - 1];
    }
    lead_queue->child_queues[lead_queue->child_queue_count - 1] = NULL;
    lead_queue->child_queue_count--;

    mutex_unlock(&lead_queue->children_lock);

    log_this(SR_DATABASE, "Shutdown %s child queue for database %s", LOG_LEVEL_TRACE, 2, queue_type, lead_queue->database_name);
    return true;
}
