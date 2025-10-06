/*
 * Database Queue Infrastructure - Main Header
 *
 * This file now serves as the main header that includes all the split functionality files.
 * The database_queue.c has been split into multiple files for better maintainability:
 *
 * - database_queue_create.c     - Creation functions
 * - database_queue_destroy.c    - Destruction/cleanup functions
 * - database_queue_submit.c     - Query submission functions
 * - database_queue_process.c    - Worker thread and processing functions
 * - database_queue_heartbeat.c  - Heartbeat and connection management
 * - database_queue_lead.c       - Lead queue specific functions
 * - database_queue_manager.c    - Queue manager operations
 *
 * Utility functions are kept in this main file for coverage and testing purposes.
 */

#include "../hydrogen.h"

// Local includes
#include "database_queue.h"
#include "database.h"

/*
 * Debug function to dump connection details - shared across files
//  */
// void debug_dump_connection(const char* label, const DatabaseHandle* conn, const char* dqm_label) {
//     log_this(dqm_label, "=== %s CONNECTION DUMP ===", LOG_LEVEL_ERROR, 1, label);
//     log_this(dqm_label, "Connection pointer: %p", LOG_LEVEL_ERROR, 1, (void*)conn);

//     if (!conn) {
//         log_this(dqm_label, "Connection is NULL", LOG_LEVEL_ERROR, 0);
//     } else if ((uintptr_t)conn < 0x1000) {
//         log_this(dqm_label, "Connection is invalid pointer: 0x%lx", LOG_LEVEL_ERROR, 1, (uintptr_t)conn);
//     } else {
//         log_this(dqm_label, "Engine type: %d", LOG_LEVEL_ERROR, 1, (int)conn->engine_type);
//         log_this(dqm_label, "Status: %d", LOG_LEVEL_ERROR, 1, (int)conn->status);
//         log_this(dqm_label, "Connection handle: %p", LOG_LEVEL_ERROR, 1, conn->connection_handle);
//         log_this(dqm_label, "Designator: %s", LOG_LEVEL_ERROR, 1, conn->designator ? conn->designator : "NULL");
//         log_this(dqm_label, "Connected since: %ld", LOG_LEVEL_ERROR, 1, (long)conn->connected_since);
//         log_this(dqm_label, "In use: %s", LOG_LEVEL_ERROR, 1, conn->in_use ? "true" : "false");
//     }
//     log_this(dqm_label, "=== END %s DUMP ===", LOG_LEVEL_ERROR, 1, label);
// }

/*
 * Debug function to dump engine interface details
//  */
// void debug_dump_engine(const char* label, DatabaseEngineInterface* engine, const char* dqm_label) {
//     log_this(dqm_label, "=== %s ENGINE DUMP ===", LOG_LEVEL_ERROR, 1, label);
//     log_this(dqm_label, "Engine pointer: %p", LOG_LEVEL_ERROR, 1, (void*)engine);

//     if (!engine) {
//         log_this(dqm_label, "Engine is NULL", LOG_LEVEL_ERROR, 0);
//     } else if ((uintptr_t)engine < 0x1000) {
//         log_this(dqm_label, "Engine is invalid pointer: 0x%lx", LOG_LEVEL_ERROR, 1, (uintptr_t)engine);
//     } else {
//         log_this(dqm_label, "Engine type: %d", LOG_LEVEL_ERROR, 1, (int)engine->engine_type);
//         log_this(dqm_label, "Name: %s", LOG_LEVEL_ERROR, 1, engine->name ? engine->name : "NULL");
//         log_this(dqm_label, "execute_query: %p", LOG_LEVEL_ERROR, 1, (void*)(uintptr_t)engine->execute_query);
//         log_this(dqm_label, "connect: %p", LOG_LEVEL_ERROR, 1, (void*)(uintptr_t)engine->connect);
//         log_this(dqm_label, "disconnect: %p", LOG_LEVEL_ERROR, 1, (void*)(uintptr_t)engine->disconnect);
//         log_this(dqm_label, "health_check: %p", LOG_LEVEL_ERROR, 1, (void*)(uintptr_t)engine->health_check);
//     }
//     log_this(dqm_label, "=== END %s ENGINE DUMP ===", LOG_LEVEL_ERROR, 1, label);
// }

/*
 * Get total queue depth across all queues
 */
size_t database_queue_get_depth(DatabaseQueue* db_queue) {
    return database_queue_get_depth_with_designator(db_queue, SR_DATABASE);
}

/*
 * Get total queue depth across all queues with custom designator
 */
size_t database_queue_get_depth_with_designator(DatabaseQueue* db_queue, const char* designator) {
    if (!db_queue || !db_queue->queue) return 0;

    // Get queue size directly with proper designator
    size_t queue_depth = 0;
    MutexResult lock_result_1 = MUTEX_LOCK(&db_queue->queue->mutex, designator);
    if (lock_result_1 == MUTEX_SUCCESS) {
        queue_depth = db_queue->queue->size;
        mutex_unlock(&db_queue->queue->mutex);
    }

    size_t total_depth = queue_depth;

    // If this is a Lead queue, include child queue depths
    if (db_queue->is_lead_queue && db_queue->child_queues) {
        MutexResult lock_result_2 = MUTEX_LOCK(&db_queue->children_lock, designator);
        if (lock_result_2 == MUTEX_SUCCESS) {
            for (int i = 0; i < db_queue->child_queue_count; i++) {
                if (db_queue->child_queues[i]) {
                    total_depth += database_queue_get_depth_with_designator(db_queue->child_queues[i], designator);
                }
            }
            mutex_unlock(&db_queue->children_lock);
        }
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
        log_this(SR_DATABASE, "Queue depth too high: %zu for %s", LOG_LEVEL_ALERT, 2, total_depth, db_queue->database_name);
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
 * Generate the full DQM label for logging
 * For queue 00 (Lead), exclude 'L' from tags as it's implied
 */
char* database_queue_generate_label(DatabaseQueue* db_queue) {
    if (!db_queue) return NULL;

    char label[128];
    const char* tags_to_show = db_queue->tags;

    // For queue 00 (Lead), exclude 'L' from tags as it's implied
    char filtered_tags[16] = {0};
    if (db_queue->queue_number == 0 && db_queue->tags) {
        size_t len = strlen(db_queue->tags);
        size_t filtered_idx = 0;
        for (size_t i = 0; i < len; i++) {
            if (db_queue->tags[i] != 'L') {
                filtered_tags[filtered_idx++] = db_queue->tags[i];
            }
        }
        filtered_tags[filtered_idx] = '\0';
        tags_to_show = filtered_tags;
    }

    snprintf(label, sizeof(label), "DQM-%s-%02d-%s",
             db_queue->database_name ? db_queue->database_name : "unknown",
             db_queue->queue_number >= 0 ? db_queue->queue_number : 0,
             tags_to_show ? tags_to_show : "");

    return strdup(label);
}

/*
 * Tag Management Functions
 */

/*
 * Set tags for a database queue
 */
bool database_queue_set_tags(DatabaseQueue* db_queue, const char* tags) {
    if (!db_queue || !tags) return false;

    free(db_queue->tags);
    db_queue->tags = strdup(tags);
    return db_queue->tags != NULL;
}

/*
 * Get current tags for a database queue
 */
char* database_queue_get_tags(const DatabaseQueue* db_queue) {
    if (!db_queue || !db_queue->tags) return NULL;
    return strdup(db_queue->tags);
}

/*
 * Add a tag to a database queue
 */
bool database_queue_add_tag(DatabaseQueue* db_queue, char tag) {
    if (!db_queue || !db_queue->tags) return false;

    // Check if tag already exists
    if (strchr(db_queue->tags, tag)) return true;

    // Add tag to the end
    size_t new_len = strlen(db_queue->tags) + 2; // +1 for new tag, +1 for null
    char* new_tags = realloc(db_queue->tags, new_len);
    if (!new_tags) return false;

    db_queue->tags = new_tags;
    db_queue->tags[strlen(db_queue->tags)] = tag;
    db_queue->tags[strlen(db_queue->tags) + 1] = '\0';

    return true;
}

/*
 * Remove a tag from a database queue
 */
bool database_queue_remove_tag(DatabaseQueue* db_queue, char tag) {
    if (!db_queue || !db_queue->tags) return false;

    char* tag_pos = strchr(db_queue->tags, tag);
    if (!tag_pos) return false;

    // Remove the tag by shifting remaining characters
    memmove(tag_pos, tag_pos + 1, strlen(tag_pos));

    return true;
}














