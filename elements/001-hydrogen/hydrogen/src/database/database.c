// Project includes
#include <src/hydrogen.h>
#include <src/network/network.h> // Ping

// Local includes
#include "database.h"
#include "dbqueue/dbqueue.h"
#include "database_connstring.h"
#include "database_manage.h"
#include "database_execute.h"
/*
 * Database Subsystem Core Implementation
 *
 * Implements the core database subsystem functionality including
 * subsystem initialization, database management, and API functions.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/network/network.h> // Ping

// Local incluses
#include "database.h"
#include "dbqueue/dbqueue.h"
#include "database_connstring.h"
#include "database_manage.h"

// Engine description function declarations
const char* postgresql_engine_get_description(void);
const char* sqlite_engine_get_description(void);
const char* mysql_engine_get_description(void);
const char* db2_engine_get_description(void);

// Global database subsystem instance
DatabaseSubsystem* database_subsystem = NULL;
static pthread_mutex_t database_subsystem_mutex = PTHREAD_MUTEX_INITIALIZER;


/*
 * Database Subsystem Core API
 */

// Initialize the database subsystem
bool database_subsystem_init(void) {
    // log_this(SR_DATABASE, "Starting database subsystem initialization", LOG_LEVEL_DEBUG, 0);

    MutexResult result = MUTEX_LOCK(&database_subsystem_mutex, SR_DATABASE);
    if (result != MUTEX_SUCCESS) {
        return false;
    }

    if (database_subsystem) {
        MUTEX_UNLOCK(&database_subsystem_mutex, SR_DATABASE);
        return true; // Already initialized
    }

    database_subsystem = calloc(1, sizeof(DatabaseSubsystem));
    if (!database_subsystem) {
        log_this(SR_DATABASE, "Failed to allocate database subsystem", LOG_LEVEL_ERROR, 0);
        MUTEX_UNLOCK(&database_subsystem_mutex, SR_DATABASE);
        return false;
    }

    // Initialize subsystem
    database_subsystem->initialized = false;
    database_subsystem->shutdown_requested = false;
    database_subsystem->queue_manager = NULL;
    database_subsystem->start_time = time(NULL);
    database_subsystem->total_queries_processed = 0;
    database_subsystem->successful_queries = 0;
    database_subsystem->failed_queries = 0;
    database_subsystem->timeout_queries = 0;
    database_subsystem->max_connections_per_database = 16;
    database_subsystem->default_worker_threads = 2;
    database_subsystem->query_timeout_seconds = 30;

    // Initialize engine registry
    memset(database_subsystem->engines, 0, sizeof(database_subsystem->engines));

    database_subsystem->initialized = true;
    MUTEX_UNLOCK(&database_subsystem_mutex, SR_DATABASE);

    // Initialize database thread tracking
    init_service_threads(&database_threads, SR_DATABASE);

    // log_this(SR_DATABASE, "Database subsystem initialization completed successfully", LOG_LEVEL_DEBUG, 0);
    return true;
}

// Shut down the database subsystem
void database_subsystem_shutdown(void) {
    MutexResult result = MUTEX_LOCK(&database_subsystem_mutex, SR_DATABASE);
    if (result != MUTEX_SUCCESS) {
        return;
    }

    if (!database_subsystem) {
        MUTEX_UNLOCK(&database_subsystem_mutex, SR_DATABASE);
        return;
    }

    database_subsystem->shutdown_requested = true;

    // TODO: Clean shutdown of queue manager, connections, etc.

    free(database_subsystem);
    database_subsystem = NULL;

    MUTEX_UNLOCK(&database_subsystem_mutex, SR_DATABASE);

    log_this(SR_DATABASE, "Database subsystem shutdown complete", LOG_LEVEL_DEBUG, 0);
}


// Get database statistics
void database_get_stats(char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) {
        return;
    }

    if (!database_subsystem) {
        snprintf(buffer, buffer_size, "Database subsystem not initialized");
        return;
    }

    snprintf(buffer, buffer_size,
             "Database Stats: Total=%lld, Success=%lld, Failed=%lld, Timeout=%lld",
             database_subsystem->total_queries_processed,
             database_subsystem->successful_queries,
             database_subsystem->failed_queries,
             database_subsystem->timeout_queries);
}

// Health check for entire subsystem
bool database_health_check(void) {
    if (!database_subsystem) {
        return false;
    }

    // TODO: Implement comprehensive health check
    return database_subsystem->initialized && !database_subsystem->shutdown_requested;
}

/*
 * Query Processing API (Phase 2 integration points)
 * Moved to database_execute.c
 */

/*
 * Configuration and Maintenance API
 */

// Reload database configurations
bool database_reload_config(void) {
    if (!database_subsystem) {
        return false;
    }

    // TODO: Implement configuration reload
    log_this(SR_DATABASE, "Configuration reload not yet implemented", LOG_LEVEL_TRACE, 0);
    return false;
}


/*
 * Integration points for other subsystems
 */

// For API subsystem integration
bool database_process_api_query(const char* database __attribute__((unused)), const char* query_path __attribute__((unused)),
                               const char* parameters __attribute__((unused)), const char* result_buffer __attribute__((unused)), size_t buffer_size __attribute__((unused))) {
    if (!database_subsystem || !database || !query_path || !result_buffer || buffer_size == 0) {
        return false;
    }

    // TODO: Implement API query processing
    log_this(SR_DATABASE, "API query processing not yet implemented", LOG_LEVEL_TRACE, 0);
    return false;
}

/*
 * Utility Functions
 */

// Validate query template
bool database_validate_query(const char* query_template) {
    if (!query_template) {
        return false;
    }

    // Basic validation - check for SQL injection patterns
    // TODO: Implement more comprehensive validation
    return strlen(query_template) > 0;
}

// Escape query parameters
char* database_escape_parameter(const char* parameter) {
    if (!parameter) {
        return NULL;
    }

    // Basic SQL escaping - escape single quotes and backslashes
    // Engine-specific escaping should be preferred when available
    size_t len = strlen(parameter);
    size_t escaped_len = len * 2 + 1; // Worst case: every char needs escaping
    char* escaped = malloc(escaped_len);
    if (!escaped) {
        return NULL;
    }

    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        char c = parameter[i];
        // Escape single quotes and backslashes
        if (c == '\'' || c == '\\') {
            escaped[j++] = '\\';
        }
        escaped[j++] = c;
    }
    escaped[j] = '\0';

    return escaped;
}


// Get total number of database queues (lead + child queues)
int database_get_total_queue_count(void) {
    if (!global_queue_manager) {
        return 0;
    }

    MutexResult lock_result = MUTEX_LOCK(&global_queue_manager->manager_lock, SR_DATABASE);
    if (lock_result != MUTEX_SUCCESS) {
        return 0;
    }

    int total_queues = 0;
    for (size_t i = 0; i < global_queue_manager->database_count; i++) {
        const DatabaseQueue* db_queue = global_queue_manager->databases[i];
        if (db_queue) {
            // Count the lead queue
            total_queues++;
            // Count child queues
            total_queues += db_queue->child_queue_count;
        }
    }

    mutex_unlock(&global_queue_manager->manager_lock);
    return total_queues;
}

// Get queue counts by type
void database_get_queue_counts_by_type(int* lead_count, int* slow_count, int* medium_count, int* fast_count, int* cache_count) {
    if (!global_queue_manager) {
        if (lead_count) *lead_count = 0;
        if (slow_count) *slow_count = 0;
        if (medium_count) *medium_count = 0;
        if (fast_count) *fast_count = 0;
        if (cache_count) *cache_count = 0;
        return;
    }

    MutexResult lock_result = MUTEX_LOCK(&global_queue_manager->manager_lock, SR_DATABASE);
    if (lock_result != MUTEX_SUCCESS) {
        if (lead_count) *lead_count = 0;
        if (slow_count) *slow_count = 0;
        if (medium_count) *medium_count = 0;
        if (fast_count) *fast_count = 0;
        if (cache_count) *cache_count = 0;
        return;
    }

    *lead_count = 0;
    *slow_count = 0;
    *medium_count = 0;
    *fast_count = 0;
    *cache_count = 0;

    for (size_t i = 0; i < global_queue_manager->database_count; i++) {
        DatabaseQueue* db_queue = global_queue_manager->databases[i];
        if (db_queue) {
            if (db_queue->is_lead_queue) {
                (*lead_count)++;
            }

            // Count child queues by type
            for (int j = 0; j < db_queue->child_queue_count; j++) {
                const DatabaseQueue* child_queue = db_queue->child_queues[j];
                if (child_queue) {
                    if (strcmp(child_queue->queue_type, "slow") == 0) {
                        (*slow_count)++;
                    } else if (strcmp(child_queue->queue_type, "medium") == 0) {
                        (*medium_count)++;
                    } else if (strcmp(child_queue->queue_type, "fast") == 0) {
                        (*fast_count)++;
                    } else if (strcmp(child_queue->queue_type, "cache") == 0) {
                        (*cache_count)++;
                    }
                }
            }
        }
    }

    mutex_unlock(&global_queue_manager->manager_lock);
}
