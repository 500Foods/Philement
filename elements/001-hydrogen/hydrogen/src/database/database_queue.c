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
DatabaseQueueManager* global_queue_manager = NULL;

// External references
extern volatile sig_atomic_t database_stopping;

// Forward declaration for generic worker thread
void* database_queue_worker_thread(void* arg);

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
    log_this(SR_DATABASE, "Initializing database queue system", LOG_LEVEL_STATE);

    if (global_queue_manager != NULL) {
        return true;
    }

    // Ensure the global queue system is initialized first
    extern int queue_system_initialized;
    if (!queue_system_initialized) {
        queue_system_init();
    }

    // Create global queue manager with capacity for up to 8 databases
    global_queue_manager = database_queue_manager_create(8);
    if (!global_queue_manager) {
        log_this(SR_DATABASE, "Failed to create database queue manager", LOG_LEVEL_ERROR);
        return false;
    }

    log_this(SR_DATABASE, "Database queue system initialized successfully", LOG_LEVEL_STATE);
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
/*
 * Create a Lead queue for a database - this is the primary queue that manages other queues
 */
DatabaseQueue* database_queue_create_lead(const char* database_name, const char* connection_string) {
    log_this(SR_DATABASE, "Creating Lead queue for database: %s", LOG_LEVEL_STATE, database_name);

    if (!database_name || !connection_string || strlen(database_name) == 0) {
        log_this(SR_DATABASE, "Invalid parameters for Lead queue creation", LOG_LEVEL_ERROR);
        return NULL;
    }

    // Initialize the global queue system if not already done
    extern int queue_system_initialized;
    if (!queue_system_initialized) {
        queue_system_init();
    }

    DatabaseQueue* db_queue = malloc(sizeof(DatabaseQueue));
    if (!db_queue) {
        log_this(SR_DATABASE, "Failed to allocate Lead queue", LOG_LEVEL_ERROR);
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

    // Set queue type and role
    db_queue->queue_type = strdup("Lead");
    db_queue->is_lead_queue = true;
    db_queue->can_spawn_queues = true;

    // Create the Lead queue with descriptive name
    char lead_queue_name[256];
    snprintf(lead_queue_name, sizeof(lead_queue_name), "%s_lead", database_name);

    // Initialize queue attributes (required by queue_create)
    QueueAttributes queue_attrs = {0};
    db_queue->queue = queue_create(lead_queue_name, &queue_attrs);

    if (!db_queue->queue) {
        log_this(SR_DATABASE, "Failed to create Lead queue", LOG_LEVEL_ERROR);
        free(db_queue->queue_type);
        free(db_queue->connection_string);
        free(db_queue->database_name);
        free(db_queue);
        return NULL;
    }

    log_this(SR_DATABASE, "Lead queue created successfully", LOG_LEVEL_STATE);

    // Initialize synchronization primitives
    if (pthread_mutex_init(&db_queue->queue_access_lock, NULL) != 0) {
        log_this(SR_DATABASE, "Failed to initialize queue access mutex", LOG_LEVEL_ERROR);
        database_queue_destroy(db_queue);
        return NULL;
    }

    if (sem_init(&db_queue->worker_semaphore, 0, 0) != 0) {
        log_this(SR_DATABASE, "Failed to initialize worker semaphore", LOG_LEVEL_ERROR);
        pthread_mutex_destroy(&db_queue->queue_access_lock);
        database_queue_destroy(db_queue);
        return NULL;
    }

    // Initialize Lead queue management
    if (pthread_mutex_init(&db_queue->children_lock, NULL) != 0) {
        log_this(SR_DATABASE, "Failed to initialize children mutex", LOG_LEVEL_ERROR);
        sem_destroy(&db_queue->worker_semaphore);
        pthread_mutex_destroy(&db_queue->queue_access_lock);
        database_queue_destroy(db_queue);
        return NULL;
    }

    // Allocate child queue array (max 4: slow, medium, fast, cache)
    db_queue->max_child_queues = 4;
    db_queue->child_queues = calloc((size_t)db_queue->max_child_queues, sizeof(DatabaseQueue*));
    if (!db_queue->child_queues) {
        log_this(SR_DATABASE, "Failed to allocate child queue array", LOG_LEVEL_ERROR);
        pthread_mutex_destroy(&db_queue->children_lock);
        sem_destroy(&db_queue->worker_semaphore);
        pthread_mutex_destroy(&db_queue->queue_access_lock);
        database_queue_destroy(db_queue);
        return NULL;
    }

    // Initialize flags and statistics
    db_queue->shutdown_requested = false;
    db_queue->is_connected = false;
    db_queue->active_connections = 0;
    db_queue->total_queries_processed = 0;
    db_queue->current_queue_depth = 0;
    db_queue->child_queue_count = 0;

    log_this(SR_DATABASE, "Lead queue creation completed successfully", LOG_LEVEL_STATE);
    return db_queue;
}

/*
 * Create a worker queue for a specific queue type (slow, medium, fast, cache)
 */
DatabaseQueue* database_queue_create_worker(const char* database_name, const char* connection_string, const char* queue_type) {
    log_this(SR_DATABASE, "Creating %s worker queue for database: %s", LOG_LEVEL_STATE, queue_type, database_name);

    if (!database_name || !connection_string || !queue_type) {
        log_this(SR_DATABASE, "Invalid parameters for worker queue creation", LOG_LEVEL_ERROR);
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

    // Create the worker queue
    char worker_queue_name[256];
    snprintf(worker_queue_name, sizeof(worker_queue_name), "%s_%s", database_name, queue_type);

    QueueAttributes queue_attrs = {0};
    db_queue->queue = queue_create(worker_queue_name, &queue_attrs);

    if (!db_queue->queue) {
        log_this(SR_DATABASE, "Failed to create %s worker queue", LOG_LEVEL_ERROR, queue_type);
        free(db_queue->queue_type);
        free(db_queue->connection_string);
        free(db_queue->database_name);
        free(db_queue);
        return NULL;
    }

    // Initialize synchronization primitives (workers need these too)
    if (pthread_mutex_init(&db_queue->queue_access_lock, NULL) != 0 ||
        sem_init(&db_queue->worker_semaphore, 0, 0) != 0) {
        log_this(SR_DATABASE, "Failed to initialize synchronization for %s worker", LOG_LEVEL_ERROR, queue_type);
        queue_destroy(db_queue->queue);
        free(db_queue->queue_type);
        free(db_queue->connection_string);
        free(db_queue->database_name);
        free(db_queue);
        return NULL;
    }

    // Initialize flags and statistics
    db_queue->shutdown_requested = false;
    db_queue->is_connected = false;
    db_queue->active_connections = 0;
    db_queue->total_queries_processed = 0;
    db_queue->current_queue_depth = 0;

    // No child queues for workers
    db_queue->child_queues = NULL;
    db_queue->child_queue_count = 0;
    db_queue->max_child_queues = 0;

    log_this(SR_DATABASE, "%s worker queue created successfully", LOG_LEVEL_STATE, queue_type);
    return db_queue;
}

/*
 * Destroy database queue and all associated resources
 *
 * Ensures clean shutdown of all worker threads and proper cleanup.
 */
void database_queue_destroy(DatabaseQueue* db_queue) {
    if (!db_queue) return;

    // Create DQM component name for logging
    char dqm_component[64];
    snprintf(dqm_component, sizeof(dqm_component), "DQM-%s",
             db_queue->database_name ? db_queue->database_name : "unknown");

    log_this(dqm_component, "Destroying %s queue", LOG_LEVEL_STATE, true, true, true,
             db_queue->queue_type ? db_queue->queue_type : "unknown");

    // Signal shutdown to worker threads
    db_queue->shutdown_requested = true;

    // Wait for worker thread to finish
    database_queue_stop_worker(db_queue);

    // If this is a Lead queue, clean up child queues first
    if (db_queue->is_lead_queue && db_queue->child_queues) {
        pthread_mutex_lock(&db_queue->children_lock);
        for (int i = 0; i < db_queue->child_queue_count; i++) {
            if (db_queue->child_queues[i]) {
                database_queue_destroy(db_queue->child_queues[i]);
                db_queue->child_queues[i] = NULL;
            }
        }
        db_queue->child_queue_count = 0;
        pthread_mutex_unlock(&db_queue->children_lock);
        
        free(db_queue->child_queues);
        pthread_mutex_destroy(&db_queue->children_lock);
    }

    // Clean up the single queue
    if (db_queue->queue) {
        queue_destroy(db_queue->queue);
    }

    // Clean up synchronization primitives
    pthread_mutex_destroy(&db_queue->queue_access_lock);
    sem_destroy(&db_queue->worker_semaphore);

    // Free strings
    free(db_queue->database_name);
    free(db_queue->connection_string);
    free(db_queue->queue_type);

    free(db_queue);
}

/*
 * Create queue manager to coordinate multiple databases
 *
 * Implements round-robin distribution and centralized statistics.
 */
DatabaseQueueManager* database_queue_manager_create(size_t max_databases) {
    DatabaseQueueManager* manager = malloc(sizeof(DatabaseQueueManager));
    if (!manager) {
        log_this(SR_DATABASE, "Failed to malloc queue manager", LOG_LEVEL_ERROR);
        return NULL;
    }

    memset(manager, 0, sizeof(DatabaseQueueManager));

    manager->databases = calloc(max_databases, sizeof(DatabaseQueue*));
    if (!manager->databases) {
        log_this(SR_DATABASE, "Failed to calloc database array", LOG_LEVEL_ERROR);
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
        log_this(SR_DATABASE, "Failed to initialize manager mutex", LOG_LEVEL_ERROR);
        free(manager->databases);
        free(manager);
        return NULL;
    }

    manager->initialized = true;
    log_this(SR_DATABASE, "Queue manager creation completed successfully", LOG_LEVEL_STATE);
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
    if (!manager || !db_queue) {
        log_this(SR_DATABASE, "Invalid parameters for add database", LOG_LEVEL_ERROR);
        return false;
    }

    pthread_mutex_lock(&manager->manager_lock);
    if (manager->database_count >= manager->max_databases) {
        pthread_mutex_unlock(&manager->manager_lock);
        log_this(SR_DATABASE, "Cannot add database: maximum capacity reached", LOG_LEVEL_ALERT, true, true, true);
        return false;
    }

    manager->databases[manager->database_count++] = db_queue;
    pthread_mutex_unlock(&manager->manager_lock);

    // Create DQM component name for logging
    char dqm_component[64];
    snprintf(dqm_component, sizeof(dqm_component), "DQM-%s", db_queue->database_name);

    log_this(dqm_component, "Added to global queue manager", LOG_LEVEL_STATE, true, true, true);
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

    // For Lead queues, route queries to appropriate child queues
    if (db_queue->is_lead_queue) {
        // Find appropriate child queue based on query type hint
        const char* target_queue_type = database_queue_type_to_string(query->queue_type_hint);
        
        pthread_mutex_lock(&db_queue->children_lock);
        DatabaseQueue* target_child = NULL;
        for (int i = 0; i < db_queue->child_queue_count; i++) {
            if (db_queue->child_queues[i] &&
                db_queue->child_queues[i]->queue_type &&
                strcmp(db_queue->child_queues[i]->queue_type, target_queue_type) == 0) {
                target_child = db_queue->child_queues[i];
                break;
            }
        }
        pthread_mutex_unlock(&db_queue->children_lock);

        if (target_child) {
            // Route to child queue
            return database_queue_submit_query(target_child, query);
        } else {
            // No appropriate child queue exists, use Lead queue itself for now
            // log_this(SR_DATABASE, "No %s child queue found, using Lead queue for query: %s", LOG_LEVEL_DEBUG, target_queue_type, query->query_id);
        }
    }

    // Submit to this queue's single queue
    if (!db_queue->queue) {
        log_this(SR_DATABASE, "No queue available for query: %s", LOG_LEVEL_ERROR, true, true, true, query->query_id);
        return false;
    }

    // Serialize query to JSON for queue storage
    char* query_json = serialize_query_to_json(query);
    if (!query_json) return false;

    // Submit to queue with priority based on queue type
    bool success = queue_enqueue(db_queue->queue, query_json, strlen(query_json), query->queue_type_hint);

    free(query_json);

    if (success) {
        __sync_fetch_and_add(&db_queue->current_queue_depth, 1);
        query->submitted_at = time(NULL);

        // log_this(SR_DATABASE, "Submitted query %s to %s queue %s", LOG_LEVEL_DEBUG, true, true, true, query->query_id, db_queue->queue_type, db_queue->database_name);
    }

    return success;
}

/*
 * Process next query from specified queue type
 */
DatabaseQuery* database_queue_process_next(DatabaseQueue* db_queue) {
    if (!db_queue || !db_queue->queue) return NULL;

    if (queue_size(db_queue->queue) == 0) return NULL;

    // Dequeue query from this queue's single queue
    size_t data_size;
    int priority;
    char* query_data = queue_dequeue(db_queue->queue, &data_size, &priority);

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
/*
 * Start a single worker thread for this queue
 */
bool database_queue_start_worker(DatabaseQueue* db_queue) {
    // Create DQM component name for logging
    char dqm_component[64];
    snprintf(dqm_component, sizeof(dqm_component), "DQM-%s", db_queue->database_name);
    
    log_this(dqm_component, "Starting %s worker thread", LOG_LEVEL_STATE, db_queue->queue_type);

    if (!db_queue) {
        log_this(dqm_component, "Invalid database queue parameter", LOG_LEVEL_ERROR);
        return false;
    }

    // Create the single worker thread
    if (pthread_create(&db_queue->worker_thread, NULL, database_queue_worker_thread, db_queue) != 0) {
        log_this(dqm_component, "Failed to start %s worker thread", LOG_LEVEL_ERROR, db_queue->queue_type);
        return false;
    }

    // Register thread with thread tracking system
    extern ServiceThreads database_threads;
    add_service_thread(&database_threads, db_queue->worker_thread);

    log_this(dqm_component, "%s worker thread created and registered successfully", LOG_LEVEL_STATE, db_queue->queue_type);
    return true;
}

/*
 * Stop worker thread and wait for completion
 */
void database_queue_stop_worker(DatabaseQueue* db_queue) {
    if (!db_queue) return;

    // Create DQM component name for logging
    char dqm_component[64];
    snprintf(dqm_component, sizeof(dqm_component), "DQM-%s", db_queue->database_name);

    log_this(dqm_component, "Stopping %s worker thread", LOG_LEVEL_STATE, db_queue->queue_type);

    db_queue->shutdown_requested = true;

    // Cancel and join worker thread
    pthread_cancel(db_queue->worker_thread);
    pthread_join(db_queue->worker_thread, NULL);

    log_this(dqm_component, "Stopped %s worker thread", LOG_LEVEL_STATE, db_queue->queue_type);
}

/*
 * Single generic worker thread function that works for all queue types
 */
void* database_queue_worker_thread(void* arg) {
    DatabaseQueue* db_queue = (DatabaseQueue*)arg;
    
    // Create DQM component name for logging
    char dqm_component[64];
    snprintf(dqm_component, sizeof(dqm_component), "DQM-%s", db_queue->database_name);
    
    log_this(dqm_component, "%s queue worker thread started", LOG_LEVEL_STATE, db_queue->queue_type);

    // Main worker loop - stay alive until shutdown is requested
    while (!db_queue->shutdown_requested && !database_stopping) {
        // Wait for work with a timeout to check shutdown periodically
        struct timespec timeout;
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += 1; // 1 second timeout

        // Try to wait for work signal
        if (sem_timedwait(&db_queue->worker_semaphore, &timeout) == 0) {
            // Check again if we should still process (avoid race conditions)
            if (!db_queue->shutdown_requested && !database_stopping) {
                // Process next query from this queue
                DatabaseQuery* query = database_queue_process_next(db_queue);
                if (query) {
                    // log_this(dqm_component, "%s queue processing query: %s", LOG_LEVEL_DEBUG, db_queue->queue_type, query->query_id ? query->query_id : "unknown");
                    
                    // TODO: Actual database query execution will be implemented in Phase 2
                    // For now, just simulate processing time based on queue type
                    if (strcmp(db_queue->queue_type, "slow") == 0) {
                        usleep(500000); // 500ms for slow queries
                    } else if (strcmp(db_queue->queue_type, "medium") == 0) {
                        usleep(200000); // 200ms for medium queries
                    } else if (strcmp(db_queue->queue_type, "fast") == 0) {
                        usleep(50000);  // 50ms for fast queries
                    } else if (strcmp(db_queue->queue_type, "cache") == 0) {
                        usleep(10000);  // 10ms for cache queries
                    } else if (strcmp(db_queue->queue_type, "Lead") == 0) {
                        usleep(100000); // 100ms for Lead queue queries
                        // Lead queue can also manage child queues here
                        if (db_queue->is_lead_queue) {
                            database_queue_manage_child_queues(db_queue);
                        }
                    }
                    
                    // Clean up query
                    if (query->query_id) free(query->query_id);
                    if (query->query_template) free(query->query_template);
                    if (query->parameter_json) free(query->parameter_json);
                    if (query->error_message) free(query->error_message);
                    free(query);
                }
            }
        }
        // Continue loop - timeout is expected and normal
    }

    // Clean up thread tracking before exit
    extern ServiceThreads database_threads;
    remove_service_thread(&database_threads, pthread_self());

    log_this(dqm_component, "%s queue worker thread exiting", LOG_LEVEL_STATE, db_queue->queue_type);
    return NULL;
}

/*
 * Get total queue depth across all queues
 */
size_t database_queue_get_depth(DatabaseQueue* db_queue) {
    if (!db_queue || !db_queue->queue) return 0;

    size_t total_depth = queue_size(db_queue->queue);

    // If this is a Lead queue, include child queue depths
    if (db_queue->is_lead_queue && db_queue->child_queues) {
        pthread_mutex_lock(&db_queue->children_lock);
        for (int i = 0; i < db_queue->child_queue_count; i++) {
            if (db_queue->child_queues[i]) {
                total_depth += database_queue_get_depth(db_queue->child_queues[i]);
            }
        }
        pthread_mutex_unlock(&db_queue->children_lock);
    }

    return total_depth;
}

/*
 * Get queue statistics as formatted string
 */
void database_queue_get_stats(DatabaseQueue* db_queue, char* buffer, size_t buffer_size) {
    if (!db_queue || !buffer || buffer_size == 0) return;

    size_t queue_depth = db_queue->queue ? queue_size(db_queue->queue) : 0;

    if (db_queue->is_lead_queue) {
        snprintf(buffer, buffer_size,
                 "Database %s [%s] - Active: %s, Queries: %d, Depth: %zu (Lead + %d children)",
                 db_queue->database_name,
                 db_queue->queue_type,
                 db_queue->is_connected ? "YES" : "NO",
                 db_queue->total_queries_processed,
                 database_queue_get_depth(db_queue),
                 db_queue->child_queue_count);
    } else {
        snprintf(buffer, buffer_size,
                 "Database %s [%s] - Active: %s, Queries: %d, Depth: %zu",
                 db_queue->database_name,
                 db_queue->queue_type,
                 db_queue->is_connected ? "YES" : "NO",
                 db_queue->total_queries_processed,
                 queue_depth);
    }
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

/*
 * Lead Queue Management Functions
 */

/*
 * Spawn a child queue of the specified type
 */
bool database_queue_spawn_child_queue(DatabaseQueue* lead_queue, const char* queue_type) {
    if (!lead_queue || !lead_queue->is_lead_queue || !queue_type) {
        return false;
    }

    pthread_mutex_lock(&lead_queue->children_lock);

    // Check if we already have a child queue of this type
    for (int i = 0; i < lead_queue->child_queue_count; i++) {
        if (lead_queue->child_queues[i] &&
            lead_queue->child_queues[i]->queue_type &&
            strcmp(lead_queue->child_queues[i]->queue_type, queue_type) == 0) {
            pthread_mutex_unlock(&lead_queue->children_lock);
            // log_this(SR_DATABASE, "Child queue %s already exists for database %s", LOG_LEVEL_DEBUG, queue_type, lead_queue->database_name);
            return true; // Already exists
        }
    }

    // Check if we have space for more child queues
    if (lead_queue->child_queue_count >= lead_queue->max_child_queues) {
        pthread_mutex_unlock(&lead_queue->children_lock);
        // log_this(SR_DATABASE, "Cannot spawn %s queue: maximum child queues reached for %s", LOG_LEVEL_ERROR, queue_type, lead_queue->database_name);
        return false;
    }

    // Create the child worker queue
    DatabaseQueue* child_queue = database_queue_create_worker(
        lead_queue->database_name,
        lead_queue->connection_string,
        queue_type
    );

    if (!child_queue) {
        pthread_mutex_unlock(&lead_queue->children_lock);
        log_this(SR_DATABASE, "Failed to create %s child queue for %s",
                LOG_LEVEL_ERROR, queue_type, lead_queue->database_name);
        return false;
    }

    // Start the worker thread for the child queue
    if (!database_queue_start_worker(child_queue)) {
        pthread_mutex_unlock(&lead_queue->children_lock);
        database_queue_destroy(child_queue);
        log_this(SR_DATABASE, "Failed to start worker for %s child queue for %s",
                LOG_LEVEL_ERROR, queue_type, lead_queue->database_name);
        return false;
    }

    // Add to child queue array
    lead_queue->child_queues[lead_queue->child_queue_count] = child_queue;
    lead_queue->child_queue_count++;

    pthread_mutex_unlock(&lead_queue->children_lock);

    log_this(SR_DATABASE, "Spawned %s child queue for database %s",
            LOG_LEVEL_STATE, queue_type, lead_queue->database_name);
    return true;
}

/*
 * Shutdown a child queue of the specified type
 */
bool database_queue_shutdown_child_queue(DatabaseQueue* lead_queue, const char* queue_type) {
    if (!lead_queue || !lead_queue->is_lead_queue || !queue_type) {
        return false;
    }

    pthread_mutex_lock(&lead_queue->children_lock);

    // Find the child queue to shutdown
    int target_index = -1;
    for (int i = 0; i < lead_queue->child_queue_count; i++) {
        if (lead_queue->child_queues[i] &&
            lead_queue->child_queues[i]->queue_type &&
            strcmp(lead_queue->child_queues[i]->queue_type, queue_type) == 0) {
            target_index = i;
            break;
        }
    }

    if (target_index == -1) {
        pthread_mutex_unlock(&lead_queue->children_lock);
        // log_this(SR_DATABASE, "Child queue %s not found for database %s", LOG_LEVEL_DEBUG, queue_type, lead_queue->database_name);
        return false;
    }

    // Destroy the child queue
    DatabaseQueue* child_queue = lead_queue->child_queues[target_index];
    database_queue_destroy(child_queue);

    // Compact the array by moving the last element to the removed position
    if (target_index < lead_queue->child_queue_count - 1) {
        lead_queue->child_queues[target_index] = lead_queue->child_queues[lead_queue->child_queue_count - 1];
    }
    lead_queue->child_queues[lead_queue->child_queue_count - 1] = NULL;
    lead_queue->child_queue_count--;

    pthread_mutex_unlock(&lead_queue->children_lock);

    log_this(SR_DATABASE, "Shutdown %s child queue for database %s",
            LOG_LEVEL_STATE, queue_type, lead_queue->database_name);
    return true;
}

/*
 * Manage child queues based on workload and configuration
 * This function is called by the Lead queue worker thread periodically
 */
void database_queue_manage_child_queues(DatabaseQueue* lead_queue) {
    if (!lead_queue || !lead_queue->is_lead_queue) {
        return;
    }

    // TODO: Implement intelligent queue management based on:
    // - Current queue depths
    // - Processing rates
    // - Configuration settings
    // - System load
    
    // For now, this is a placeholder that could spawn queues based on simple rules
    // Real implementation will be added in Phase 2

    // Example logic (commented out for now):
    /*
    size_t lead_depth = queue_size(lead_queue->queue);
    if (lead_depth > 10) {
        // High load - consider spawning a medium queue if not exists
        database_queue_spawn_child_queue(lead_queue, "medium");
    }
    */

    // Create DQM component name for logging
    char dqm_component[64];
    snprintf(dqm_component, sizeof(dqm_component), "DQM-%s", lead_queue->database_name);
    
    log_this(dqm_component, "Lead queue managing child queues (placeholder)", LOG_LEVEL_TRACE);
}
