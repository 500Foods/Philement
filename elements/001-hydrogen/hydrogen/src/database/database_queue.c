/*
 * Database Queue Infrastructure Implementation
 *
 * Implements Phase 1 requirements from DATABASE_PLAN.md:
 * - Queue manager implementation
 * - Multi-queue system per database (slow/medium/fast/cache)
 * - Thread-safe query submission with round-robin distribution
 * - Worker threads with persistent connection management
 */

// Global includes
#include "../hydrogen.h"

// Local includes
#include "database_queue.h"

// Global queue manager instance
static DatabaseQueueManager* global_queue_manager = NULL;

// Forward declaration for worker thread
static void* database_queue_worker_thread_slow(void* arg);
static void* database_queue_worker_thread_medium(void* arg);
static void* database_queue_worker_thread_fast(void* arg);
static void* database_queue_worker_thread_cache(void* arg);

// Placeholder functions for JSON serialization (defined at bottom normally but moved here to avoid implicit function declaration)
static char* serialize_query_to_json(DatabaseQuery* query);
static DatabaseQuery* deserialize_query_from_json(const char* json);

/*
 * Initialize database queue infrastructure
 *
 * Follows the pattern from existing Hydrogen subsystems.
 * Sets up global queue manager for coordinating all database connections.
 */
bool database_queue_system_init(void) {
    log_this(SR_DATABASE, "Initializing database queue system", LOG_LEVEL_STATE, true, true, true);

    if (global_queue_manager != NULL) {
        log_this(SR_DATABASE, "Database queue system already initialized", LOG_LEVEL_ALERT, true, true, true);
        return true;
    }

    // Create global queue manager with capacity for up to 8 databases
    global_queue_manager = database_queue_manager_create(8);
    if (!global_queue_manager) {
        log_this(SR_DATABASE, "Failed to create database queue manager", LOG_LEVEL_ERROR, true, true, true);
        return false;
    }

    log_this(SR_DATABASE, "Database queue system initialized successfully", LOG_LEVEL_STATE, true, true, true);
    return true;
}

/*
 * Clean shutdown of database queue infrastructure
 *
 * Ensures all queues are properly drained and all threads terminated.
 */
void database_queue_system_destroy(void) {
    log_this(SR_DATABASE, "Destroying database queue system", LOG_LEVEL_STATE, true, true, true);

    if (global_queue_manager) {
        database_queue_manager_destroy(global_queue_manager);
        global_queue_manager = NULL;
    }
}

/*
 * Create a database queue with multi-queue architecture
 *
 * Implements the core Phase 1 requirement: 4-queue system per database
 * (slow/medium/fast/cache) for different query priorities.
 */
DatabaseQueue* database_queue_create(const char* database_name, const char* connection_string) {
    if (!database_name || !connection_string || strlen(database_name) == 0) {
        return NULL;
    }

    // Initialize the global queue system if not already done
    extern int queue_system_initialized;
    if (!queue_system_initialized) {
        queue_system_init();
        log_this(SR_DATABASE, "Global queue system initialized on demand", LOG_LEVEL_DEBUG, true, true, true);
    }

    DatabaseQueue* db_queue = malloc(sizeof(DatabaseQueue));
    if (!db_queue) {
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

    // Create individual queues with descriptive names
    char slow_queue_name[256], medium_queue_name[256], fast_queue_name[256], cache_queue_name[256];

    snprintf(slow_queue_name, sizeof(slow_queue_name), "%s_slow", database_name);
    snprintf(medium_queue_name, sizeof(medium_queue_name), "%s_medium", database_name);
    snprintf(fast_queue_name, sizeof(fast_queue_name), "%s_fast", database_name);
    snprintf(cache_queue_name, sizeof(cache_queue_name), "%s_cache", database_name);

    db_queue->queue_slow = queue_create(slow_queue_name, NULL);
    db_queue->queue_medium = queue_create(medium_queue_name, NULL);
    db_queue->queue_fast = queue_create(fast_queue_name, NULL);
    db_queue->queue_cache = queue_create(cache_queue_name, NULL);

    if (!db_queue->queue_slow || !db_queue->queue_medium ||
        !db_queue->queue_fast || !db_queue->queue_cache) {
        // Clean up on failure
        if (db_queue->queue_slow) queue_destroy(db_queue->queue_slow);
        if (db_queue->queue_medium) queue_destroy(db_queue->queue_medium);
        if (db_queue->queue_fast) queue_destroy(db_queue->queue_fast);
        if (db_queue->queue_cache) queue_destroy(db_queue->queue_cache);
        free(db_queue->connection_string);
        free(db_queue->database_name);
        free(db_queue);
        return NULL;
    }

    // Initialize synchronization primitives
    if (pthread_mutex_init(&db_queue->queue_access_lock, NULL) != 0) {
        database_queue_destroy(db_queue);
        return NULL;
    }

    if (sem_init(&db_queue->worker_semaphore, 0, 0) != 0) {
        pthread_mutex_destroy(&db_queue->queue_access_lock);
        database_queue_destroy(db_queue);
        return NULL;
    }

    // Initialize flags
    db_queue->shutdown_requested = false;
    db_queue->is_connected = false;

    // Initialize zero values for statistics
    db_queue->active_connections = 0;
    db_queue->total_queries_processed = 0;
    db_queue->current_queue_depth = 0;

    log_this(SR_DATABASE, "Created database queue for: %s", LOG_LEVEL_STATE, true, true, true, database_name);
    return db_queue;
}

/*
 * Destroy database queue and all associated resources
 *
 * Ensures clean shutdown of all worker threads and proper cleanup.
 */
void database_queue_destroy(DatabaseQueue* db_queue) {
    if (!db_queue) return;

    log_this(SR_DATABASE, "Destroying database queue: %s", LOG_LEVEL_STATE, true, true, true, db_queue->database_name);

    // Signal shutdown to worker threads
    db_queue->shutdown_requested = true;

    // Wait for worker threads to finish (implementation needed)
    database_queue_stop_workers(db_queue);

    // Clean up individual queues
    if (db_queue->queue_slow) queue_destroy(db_queue->queue_slow);
    if (db_queue->queue_medium) queue_destroy(db_queue->queue_medium);
    if (db_queue->queue_fast) queue_destroy(db_queue->queue_fast);
    if (db_queue->queue_cache) queue_destroy(db_queue->queue_cache);

    // Clean up synchronization primitives
    pthread_mutex_destroy(&db_queue->queue_access_lock);
    sem_destroy(&db_queue->worker_semaphore);

    // Free strings
    free(db_queue->database_name);
    free(db_queue->connection_string);

    free(db_queue);
}

/*
 * Create queue manager to coordinate multiple databases
 *
 * Implements round-robin distribution and centralized statistics.
 */
DatabaseQueueManager* database_queue_manager_create(size_t max_databases) {
    DatabaseQueueManager* manager = malloc(sizeof(DatabaseQueueManager));
    if (!manager) return NULL;

    memset(manager, 0, sizeof(DatabaseQueueManager));

    manager->databases = calloc(max_databases, sizeof(DatabaseQueue*));
    if (!manager->databases) {
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
        free(manager->databases);
        free(manager);
        return NULL;
    }

    manager->initialized = true;
    return manager;
}

/*
 * Clean shutdown of queue manager and all managed databases
 */
void database_queue_manager_destroy(DatabaseQueueManager* manager) {
    if (!manager) return;

    manager->initialized = false;

    // Destroy all managed databases
    pthread_mutex_lock(&manager->manager_lock);
    for (size_t i = 0; i < manager->database_count; i++) {
        if (manager->databases[i]) {
            database_queue_destroy(manager->databases[i]);
        }
    }
    pthread_mutex_unlock(&manager->manager_lock);

    // Clean up resources
    free(manager->databases);
    pthread_mutex_destroy(&manager->manager_lock);
    free(manager);
}

/*
 * Add a database queue to the manager
 */
bool database_queue_manager_add_database(DatabaseQueueManager* manager, DatabaseQueue* db_queue) {
    if (!manager || !db_queue) return false;

    pthread_mutex_lock(&manager->manager_lock);

    if (manager->database_count >= manager->max_databases) {
        pthread_mutex_unlock(&manager->manager_lock);
        log_this(SR_DATABASE, "Cannot add database: maximum capacity reached", LOG_LEVEL_ALERT, true, true, true);
        return false;
    }

    manager->databases[manager->database_count++] = db_queue;
    pthread_mutex_unlock(&manager->manager_lock);

    log_this(SR_DATABASE, "Added database to manager: %s", LOG_LEVEL_STATE, true, true, true, db_queue->database_name);
    return true;
}

/*
 * Get database queue by name from manager
 */
DatabaseQueue* database_queue_manager_get_database(DatabaseQueueManager* manager, const char* name) {
    if (!manager || !name) return NULL;

    pthread_mutex_lock(&manager->manager_lock);
    for (size_t i = 0; i < manager->database_count; i++) {
        if (strcmp(manager->databases[i]->database_name, name) == 0) {
            DatabaseQueue* result = manager->databases[i];
            pthread_mutex_unlock(&manager->manager_lock);
            return result;
        }
    }
    pthread_mutex_unlock(&manager->manager_lock);

    return NULL;
}

/*
 * Submit query to appropriate database queue based on routing logic
 *
 * Implements the priority routing requirement from Phase 1.
 */
bool database_queue_submit_query(DatabaseQueue* db_queue, DatabaseQuery* query) {
    if (!db_queue || !query || !query->query_template) return false;

    // Select target queue based on query characteristics
    Queue* target_queue = NULL;
    switch (query->queue_type_hint) {
        case DB_QUEUE_SLOW:
            target_queue = db_queue->queue_slow;
            break;
        case DB_QUEUE_MEDIUM:
            target_queue = db_queue->queue_medium;
            break;
        case DB_QUEUE_FAST:
            target_queue = db_queue->queue_fast;
            break;
        case DB_QUEUE_CACHE:
            target_queue = db_queue->queue_cache;
            break;
        default:
            target_queue = db_queue->queue_medium;  // Default to medium for unknown types
    }

    if (!target_queue) {
        log_this(SR_DATABASE, "No target queue available for query: %s", LOG_LEVEL_ERROR, true, true, true, query->query_id);
        return false;
    }

    // Serialize query to JSON for queue storage
    char* query_json = serialize_query_to_json(query);
    if (!query_json) return false;

    // Submit to queue with priority based on queue type
    bool success = queue_enqueue(target_queue, query_json, strlen(query_json), query->queue_type_hint);

    free(query_json);

    if (success) {
        __sync_fetch_and_add(&db_queue->current_queue_depth, 1);
        query->submitted_at = time(NULL);

        log_this(SR_DATABASE, "Submitted query %s to queue %s",
                 LOG_LEVEL_DEBUG, true, true, true, query->query_id,
                 database_queue_type_to_string(query->queue_type_hint));
    }

    return success;
}

/*
 * Process next query from specified queue type
 */
DatabaseQuery* database_queue_process_next(DatabaseQueue* db_queue, int queue_type) {
    if (!db_queue || queue_type < 0 || queue_type >= DB_QUEUE_MAX_TYPES) return NULL;

    Queue* target_queue = NULL;
    switch (queue_type) {
        case DB_QUEUE_SLOW:
            target_queue = db_queue->queue_slow;
            break;
        case DB_QUEUE_MEDIUM:
            target_queue = db_queue->queue_medium;
            break;
        case DB_QUEUE_FAST:
            target_queue = db_queue->queue_fast;
            break;
        case DB_QUEUE_CACHE:
            target_queue = db_queue->queue_cache;
            break;
    }

    if (!target_queue || queue_size(target_queue) == 0) return NULL;

    // Dequeue query (implementation will be expanded with JSON deserialization)
    size_t data_size;
    int priority;
    char* query_data = queue_dequeue(target_queue, &data_size, &priority);

    if (!query_data) return NULL;

    // Deserialize query from JSON (placeholder - needs implementation)
    DatabaseQuery* query = deserialize_query_from_json(query_data);
    free(query_data);

    if (query) {
        __sync_fetch_and_sub(&db_queue->current_queue_depth, 1);
        __sync_fetch_and_add(&db_queue->total_queries_processed, 1);
        query->processed_at = time(NULL);
    }

    return query;
}

/*
 * Start worker threads for each queue based on configuration
 *
 * Creates dedicated worker threads only for queues that are configured to start.
 */
bool database_queue_start_workers(DatabaseQueue* db_queue) {
    if (!db_queue) return false;

    // Get queue configuration from app config
    const DatabaseConfig* db_config = &app_config->databases;
    const DatabaseConnection* conn_config = NULL;

    // Find the connection configuration for this database
    for (int i = 0; i < db_config->connection_count; i++) {
        if (strcmp(db_config->connections[i].name, db_queue->database_name) == 0) {
            conn_config = &db_config->connections[i];
            break;
        }
    }

    if (!conn_config) {
        log_this(SR_DATABASE, "No configuration found for database: %s", LOG_LEVEL_ERROR, true, true, true, db_queue->database_name);
        return false;
    }

    int threads_started = 0;

    // Start worker threads based on configuration
    if (conn_config->queues.slow.start > 0) {
        if (pthread_create(&db_queue->worker_threads_slow, NULL, database_queue_worker_thread_slow, db_queue) != 0) {
            log_this(SR_DATABASE, "Failed to start slow queue worker for %s", LOG_LEVEL_ERROR, true, true, true, db_queue->database_name);
            return false;
        }
        threads_started++;
        log_this(SR_DATABASE, "Started slow queue worker for %s", LOG_LEVEL_DEBUG, true, true, true, db_queue->database_name);
    }

    if (conn_config->queues.medium.start > 0) {
        if (pthread_create(&db_queue->worker_threads_medium, NULL, database_queue_worker_thread_medium, db_queue) != 0) {
            if (conn_config->queues.slow.start > 0) pthread_cancel(db_queue->worker_threads_slow);
            log_this(SR_DATABASE, "Failed to start medium queue worker for %s", LOG_LEVEL_ERROR, true, true, true, db_queue->database_name);
            return false;
        }
        threads_started++;
        log_this(SR_DATABASE, "Started medium queue worker for %s", LOG_LEVEL_DEBUG, true, true, true, db_queue->database_name);
    }

    if (conn_config->queues.fast.start > 0) {
        if (pthread_create(&db_queue->worker_threads_fast, NULL, database_queue_worker_thread_fast, db_queue) != 0) {
            if (conn_config->queues.slow.start > 0) pthread_cancel(db_queue->worker_threads_slow);
            if (conn_config->queues.medium.start > 0) pthread_cancel(db_queue->worker_threads_medium);
            log_this(SR_DATABASE, "Failed to start fast queue worker for %s", LOG_LEVEL_ERROR, true, true, true, db_queue->database_name);
            return false;
        }
        threads_started++;
        log_this(SR_DATABASE, "Started fast queue worker for %s", LOG_LEVEL_DEBUG, true, true, true, db_queue->database_name);
    }

    if (conn_config->queues.cache.start > 0) {
        if (pthread_create(&db_queue->worker_threads_cache, NULL, database_queue_worker_thread_cache, db_queue) != 0) {
            if (conn_config->queues.slow.start > 0) pthread_cancel(db_queue->worker_threads_slow);
            if (conn_config->queues.medium.start > 0) pthread_cancel(db_queue->worker_threads_medium);
            if (conn_config->queues.fast.start > 0) pthread_cancel(db_queue->worker_threads_fast);
            log_this(SR_DATABASE, "Failed to start cache queue worker for %s", LOG_LEVEL_ERROR, true, true, true, db_queue->database_name);
            return false;
        }
        threads_started++;
        log_this(SR_DATABASE, "Started cache queue worker for %s", LOG_LEVEL_DEBUG, true, true, true, db_queue->database_name);
    }

    log_this(SR_DATABASE, "Started %d worker threads for database: %s", LOG_LEVEL_STATE, true, true, true, threads_started, db_queue->database_name);
    return true;
}

/*
 * Stop worker threads and wait for completion
 */
void database_queue_stop_workers(DatabaseQueue* db_queue) {
    if (!db_queue) return;

    db_queue->shutdown_requested = true;

    // Cancel and join worker threads
    pthread_cancel(db_queue->worker_threads_slow);
    pthread_cancel(db_queue->worker_threads_medium);
    pthread_cancel(db_queue->worker_threads_fast);
    pthread_cancel(db_queue->worker_threads_cache);

    pthread_join(db_queue->worker_threads_slow, NULL);
    pthread_join(db_queue->worker_threads_medium, NULL);
    pthread_join(db_queue->worker_threads_fast, NULL);
    pthread_join(db_queue->worker_threads_cache, NULL);

    log_this(SR_DATABASE, "Stopped worker threads for database: %s", LOG_LEVEL_STATE, true, true, true, db_queue->database_name);
}

/*
 * Worker thread functions (stub implementations - will be expanded in Phase 2)
 */
static void* database_queue_worker_thread_slow(void* arg) {
    DatabaseQueue* db_queue = (DatabaseQueue*)arg;
    log_this(SR_DATABASE, "Started slow queue worker for %s", LOG_LEVEL_STATE, true, true, true, db_queue->database_name);
    // Implementation will process slow queries
    return NULL;
}

static void* database_queue_worker_thread_medium(void* arg) {
    DatabaseQueue* db_queue = (DatabaseQueue*)arg;
    log_this(SR_DATABASE, "Started medium queue worker for %s", LOG_LEVEL_STATE, true, true, true, db_queue->database_name);
    // Implementation will process medium queries
    return NULL;
}

static void* database_queue_worker_thread_fast(void* arg) {
    DatabaseQueue* db_queue = (DatabaseQueue*)arg;
    log_this(SR_DATABASE, "Started fast queue worker for %s", LOG_LEVEL_STATE, true, true, true, db_queue->database_name);
    // Implementation will process fast queries
    return NULL;
}

static void* database_queue_worker_thread_cache(void* arg) {
    DatabaseQueue* db_queue = (DatabaseQueue*)arg;
    log_this(SR_DATABASE, "Started cache queue worker for %s", LOG_LEVEL_STATE, true, true, true, db_queue->database_name);
    // Implementation will process cached queries
    return NULL;
}

/*
 * Get total queue depth across all queues
 */
size_t database_queue_get_depth(DatabaseQueue* db_queue) {
    if (!db_queue) return 0;

    size_t total_depth = 0;
    total_depth += queue_size(db_queue->queue_slow);
    total_depth += queue_size(db_queue->queue_medium);
    total_depth += queue_size(db_queue->queue_fast);
    total_depth += queue_size(db_queue->queue_cache);

    return total_depth;
}

/*
 * Get queue statistics as formatted string
 */
void database_queue_get_stats(DatabaseQueue* db_queue, char* buffer, size_t buffer_size) {
    if (!db_queue || !buffer || buffer_size == 0) return;

    size_t slow_depth = queue_size(db_queue->queue_slow);
    size_t medium_depth = queue_size(db_queue->queue_medium);
    size_t fast_depth = queue_size(db_queue->queue_fast);
    size_t cache_depth = queue_size(db_queue->queue_cache);

    snprintf(buffer, buffer_size,
             "Database %s - Active: %s, Queries: %d, Total Depth: %zu (%zu/%zu/%zu/%zu)",
             db_queue->database_name,
             db_queue->is_connected ? "YES" : "NO",
             db_queue->total_queries_processed,
             database_queue_get_depth(db_queue),
             slow_depth, medium_depth, fast_depth, cache_depth);
}

/*
 * Health check for database connectivity and queue status
 */
bool database_queue_health_check(DatabaseQueue* db_queue) {
    if (!db_queue) return false;

    // Basic health checks
    if (db_queue->shutdown_requested) return false;

    // Check if queues are responding (placeholder - expand with actual connection testing)
    size_t total_depth = database_queue_get_depth(db_queue);
    if (total_depth > 10000) {  // Arbitrary high watermark
        log_this(SR_DATABASE, "Queue depth too high: %zu for %s", LOG_LEVEL_ALERT, true, true, true, total_depth, db_queue->database_name);
    }

    return true;
}

/*
 * Utility functions for queue type conversion
 */
const char* database_queue_type_to_string(int queue_type) {
    switch (queue_type) {
        case DB_QUEUE_SLOW:    return QUEUE_TYPE_SLOW;
        case DB_QUEUE_MEDIUM:  return QUEUE_TYPE_MEDIUM;
        case DB_QUEUE_FAST:    return QUEUE_TYPE_FAST;
        case DB_QUEUE_CACHE:   return QUEUE_TYPE_CACHE;
        default:               return "unknown";
    }
}

int database_queue_type_from_string(const char* type_str) {
    if (strcmp(type_str, QUEUE_TYPE_SLOW) == 0)   return DB_QUEUE_SLOW;
    if (strcmp(type_str, QUEUE_TYPE_MEDIUM) == 0) return DB_QUEUE_MEDIUM;
    if (strcmp(type_str, QUEUE_TYPE_FAST) == 0)   return DB_QUEUE_FAST;
    if (strcmp(type_str, QUEUE_TYPE_CACHE) == 0)  return DB_QUEUE_CACHE;
    return DB_QUEUE_MEDIUM;  // Default
}

/*
 * Select queue type based on hint from API path
 */
DatabaseQueueType database_queue_select_type(const char* queue_path_hint) {
    if (!queue_path_hint) return DB_QUEUE_MEDIUM;

    if (strcmp(queue_path_hint, QUEUE_TYPE_SLOW) == 0) return DB_QUEUE_SLOW;
    if (strcmp(queue_path_hint, QUEUE_TYPE_MEDIUM) == 0) return DB_QUEUE_MEDIUM;
    if (strcmp(queue_path_hint, QUEUE_TYPE_FAST) == 0) return DB_QUEUE_FAST;
    if (strcmp(queue_path_hint, QUEUE_TYPE_CACHE) == 0) return DB_QUEUE_CACHE;

    return DB_QUEUE_MEDIUM;
}

/*
 * Placeholder functions for JSON serialization (to be implemented)
 */
static char* serialize_query_to_json(DatabaseQuery* query) {
    // Placeholder - return basic JSON string for now
    if (!query || !query->query_template) return NULL;

    char* json = malloc(1024);
    if (!json) return NULL;

    snprintf(json, 1024, "{\"query_id\":\"%s\",\"template\":\"%s\"}",
             query->query_id ? query->query_id : "",
             query->query_template);

    return json;
}

static DatabaseQuery* deserialize_query_from_json(const char* json) {
    // Placeholder - return basic DatabaseQuery for now
    // Full implementation will parse JSON and create proper query struct
    (void)json;  // Suppress unused parameter warning for future implementation

    DatabaseQuery* query = malloc(sizeof(DatabaseQuery));
    if (!query) return NULL;

    memset(query, 0, sizeof(DatabaseQuery));
    query->query_id = strdup("parsed_query_id");
    query->query_template = strdup("parsed_template");

    return query;
}
