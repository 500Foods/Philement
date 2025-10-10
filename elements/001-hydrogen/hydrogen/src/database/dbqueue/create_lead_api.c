/*
 * Database Lead Queue Creation - API Layer
 *
 * High-level API and orchestration functions for Lead database queue creation.
 * Contains parameter validation, system initialization, and main public interface.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/utils/utils_queue.h>

// Local includes
#include "dbqueue.h"

/*
 * API and Orchestration Functions
 */

// Validate parameters for Lead queue creation
bool database_queue_validate_lead_params(const char* database_name, const char* connection_string) {
    if (!database_name || !connection_string) {
        return false;
    }
    if (strlen(database_name) == 0) {
        return false;
    }
    return true;
}

// Ensure queue system is initialized
bool database_queue_ensure_system_initialized(void) {
    if (!queue_system_initialized) {
        queue_system_init();
    }
    return queue_system_initialized;
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

// Create a complete Lead queue with all initialization steps
DatabaseQueue* database_queue_create_lead_complete(const char* database_name, const char* connection_string, const char* bootstrap_query) {
    // Allocate and initialize basic structure
    DatabaseQueue* db_queue = database_queue_allocate_basic(database_name, connection_string, bootstrap_query);
    if (!db_queue) {
        return NULL;
    }

    // Initialize Lead queue specific properties
    if (!database_queue_init_lead_properties(db_queue)) {
        database_queue_destroy(db_queue);
        return NULL;
    }

    // Create the underlying queue structure
    if (!database_queue_create_underlying_queue(db_queue, database_name)) {
        database_queue_destroy(db_queue);
        return NULL;
    }

    // Initialize synchronization primitives for Lead queue
    if (!database_queue_init_lead_sync_primitives(db_queue, database_name)) {
        database_queue_destroy(db_queue);
        return NULL;
    }

    // Initialize final flags and statistics
    database_queue_init_lead_final_flags(db_queue);

    return db_queue;
}

/*
 * Create a Lead queue for a database - this is the primary queue that manages other queues
 */
DatabaseQueue* database_queue_create_lead(const char* database_name, const char* connection_string, const char* bootstrap_query) {
    log_this(SR_DATABASE, "Creating Lead DQM for: %s", LOG_LEVEL_TRACE, 1, database_name);

    // Validate parameters
    if (!database_queue_validate_lead_params(database_name, connection_string)) {
        log_this(SR_DATABASE, "Invalid parameters for Lead DQM creation", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // Ensure queue system is initialized
    if (!database_queue_ensure_system_initialized()) {
        log_this(SR_DATABASE, "Failed to initialize queue system", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // Create the complete Lead queue
    DatabaseQueue* db_queue = database_queue_create_lead_complete(database_name, connection_string, bootstrap_query);
    if (!db_queue) {
        log_this(SR_DATABASE, "Failed to create Lead DQM for: %s", LOG_LEVEL_ERROR, 1, database_name);
        return NULL;
    }

    return db_queue;
}