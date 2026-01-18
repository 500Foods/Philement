/*
 * Database Queue Manager Creation Functions
 *
 * Implements creation functions for Database Queue Managers in the Hydrogen database subsystem.
 * Split from database_queue_create.c for better maintainability.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/utils/utils_queue.h>

// Local includes
#include "dbqueue.h"

// Global queue manager instance
DatabaseQueueManager* global_queue_manager = NULL;

/*
 * Create queue manager to coordinate multiple databases
 */
DatabaseQueueManager* database_queue_manager_create(size_t max_databases) {
    DatabaseQueueManager* manager = malloc(sizeof(DatabaseQueueManager));
    if (!manager) {
        log_this(SR_DATABASE, "Failed to malloc queue manager", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    memset(manager, 0, sizeof(DatabaseQueueManager));

    manager->databases = calloc(max_databases, sizeof(DatabaseQueue*));
    if (!manager->databases) {
        log_this(SR_DATABASE, "Failed to calloc database array", LOG_LEVEL_ERROR, 0);
        free(manager);
        return NULL;
    }

    manager->database_count = 0;
    manager->max_databases = max_databases;
    manager->next_database_index = 0;

    // Initialize statistics
    manager->total_queries = 0;
    manager->successful_queries = 0;
    manager->failed_queries = 0;

    // Initialize DQM statistics
    database_queue_manager_init_stats(manager);

    if (pthread_mutex_init(&manager->manager_lock, NULL) != 0) {
        log_this(SR_DATABASE, "Failed to initialize manager mutex", LOG_LEVEL_ERROR, 0);
        free(manager->databases);
        free(manager);
        return NULL;
    }

    manager->initialized = true;
    // log_this(SR_DATABASE, "Queue manager creation completed successfully", LOG_LEVEL_TRACE, 0);
    return manager;
}

/*
 * Initialize the global database queue system
 */
bool database_queue_system_init(void) {
    if (global_queue_manager) {
        // Already initialized
        return true;
    }

    global_queue_manager = database_queue_manager_create(10);  // Default max 10 databases
    return global_queue_manager != NULL;
}

/*
 * Destroy the global database queue system
 */
void database_queue_system_destroy(void) {
    if (global_queue_manager) {
        database_queue_manager_destroy(global_queue_manager);
        global_queue_manager = NULL;
    }
}

/*
 * Add a database queue to the manager
 */
bool database_queue_manager_add_database(DatabaseQueueManager* manager, DatabaseQueue* db_queue) {
    if (!manager || !db_queue) {
        return false;
    }

    MutexResult lock_result = MUTEX_LOCK(&manager->manager_lock, SR_DATABASE);
    if (lock_result != MUTEX_SUCCESS) {
        return false;
    }

    // Find an available slot
    for (size_t i = 0; i < manager->max_databases; i++) {
        if (!manager->databases[i]) {
            manager->databases[i] = db_queue;
            manager->database_count++;
            mutex_unlock(&manager->manager_lock);
            return true;
        }
    }

    mutex_unlock(&manager->manager_lock);
    return false;  // No available slots
}

/*
 * Get a database queue from the manager by name
 */
DatabaseQueue* database_queue_manager_get_database(DatabaseQueueManager* manager, const char* name) {
    if (!manager || !name) {
        return NULL;
    }

    MutexResult lock_result = MUTEX_LOCK(&manager->manager_lock, SR_DATABASE);
    if (lock_result != MUTEX_SUCCESS) {
        return NULL;
    }

    // Find database by name
    for (size_t i = 0; i < manager->database_count; i++) {
        DatabaseQueue* db_queue = manager->databases[i];
        if (db_queue && strcmp(db_queue->database_name, name) == 0) {
            mutex_unlock(&manager->manager_lock);
            return db_queue;
        }
    }

    mutex_unlock(&manager->manager_lock);
    return NULL;  // Not found
}

/*
 * Initialize DQM statistics structure
 */
void database_queue_manager_init_stats(DatabaseQueueManager* manager) {
    if (!manager) return;

    memset(&manager->dqm_stats, 0, sizeof(DQMStatistics));

    // Initialize last_used timestamps to current time
    time_t now = time(NULL);
    for (int i = 0; i < 5; i++) {
        manager->dqm_stats.per_queue_stats[i].last_used = now;
    }
}

/*
 * Increment queue selection counter
 */
void database_queue_manager_increment_queue_selection(DatabaseQueueManager* manager, int queue_type_index) {
    if (!manager || queue_type_index < 0 || queue_type_index >= 5) return;

    __sync_fetch_and_add(&manager->dqm_stats.queue_selection_counters[queue_type_index], 1);
}

/*
 * Record query submission
 */
void database_queue_manager_record_query_submission(DatabaseQueueManager* manager, int queue_type_index) {
    if (!manager || queue_type_index < 0 || queue_type_index >= 5) return;

    __sync_fetch_and_add(&manager->dqm_stats.total_queries_submitted, 1);
    __sync_fetch_and_add(&manager->dqm_stats.per_queue_stats[queue_type_index].submitted, 1);
    manager->dqm_stats.per_queue_stats[queue_type_index].last_used = time(NULL);
}

/*
 * Record query completion with execution time
 */
void database_queue_manager_record_query_completion(DatabaseQueueManager* manager, int queue_type_index, unsigned long long execution_time_ms) {
    if (!manager || queue_type_index < 0 || queue_type_index >= 5) return;

    __sync_fetch_and_add(&manager->dqm_stats.total_queries_completed, 1);
    __sync_fetch_and_add(&manager->dqm_stats.per_queue_stats[queue_type_index].completed, 1);

    // Update average execution time (simple moving average)
    volatile unsigned long long* avg_ptr = &manager->dqm_stats.per_queue_stats[queue_type_index].avg_execution_time_ms;
    unsigned long long current_avg = *avg_ptr;
    unsigned long long total_completed = manager->dqm_stats.per_queue_stats[queue_type_index].completed;

    if (total_completed == 1) {
        *avg_ptr = execution_time_ms;
    } else {
        *avg_ptr = (current_avg * (total_completed - 1) + execution_time_ms) / total_completed;
    }
}

/*
 * Record query failure
 */
void database_queue_manager_record_query_failure(DatabaseQueueManager* manager, int queue_type_index) {
    if (!manager || queue_type_index < 0 || queue_type_index >= 5) return;

    __sync_fetch_and_add(&manager->dqm_stats.total_queries_failed, 1);
    __sync_fetch_and_add(&manager->dqm_stats.per_queue_stats[queue_type_index].failed, 1);
}

/*
 * Record timeout
 */
void database_queue_manager_record_timeout(DatabaseQueueManager* manager) {
    if (!manager) return;

    __sync_fetch_and_add(&manager->dqm_stats.total_timeouts, 1);
}

/*
 * Get DQM statistics as JSON
 */
json_t* database_queue_manager_get_stats_json(DatabaseQueueManager* manager) {
    if (!manager) return NULL;

    json_t* stats = json_object();

    // Overall statistics
    json_object_set_new(stats, "total_queries_submitted", json_integer((json_int_t)manager->dqm_stats.total_queries_submitted));
    json_object_set_new(stats, "total_queries_completed", json_integer((json_int_t)manager->dqm_stats.total_queries_completed));
    json_object_set_new(stats, "total_queries_failed", json_integer((json_int_t)manager->dqm_stats.total_queries_failed));
    json_object_set_new(stats, "total_timeouts", json_integer((json_int_t)manager->dqm_stats.total_timeouts));

    // Queue selection counters
    json_t* selection_counters = json_array();
    for (int i = 0; i < 5; i++) {
        json_array_append_new(selection_counters, json_integer((json_int_t)manager->dqm_stats.queue_selection_counters[i]));
    }
    json_object_set_new(stats, "queue_selection_counters", selection_counters);

    // Per-queue statistics
    json_t* per_queue = json_array();
    const char* queue_names[] = {"slow", "medium", "fast", "cache", "lead"};

    for (int i = 0; i < 5; i++) {
        json_t* queue_stat = json_object();
        json_object_set_new(queue_stat, "queue_type", json_string(queue_names[i]));
        json_object_set_new(queue_stat, "submitted", json_integer((json_int_t)manager->dqm_stats.per_queue_stats[i].submitted));
        json_object_set_new(queue_stat, "completed", json_integer((json_int_t)manager->dqm_stats.per_queue_stats[i].completed));
        json_object_set_new(queue_stat, "failed", json_integer((json_int_t)manager->dqm_stats.per_queue_stats[i].failed));
        json_object_set_new(queue_stat, "avg_execution_time_ms", json_integer((json_int_t)manager->dqm_stats.per_queue_stats[i].avg_execution_time_ms));
        json_object_set_new(queue_stat, "last_used", json_integer((json_int_t)manager->dqm_stats.per_queue_stats[i].last_used));
        json_array_append_new(per_queue, queue_stat);
    }
    json_object_set_new(stats, "per_queue_stats", per_queue);

    return stats;
}
