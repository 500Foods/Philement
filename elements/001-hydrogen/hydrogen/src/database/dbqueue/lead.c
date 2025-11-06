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
 *
 * NOTE: Migration implementation details (LOAD, APPLY, TEST) have been moved to:
 * - lead_load.c: Migration LOAD phase (populating Queries table from Lua scripts)
 * - lead_apply.c: Migration APPLY phase (forward migrations)
 * - lead_test.c: Migration TEST phase (reverse migrations for testing)
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

    // CRITICAL: Validate migrations FIRST to set latest_available_migration (AVAIL)
    // This must happen before bootstrap query so AVAIL is correct when bootstrap logs migration status
    database_queue_lead_validate_migrations(lead_queue);

    // Now execute the bootstrap query - it will correctly preserve AVAIL from validation
    // and extract LOAD and APPLY values from the database query results
    database_queue_execute_bootstrap_query(lead_queue);

    free(dqm_label);
    return true;
}

/*
 * Determine what migration action should be taken based on documented algorithm
 */
MigrationAction database_queue_lead_determine_migration_action(const DatabaseQueue* lead_queue) {
    // AVAIL: The highest number of Lua scripts available
    // LOAD: The highest number from Bootstrap query where type = 1000
    // APPLY: The highest number from Bootstrap query where type = 1003

    long long available = lead_queue->latest_available_migration;
    long long loaded = lead_queue->latest_loaded_migration;
    long long applied = lead_queue->latest_applied_migration;

    // CASE 0: Migrations are up to date - no action needed
    // if AVAIL = APPLY, our migration is up to date and we don't need to do anything
    if (available == applied) {
        return MIGRATION_ACTION_NONE;
    }

    // CASE 1: Database is empty, so we want to initialize it:
    // if AVAIL >= 1000 and LOAD < 1000
    if (available >= 1000 && loaded < 1000) {
        return MIGRATION_ACTION_LOAD;
    }

    // CASE 2: Newer Migrations are available than what is in the database
    // if AVAIL >= 1000 and LOAD < AVAIL
    if (available >= 1000 && loaded < available) {
        return MIGRATION_ACTION_LOAD;
    }

    // CASE 3: Migrations that were previously loaded have not been applied
    // if LOAD > APPLY (meaning there are loaded migrations that haven't been applied)
    if (loaded > applied) {
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
    long long loaded = lead_queue->latest_loaded_migration;
    long long applied = lead_queue->latest_applied_migration;

    if (strcmp(action, "current") == 0) {
        log_this(dqm_label, "Migration Current: AVAIL = %lld, LOAD = %lld, APPLY = %lld",
                 LOG_LEVEL_DEBUG, 3, available, loaded, applied);
    } else if (strcmp(action, "updating") == 0) {
        log_this(dqm_label, "Migration Updating: AVAIL = %lld, LOAD = %lld, APPLY = %lld",
                 LOG_LEVEL_DEBUG, 3, available, loaded, applied);
    } else if (strcmp(action, "loading") == 0) {
        log_this(dqm_label, "Migration Loading: AVAIL = %lld, LOAD = %lld, APPLY = %lld",
                 LOG_LEVEL_DEBUG, 3, available, loaded, applied);
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
 * Execute migration process according to the documented algorithm
 *
 * Migration Logic:
 * 1. Determine what action is needed (LOAD, APPLY, or NONE)
 * 2. If LOAD needed: populate Queries table with migration metadata
 * 3. If APPLY needed: execute loaded migrations (APPLY phase handles its own loop)
 * 4. APPLY phase stops when no more migrations or on error
 */
bool database_queue_lead_execute_migration_process(DatabaseQueue* lead_queue, char* dqm_label) {
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
                    if (!database_queue_lead_execute_migration_apply(lead_queue)) {
                        log_this(dqm_label, "Migration APPLY phase failed", LOG_LEVEL_ERROR, 0);
                        success = false;
                    }
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
 * AVAIL: The highest number of Lua scripts available
 * LOAD: The highest number from Bootstrap query where type = 1000
 * APPLY: The highest number from Bootstrap query where type = 1003
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

        // If no migration was run, ensure QTC is populated since this is the final step
        // (bootstrap query already ran and populated QTC, but this ensures it happens)
        log_this(dqm_label, "No migration needed - QTC already populated from bootstrap", LOG_LEVEL_DEBUG, 0);

        free(dqm_label);
        return true;
    }

    log_this(dqm_label, "Automatic Migration enabled", LOG_LEVEL_DEBUG, 0);

    // Execute migration process (handles LOAD and/or APPLY as needed)
    // APPLY phase has its own internal loop for applying multiple migrations
    bool success = database_queue_lead_execute_migration_process(lead_queue, dqm_label);
    
    if (!success) {
        log_this(dqm_label, "Migration process failed", LOG_LEVEL_ERROR, 0);
    }

    // End the migration timer
    if (migration_timer_running) {
        clock_gettime(CLOCK_MONOTONIC, &migration_end_time);
        double elapsed_seconds = calc_elapsed_time(&migration_end_time, &migration_start_time);
        log_this(dqm_label, "Migration completed in %.3fs", LOG_LEVEL_TRACE, 1, elapsed_seconds);
//        migration_timer_running = false;
    }

    // Migration process completed - QTC already populated by bootstrap queries during migration
    // This allows the application to continue even if migration failed, enabling troubleshooting
    log_this(dqm_label, "Migration process completed (%s) - QTC populated from bootstrap queries",
             LOG_LEVEL_DEBUG, 1, success ? "success" : "failed but continuing");

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

        // Acquire connection for migration test execution
        if (!database_queue_lead_acquire_migration_connection(lead_queue, dqm_label)) {
            log_this(dqm_label, "Failed to acquire connection for TestMigration", LOG_LEVEL_ERROR, 0);
            free(dqm_label);
            return false;
        }

        // Execute TestMigration process (reverses applied migrations)
        bool success = database_queue_lead_execute_migration_test_process(lead_queue, dqm_label);

        database_queue_lead_release_migration_connection(lead_queue);

        if (!success) {
            log_this(dqm_label, "TestMigration process failed", LOG_LEVEL_ERROR, 0);
        } else {
            log_this(dqm_label, "TestMigration completed successfully", LOG_LEVEL_DEBUG, 0);
        }
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

    // Wait for threads to actually stop before proceeding with destruction
    // Check thread status instead of using fixed delay for better reliability
    bool all_threads_stopped = false;
    int wait_iterations = 0;
    const int max_wait_iterations = 50; // Max 500ms (50 * 10ms)

    while (!all_threads_stopped && wait_iterations < max_wait_iterations) {
        all_threads_stopped = true;
        for (int i = 0; i < lead_queue->child_queue_count; i++) {
            if (lead_queue->child_queues[i] && lead_queue->child_queues[i]->worker_thread_started) {
                all_threads_stopped = false;
                break;
            }
        }
        if (!all_threads_stopped) {
            usleep(10000); // Wait 10ms before checking again
            wait_iterations++;
        }
    }

    if (!all_threads_stopped) {
        log_this(dqm_label, "Warning: Not all child queue threads stopped within timeout", LOG_LEVEL_ALERT, 0);
    }

    // Now acquire lock for destruction - should succeed now that threads are stopped
    lock_result = MUTEX_LOCK(&lead_queue->children_lock, dqm_label);
    if (lock_result != MUTEX_SUCCESS) {
        log_this(dqm_label, "Failed to acquire children_lock for shutdown after thread stop", LOG_LEVEL_ERROR, 0);
        free(dqm_label);
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
        free(dqm_label);
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

    log_this(dqm_label, "Shutdown %s child queue for database %s", LOG_LEVEL_TRACE, 2, queue_type, lead_queue->database_name);
    free(dqm_label);
    return true;
}
