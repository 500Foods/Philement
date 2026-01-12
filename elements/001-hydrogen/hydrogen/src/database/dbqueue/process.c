/*
 * Database Queue Processing Functions
 *
 * Implements worker thread and processing functions for the Hydrogen database subsystem.
 * Split from database_queue.c for better maintainability.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/database/database_pending.h>

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
 * Helper function: Process a single query from the queue
 * Extracted for testability - can be called directly in unit tests
 */
void database_queue_process_single_query(DatabaseQueue* db_queue) {
    if (!db_queue) {
        return;
    }

    // Process next query from this queue
    DatabaseQuery* query = database_queue_process_next(db_queue);
    if (query) {
        char* dqm_label_exec = database_queue_generate_label(db_queue);
        
        // Execute actual database query if we have a persistent connection
        if (db_queue->persistent_connection && query->query_template) {
            // Create QueryRequest from DatabaseQuery
            QueryRequest request = {0};
            request.query_id = query->query_id;
            request.sql_template = query->query_template;
            request.parameters_json = query->parameter_json;
            request.timeout_seconds = 30; // Default timeout
            request.isolation_level = DB_ISOLATION_READ_COMMITTED;
            request.use_prepared_statement = false;
            request.prepared_statement_name = NULL;
            
            // Execute query using database engine
            QueryResult* result = NULL;
            bool success = database_engine_execute(db_queue->persistent_connection, &request, &result);
            
            if (success && result) {
                log_this(dqm_label_exec, "Query executed successfully: %s (rows: %zu, time: %ld ms)",
                        LOG_LEVEL_TRACE, 3,
                        query->query_id ? query->query_id : "unknown",
                        result->row_count,
                        result->execution_time_ms);

                // Signal pending result if this query was synchronous
                if (query->query_id) {
                    PendingResultManager* pending_mgr = get_pending_result_manager();
                    if (pending_mgr) {
                        pending_result_signal_ready(pending_mgr, query->query_id, result, dqm_label_exec);
                        result = NULL; // Ownership transferred to pending result
                    }
                }

                // Update query statistics
                if (database_subsystem) {
                    __sync_fetch_and_add(&database_subsystem->successful_queries, 1);
                }

                // Clean up result (only if not transferred to pending result)
                if (result) {
                    database_engine_cleanup_result(result);
                }
            } else {
                log_this(dqm_label_exec, "Query execution failed: %s",
                        LOG_LEVEL_TRACE, 1,
                        query->query_id ? query->query_id : "unknown");

                // Signal pending result with NULL result on failure
                if (query->query_id) {
                    PendingResultManager* pending_mgr = get_pending_result_manager();
                    if (pending_mgr) {
                        pending_result_signal_ready(pending_mgr, query->query_id, NULL, dqm_label_exec);
                    }
                }

                // Update failure statistics
                if (database_subsystem) {
                    __sync_fetch_and_add(&database_subsystem->failed_queries, 1);
                }
            }
        } else {
            // No persistent connection or query template - simulate processing time
            if (strcmp(db_queue->queue_type, "slow") == 0) {
                usleep(5);
            } else if (strcmp(db_queue->queue_type, "medium") == 0) {
                usleep(2);
            } else if (strcmp(db_queue->queue_type, "fast") == 0) {
                usleep(5);
            } else if (strcmp(db_queue->queue_type, "cache") == 0) {
                usleep(5);
            } else if (strcmp(db_queue->queue_type, "Lead") == 0) {
                usleep(5);
            }
        }
        
        // Lead queue can also manage child queues here
        if (db_queue->is_lead_queue) {
            database_queue_manage_child_queues(db_queue);
        }
        
        free(dqm_label_exec);

        // Clean up query
        if (query->query_id) free(query->query_id);
        if (query->query_template) free(query->query_template);
        if (query->parameter_json) free(query->parameter_json);
        if (query->error_message) free(query->error_message);
        free(query);
    }
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
                // Migration phases are independent:
                // - AutoMigration controls LOAD and APPLY phases
                // - TestMigration controls REVERSE (test) phase
                // They can be enabled/disabled independently
                database_queue_lead_run_migration(db_queue);         // Checks AutoMigration flag
                database_queue_lead_run_migration_test(db_queue);    // Checks TestMigration flag independently
                database_queue_lead_launch_additional_queues(db_queue);
                // database_queue_lead_manage_heartbeats(db_queue); // Disabled for now - causing mutex issues
                db_queue->conductor_sequence_completed = true; // Mark as completed to prevent re-execution
            }

        }

        // NOTE: This is needed by tests 32, 33, 34, and 35 as confirmation of Lead DQM Launch 
        log_this(dqm_label, "Lead DQM initialization is complete", LOG_LEVEL_DEBUG, 0);

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
                // Process query using extracted helper function
                database_queue_process_single_query(db_queue);
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
 *
 * NOTE: Auto-scaling is DISABLED to prevent race conditions.
 * The scale-up/scale-down logic was causing use-after-free bugs when:
 * 1. Scale-down destroys a child queue
 * 2. Concurrent threads still try to use the destroyed queue
 *
 * Child queues are spawned at startup based on config and remain until shutdown.
 * To re-enable auto-scaling, proper reference counting needs to be implemented.
 */
void database_queue_manage_child_queues(DatabaseQueue* lead_queue) {
    if (!lead_queue || !lead_queue->is_lead_queue) {
        return;
    }

    // AUTO-SCALING DISABLED - see note above
    // Child queues persist for the lifetime of the lead queue
    // This prevents race conditions where queues are destroyed while in use
    
    (void)lead_queue;  // Suppress unused warning
}