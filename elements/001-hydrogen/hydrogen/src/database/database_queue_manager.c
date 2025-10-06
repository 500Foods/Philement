/*
 * Database Queue Manager Functions
 *
 * Implements queue manager operations for the Hydrogen database subsystem.
 * Split from database_queue.c for better maintainability.
 */

#include "../hydrogen.h"
#include <assert.h>

// Local includes
#include "database_queue.h"
#include "database.h"

// Global queue manager instance
DatabaseQueueManager* global_queue_manager = NULL;

/*
 * Initialize database queue infrastructure
 */
bool database_queue_system_init(void) {

    if (global_queue_manager != NULL) {
        return true;
    }

    // Ensure the global queue system is initialized first
    if (!queue_system_initialized) {
        queue_system_init();
    }

    // Create global queue manager with capacity for up to 8 databases
    global_queue_manager = database_queue_manager_create(8);
    if (!global_queue_manager) {
        log_this(SR_DATABASE, "Failed to create database queue manager", LOG_LEVEL_DEBUG, 0);
        return false;
    }

    return true;
}

/*
 * Clean shutdown of database queue infrastructure
 */
void database_queue_system_destroy(void) {
    log_this(SR_DATABASE, "Destroying database queue system", LOG_LEVEL_STATE, 0);

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
        log_this(SR_DATABASE, "Invalid parameters for add database", LOG_LEVEL_ERROR, 0);
        return false;
    }

    MutexResult lock_result = MUTEX_LOCK(&manager->manager_lock, SR_DATABASE);
    if (lock_result != MUTEX_SUCCESS) {
        log_this(SR_DATABASE, "Failed to lock manager mutex", LOG_LEVEL_ERROR, 0);
        return false;
    }

    if (manager->database_count >= manager->max_databases) {
        mutex_unlock(&manager->manager_lock);
        log_this(SR_DATABASE, "Cannot add database: maximum capacity reached", LOG_LEVEL_ALERT, 0);
        return false;
    }

    manager->databases[manager->database_count++] = db_queue;
    mutex_unlock(&manager->manager_lock);

    // Create DQM component name with full label for logging
    char* dqm_label = database_queue_generate_label(db_queue);
    // log_this(dqm_label, "Added to global queue manager", LOG_LEVEL_STATE, 0);
    free(dqm_label);
    return true;
}

/*
 * Get database queue by name from manager
 */
DatabaseQueue* database_queue_manager_get_database(DatabaseQueueManager* manager, const char* name) {
    if (!manager || !name) return NULL;

    MutexResult lock_result = MUTEX_LOCK(&manager->manager_lock, SR_DATABASE);
    if (lock_result != MUTEX_SUCCESS) {
        log_this(SR_DATABASE, "Failed to lock manager mutex for database lookup", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    for (size_t i = 0; i < manager->database_count; i++) {
        if (strcmp(manager->databases[i]->database_name, name) == 0) {
            DatabaseQueue* result = manager->databases[i];
            mutex_unlock(&manager->manager_lock);
            return result;
        }
    }
    mutex_unlock(&manager->manager_lock);

    return NULL;
}