/*
 * Database Queue Manager Creation Functions
 *
 * Implements creation functions for Database Queue Managers in the Hydrogen database subsystem.
 * Split from database_queue_create.c for better maintainability.
 */

#include "../../hydrogen.h"
#include <assert.h>

// Local includes
#include "database_queue.h"
#include "../database.h"
#include "../../utils/utils_queue.h"

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
