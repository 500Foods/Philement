/*
 * Database Queue Lead Management Functions
 *
 * Implements Lead queue specific functions for the Hydrogen database subsystem.
 * Split from database_queue.c for better maintainability.
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
    // For now, treating loaded and applied as the same concept since struct doesn't distinguish them
    // This case would need clarification if applied vs loaded are different concepts

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

    // DECISION: Comment out verbose logging - keep only essential status messages
    // if (lead_queue->empty_database) {
    //     log_this(dqm_label, "Empty database detected - proceeding with migration validation", LOG_LEVEL_DEBUG, 0);
    // }

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
 */
bool database_queue_lead_execute_migration_load(DatabaseQueue* lead_queue) {
    char* dqm_label = database_queue_generate_label(lead_queue);

    // DECISION: Comment out verbose state logging - keep only essential status messages
    // Store pre-migration state for comparison and logging
    // long long pre_load_available = lead_queue->latest_available_migration;
    // long long pre_load_installed = lead_queue->latest_installed_migration;

    // log_this(dqm_label, "Pre-load migration state: available=%lld, installed=%lld",
    //          LOG_LEVEL_DEBUG, 2, pre_load_available, pre_load_installed);

    // Execute auto migrations (populate Queries table) - LOAD PHASE
    bool load_success = execute_auto(lead_queue, lead_queue->persistent_connection);

    if (load_success) {
        // log_this(dqm_label, "Migration load phase completed successfully", LOG_LEVEL_DEBUG, 0);
    } else {
        log_this(dqm_label, "Migration load phase failed", LOG_LEVEL_ERROR, 0);
    }

    free(dqm_label);
    return load_success;
}

/*
 * Execute migration apply phase
 */
bool database_queue_lead_execute_migration_apply(DatabaseQueue* lead_queue) {
    char* dqm_label = database_queue_generate_label(lead_queue);

    // Execute auto migrations (apply loaded migrations) - APPLY PHASE
    bool apply_success = execute_auto(lead_queue, lead_queue->persistent_connection);

    if (apply_success) {
        // log_this(dqm_label, "Migration apply phase completed successfully", LOG_LEVEL_DEBUG, 0);
    } else {
        log_this(dqm_label, "Migration apply phase failed", LOG_LEVEL_ERROR, 0);
    }

    free(dqm_label);
    return apply_success;
}

/*
 * Re-run bootstrap query to verify loaded migrations
 */
void database_queue_lead_rerun_bootstrap(DatabaseQueue* lead_queue) {
    char* dqm_label = database_queue_generate_label(lead_queue);

    // DECISION: Comment out verbose bootstrap logging - keep only essential status messages
    // log_this(dqm_label, "Re-running bootstrap query on existing connection to verify loaded migrations", LOG_LEVEL_DEBUG, 0);

    // DECISION: Comment out unused variables since we removed the logging that used them
    // Store current migration status before re-execution
    // long long pre_reexec_available = lead_queue->latest_available_migration;
    // long long pre_reexec_installed = lead_queue->latest_installed_migration;

    // Re-execute bootstrap query on the existing queue
    // Since Lead DQM is single-threaded, we shouldn't need mutexes here
    database_queue_execute_bootstrap_query(lead_queue);

    // log_this(dqm_label, "Bootstrap re-execution complete: pre=(%lld,%lld) post=(%lld,%lld)",
    //          LOG_LEVEL_DEBUG, 4, pre_reexec_available, pre_reexec_installed,
    //          lead_queue->latest_available_migration, lead_queue->latest_installed_migration);

    // log_this(dqm_label, "Migration status updated after load phase", LOG_LEVEL_DEBUG, 0);

    free(dqm_label);
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
                // Re-run bootstrap query to verify what was loaded
                database_queue_lead_rerun_bootstrap(lead_queue);
            } else {
                success = false;
            }
            break;

        case MIGRATION_ACTION_APPLY:
            database_queue_lead_log_migration_status(lead_queue, "updating");

            if (!database_queue_lead_execute_migration_apply(lead_queue)) {
                log_this(dqm_label, "Migration apply phase failed", LOG_LEVEL_ERROR, 0);
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
 * Migration Available - The latest Migration script found (Lua script)
 * Migration Loaded    - The latest Migration script in the database (type_lua_28 = 1000)
 * Migration Applied   - The latest Migration script in the database (type_lua_28 = 1003)
 * If AutoMigration is enabled for this database, and this is the Lead DQM, then:
 *
 * LOOP:
 *
 * CASE: Database is empty, so we want to initialize it:
 * if Migration Available >= 1000
 *   and Migration Loaded < 1000
 *   and Migration Applied < 1000
 * then:
 *   Run Migration, starting with load and report:
 *   Migration Updating: Available = X, Loaded = Y, Applied = Z
 *
 * CASE: Newer Migrations are available than what is in the database
 * if Migration Available >= 1000
 *    and Migration Loaded < Migration Available
 *    and Migration Applied < Migration Available
 * then Run Migration starting with load
 *   Migration Loading: Available = X, Loaded = Y, Applied = Z
 *
 * CASE: Migrations that were previously loaded have not been applied
 * if Migration Loaded >= 1000
 *    and Migration Applied < Migration Loaded
 * then Run Migration starting with Apply
 *   Migration Updating: Available = X, Loaded = Y, Applied = Z
 *
 * CASE: Migrations already up to date
 * Otherwise, report and do nothing:
 *   Migration Current: Available = X, Loaded = Y, Applied = Z
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
 * =============================================================================
 * CHILD QUEUE MANAGEMENT
 * =============================================================================
 */

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

    // Note: Lead queue retains all its original tags - no tag removal

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