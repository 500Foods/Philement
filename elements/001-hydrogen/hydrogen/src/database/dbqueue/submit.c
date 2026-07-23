/*
 * Database Queue Submission Functions
 *
 * Implements query submission and processing functions for the Hydrogen database subsystem.
 * Split from database_queue.c for better maintainability.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/database/database_pending.h>
#include <src/database/dbqueue/query_result_cache.h>

// Local includes
#include "dbqueue.h"

/*
 * Serialize a DatabaseQuery to JSON string for queue storage
 *
 * Serializes the essential fields needed for query execution:
 * - query_id: Unique identifier for tracking the query
 * - query_template: The SQL template to execute
 * - parameter_json: JSON string containing named parameters
 * - queue_type_hint: Queue priority hint for routing
 *
 * Returns: Allocated JSON string (caller must free), or NULL on error
 */
char* serialize_query_to_json(DatabaseQuery* query) {
    if (!query || !query->query_template) return NULL;

    // Create JSON object using jansson for proper escaping
    json_t* root = json_object();
    if (!root) return NULL;

    // Add query_id (may be NULL)
    if (query->query_id) {
        json_object_set_new(root, "query_id", json_string(query->query_id));
    } else {
        json_object_set_new(root, "query_id", json_null());
    }

    // Add query_template (required)
    json_object_set_new(root, "query_template", json_string(query->query_template));

    // Add parameter_json (may be NULL)
    if (query->parameter_json) {
        json_object_set_new(root, "parameter_json", json_string(query->parameter_json));
    } else {
        json_object_set_new(root, "parameter_json", json_null());
    }

    // Add queue_type_hint
    json_object_set_new(root, "queue_type_hint", json_integer(query->queue_type_hint));

    // Add submitted_at
    json_object_set_new(root, "submitted_at", json_integer((json_int_t)query->submitted_at));

    // Add submitted_at_ns
    json_object_set_new(root, "submitted_at_ns", json_integer((json_int_t)query->submitted_at_ns));

    // Serialize to string
    char* json_str = json_dumps(root, JSON_COMPACT);
    json_decref(root);

    return json_str;
}

/*
 * Build a DatabaseQuery from a completed QueryResult
 *
 * Converts a worker-produced QueryResult into the DatabaseQuery that
 * database_queue_await_result returns to its synchronous caller. On the
 * success path it copies query_template (from data_json), error_message,
 * and affected_rows. When query_result is NULL (the query completed but
 * produced no result) it synthesizes an error message. The caller owns the
 * returned DatabaseQuery and is responsible for freeing query_id,
 * query_template, and error_message.
 *
 * Returns: Allocated DatabaseQuery (caller must free fields and struct), or
 *          NULL on allocation failure.
 */
DatabaseQuery* database_query_build_from_result(const char* query_id, QueryResult* query_result, const char* dqm_label) {
    if (!query_id) return NULL;

    DatabaseQuery* db_query = malloc(sizeof(DatabaseQuery));
    if (!db_query) {
        log_this(dqm_label ? dqm_label : SR_DATABASE, "Memory allocation failed for DatabaseQuery", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    memset(db_query, 0, sizeof(DatabaseQuery));
    db_query->query_id = strdup(query_id);
    db_query->processed_at = time(NULL);

    if (query_result) {
        // Store the JSON result data as the query template (for backward compatibility)
        // In a real implementation, we'd have a proper result field in DatabaseQuery
        if (query_result->data_json) {
            db_query->query_template = strdup(query_result->data_json);
        }

        // Store error message if present
        if (query_result->error_message) {
            db_query->error_message = strdup(query_result->error_message);
        }

        // Phase 14: propagate affected_rows for atomic task claiming.
        // The engine layer (sqlite/postgres/mysql/db2) populates
        // QueryResult.affected_rows from the underlying driver
        // (PQcmdTuples / sqlite3_changes / mysql_affected_rows /
        // DB2 sqlca.sqlerrd[2]). A conditional UPDATE such as
        //   UPDATE tasks SET status='taken'
        //    WHERE id=:id AND status='open'
        // reports 1 to the winner and 0 to the losers, so the
        // scripting host and the conduit REST layer can rely on
        // db_query->affected_rows for exactly-once claim semantics.
        db_query->affected_rows = query_result->affected_rows;

        // Log success with statistics
        log_this(dqm_label, "Query completed successfully: %s (rows: %zu, columns: %zu, affected: %d, time: %ld ms)",
                LOG_LEVEL_TRACE, 5, query_id, query_result->row_count,
                query_result->column_count, query_result->affected_rows,
                query_result->execution_time_ms);
    } else {
        // Query failed - result is NULL
        db_query->error_message = strdup("Query execution failed or timed out");
        log_this(dqm_label, "Query completed with NULL result: %s", LOG_LEVEL_ALERT, 1, query_id);
    }

    return db_query;
}

/*
 * Deserialize a JSON string back to a DatabaseQuery structure
 *
 * Parses JSON and extracts:
 * - query_id: Unique identifier
 * - query_template: SQL template to execute
 * - parameter_json: Named parameters as JSON string
 * - queue_type_hint: Queue priority
 *
 * Returns: Allocated DatabaseQuery (caller must free fields and struct), or NULL on error
 */
DatabaseQuery* deserialize_query_from_json(const char* json_str) {
    if (!json_str) return NULL;

    // Parse JSON string
    json_error_t error;
    json_t* root = json_loads(json_str, 0, &error);
    if (!root) {
        log_this(SR_DATABASE, "Failed to parse query JSON: %s at line %d", LOG_LEVEL_ERROR, 2, error.text, error.line);
        return NULL;
    }

    // Allocate DatabaseQuery structure
    DatabaseQuery* query = malloc(sizeof(DatabaseQuery));
    if (!query) {
        json_decref(root);
        return NULL;
    }
    memset(query, 0, sizeof(DatabaseQuery));

    // Extract query_id
    json_t* query_id_json = json_object_get(root, "query_id");
    if (query_id_json && json_is_string(query_id_json)) {
        query->query_id = strdup(json_string_value(query_id_json));
    }

    // Extract query_template (required)
    json_t* template_json = json_object_get(root, "query_template");
    if (template_json && json_is_string(template_json)) {
        query->query_template = strdup(json_string_value(template_json));
    } else {
        // query_template is required - fail if missing
        log_this(SR_DATABASE, "Query JSON missing required 'query_template' field", LOG_LEVEL_ERROR, 0);
        free(query->query_id);
        free(query);
        json_decref(root);
        return NULL;
    }

    // Extract parameter_json
    json_t* params_json = json_object_get(root, "parameter_json");
    if (params_json && json_is_string(params_json)) {
        query->parameter_json = strdup(json_string_value(params_json));
    }

    // Extract queue_type_hint
    json_t* hint_json = json_object_get(root, "queue_type_hint");
    if (hint_json && json_is_integer(hint_json)) {
        query->queue_type_hint = (int)json_integer_value(hint_json);
    }

    // Extract submitted_at
    json_t* submitted_json = json_object_get(root, "submitted_at");
    if (submitted_json && json_is_integer(submitted_json)) {
        query->submitted_at = (time_t)json_integer_value(submitted_json);
    }

    // Extract submitted_at_ns
    json_t* submitted_ns_json = json_object_get(root, "submitted_at_ns");
    if (submitted_ns_json && json_is_integer(submitted_ns_json)) {
        query->submitted_at_ns = (uint64_t)json_integer_value(submitted_ns_json);
    }

    json_decref(root);
    return query;
}

/*
 * Submit query to appropriate database queue based on routing logic
 */
bool database_queue_submit_query(DatabaseQueue* db_queue, DatabaseQuery* query) {
    if (!db_queue || !query || !query->query_template) return false;

    // Check the global query-result cache for cache-type queries.  If we have a
    // cached result, signal the already-registered pending result and skip the
    // queue entirely.  This is transparent to all callers.
    if (query->query_id && query->queue_type_hint == DB_QUEUE_CACHE) {
        QueryResultCache* cache = query_result_cache_get_global();
        if (cache) {
            json_t* cached_data = NULL;
            size_t row_count = 0;
            size_t column_count = 0;
            int affected_rows = 0;
            time_t execution_time_ms = 0;

            if (query_result_cache_get(cache, db_queue->database_name, query->query_template,
                                       query->parameter_json, &cached_data, &row_count,
                                       &column_count, &affected_rows, &execution_time_ms)) {
                QueryResult* result = calloc(1, sizeof(QueryResult));
                if (result) {
                    result->success = true;
                    result->data_json = json_dumps(cached_data, JSON_COMPACT);
                    result->row_count = row_count;
                    result->column_count = column_count;
                    result->affected_rows = affected_rows;
                    result->execution_time_ms = execution_time_ms;
                    result->error_class = DB_ERR_NONE;

                    if (result->data_json) {
                        PendingResultManager* pending_mgr = get_pending_result_manager();
                        bool signaled = false;
                        if (pending_mgr) {
                            signaled = pending_result_signal_ready(pending_mgr, query->query_id, result, NULL);
                        }

                        if (!signaled) {
                            database_engine_cleanup_result(result);
                        }

                        json_decref(cached_data);
                        return true;
                    }

                    free(result);
                }
                json_decref(cached_data);
            }
        }
    }

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

            if (target_child) {
                // Route to child queue while still holding children_lock
                // This prevents race condition where another thread could destroy
                // the child queue between finding it and using it (use-after-free)
                // Safe because child queues don't access parent's children_lock
                bool result = database_queue_submit_query(target_child, query);
                mutex_unlock(&db_queue->children_lock);
                return result;
            }
            mutex_unlock(&db_queue->children_lock);
        }

        if (!target_child) {
            // No appropriate child queue exists, use Lead queue itself for now
            // log_this(SR_DATABASE, "No %s child queue found, using Lead queue for query: %s", LOG_LEVEL_TRACE, 2, target_queue_type, query->query_id);
        }
    }

    // Submit to this queue's single queue
    if (!db_queue->queue) {
        log_this(SR_DATABASE, "No queue available for query: %s", LOG_LEVEL_ERROR, 1, query->query_id);
        return false;
    }

    // Set timestamps before serialization
    query->submitted_at = time(NULL);

    // Set high-precision timestamp for accurate timing
    struct timeval submit_time;
    gettimeofday(&submit_time, NULL);
    query->submitted_at_ns = (uint64_t)submit_time.tv_sec * 1000000ULL + (uint64_t)submit_time.tv_usec;

    // Serialize query to JSON for queue storage
    char* query_json = serialize_query_to_json(query);
    if (!query_json) return false;

    // Submit to queue with priority based on queue type
    bool success = queue_enqueue(db_queue->queue, query_json, strlen(query_json), query->queue_type_hint);

    free(query_json);

    if (success) {
        __sync_fetch_and_add(&db_queue->current_queue_depth, 1);

        // Update last request time for queue selection algorithm
        db_queue->last_request_time = time(NULL);

        // Record DQM statistics
        int queue_type_index = query->queue_type_hint;  // 0=slow, 1=medium, 2=fast, 3=cache
        if (db_queue->is_lead_queue) {
            queue_type_index = 4;  // Lead queue
        }
        database_queue_record_query_submission(db_queue, queue_type_index);

        // Signal worker thread that work is available
        sem_post(&db_queue->worker_semaphore);

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

/*
 * Wait for query result with timeout (synchronous execution)
 *
 * This function implements synchronous query execution by:
 * 1. Registering a pending result for the query_id
 * 2. Waiting for the worker thread to signal completion
 * 3. Converting the QueryResult back to DatabaseQuery structure
 *
 * Returns: DatabaseQuery with result data, or NULL on timeout/error
 */
DatabaseQuery* database_queue_await_result(DatabaseQueue* db_queue, const char* query_id, int timeout_seconds) {
    if (!db_queue || !query_id) {
        log_this(SR_DATABASE, "Invalid parameters for await_result", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // Get DQM label for logging
    char* dqm_label = database_queue_generate_label(db_queue);
    if (!dqm_label) {
        log_this(SR_DATABASE, "Failed to generate DQM label", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // Get the pending result manager
    PendingResultManager* pending_mgr = get_pending_result_manager();
    if (!pending_mgr) {
        log_this(dqm_label, "Pending result manager not initialized", LOG_LEVEL_ERROR, 0);
        free(dqm_label);
        return NULL;
    }

    /*
     * Prefer a waiter that was registered before queue submit (H.query /
     * conduit / orchestrator). Re-registering after a fast worker has
     * already signaled loses the result and hangs until timeout.
     */
    PendingQueryResult* pending = pending_result_find(pending_mgr, query_id);
    if (!pending) {
        pending = pending_result_register(pending_mgr, query_id, timeout_seconds, dqm_label);
    }
    if (!pending) {
        log_this(dqm_label, "Failed to register pending result for query: %s", LOG_LEVEL_ERROR, 1, query_id);
        free(dqm_label);
        return NULL;
    }

    log_this(dqm_label, "Waiting for result of query: %s (timeout: %d seconds)", LOG_LEVEL_TRACE, 2, query_id, timeout_seconds);

    // Wait for the result (blocks until completed or timeout)
    int wait_result = pending_result_wait(pending, dqm_label);

    if (wait_result != 0) {
        // Check if timeout before unregistering
        bool timed_out = pending_result_is_timed_out(pending);
        
        // Clean up the pending entry
        pending_result_unregister(pending_mgr, pending, dqm_label);
        
        if (timed_out) {
            log_this(dqm_label, "Query timed out: %s", LOG_LEVEL_ALERT, 1, query_id);
            // Record timeout in DQM statistics
            database_queue_record_timeout(db_queue);
        } else {
            log_this(dqm_label, "Query wait failed: %s", LOG_LEVEL_ERROR, 1, query_id);
        }
        free(dqm_label);
        return NULL;
    }

    // Get the result (pointer into pending->result, not a copy)
    QueryResult* query_result = pending_result_get(pending);

    // Convert the QueryResult into the DatabaseQuery we return. This is
    // split out into a separate helper so it can be unit-tested directly
    // without the synchronous wait machinery.
    DatabaseQuery* db_query = database_query_build_from_result(query_id, query_result, dqm_label);
    if (!db_query) {
        // Clean up the pending entry even on allocation failure
        pending_result_unregister(pending_mgr, pending, dqm_label);
        free(dqm_label);
        return NULL;
    }

    // Clean up the pending entry (frees pending struct, query_id, QueryResult, sync primitives)
    // This must be done AFTER we've copied all data from query_result via strdup above
    pending_result_unregister(pending_mgr, pending, dqm_label);

    free(dqm_label);
    return db_query;
}
