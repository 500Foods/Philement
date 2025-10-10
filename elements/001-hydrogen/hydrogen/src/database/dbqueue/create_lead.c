/*
 * Database Lead Queue Creation - Core Implementation
 *
 * Core infrastructure functions for Lead database queue creation.
 * Contains memory allocation, property initialization, and synchronization primitives.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/utils/utils_queue.h>

// Local includes
#include "dbqueue.h"

/*
 * Core Infrastructure Functions
 */

// Allocate and initialize basic DatabaseQueue structure
DatabaseQueue* database_queue_allocate_basic(const char* database_name, const char* connection_string, const char* bootstrap_query) {
    if (!database_name || !connection_string) {
        return NULL;
    }

    DatabaseQueue* db_queue = malloc(sizeof(DatabaseQueue));
    if (!db_queue) {
        return NULL;
    }

    memset(db_queue, 0, sizeof(DatabaseQueue));

    // Store database name
    db_queue->database_name = strdup(database_name);
    if (!db_queue->database_name) {
        free(db_queue);
        return NULL;
    }

    // Store connection string
    db_queue->connection_string = strdup(connection_string);
    if (!db_queue->connection_string) {
        free(db_queue->database_name);
        free(db_queue);
        return NULL;
    }

    // Store bootstrap query if provided
    if (bootstrap_query) {
        db_queue->bootstrap_query = strdup(bootstrap_query);
        if (!db_queue->bootstrap_query) {
            free(db_queue->connection_string);
            free(db_queue->database_name);
            free(db_queue);
            return NULL;
        }
    }

    return db_queue;
}


// Initialize basic synchronization primitives (queue access and worker semaphore)
bool database_queue_init_basic_sync_primitives(DatabaseQueue* db_queue) {
    if (!db_queue) return false;

    if (pthread_mutex_init(&db_queue->queue_access_lock, NULL) != 0) {
        return false;
    }

    if (sem_init(&db_queue->worker_semaphore, 0, 0) != 0) {
        pthread_mutex_destroy(&db_queue->queue_access_lock);
        return false;
    }

    return true;
}

// Initialize children queue management primitives
bool database_queue_init_children_management(DatabaseQueue* db_queue) {
    if (!db_queue) return false;

    if (pthread_mutex_init(&db_queue->children_lock, NULL) != 0) {
        return false;
    }

    // Allocate child queue array (max 20: allow for scaling)
    db_queue->max_child_queues = 20;
    db_queue->child_queues = calloc((size_t)db_queue->max_child_queues, sizeof(DatabaseQueue*));
    if (!db_queue->child_queues) {
        pthread_mutex_destroy(&db_queue->children_lock);
        return false;
    }

    return true;
}

// Initialize connection management primitives
bool database_queue_init_connection_sync(DatabaseQueue* db_queue) {
    if (!db_queue) return false;

    if (pthread_mutex_init(&db_queue->connection_lock, NULL) != 0) {
        return false;
    }

    return true;
}

// Initialize bootstrap synchronization primitives
bool database_queue_init_bootstrap_sync(DatabaseQueue* db_queue) {
    if (!db_queue) return false;

    if (pthread_mutex_init(&db_queue->bootstrap_lock, NULL) != 0) {
        return false;
    }

    if (pthread_cond_init(&db_queue->bootstrap_cond, NULL) != 0) {
        pthread_mutex_destroy(&db_queue->bootstrap_lock);
        return false;
    }

    return true;
}

// Initialize initial connection synchronization primitives
bool database_queue_init_initial_connection_sync(DatabaseQueue* db_queue) {
    if (!db_queue) return false;

    if (pthread_mutex_init(&db_queue->initial_connection_lock, NULL) != 0) {
        return false;
    }

    if (pthread_cond_init(&db_queue->initial_connection_cond, NULL) != 0) {
        pthread_mutex_destroy(&db_queue->initial_connection_lock);
        return false;
    }

    return true;
}

// Initialize synchronization primitives for Lead queue
bool database_queue_init_lead_sync_primitives(DatabaseQueue* db_queue, const char* database_name) {
    if (!db_queue || !database_name) return false;

    // Initialize each group of primitives, cleaning up on failure
    if (!database_queue_init_basic_sync_primitives(db_queue)) {
        return false;
    }

    if (!database_queue_init_children_management(db_queue)) {
        sem_destroy(&db_queue->worker_semaphore);
        pthread_mutex_destroy(&db_queue->queue_access_lock);
        return false;
    }

    if (!database_queue_init_connection_sync(db_queue)) {
        free(db_queue->child_queues);
        pthread_mutex_destroy(&db_queue->children_lock);
        sem_destroy(&db_queue->worker_semaphore);
        pthread_mutex_destroy(&db_queue->queue_access_lock);
        return false;
    }

    if (!database_queue_init_bootstrap_sync(db_queue)) {
        pthread_mutex_destroy(&db_queue->connection_lock);
        free(db_queue->child_queues);
        pthread_mutex_destroy(&db_queue->children_lock);
        sem_destroy(&db_queue->worker_semaphore);
        pthread_mutex_destroy(&db_queue->queue_access_lock);
        return false;
    }

    if (!database_queue_init_initial_connection_sync(db_queue)) {
        pthread_cond_destroy(&db_queue->bootstrap_cond);
        pthread_mutex_destroy(&db_queue->bootstrap_lock);
        pthread_mutex_destroy(&db_queue->connection_lock);
        free(db_queue->child_queues);
        pthread_mutex_destroy(&db_queue->children_lock);
        sem_destroy(&db_queue->worker_semaphore);
        pthread_mutex_destroy(&db_queue->queue_access_lock);
        return false;
    }

    return true;
}


