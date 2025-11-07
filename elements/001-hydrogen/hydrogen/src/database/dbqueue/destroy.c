/*
 * Database Queue Destruction Functions
 *
 * Implements destruction and cleanup functions for the Hydrogen database subsystem.
 * Split from database_queue.c for better maintainability.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/utils/utils_queue.h>

// Local includes
#include "dbqueue.h"

/*
 * Destroy database queue and all associated resources
 */
void database_queue_destroy(DatabaseQueue* db_queue) {
    if (!db_queue) return;

    // Create DQM component name with full label for logging
    char* dqm_label = database_queue_generate_label(db_queue);
    log_this(dqm_label, "Destroying queue", LOG_LEVEL_TRACE, 0);
    free(dqm_label);

    // Wait for worker thread to finish (this will set shutdown_requested internally)
    database_queue_stop_worker(db_queue);

    // If this is a Lead queue, clean up child queues first
    if (db_queue->is_lead_queue && db_queue->child_queues) {
        MutexResult lock_result = MUTEX_LOCK(&db_queue->children_lock, SR_DATABASE);
        if (lock_result == MUTEX_SUCCESS) {
            for (int i = 0; i < db_queue->child_queue_count; i++) {
                if (db_queue->child_queues[i]) {
                    database_queue_destroy(db_queue->child_queues[i]);
                    db_queue->child_queues[i] = NULL;
                }
            }
            db_queue->child_queue_count = 0;
            mutex_unlock(&db_queue->children_lock);
        }

        free(db_queue->child_queues);
        pthread_mutex_destroy(&db_queue->children_lock);
    }

    // Clean up the single queue
    // CRITICAL: Multiple DatabaseQueues may share the same underlying Queue* (via queue_create_with_label reuse)
    // Check if this queue pointer is still valid before attempting to destroy it
    // This prevents double-free when multiple DatabaseQueues share the same Queue*
    if (db_queue->queue) {
        // Store queue pointer for comparison (don't access queue->name - could be freed!)
        Queue* queue_ptr = db_queue->queue;
        
        // Only destroy if queue pointer looks valid (basic sanity check)
        // A more robust check would require queue reference counting, but this prevents the worst crashes
        if ((uintptr_t)queue_ptr >= 0x1000) {
            queue_destroy(queue_ptr);
        }
        db_queue->queue = NULL;  // Clear pointer regardless
    }

    // Clean up synchronization primitives
    pthread_mutex_destroy(&db_queue->queue_access_lock);
    sem_destroy(&db_queue->worker_semaphore);

    // Free strings
    free(db_queue->database_name);
    free(db_queue->connection_string);
    free(db_queue->queue_type);
    free(db_queue->tags);

    // Track memory deallocation for the database queue
    track_queue_deallocation(&database_queue_memory, sizeof(DatabaseQueue));

    free(db_queue);
}

/*
 * Clean shutdown of queue manager and all managed databases
 */
void database_queue_manager_destroy(DatabaseQueueManager* manager) {
    if (!manager) return;

    manager->initialized = false;

    // Destroy all managed databases
    MutexResult lock_result = MUTEX_LOCK(&manager->manager_lock, SR_DATABASE);
    if (lock_result == MUTEX_SUCCESS) {
        for (size_t i = 0; i < manager->database_count; i++) {
            if (manager->databases[i]) {
                database_queue_destroy(manager->databases[i]);
            }
        }
        mutex_unlock(&manager->manager_lock);
    }

    // Clean up resources
    free(manager->databases);
    pthread_mutex_destroy(&manager->manager_lock);
    free(manager);
}

/*
 * Stop worker thread and wait for completion
 */
void database_queue_stop_worker(DatabaseQueue* db_queue) {
    if (!db_queue) return;

    // Create DQM component name with full label for logging
    char* dqm_label = database_queue_generate_label(db_queue);
    log_this(dqm_label, "Stopping worker thread", LOG_LEVEL_TRACE, 0);
    free(dqm_label);

    db_queue->shutdown_requested = true;

    // Wake worker thread only if it was started
    if (db_queue->worker_thread_started) {
        // Wake the worker thread via semaphore so it can see shutdown_requested
        sem_post(&db_queue->worker_semaphore);
        
        // Wait for thread to exit gracefully with timeout
        struct timespec timeout;
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += 5;  // 5 second timeout
        
        int join_result = pthread_timedjoin_np(db_queue->worker_thread, NULL, &timeout);
        if (join_result == ETIMEDOUT) {
            char* timeout_label = database_queue_generate_label(db_queue);
            log_this(timeout_label, "Worker thread did not exit within timeout", LOG_LEVEL_ALERT, 0);
            free(timeout_label);
        }
        
        db_queue->worker_thread = 0;  // Reset to indicate thread is stopped
        db_queue->worker_thread_started = false;  // Reset flag
    }

    char* dqm_label_stop = database_queue_generate_label(db_queue);
    log_this(dqm_label_stop, "Stopped worker thread", LOG_LEVEL_TRACE, 0);
    free(dqm_label_stop);
}