/*
 * Database Lead Queue Creation Functions
 *
 * Implements creation functions for Lead database queues in the Hydrogen database subsystem.
 * Split from database_queue_create.c for better maintainability.
 */

#include "../hydrogen.h"
#include <assert.h>

// Local includes
#include "database_queue.h"
#include "database.h"
#include "../utils/utils_queue.h"

/*
 * Helper Functions for database_queue_create_lead
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

// Initialize Lead queue specific properties
bool database_queue_init_lead_properties(DatabaseQueue* db_queue) {
    if (!db_queue) return false;

    // Set queue type and role
    db_queue->queue_type = strdup("Lead");
    if (!db_queue->queue_type) return false;

    db_queue->is_lead_queue = true;
    db_queue->can_spawn_queues = true;

    // Initialize tag management - Lead starts with all tags
    db_queue->tags = strdup("LSMFC");  // Lead, Slow, Medium, Fast, Cache
    if (!db_queue->tags) {
        free(db_queue->queue_type);
        return false;
    }

    db_queue->queue_number = 0;  // Lead is always queue 00

    // Initialize heartbeat management
    db_queue->heartbeat_interval_seconds = 30;  // Default 30 seconds
    db_queue->last_heartbeat = 0;
    db_queue->last_connection_attempt = 0;

    return true;
}

// Create the underlying queue structure
bool database_queue_create_underlying_queue(DatabaseQueue* db_queue, const char* database_name) {
    if (!db_queue || !database_name) return false;

    // Create the Lead queue with descriptive name
    char lead_queue_name[256];
    snprintf(lead_queue_name, sizeof(lead_queue_name), "%s_lead", database_name);

    // Initialize queue attributes (required by queue_create)
    QueueAttributes queue_attrs = {0};
    db_queue->queue = queue_create(lead_queue_name, &queue_attrs);

    if (!db_queue->queue) {
        return false;
    }

    // Track memory allocation for the Lead queue
    track_queue_allocation(&database_queue_memory, sizeof(DatabaseQueue));

    return true;
}

// Initialize synchronization primitives for Lead queue
bool database_queue_init_lead_sync_primitives(DatabaseQueue* db_queue, const char* database_name) {
    if (!db_queue || !database_name) return false;

    // Initialize synchronization primitives
    if (pthread_mutex_init(&db_queue->queue_access_lock, NULL) != 0) {
        return false;
    }

    if (sem_init(&db_queue->worker_semaphore, 0, 0) != 0) {
        pthread_mutex_destroy(&db_queue->queue_access_lock);
        return false;
    }

    // Initialize Lead queue management
    if (pthread_mutex_init(&db_queue->children_lock, NULL) != 0) {
        sem_destroy(&db_queue->worker_semaphore);
        pthread_mutex_destroy(&db_queue->queue_access_lock);
        return false;
    }

    // Allocate child queue array (max 20: allow for scaling)
    db_queue->max_child_queues = 20;
    db_queue->child_queues = calloc((size_t)db_queue->max_child_queues, sizeof(DatabaseQueue*));
    if (!db_queue->child_queues) {
        pthread_mutex_destroy(&db_queue->children_lock);
        sem_destroy(&db_queue->worker_semaphore);
        pthread_mutex_destroy(&db_queue->queue_access_lock);
        return false;
    }

    // Initialize connection lock for persistent connection management
    if (pthread_mutex_init(&db_queue->connection_lock, NULL) != 0) {
        free(db_queue->child_queues);
        pthread_mutex_destroy(&db_queue->children_lock);
        sem_destroy(&db_queue->worker_semaphore);
        pthread_mutex_destroy(&db_queue->queue_access_lock);
        return false;
    }

    // Initialize bootstrap completion synchronization (Lead queues only)
    if (pthread_mutex_init(&db_queue->bootstrap_lock, NULL) != 0) {
        pthread_mutex_destroy(&db_queue->connection_lock);
        free(db_queue->child_queues);
        pthread_mutex_destroy(&db_queue->children_lock);
        sem_destroy(&db_queue->worker_semaphore);
        pthread_mutex_destroy(&db_queue->queue_access_lock);
        return false;
    }

    if (pthread_cond_init(&db_queue->bootstrap_cond, NULL) != 0) {
        pthread_mutex_destroy(&db_queue->bootstrap_lock);
        pthread_mutex_destroy(&db_queue->connection_lock);
        free(db_queue->child_queues);
        pthread_mutex_destroy(&db_queue->children_lock);
        sem_destroy(&db_queue->worker_semaphore);
        pthread_mutex_destroy(&db_queue->queue_access_lock);
        return false;
    }

    // Initialize initial connection attempt synchronization (Lead queues only)
    if (pthread_mutex_init(&db_queue->initial_connection_lock, NULL) != 0) {
        pthread_cond_destroy(&db_queue->bootstrap_cond);
        pthread_mutex_destroy(&db_queue->bootstrap_lock);
        pthread_mutex_destroy(&db_queue->connection_lock);
        free(db_queue->child_queues);
        pthread_mutex_destroy(&db_queue->children_lock);
        sem_destroy(&db_queue->worker_semaphore);
        pthread_mutex_destroy(&db_queue->queue_access_lock);
        return false;
    }

    if (pthread_cond_init(&db_queue->initial_connection_cond, NULL) != 0) {
        pthread_mutex_destroy(&db_queue->initial_connection_lock);
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

// Initialize final flags and statistics for Lead queue
void database_queue_init_lead_final_flags(DatabaseQueue* db_queue) {
    if (!db_queue) return;

    // Initialize flags and statistics
    db_queue->shutdown_requested = false;
    db_queue->is_connected = false;
    db_queue->bootstrap_completed = false;
    db_queue->initial_connection_attempted = false;
    db_queue->persistent_connection = NULL;
    db_queue->active_connections = 0;
    db_queue->total_queries_processed = 0;
    db_queue->current_queue_depth = 0;
    db_queue->child_queue_count = 0;
}

/*
 * Create a Lead queue for a database - this is the primary queue that manages other queues
 */
DatabaseQueue* database_queue_create_lead(const char* database_name, const char* connection_string, const char* bootstrap_query) {
    log_this(SR_DATABASE, "Creating Lead DQM for: %s", LOG_LEVEL_TRACE, 1, database_name);

    if (!database_name || !connection_string || strlen(database_name) == 0) {
        log_this(SR_DATABASE, "Invalid parameters for Lead DQM creation", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // Initialize the global queue system if not already done
    if (!queue_system_initialized) {
        queue_system_init();
    }

    // Allocate and initialize basic structure
    DatabaseQueue* db_queue = database_queue_allocate_basic(database_name, connection_string, bootstrap_query);
    if (!db_queue) {
        log_this(SR_DATABASE, "Failed to allocate Lead DQM for: %s", LOG_LEVEL_ERROR, 1, database_name);
        return NULL;
    }

    // Initialize Lead queue specific properties
    if (!database_queue_init_lead_properties(db_queue)) {
        log_this(SR_DATABASE, "Failed to initialize Lead properties for: %s", LOG_LEVEL_ERROR, 1, database_name);
        database_queue_destroy(db_queue);
        return NULL;
    }

    // Create the underlying queue structure
    if (!database_queue_create_underlying_queue(db_queue, database_name)) {
        log_this(SR_DATABASE, "Failed to create Lead queue for: %s", LOG_LEVEL_ERROR, 1, database_name);
        database_queue_destroy(db_queue);
        return NULL;
    }

    // Initialize synchronization primitives for Lead queue
    if (!database_queue_init_lead_sync_primitives(db_queue, database_name)) {
        log_this(SR_DATABASE, "Failed to initialize Lead sync primitives for: %s", LOG_LEVEL_ERROR, 1, database_name);
        database_queue_destroy(db_queue);
        return NULL;
    }

    // Initialize final flags and statistics
    database_queue_init_lead_final_flags(db_queue);

    return db_queue;
}