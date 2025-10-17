/*
 * Database Queue Processing Functions
 *
 * Implements worker thread and processing functions for the Hydrogen database subsystem.
 * Split from database_queue.c for better maintainability.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>

// Local includes
#include "dbqueue.h"

// External references
extern volatile sig_atomic_t database_stopping;

/*
 * Start a single worker thread for this queue
 */
bool database_queue_start_worker(DatabaseQueue* db_queue) {
    if (!db_queue) {
        log_this(SR_DATABASE, "Invalid database queue parameter", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Create DQM component name with full label for logging
    char* dqm_label = database_queue_generate_label(db_queue);
    log_this(dqm_label, "Starting worker thread", LOG_LEVEL_TRACE, 0);
    free(dqm_label);

    // Create the single worker thread
    if (pthread_create(&db_queue->worker_thread, NULL, database_queue_worker_thread, db_queue) != 0) {
        char* dqm_label_error = database_queue_generate_label(db_queue);
        log_this(dqm_label_error, "Failed to start worker thread", LOG_LEVEL_ERROR, 0);
        free(dqm_label_error);
        return false;
    }

    // Mark thread as started
    db_queue->worker_thread_started = true;

    // Register thread with thread tracking system
    char* thread_dqm_label = database_queue_generate_label(db_queue);
    add_service_thread_with_subsystem(&database_threads, db_queue->worker_thread, thread_dqm_label, NULL);
    free(thread_dqm_label);

    char* dqm_label_success = database_queue_generate_label(db_queue);
    // log_this(dqm_label_success, "Worker thread created and registered successfully", LOG_LEVEL_TRACE, 0);
    free(dqm_label_success);
    return true;
}

/*
 * Single generic worker thread function that works for all queue types
 */
void* database_queue_worker_thread(void* arg) {
    DatabaseQueue* db_queue = (DatabaseQueue*)arg;

    // Create DQM component name with full label for logging
    char* dqm_label = database_queue_generate_label(db_queue);

    // NOTE: This is what Test 30 (Database) is looking for
    log_this(dqm_label, "Worker thread started", LOG_LEVEL_TRACE, 0);

    // Start heartbeat monitoring immediately
    database_queue_start_heartbeat(db_queue);

    // For Lead queues, trigger the conductor pattern sequence once
    if (db_queue->is_lead_queue && !db_queue->conductor_sequence_completed) {
        // Sequence: Connect -> Bootstrap -> Migration -> Launch Queues -> Heartbeats -> Query Processing
        // Only establish connection if not already connected
        bool connection_ready = db_queue->is_connected;
        if (!connection_ready) {
            connection_ready = database_queue_lead_establish_connection(db_queue);
        }

        if (connection_ready) {
            if (database_queue_lead_run_bootstrap(db_queue)) {
                database_queue_lead_run_migration(db_queue);
                database_queue_lead_run_migration_test(db_queue);
                database_queue_lead_launch_additional_queues(db_queue);
                // database_queue_lead_manage_heartbeats(db_queue); // Disabled for now - causing mutex issues
                db_queue->conductor_sequence_completed = true; // Mark as completed to prevent re-execution
            }
        }
    }

    free(dqm_label);

    // Main worker loop - stay alive until shutdown is requested
    while (!db_queue->shutdown_requested && !database_stopping) {
        // Perform heartbeat check if interval has elapsed
        time_t current_time = time(NULL);
        if (current_time - db_queue->last_heartbeat >= db_queue->heartbeat_interval_seconds) {
            database_queue_perform_heartbeat(db_queue);
        }

        // Wait for work with a timeout to check shutdown periodically
        struct timespec timeout;
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += 1; // 1 second timeout

        // Try to wait for work signal
        if (sem_timedwait(&db_queue->worker_semaphore, &timeout) == 0) {
            // Check again if we should still process (avoid race conditions)
            if (!db_queue->shutdown_requested && !database_stopping) {
                // Process next query from this queue
                DatabaseQuery* query = database_queue_process_next(db_queue);
                if (query) {
                    // log_this(dqm_component, "%s queue processing query: %s", LOG_LEVEL_TRACE, 2, db_queue->queue_type, query->query_id ? query->query_id : "unknown");

                    // TODO: Actual database query execution will be implemented in Phase 2
                    // For now, just simulate processing time based on queue type
                    if (strcmp(db_queue->queue_type, "slow") == 0) {
                        usleep(5); // 500ms for slow queries
                    } else if (strcmp(db_queue->queue_type, "medium") == 0) {
                        usleep(2); // 200ms for medium queries
                    } else if (strcmp(db_queue->queue_type, "fast") == 0) {
                        usleep(5);  // 50ms for fast queries
                    } else if (strcmp(db_queue->queue_type, "cache") == 0) {
                        usleep(5);  // 10ms for cache queries
                    } else if (strcmp(db_queue->queue_type, "Lead") == 0) {
                        usleep(5); // 100ms for Lead queue queries
                        // Lead queue can also manage child queues here
                        if (db_queue->is_lead_queue) {
                            database_queue_manage_child_queues(db_queue);
                        }
                    }

                    // Clean up query
                    if (query->query_id) free(query->query_id);
                    if (query->query_template) free(query->query_template);
                    if (query->parameter_json) free(query->parameter_json);
                    if (query->error_message) free(query->error_message);
                    free(query);
                }
            }
        }
        // Continue loop - timeout is expected and normal
    }

    // Clean up thread tracking before exit
    remove_service_thread(&database_threads, pthread_self());

    char* dqm_label_exit = database_queue_generate_label(db_queue);
    log_this(dqm_label_exit, "Worker thread exiting", LOG_LEVEL_TRACE, 0);
    free(dqm_label_exit);
    return NULL;
}

/*
 * Manage child queues based on workload and configuration
 */
void database_queue_manage_child_queues(DatabaseQueue* lead_queue) {
    if (!lead_queue || !lead_queue->is_lead_queue) {
        return;
    }

    // TODO: Implement intelligent queue management based on:
    // - Current queue depths
    // - Processing rates
    // - Configuration settings
    // - System load

    // For now, this is a placeholder that could spawn queues based on simple rules
    // Real implementation will be added in Phase 2

    // Example logic (commented out for now):
    /*
    size_t lead_depth = queue_size(lead_queue->queue);
    if (lead_depth > 10) {
        // High load - consider spawning a medium queue if not exists
        database_queue_spawn_child_queue(lead_queue, "medium");
    }
    */

    // Commented out trace logging to reduce log noise during heartbeat
    // Create DQM component name with full label for logging
    // char* dqm_label = database_queue_generate_label(lead_queue);
    // log_this(dqm_label, "Lead queue managing child queues", LOG_LEVEL_TRACE, 0);
    // free(dqm_label);

    // Implement scaling logic based on queue utilization
    char* dqm_label = database_queue_generate_label(lead_queue);
    MutexResult lock_result = MUTEX_LOCK(&lead_queue->children_lock, dqm_label);
    if (lock_result == MUTEX_SUCCESS) {
        // Check each child queue for scaling decisions
        for (int i = 0; i < lead_queue->child_queue_count; i++) {
            if (lead_queue->child_queues[i]) {
                DatabaseQueue* child = lead_queue->child_queues[i];
                size_t queue_depth = database_queue_get_depth(child);

                // Scale up: if all queues of this type are non-empty
                if (queue_depth > 0) {
                    // Check if we can spawn another queue of this type
                    int same_type_count = 0;
                    for (int j = 0; j < lead_queue->child_queue_count; j++) {
                        if (lead_queue->child_queues[j] &&
                            strcmp(lead_queue->child_queues[j]->queue_type, child->queue_type) == 0) {
                            same_type_count++;
                        }
                    }

                    // If we have fewer than max queues of this type, consider scaling up
                    if (same_type_count < 3) {  // Configurable max per type
                        database_queue_spawn_child_queue(lead_queue, child->queue_type);
                    }
                }
                // Scale down: if all queues of this type are empty
                else {
                    // Count empty queues of this type
                    int empty_count = 0;
                    int total_count = 0;
                    for (int j = 0; j < lead_queue->child_queue_count; j++) {
                        if (lead_queue->child_queues[j] &&
                            strcmp(lead_queue->child_queues[j]->queue_type, child->queue_type) == 0) {
                            total_count++;
                            if (database_queue_get_depth(lead_queue->child_queues[j]) == 0) {
                                empty_count++;
                            }
                        }
                    }

                    // If all queues of this type are empty and we have more than minimum, scale down
                    if (empty_count == total_count && total_count > 1) {  // Keep at least 1
                        database_queue_shutdown_child_queue(lead_queue, child->queue_type);
                    }
                }
            }
        }

        mutex_unlock(&lead_queue->children_lock);
        free(dqm_label);
    }
}