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
 * Destroy database queue and all associated resources.
 * Returns false when a worker is still inside a call on this queue.
 * The object is left allocated in that case.
 */
bool database_queue_destroy(DatabaseQueue* db_queue) {
    if (!db_queue) return true;

    // Create DQM component name with full label for logging
    char* dqm_label = database_queue_generate_label(db_queue);
    log_this(dqm_label, "Destroying queue", LOG_LEVEL_TRACE, 0);

    // Wait for worker thread to finish (this will set shutdown_requested internally)
    database_queue_stop_worker(db_queue);

    // If this is a Lead queue, clean up child queues first.
    // A child that is still running keeps its object, and this lead
    // stays too, because the child may still be using the shared cache.
    bool child_still_running = false;
    if (db_queue->is_lead_queue && db_queue->child_queues) {
        MutexResult lock_result = MUTEX_LOCK(&db_queue->children_lock, SR_DATABASE);
        if (lock_result == MUTEX_SUCCESS) {
            for (int i = 0; i < db_queue->child_queue_count; i++) {
                DatabaseQueue* child = db_queue->child_queues[i];
                if (!child) {
                    continue;
                }
                if (database_queue_destroy(child)) {
                    db_queue->child_queues[i] = NULL;
                } else {
                    child_still_running = true;
                }
            }
            if (!child_still_running) {
                db_queue->child_queue_count = 0;
            }
            mutex_unlock(&db_queue->children_lock);
        } else if (db_queue->child_queue_count > 0) {
            child_still_running = true;
        }
    }

    if (db_queue->worker_thread_started || child_still_running) {
        log_this(dqm_label, "Worker still running; leaving this queue allocated", LOG_LEVEL_ERROR, 0);
        free(dqm_label);
        return false;
    }
    free(dqm_label);

    if (db_queue->is_lead_queue && db_queue->child_queues) {
        free(db_queue->child_queues);
        db_queue->child_queues = NULL;
        pthread_mutex_destroy(&db_queue->children_lock);
    }

    // Clean up the single queue
    // CRITICAL: Multiple DatabaseQueues may share the same underlying Queue* (via queue_create_with_label reuse)
    // Use proper reference counting to avoid leaks and double-free
    if (db_queue->queue) {
        queue_release(db_queue->queue);
        db_queue->queue = NULL;
    }

    // Clean up persistent connection BEFORE freeing strings (needs labels for logging)
    if (db_queue->persistent_connection) {
        database_engine_cleanup_connection(db_queue->persistent_connection);
        db_queue->persistent_connection = NULL;
    }

    // Clean up query cache if present (needs labels for logging)
    if (db_queue->query_cache) {
        char* cache_label = database_queue_generate_label(db_queue);
        query_cache_destroy(db_queue->query_cache, cache_label);
        free(cache_label);
        db_queue->query_cache = NULL;
    }

    // Clean up synchronization primitives
    pthread_mutex_destroy(&db_queue->queue_access_lock);
    pthread_cond_destroy(&db_queue->initial_connection_cond);
    pthread_mutex_destroy(&db_queue->initial_connection_lock);
    pthread_mutex_destroy(&db_queue->connection_lock);
    sem_destroy(&db_queue->worker_semaphore);

    // Free strings AFTER all operations that might need them for labels
    free(db_queue->database_name);
    free(db_queue->connection_string);
    free(db_queue->bootstrap_query);
    free(db_queue->queue_type);
    free(db_queue->tags);

    // Track memory deallocation for the database queue
    track_queue_deallocation(&database_queue_memory, sizeof(DatabaseQueue));

    free(db_queue);
    return true;
}

/*
 * Ask the engine to abort whatever this worker is blocked in.
 * Safe to call when the connection is idle. The watchdog uses the
 * same hook from another thread while a query is running.
 */
void database_queue_cancel_worker_query(DatabaseQueue* db_queue) {
    DatabaseHandle* connection;
    DatabaseEngineInterface* engine;

    if (!db_queue) {
        return;
    }
    connection = db_queue->persistent_connection;
    if (!connection || connection->engine_type >= DB_ENGINE_MAX) {
        return;
    }
    engine = database_engine_get(connection->engine_type);
    if (engine && engine->cancel_inflight) {
        engine->cancel_inflight(connection);
    }
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

    db_queue->shutdown_requested = true;

    // Wake worker thread only if it was started.
    // The loop only notices shutdown between queries. A remote query
    // can outlast the join. Cancel it, and do not claim the thread
    // has stopped until pthread_timedjoin_np says it has. Clearing
    // the flag early is what let destroy close the connection under
    // PQexec / mysql_real_query.
    if (db_queue->worker_thread_started) {
        struct timespec timeout;
        int join_result;

        sem_post(&db_queue->worker_semaphore);
        database_queue_cancel_worker_query(db_queue);

        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += 5;
        join_result = pthread_timedjoin_np(db_queue->worker_thread, NULL, &timeout);
        if (join_result == ETIMEDOUT) {
            log_this(dqm_label, "Worker thread did not exit within timeout", LOG_LEVEL_ALERT, 0);
            database_queue_cancel_worker_query(db_queue);
            clock_gettime(CLOCK_REALTIME, &timeout);
            timeout.tv_sec += 5;
            join_result = pthread_timedjoin_np(db_queue->worker_thread, NULL, &timeout);
        }

        if (join_result == 0) {
            db_queue->worker_thread = 0;
            db_queue->worker_thread_started = false;
            log_this(dqm_label, "Stopped worker thread", LOG_LEVEL_TRACE, 0);
        } else {
            log_this(dqm_label, "Worker thread still running after cancel", LOG_LEVEL_ERROR, 0);
        }
    } else {
        log_this(dqm_label, "Stopped worker thread", LOG_LEVEL_TRACE, 0);
    }

    free(dqm_label);
}