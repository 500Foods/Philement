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
 * Create a Lead queue for a database - this is the primary queue that manages other queues
 */
DatabaseQueue* database_queue_create_lead(const char* database_name, const char* connection_string, const char* bootstrap_query) {
    log_this(SR_DATABASE, "Creating Lead DQM for: %s", LOG_LEVEL_DEBUG, 1, database_name);

    if (!database_name || !connection_string || strlen(database_name) == 0) {
        log_this(SR_DATABASE, "Invalid parameters for Lead DQM creation", LOG_LEVEL_DEBUG, 0);
        return NULL;
    }

    // Initialize the global queue system if not already done
    if (!queue_system_initialized) {
        queue_system_init();
    }

    DatabaseQueue* db_queue = malloc(sizeof(DatabaseQueue));
    if (!db_queue) {
        log_this(SR_DATABASE, "Failed to allocate Lead DQM for: %s", LOG_LEVEL_DEBUG, 1, database_name);
        return NULL;
    }

    memset(db_queue, 0, sizeof(DatabaseQueue));

    // Store database name and connection string
    db_queue->database_name = strdup(database_name);
    if (!db_queue->database_name) {
        free(db_queue);
        return NULL;
    }

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

    // Set queue type and role
    db_queue->queue_type = strdup("Lead");
    db_queue->is_lead_queue = true;
    db_queue->can_spawn_queues = true;

    // Initialize tag management - Lead starts with all tags
    db_queue->tags = strdup("LSMFC");  // Lead, Slow, Medium, Fast, Cache
    db_queue->queue_number = 0;  // Lead is always queue 00

    // Initialize heartbeat management
    db_queue->heartbeat_interval_seconds = 30;  // Default 30 seconds
    db_queue->last_heartbeat = 0;
    db_queue->last_connection_attempt = 0;

    // Create the Lead queue with descriptive name
    char lead_queue_name[256];
    snprintf(lead_queue_name, sizeof(lead_queue_name), "%s_lead", database_name);

    // Initialize queue attributes (required by queue_create)
    QueueAttributes queue_attrs = {0};
    db_queue->queue = queue_create(lead_queue_name, &queue_attrs);

    if (!db_queue->queue) {
        log_this(SR_DATABASE, "Failed to create Lead queue for: %s", LOG_LEVEL_DEBUG, 1, database_name);
        free(db_queue->queue_type);
        free(db_queue->connection_string);
        free(db_queue->database_name);
        free(db_queue);
        return NULL;
    }

    // Track memory allocation for the Lead queue
    track_queue_allocation(&database_queue_memory, sizeof(DatabaseQueue));

    // Initialize synchronization primitives
    if (pthread_mutex_init(&db_queue->queue_access_lock, NULL) != 0) {
        log_this(SR_DATABASE, "Failed to initialize queue access mutex for: %s", LOG_LEVEL_DEBUG, 1, database_name);
        database_queue_destroy(db_queue);
        return NULL;
    }

    if (sem_init(&db_queue->worker_semaphore, 0, 0) != 0) {
        log_this(SR_DATABASE, "Failed to initialize worker semaphore for: %s", LOG_LEVEL_DEBUG, 1, database_name);
        pthread_mutex_destroy(&db_queue->queue_access_lock);
        database_queue_destroy(db_queue);
        return NULL;
    }

    // Initialize Lead queue management
    if (pthread_mutex_init(&db_queue->children_lock, NULL) != 0) {
        log_this(SR_DATABASE, "Failed to initialize children mutex for: %s", LOG_LEVEL_DEBUG, 1, database_name);
        sem_destroy(&db_queue->worker_semaphore);
        pthread_mutex_destroy(&db_queue->queue_access_lock);
        database_queue_destroy(db_queue);
        return NULL;
    }

    // Allocate child queue array (max 4: slow, medium, fast, cache)
    db_queue->max_child_queues = 4;
    db_queue->child_queues = calloc((size_t)db_queue->max_child_queues, sizeof(DatabaseQueue*));
    if (!db_queue->child_queues) {
        log_this(SR_DATABASE, "Failed to allocate child queue array for: %s", LOG_LEVEL_DEBUG, 1, database_name);
        pthread_mutex_destroy(&db_queue->children_lock);
        sem_destroy(&db_queue->worker_semaphore);
        pthread_mutex_destroy(&db_queue->queue_access_lock);
        database_queue_destroy(db_queue);
        return NULL;
    }

    // Initialize connection lock for persistent connection management
    if (pthread_mutex_init(&db_queue->connection_lock, NULL) != 0) {
        log_this(SR_DATABASE, "Failed to initialize connection mutex for: %s", LOG_LEVEL_DEBUG, 1, database_name);
        pthread_mutex_destroy(&db_queue->children_lock);
        sem_destroy(&db_queue->worker_semaphore);
        pthread_mutex_destroy(&db_queue->queue_access_lock);
        database_queue_destroy(db_queue);
        return NULL;
    }

    // Initialize bootstrap completion synchronization (Lead queues only)
    if (pthread_mutex_init(&db_queue->bootstrap_lock, NULL) != 0) {
        log_this(SR_DATABASE, "Failed to initialize bootstrap mutex for: %s", LOG_LEVEL_DEBUG, 1, database_name);
        pthread_mutex_destroy(&db_queue->connection_lock);
        pthread_mutex_destroy(&db_queue->children_lock);
        sem_destroy(&db_queue->worker_semaphore);
        pthread_mutex_destroy(&db_queue->queue_access_lock);
        database_queue_destroy(db_queue);
        return NULL;
    }

    if (pthread_cond_init(&db_queue->bootstrap_cond, NULL) != 0) {
        log_this(SR_DATABASE, "Failed to initialize bootstrap condition variable for: %s", LOG_LEVEL_DEBUG, 1, database_name);
        pthread_mutex_destroy(&db_queue->bootstrap_lock);
        pthread_mutex_destroy(&db_queue->connection_lock);
        pthread_mutex_destroy(&db_queue->children_lock);
        sem_destroy(&db_queue->worker_semaphore);
        pthread_mutex_destroy(&db_queue->queue_access_lock);
        database_queue_destroy(db_queue);
        return NULL;
    }

    // Initialize initial connection attempt synchronization (Lead queues only)
    if (pthread_mutex_init(&db_queue->initial_connection_lock, NULL) != 0) {
        log_this(SR_DATABASE, "Failed to initialize initial connection mutex for: %s", LOG_LEVEL_DEBUG, 1, database_name);
        pthread_cond_destroy(&db_queue->bootstrap_cond);
        pthread_mutex_destroy(&db_queue->bootstrap_lock);
        pthread_mutex_destroy(&db_queue->connection_lock);
        pthread_mutex_destroy(&db_queue->children_lock);
        sem_destroy(&db_queue->worker_semaphore);
        pthread_mutex_destroy(&db_queue->queue_access_lock);
        database_queue_destroy(db_queue);
        return NULL;
    }

    if (pthread_cond_init(&db_queue->initial_connection_cond, NULL) != 0) {
        log_this(SR_DATABASE, "Failed to initialize initial connection condition variable for: %s", LOG_LEVEL_DEBUG, 1, database_name);
        pthread_mutex_destroy(&db_queue->initial_connection_lock);
        pthread_cond_destroy(&db_queue->bootstrap_cond);
        pthread_mutex_destroy(&db_queue->bootstrap_lock);
        pthread_mutex_destroy(&db_queue->connection_lock);
        pthread_mutex_destroy(&db_queue->children_lock);
        sem_destroy(&db_queue->worker_semaphore);
        pthread_mutex_destroy(&db_queue->queue_access_lock);
        database_queue_destroy(db_queue);
        return NULL;
    }

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

    return db_queue;
}

/*
 * Create a worker queue for a specific queue type (slow, medium, fast, cache)
 */
DatabaseQueue* database_queue_create_worker(const char* database_name, const char* connection_string, const char* queue_type) {
    log_this(SR_DATABASE, "Creating %s worker queue for database: %s", LOG_LEVEL_STATE, 2, queue_type, database_name);

    if (!database_name || !connection_string || !queue_type) {
        log_this(SR_DATABASE, "Invalid parameters for worker queue creation", LOG_LEVEL_ERROR, 0);
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

    // Queue number will be assigned when added to Lead queue
    db_queue->queue_number = -1;  // Will be set by Lead queue

    // Initialize heartbeat management
    db_queue->heartbeat_interval_seconds = 30;
    db_queue->last_heartbeat = 0;
    db_queue->last_connection_attempt = 0;

    // Create the worker queue
    char worker_queue_name[256];
    snprintf(worker_queue_name, sizeof(worker_queue_name), "%s_%s", database_name, queue_type);

    QueueAttributes queue_attrs = {0};
    db_queue->queue = queue_create(worker_queue_name, &queue_attrs);

    if (!db_queue->queue) {
        log_this(SR_DATABASE, "Failed to create %s worker queue", LOG_LEVEL_ERROR, 1, queue_type);
        free(db_queue->queue_type);
        free(db_queue->connection_string);
        free(db_queue->database_name);
        free(db_queue);
        return NULL;
    }

    // Initialize synchronization primitives (workers need these too)
    if (pthread_mutex_init(&db_queue->queue_access_lock, NULL) != 0 ||
        sem_init(&db_queue->worker_semaphore, 0, 0) != 0) {
        log_this(SR_DATABASE, "Failed to initialize synchronization for %s worker", LOG_LEVEL_ERROR, 1, queue_type);
        queue_destroy(db_queue->queue);
        free(db_queue->queue_type);
        free(db_queue->connection_string);
        free(db_queue->database_name);
        free(db_queue);
        return NULL;
    }

    // Initialize connection lock for persistent connection management
    if (pthread_mutex_init(&db_queue->connection_lock, NULL) != 0) {
        log_this(SR_DATABASE, "Failed to initialize connection mutex", LOG_LEVEL_ERROR, 0);
        sem_destroy(&db_queue->worker_semaphore);
        pthread_mutex_destroy(&db_queue->queue_access_lock);
        database_queue_destroy(db_queue);
        return NULL;
    }

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

    // Track memory allocation for the worker queue
    track_queue_allocation(&database_queue_memory, sizeof(DatabaseQueue));

    log_this(SR_DATABASE, "%s worker queue created successfully", LOG_LEVEL_DEBUG, 1, queue_type);
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
    // log_this(SR_DATABASE, "Queue manager creation completed successfully", LOG_LEVEL_STATE, 0);
    return manager;
}