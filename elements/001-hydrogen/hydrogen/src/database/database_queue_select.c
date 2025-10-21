/*
 * Database Queue Selection Algorithm
 *
 * Implements intelligent queue selection for Conduit service based on queue depth
 * and last request timestamp as described in CONDUIT.md Phase 2.
 */

#include <src/hydrogen.h>
#include "dbqueue/dbqueue.h"
#include "database_queue_select.h"

/*
 * Select optimal database queue for query execution
 *
 * Algorithm:
 * 1. Filter queues by database name
 * 2. Filter by queue type (from QTC recommendation)
 * 3. Find minimum depth
 * 4. Among queues with min depth, select earliest last_request_time
 * 5. If all depths are 0, naturally round-robins via timestamps
 */
DatabaseQueue* select_optimal_queue(
    const char* database_name,
    const char* queue_type_hint,
    DatabaseQueueManager* manager
) {
    if (!database_name || !manager || !manager->databases) {
        return NULL;
    }

    DatabaseQueue* selected_queue = NULL;
    int min_depth = INT_MAX;
    time_t earliest_timestamp = (time_t)LONG_MAX;

    // Iterate through all databases managed by this manager
    for (size_t i = 0; i < manager->database_count; i++) {
        DatabaseQueue* db_queue = manager->databases[i];
        if (!db_queue) continue;

        // Filter by database name
        if (strcmp(db_queue->database_name, database_name) != 0) {
            continue;
        }

        // Filter by queue type (if hint provided)
        if (queue_type_hint && strcmp(db_queue->queue_type, queue_type_hint) != 0) {
            continue;
        }

        // Get current queue depth
        int current_depth = db_queue->current_queue_depth;

        // Check if this queue has better (lower) depth
        if (current_depth < min_depth) {
            min_depth = current_depth;
            earliest_timestamp = db_queue->last_request_time;
            selected_queue = db_queue;
        }
        // If same depth, check timestamp (earlier timestamp wins)
        else if (current_depth == min_depth) {
            if (db_queue->last_request_time < earliest_timestamp) {
                earliest_timestamp = db_queue->last_request_time;
                selected_queue = db_queue;
            }
        }
    }

    return selected_queue;
}

/*
 * Update last request time for a queue (called when query is submitted)
 */
void update_queue_last_request_time(DatabaseQueue* db_queue) {
    if (db_queue) {
        db_queue->last_request_time = time(NULL);
    }
}