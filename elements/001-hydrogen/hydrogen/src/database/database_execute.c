/*
 * Database Query Execution Module
 *
 * Name-based façade over the live queue + pending-result path used by
 * conduit, auth, scripting, and mailrelay. Production callers usually
 * invoke database_queue_submit_query / pending_result_* directly; these
 * helpers resolve a database by name and map query_id lifecycle onto the
 * global PendingResultManager.
 */

// Project includes
#include <src/hydrogen.h>

// Local includes
#include "database.h"
#include "dbqueue/dbqueue.h"
#include "database_connstring.h"
#include "database_manage.h"
#include "database_pending.h"
#include "database_engine.h"

// Forward declarations for helper functions
time_t calculate_queue_query_age(DatabaseQueue* db_queue);
time_t find_max_query_age_across_queues(void);

/*
 * Query Processing API — thin wrappers around the queue manager
 */

// Submit a query to the named database's lead queue (async enqueue).
// Does not register a pending waiter; pair with pending_result_register
// or database_queue_await_result when a result is required.
bool database_submit_query(const char* database_name,
                          const char* query_id,
                          const char* query_template,
                          const char* parameters_json,
                          int queue_type_hint) {
    if (!database_subsystem || !database_name || database_name[0] == '\0' ||
        !query_template || query_template[0] == '\0') {
        return false;
    }

    if (!global_queue_manager) {
        return false;
    }

    DatabaseQueue* db_queue = database_queue_manager_get_database(global_queue_manager, database_name);
    if (!db_queue) {
        return false;
    }

    DatabaseQuery query;
    memset(&query, 0, sizeof(query));
    // submit serializes from these pointers; they must remain valid for the call.
    query.query_id = (char*)(query_id && query_id[0] != '\0' ? query_id : "anonymous");
    query.query_template = (char*)query_template;
    query.parameter_json = (char*)parameters_json;
    query.queue_type_hint = queue_type_hint;
    query.submitted_at = time(NULL);

    return database_queue_submit_query(db_queue, &query);
}

// Check query result status via the global pending-result manager
DatabaseQueryStatus database_query_status(const char* query_id) {
    if (!database_subsystem || !query_id || query_id[0] == '\0') {
        return DB_QUERY_ERROR;
    }

    PendingResultManager* manager = get_pending_result_manager();
    if (!manager) {
        return DB_QUERY_ERROR;
    }

    PendingQueryResult* pending = pending_result_find(manager, query_id);
    if (!pending) {
        return DB_QUERY_ERROR;
    }

    if (pending_result_is_timed_out(pending)) {
        return DB_QUERY_TIMEOUT;
    }

    // In-flight: no dedicated pending enum on DatabaseQueryStatus.
    if (!pending_result_is_completed(pending)) {
        return DB_QUERY_ERROR;
    }

    QueryResult* result = pending_result_get(pending);
    if (!result) {
        return DB_QUERY_ERROR;
    }
    if (result->success) {
        return DB_QUERY_SUCCESS;
    }
    if (result->error_class == DB_ERR_TIMEOUT) {
        return DB_QUERY_TIMEOUT;
    }
    return DB_QUERY_ERROR;
}

// Copy completed result JSON into caller buffer (non-blocking)
bool database_get_result(const char* query_id, char* result_buffer, size_t buffer_size) {
    if (!database_subsystem || !query_id || query_id[0] == '\0' ||
        !result_buffer || buffer_size == 0) {
        return false;
    }

    PendingResultManager* manager = get_pending_result_manager();
    if (!manager) {
        return false;
    }

    PendingQueryResult* pending = pending_result_find(manager, query_id);
    if (!pending || !pending_result_is_completed(pending)) {
        return false;
    }

    QueryResult* result = pending_result_get(pending);
    if (!result || !result->data_json) {
        return false;
    }

    size_t len = strlen(result->data_json);
    if (len + 1 > buffer_size) {
        return false;
    }
    memcpy(result_buffer, result->data_json, len + 1);
    return true;
}

// Best-effort cancel of a pending waiter (does not kill engine execute)
bool database_cancel_query(const char* query_id) {
    if (!database_subsystem || !query_id || query_id[0] == '\0') {
        return false;
    }

    PendingResultManager* manager = get_pending_result_manager();
    if (!manager) {
        return false;
    }

    return pending_result_cancel(manager, query_id, SR_DATABASE);
}

// Cleanup expired pending results (max_age is advisory; pending uses per-query timeout)
size_t database_cleanup_old_results(time_t max_age_seconds __attribute__((unused))) {
    if (!database_subsystem) {
        return 0;
    }

    PendingResultManager* manager = get_pending_result_manager();
    if (!manager) {
        return 0;
    }

    return pending_result_cleanup_expired(manager, SR_DATABASE);
}

// Helper function to calculate query age from a single queue
time_t calculate_queue_query_age(DatabaseQueue* db_queue) {
    if (!db_queue || !db_queue->queue) {
        return 0;
    }

    time_t query_age = 0;

    // Check lead queue depth
    size_t depth = queue_size(db_queue->queue);
    if (depth > 0) {
        // Query might be in this queue - use oldest element age
        long age_ms = queue_oldest_element_age(db_queue->queue);
        if (age_ms > 0) {
            query_age = age_ms / 1000; // Convert ms to seconds
        }
    }

    // Check child queues
    MutexResult child_lock = MUTEX_LOCK(&db_queue->children_lock, SR_DATABASE);
    if (child_lock == MUTEX_SUCCESS) {
        for (int j = 0; j < db_queue->child_queue_count; j++) {
            DatabaseQueue* child = db_queue->child_queues[j];
            if (child && child->queue) {
                size_t child_depth = queue_size(child->queue);
                if (child_depth > 0) {
                    long child_age_ms = queue_oldest_element_age(child->queue);
                    if (child_age_ms > 0 && child_age_ms / 1000 > query_age) {
                        query_age = child_age_ms / 1000;
                    }
                }
            }
        }
        mutex_unlock(&db_queue->children_lock);
    }

    return query_age;
}

// Helper function to find maximum query age across all queues
time_t find_max_query_age_across_queues(void) {
    if (!global_queue_manager) {
        return 0;
    }

    MutexResult lock_result = MUTEX_LOCK(&global_queue_manager->manager_lock, SR_DATABASE);
    if (lock_result != MUTEX_SUCCESS) {
        return 0;
    }

    time_t max_query_age = 0;

    // Search through all database queues
    for (size_t i = 0; i < global_queue_manager->database_count; i++) {
        DatabaseQueue* db_queue = global_queue_manager->databases[i];
        time_t queue_age = calculate_queue_query_age(db_queue);
        if (queue_age > max_query_age) {
            max_query_age = queue_age;
        }
    }

    mutex_unlock(&global_queue_manager->manager_lock);
    return max_query_age;
}

// Get age of a specific pending query, or oldest in-flight queue age as fallback
time_t database_get_query_age(const char* query_id) {
    if (!database_subsystem || !query_id || query_id[0] == '\0') {
        return 0;
    }

    PendingResultManager* manager = get_pending_result_manager();
    if (manager) {
        PendingQueryResult* pending = pending_result_find(manager, query_id);
        if (pending && pending->submitted_at > 0) {
            time_t now = time(NULL);
            if (now >= pending->submitted_at) {
                return now - pending->submitted_at;
            }
            return 0;
        }
    }

    // Fallback: oldest element age across queues (not id-specific)
    return find_max_query_age_across_queues();
}
