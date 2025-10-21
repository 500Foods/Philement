/*
 * Database Queue Submission Functions
 *
 * Implements query submission and processing functions for the Hydrogen database subsystem.
 * Split from database_queue.c for better maintainability.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>

// Local includes
#include "dbqueue.h"

/*
 * Placeholder functions for JSON serialization (to be implemented)
 */
char* serialize_query_to_json(DatabaseQuery* query) {
    // Placeholder - return basic JSON string for now
    if (!query || !query->query_template) return NULL;

    char* json = malloc(1024);
    if (!json) return NULL;

    snprintf(json, 1024, "{\"query_id\":\"%s\",\"template\":\"%s\"}",
             query->query_id ? query->query_id : "",
             query->query_template);

    return json;
}

DatabaseQuery* deserialize_query_from_json(const char* json) {
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
 * Submit query to appropriate database queue based on routing logic
 */
bool database_queue_submit_query(DatabaseQueue* db_queue, DatabaseQuery* query) {
    if (!db_queue || !query || !query->query_template) return false;

    // For Lead queues, route queries to appropriate child queues
    if (db_queue->is_lead_queue) {
        // Find appropriate child queue based on query type hint
        const char* target_queue_type = database_queue_type_to_string(query->queue_type_hint);

        MutexResult lock_result = MUTEX_LOCK(&db_queue->children_lock, SR_DATABASE);
        DatabaseQueue* target_child = NULL;
        if (lock_result == MUTEX_SUCCESS) {
            for (int i = 0; i < db_queue->child_queue_count; i++) {
                if (db_queue->child_queues[i] &&
                    db_queue->child_queues[i]->queue_type &&
                    strcmp(db_queue->child_queues[i]->queue_type, target_queue_type) == 0) {
                    target_child = db_queue->child_queues[i];
                    break;
                }
            }
            mutex_unlock(&db_queue->children_lock);
        }

        if (target_child) {
            // Route to child queue
            return database_queue_submit_query(target_child, query);
        } else {
            // No appropriate child queue exists, use Lead queue itself for now
            // log_this(SR_DATABASE, "No %s child queue found, using Lead queue for query: %s", LOG_LEVEL_TRACE, 2, target_queue_type, query->query_id);
        }
    }

    // Submit to this queue's single queue
    if (!db_queue->queue) {
        log_this(SR_DATABASE, "No queue available for query: %s", LOG_LEVEL_ERROR, 1, query->query_id);
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

        // Update last request time for queue selection algorithm
        db_queue->last_request_time = time(NULL);

        // log_this(SR_DATABASE, "Submitted query %s to %s queue %s", LOG_LEVEL_TRACE, 3, query->query_id, db_queue->queue_type, db_queue->database_name);
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
