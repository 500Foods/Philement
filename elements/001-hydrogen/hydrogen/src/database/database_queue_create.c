/*
 * Database Queue Creation Functions
 *
 * Implements creation functions for the Hydrogen database subsystem.
 * Split from database_queue.c for better maintainability.
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

/*
 * Helper Functions for database_queue_create_worker
 */

// Allocate and initialize basic DatabaseQueue structure for worker
DatabaseQueue* database_queue_allocate_worker_basic(const char* database_name, const char* connection_string, const char* queue_type) {
    if (!database_name || !connection_string || !queue_type) {
        return NULL;
    }

    DatabaseQueue* db_queue = malloc(sizeof(DatabaseQueue));
    if (!db_queue) {
        return NULL;
    }

    memset(db_queue, 0, sizeof(DatabaseQueue));

    // Store database name, connection string, and queue type
    db_queue->database_name = strdup(database_name);
    db_queue->connection_string = strdup(connection_string);
    db_queue->queue_type = strdup(queue_type);

    if (!db_queue->database_name || !db_queue->connection_string || !db_queue->queue_type) {
        if (db_queue->database_name) free(db_queue->database_name);
        if (db_queue->connection_string) free(db_queue->connection_string);
        if (db_queue->queue_type) free(db_queue->queue_type);
        free(db_queue);
        return NULL;
    }

    return db_queue;
}

// Initialize worker queue specific properties
bool database_queue_init_worker_properties(DatabaseQueue* db_queue, const char* queue_type) {
    if (!db_queue || !queue_type) return false;

    // Set queue role (worker queues cannot spawn other queues)
    db_queue->is_lead_queue = false;
    db_queue->can_spawn_queues = false;

    // Initialize tag management - workers start with specific tag based on queue type
    char initial_tag[2] = {0};
    if (strcmp(queue_type, QUEUE_TYPE_SLOW) == 0) {
        initial_tag[0] = 'S';
    } else if (strcmp(queue_type, QUEUE_TYPE_MEDIUM) == 0) {
        initial_tag[0] = 'M';
    } else if (strcmp(queue_type, QUEUE_TYPE_FAST) == 0) {
        initial_tag[0] = 'F';
    } else if (strcmp(queue_type, QUEUE_TYPE_CACHE) == 0) {
        initial_tag[0] = 'C';
    }
    db_queue->tags = strdup(initial_tag);
    if (!db_queue->tags) return false;

    // Queue number will be assigned when added to Lead queue
    db_queue->queue_number = -1;  // Will be set by Lead queue

    // Initialize heartbeat management
    db_queue->heartbeat_interval_seconds = 30;
    db_queue->last_heartbeat = 0;
    db_queue->last_connection_attempt = 0;

    return true;
}

// Create the underlying queue structure for worker
bool database_queue_create_worker_underlying_queue(DatabaseQueue* db_queue, const char* database_name, const char* queue_type, const char* dqm_label) {
    if (!db_queue || !database_name || !queue_type) return false;

    const char* log_subsystem = dqm_label ? dqm_label : SR_DATABASE;

    // Create the worker queue
    char worker_queue_name[256];
    snprintf(worker_queue_name, sizeof(worker_queue_name), "%s_%s", database_name, queue_type);

    QueueAttributes queue_attrs = {0};
    db_queue->queue = queue_create_with_label(worker_queue_name, &queue_attrs, log_subsystem);

    if (!db_queue->queue) {
        log_this(log_subsystem, "Failed to create underlying queue for %s worker", LOG_LEVEL_ERROR, 1, queue_type);
        return false;
    }

    // Track memory allocation for the worker queue
    track_queue_allocation(&database_queue_memory, sizeof(DatabaseQueue));

    return true;
}

// Initialize synchronization primitives for worker queue
bool database_queue_init_worker_sync_primitives(DatabaseQueue* db_queue, const char* queue_type) {
    if (!db_queue || !queue_type) return false;

    // Initialize synchronization primitives (workers need these too)
    if (pthread_mutex_init(&db_queue->queue_access_lock, NULL) != 0 ||
        sem_init(&db_queue->worker_semaphore, 0, 0) != 0) {
        return false;
    }

    // Initialize connection lock for persistent connection management
    if (pthread_mutex_init(&db_queue->connection_lock, NULL) != 0) {
        sem_destroy(&db_queue->worker_semaphore);
        pthread_mutex_destroy(&db_queue->queue_access_lock);
        return false;
    }

    return true;
}

// Initialize final flags and statistics for worker queue
void database_queue_init_worker_final_flags(DatabaseQueue* db_queue) {
    if (!db_queue) return;

    // Initialize flags and statistics
    db_queue->shutdown_requested = false;
    db_queue->is_connected = false;
    db_queue->persistent_connection = NULL;
    db_queue->active_connections = 0;
    db_queue->total_queries_processed = 0;
    db_queue->current_queue_depth = 0;

    // No child queues for workers
    db_queue->child_queues = NULL;
    db_queue->child_queue_count = 0;
    db_queue->max_child_queues = 0;
}

/*
 * Create a worker queue for a specific queue type (slow, medium, fast, cache)
 * dqm_label: Optional DQM label for logging (uses SR_DATABASE if NULL)
 */
DatabaseQueue* database_queue_create_worker(const char* database_name, const char* connection_string, const char* queue_type, const char* dqm_label) {
    const char* log_subsystem = dqm_label ? dqm_label : SR_DATABASE;
    log_this(log_subsystem, "Creating %s worker queue for database: %s", LOG_LEVEL_TRACE, 2, queue_type, database_name);

    if (!database_name || !connection_string || !queue_type) {
        log_this(log_subsystem, "Invalid parameters for worker queue creation", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // Allocate and initialize basic structure
    DatabaseQueue* db_queue = database_queue_allocate_worker_basic(database_name, connection_string, queue_type);
    if (!db_queue) {
        return NULL;
    }

    // Initialize worker queue specific properties
    if (!database_queue_init_worker_properties(db_queue, queue_type)) {
        database_queue_destroy(db_queue);
        return NULL;
    }

    // Create the underlying queue structure
    if (!database_queue_create_worker_underlying_queue(db_queue, database_name, queue_type, dqm_label)) {
        log_this(log_subsystem, "Failed to create %s worker queue", LOG_LEVEL_ERROR, 1, queue_type);
        database_queue_destroy(db_queue);
        return NULL;
    }

    // Initialize synchronization primitives for worker queue
    if (!database_queue_init_worker_sync_primitives(db_queue, queue_type)) {
        log_this(log_subsystem, "Failed to initialize synchronization for %s worker", LOG_LEVEL_ERROR, 1, queue_type);
        database_queue_destroy(db_queue);
        return NULL;
    }

    // Initialize final flags and statistics
    database_queue_init_worker_final_flags(db_queue);

    log_this(log_subsystem, "%s worker queue created successfully", LOG_LEVEL_TRACE, 1, queue_type);
    return db_queue;
}

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