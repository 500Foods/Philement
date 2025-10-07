/*
 * Landing Database Subsystem
 *
 * This module handles the landing (shutdown) sequence for the database subsystem.
 * Manages proper shutdown of database threads and queues.
 * It provides functions for:
 * - Checking database landing readiness with thread count reporting
 * - Managing database configuration cleanup and queue shutdown
 */

// Global includes
#include "../hydrogen.h"

// Local includes
#include "landing.h"
#include "../database/database.h"
#include "../database/database_queue.h"
#include "../threads/threads.h"

// External references
extern DatabaseQueueManager* global_queue_manager;
extern ServiceThreads database_threads;

// Shutdown handler - defined in launch_database.h, implemented here
void shutdown_database(void) {
    // if (!database_stopping) {
    //     database_stopping = 1;
    //     log_this(SR_DATABASE, "Database subsystem shutting down", LOG_LEVEL_STATE, 0);
    // }
}

// Check if the database subsystem is ready to land
LaunchReadiness check_database_landing_readiness(void) {
    LaunchReadiness readiness = {0};
    readiness.subsystem = SR_DATABASE;

    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(5 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }

    // Add initial subsystem identifier
    readiness.messages[0] = strdup(SR_DATABASE);

    // Check if database is actually running
    if (!is_subsystem_running_by_name(SR_DATABASE)) {
        readiness.ready = false;
        readiness.messages[1] = strdup("  No-Go:   Database not running");
        readiness.messages[2] = strdup("  Decide:  No-Go For Landing of Database");
        readiness.messages[3] = NULL;
        readiness.messages[4] = NULL;
        return readiness;
    }

    // Count active database threads
    int active_threads = database_threads.thread_count;

    // Count active database queues
    int active_databases = 0;
    int total_child_queues = 0;
    if (global_queue_manager) {
        active_databases = (int)global_queue_manager->database_count;
        // Count child queues across all databases
        for (size_t i = 0; i < global_queue_manager->database_count; i++) {
            const DatabaseQueue* db_queue = global_queue_manager->databases[i];
            if (db_queue && db_queue->is_lead_queue) {
                total_child_queues += db_queue->child_queue_count;
            }
        }
    }

    // Database is ready for landing - report thread and queue counts
    readiness.ready = true;

    // Format thread count message
    char thread_msg[128];
    if (active_threads == 1) {
        snprintf(thread_msg, sizeof(thread_msg), "  Go:      %d database thread running", active_threads);
    } else {
        snprintf(thread_msg, sizeof(thread_msg), "  Go:      %d database threads running", active_threads);
    }
    readiness.messages[1] = strdup(thread_msg);

    // Format queue count message
    char queue_msg[128];
    if (active_databases == 1 && total_child_queues == 0) {
        snprintf(queue_msg, sizeof(queue_msg), "  Go:      %d database with %d worker queues", active_databases, total_child_queues + 1); // +1 for lead
    } else {
        snprintf(queue_msg, sizeof(queue_msg), "  Go:      %d databases with %d total queues", active_databases, active_databases + total_child_queues);
    }
    readiness.messages[2] = strdup(queue_msg);

    readiness.messages[3] = strdup("  Decide:  Go For Landing of Database");
    readiness.messages[4] = NULL;

    return readiness;
}

// Land the database subsystem
int land_database_subsystem(void) {
    log_this(SR_DATABASE, LOG_LINE_BREAK, LOG_LEVEL_STATE, 0);
    log_this(SR_DATABASE, "LANDING: DATABASE", LOG_LEVEL_STATE, 0);

    // Shut down all database queues and threads
    if (global_queue_manager) {
        log_this(SR_DATABASE, "Shutting down database queues", LOG_LEVEL_STATE, 0);

        // Iterate through all databases and shut down their child queues, then destroy the Lead queues
        for (size_t i = 0; i < global_queue_manager->database_count; i++) {
            DatabaseQueue* db_queue = global_queue_manager->databases[i];
            if (db_queue && db_queue->is_lead_queue) {
                char* dqm_label = database_queue_generate_label(db_queue);
                log_this(dqm_label, "Shutting down child queues", LOG_LEVEL_STATE, 0);

                // Shut down all child queues for this database
                // We need to collect the queue types first since shutdown modifies the array
                char* queue_types_to_shutdown[20]; // Max child queues
                int shutdown_count = 0;

                // Lock to safely access child queues and collect unique queue types
                MutexResult lock_result = MUTEX_LOCK(&db_queue->children_lock, dqm_label);
                if (lock_result == MUTEX_SUCCESS) {
                    for (int j = 0; j < db_queue->child_queue_count && shutdown_count < 20; j++) {
                        if (db_queue->child_queues[j] && db_queue->child_queues[j]->queue_type) {
                            // Check if we already have this queue type
                            bool already_collected = false;
                            for (int k = 0; k < shutdown_count; k++) {
                                if (strcmp(queue_types_to_shutdown[k], db_queue->child_queues[j]->queue_type) == 0) {
                                    already_collected = true;
                                    break;
                                }
                            }
                            if (!already_collected) {
                                queue_types_to_shutdown[shutdown_count++] = strdup(db_queue->child_queues[j]->queue_type);
                            }
                        }
                    }
                    mutex_unlock(&db_queue->children_lock);
                }

                // Now shut down each unique queue type
                for (int j = 0; j < shutdown_count; j++) {
                    log_this(dqm_label, "Shutting down %s queue", LOG_LEVEL_STATE, 1, queue_types_to_shutdown[j]);
                    database_queue_shutdown_child_queue(db_queue, queue_types_to_shutdown[j]);
                    free(queue_types_to_shutdown[j]);
                }

                log_this(dqm_label, "All child queues shut down", LOG_LEVEL_STATE, 0);

                // Clear the child queues array to prevent double destruction
                MutexResult clear_lock = MUTEX_LOCK(&db_queue->children_lock, dqm_label);
                if (clear_lock == MUTEX_SUCCESS) {
                    for (int j = 0; j < db_queue->child_queue_count; j++) {
                        db_queue->child_queues[j] = NULL;
                    }
                    db_queue->child_queue_count = 0;
                    mutex_unlock(&db_queue->children_lock);
                }

                free(dqm_label);

                // Now destroy the Lead queue itself (after its children are gone)
                database_queue_destroy(db_queue);
                global_queue_manager->databases[i] = NULL; // Mark as destroyed
            }
        }

        // Clean up the queue manager structure (but don't destroy the databases again)
        if (global_queue_manager) {
            MutexResult lock_result = MUTEX_LOCK(&global_queue_manager->manager_lock, SR_DATABASE);
            if (lock_result == MUTEX_SUCCESS) {
                free(global_queue_manager->databases);
                global_queue_manager->databases = NULL;
                global_queue_manager->database_count = 0;
                mutex_unlock(&global_queue_manager->manager_lock);
            }
            pthread_mutex_destroy(&global_queue_manager->manager_lock);
            free(global_queue_manager);
            global_queue_manager = NULL;
        }

        log_this(SR_DATABASE, "Database queue system destroyed", LOG_LEVEL_STATE, 0);
    } else {
        log_this(SR_DATABASE, "No database queue manager to shut down", LOG_LEVEL_STATE, 0);
    }

    // Clean up database configuration
    log_this(SR_DATABASE, "Cleaning up database configuration", LOG_LEVEL_STATE, 0);

    log_this(SR_DATABASE, "Database shutdown complete", LOG_LEVEL_STATE, 0);

    return 1;  // Return success
}
