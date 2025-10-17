/*
 * Database Bootstrap Query Implementation
 *
 * Implements bootstrap query execution after database connection establishment
 * for Lead queues to initialize the Query Table Cache (QTC).
 */

#include <src/hydrogen.h>

// Local includes
#include "dbqueue/dbqueue.h"
#include "database.h"
#include "database_bootstrap.h"
#include "migration/migration.h"

/*
 * Execute bootstrap query after successful Lead DQM connection
 * This loads the Query Table Cache (QTC) and confirms connection health
 */
void database_queue_execute_bootstrap_query(DatabaseQueue* db_queue) {
    if (!db_queue || !db_queue->is_lead_queue) {
        return;
    }

    char* dqm_label = database_queue_generate_label(db_queue);
    // log_this(dqm_label, "Executing bootstrap query", LOG_LEVEL_TRACE, 0);

    // Use configured bootstrap query or fallback to safe default
    const char* bootstrap_query = db_queue->bootstrap_query ? db_queue->bootstrap_query : "SELECT 42 as test_value";
    // log_this(dqm_label, "Bootstrap query SQL: %s", LOG_LEVEL_TRACE, 1, bootstrap_query);

    // Create query request
    QueryRequest* request = calloc(1, sizeof(QueryRequest));
    if (!request) {
        log_this(dqm_label, "Failed to allocate query request for bootstrap", LOG_LEVEL_ERROR, 0);
        free(dqm_label);
        return;
    }

    request->query_id = strdup("bootstrap_query");
    request->sql_template = strdup(bootstrap_query);
    request->parameters_json = strdup("{}");
    request->timeout_seconds = 1; // Very short timeout for bootstrap
    request->isolation_level = DB_ISOLATION_READ_COMMITTED;
    request->use_prepared_statement = false;

    // log_this(dqm_label, "Bootstrap query request created - timeout: %d seconds", LOG_LEVEL_TRACE, 1, request->timeout_seconds);

    // Execute query using persistent connection
    QueryResult* result = NULL;
    bool query_success = false;

    // Add timeout protection
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += 1; // 1 second timeout for the entire operation

    log_this(dqm_label, "Bootstrap query submitted", LOG_LEVEL_TRACE, 0);

    // log_this(dqm_label, "Attempting to acquire connection lock with 5 second timeout", LOG_LEVEL_TRACE, 0);

    // log_this(dqm_label, "MUTEX_BOOTSTRAP_LOCK: About to lock connection mutex for bootstrap", LOG_LEVEL_TRACE, 0);
    MutexResult lock_result = MUTEX_LOCK(&db_queue->connection_lock, dqm_label);
    if (lock_result == MUTEX_SUCCESS) {
        // log_this(dqm_label, "MUTEX_BOOTSTRAP_LOCK_SUCCESS: Connection lock acquired successfully", LOG_LEVEL_TRACE, 0);

        if (db_queue->persistent_connection) {
            // og_this(dqm_label, "Persistent connection available, executing query", LOG_LEVEL_TRACE, 0);

            // Debug the connection handle before calling database_engine_execute
            // log_this(dqm_label, "Connection handle: %p", LOG_LEVEL_TRACE, 1, (void*)db_queue->persistent_connection);

            // CRITICAL: Validate connection integrity before use
            DatabaseEngine original_engine_type = db_queue->persistent_connection->engine_type;
            // log_this(dqm_label, "Connection engine_type: %d", LOG_LEVEL_TRACE, 1, (int)original_engine_type);

            // Add memory corruption detection - check for valid engine types
            if (original_engine_type >= DB_ENGINE_MAX || original_engine_type < 0) {
                log_this(dqm_label, "CRITICAL ERROR: Connection engine_type corrupted! Invalid value %d (must be 0-%d)", LOG_LEVEL_ERROR, 2, (int)original_engine_type, DB_ENGINE_MAX-1);
                log_this(dqm_label, "Aborting bootstrap query to prevent hang", LOG_LEVEL_ERROR, 0);
                mutex_unlock(&db_queue->connection_lock);
                goto cleanup;
            }

            // log_this(dqm_label, "Request: %p, query: '%s'", LOG_LEVEL_TRACE, 2, (void*)request, request->sql_template ? request->sql_template : "NULL");

            // log_this(dqm_label, "About to call database_engine_execute - this is where it might hang", LOG_LEVEL_TRACE, 0);

            struct timespec query_start_time;
            clock_gettime(CLOCK_MONOTONIC, &query_start_time);

            // Validate connection integrity again right before the call
            if (db_queue->persistent_connection->engine_type != original_engine_type) {
                log_this(dqm_label, "CRITICAL ERROR: Connection engine_type changed from %d to %d during bootstrap!", LOG_LEVEL_ERROR, 2,
                    (int)original_engine_type,
                    (int)db_queue->persistent_connection->engine_type);
                log_this(dqm_label, "Memory corruption detected - aborting bootstrap", LOG_LEVEL_ERROR, 0);
                mutex_unlock(&db_queue->connection_lock);
                goto cleanup;
            }

            // Simple direct call with extensive debugging
            // log_this(dqm_label, "BOOTSTRAP HANG DEBUG: About to call database_engine_execute", LOG_LEVEL_ERROR, 0);
            // log_this(dqm_label, "BOOTSTRAP HANG DEBUG: If you see this but not the next message, the hang is in database_engine_execute", LOG_LEVEL_ERROR, 0);

            // CRITICAL: Stack corruption detection
            DatabaseHandle* connection_to_use = db_queue->persistent_connection;

            // Simple logging without complex format strings to avoid buffer overflow
            // log_this(dqm_label, "Stack validation: connection_to_use assigned", LOG_LEVEL_ERROR, 0);
            // log_this(dqm_label, "Stack validation: checking pointer validity", LOG_LEVEL_ERROR, 0);

            if (!connection_to_use || (uintptr_t)connection_to_use < 0x1000) {
                log_this(dqm_label, "CRITICAL ERROR: Connection pointer corrupted - aborting", LOG_LEVEL_ERROR, 0);
                mutex_unlock(&db_queue->connection_lock);
                goto cleanup;
            }

            // log_this(dqm_label, "Stack validation: checking engine type", LOG_LEVEL_ERROR, 0);

            if (connection_to_use->engine_type >= DB_ENGINE_MAX || connection_to_use->engine_type < 0) {
                log_this(dqm_label, "CRITICAL ERROR: Connection engine_type corrupted (invalid value %d) - aborting", LOG_LEVEL_ERROR, 1, (int)connection_to_use->engine_type);
                mutex_unlock(&db_queue->connection_lock);
                goto cleanup;
            }

            // log_this(dqm_label, "Stack validation: about to call function", LOG_LEVEL_ERROR, 0);

            // CRITICAL: Dump connection BEFORE the call to see if it's valid
            // debug_dump_connection("BEFORE", connection_to_use, dqm_label);

            // Add diagnostic logging for SQLite connections
            if (connection_to_use->engine_type == DB_ENGINE_SQLITE) {
                // Cast to SQLiteConnection to access db_path
                const void* sqlite_handle = connection_to_use->connection_handle;
                if (sqlite_handle) {
                    // We can't directly access the SQLiteConnection struct from here due to header dependencies
                    // Instead, log that we're using SQLite and the connection appears valid
                    log_this(dqm_label, "SQLite bootstrap query: Connection handle is valid", LOG_LEVEL_TRACE, 0);
                }
            }

            // Call with minimal parameters to reduce stack corruption risk
            query_success = database_engine_execute(connection_to_use, request, &result);

            // CRITICAL: Dump connection AFTER the call to see what changed
            // debug_dump_connection("AFTER", connection_to_use, dqm_label);

            // log_this(dqm_label, "BOOTSTRAP HANG DEBUG: database_engine_execute call completed", LOG_LEVEL_ERROR, 0);

            struct timespec query_end_time;
            clock_gettime(CLOCK_MONOTONIC, &query_end_time);

            // log_this(dqm_label, "Query execution completed (direct method)", LOG_LEVEL_TRACE, 0);

            // Log bootstrap query result (success or failure)
            if (query_success && result && result->success) {
                double execution_time = ((double)query_end_time.tv_sec - (double)query_start_time.tv_sec) +
                                       ((double)query_end_time.tv_nsec - (double)query_start_time.tv_nsec) / 1e9;

                log_this(dqm_label, "Bootstrap query completed in %.3fs: returned %zu rows, %zu columns, affected %d rows",
                         LOG_LEVEL_DEBUG, 4, execution_time, result->row_count, result->column_count, result->affected_rows);

                // Log detailed result information
                if (result->row_count > 0 && result->column_count > 0 && result->data_json) {
                    // log_this(dqm_label, "Bootstrap query result data: %s", LOG_LEVEL_TRACE, 1, result->data_json);
                }
            } else {
                log_this(dqm_label, "Bootstrap query failed: success=%d, result=%p, error=%s", LOG_LEVEL_ALERT, 3,
                        query_success,
                        (void*)result,
                        result && result->error_message ? result->error_message : "Unknown error");
                log_this(dqm_label, "Bootstrap failure is acceptable - migrations will still run if enabled", LOG_LEVEL_ALERT, 0);
            }

            // Migration processing moved to Lead DQM conductor pattern
            // Now handled by database_queue_lead_run_migration() in lead.c

            // Launch additional DQMs based on configuration regardless of bootstrap/migration results
            if (query_success && result && result->success) {

                // Launch additional DQMs based on configuration
                if (app_config) {
                    const DatabaseConnection* conn_config = NULL;
                    for (int i = 0; i < app_config->databases.connection_count; i++) {
                        if (strcmp(app_config->databases.connections[i].name, db_queue->database_name) == 0) {
                            conn_config = &app_config->databases.connections[i];
                            break;
                        }
                    }

                    if (conn_config) {
                        // Generate Lead queue label before spawning (tags will be removed)
                        char* lead_label_before = database_queue_generate_label(db_queue);

                        // Launch queues based on start configuration
                        if (conn_config->queues.slow.start > 0) {
                            for (int i = 0; i < conn_config->queues.slow.start; i++) {
                                database_queue_spawn_child_queue(db_queue, QUEUE_TYPE_SLOW);
                            }
                        }
                        if (conn_config->queues.medium.start > 0) {
                            for (int i = 0; i < conn_config->queues.medium.start; i++) {
                                database_queue_spawn_child_queue(db_queue, QUEUE_TYPE_MEDIUM);
                            }
                        }
                        if (conn_config->queues.fast.start > 0) {
                            for (int i = 0; i < conn_config->queues.fast.start; i++) {
                                database_queue_spawn_child_queue(db_queue, QUEUE_TYPE_FAST);
                            }
                        }
                        if (conn_config->queues.cache.start > 0) {
                            for (int i = 0; i < conn_config->queues.cache.start; i++) {
                                database_queue_spawn_child_queue(db_queue, QUEUE_TYPE_CACHE);
                            }
                        }

                        // Lead queue retains its original tags

                        // Log queue summary after spawning
                        int total_queues = 1; // Lead queue
                        total_queues += conn_config->queues.slow.start;
                        total_queues += conn_config->queues.medium.start;
                        total_queues += conn_config->queues.fast.start;
                        total_queues += conn_config->queues.cache.start;

                        log_group_begin();
                        log_this(dqm_label, "%s Queues: %d", LOG_LEVEL_TRACE, 2, db_queue->database_name, total_queues);

                        // Lead queue (use the label from before spawning)
                        log_this(dqm_label, "Lead: 1 (%s)", LOG_LEVEL_TRACE, 1, lead_label_before);
                        free(lead_label_before);

                        // Slow queues
                        if (conn_config->queues.slow.start > 0) {
                            char* slow_descriptors[10]; // Max 10 for logging
                            int slow_count = 0;
                            for (int i = 0; i < db_queue->child_queue_count && slow_count < conn_config->queues.slow.start; i++) {
                                if (db_queue->child_queues[i] && strcmp(db_queue->child_queues[i]->queue_type, QUEUE_TYPE_SLOW) == 0) {
                                    slow_descriptors[slow_count] = database_queue_generate_label(db_queue->child_queues[i]);
                                    slow_count++;
                                }
                            }
                            if (slow_count > 0) {
                                char descriptors_str[512] = "";
                                for (int i = 0; i < slow_count; i++) {
                                    if (i > 0) strcat(descriptors_str, ", ");
                                    strncat(descriptors_str, slow_descriptors[i], sizeof(descriptors_str) - strlen(descriptors_str) - 1);
                                    free(slow_descriptors[i]);
                                }
                                log_this(dqm_label, "Slow: %d (%s)", LOG_LEVEL_TRACE, 2, slow_count, descriptors_str);
                            }
                        } else {
                            log_this(dqm_label, "Slow: 0", LOG_LEVEL_TRACE, 0);
                        }

                        // Medium queues
                        if (conn_config->queues.medium.start > 0) {
                            char* medium_descriptors[10];
                            int medium_count = 0;
                            for (int i = 0; i < db_queue->child_queue_count && medium_count < conn_config->queues.medium.start; i++) {
                                if (db_queue->child_queues[i] && strcmp(db_queue->child_queues[i]->queue_type, QUEUE_TYPE_MEDIUM) == 0) {
                                    medium_descriptors[medium_count] = database_queue_generate_label(db_queue->child_queues[i]);
                                    medium_count++;
                                }
                            }
                            if (medium_count > 0) {
                                char descriptors_str[512] = "";
                                for (int i = 0; i < medium_count; i++) {
                                    if (i > 0) strcat(descriptors_str, ", ");
                                    strncat(descriptors_str, medium_descriptors[i], sizeof(descriptors_str) - strlen(descriptors_str) - 1);
                                    free(medium_descriptors[i]);
                                }
                                log_this(dqm_label, "Medium: %d (%s)", LOG_LEVEL_TRACE, 2, medium_count, descriptors_str);
                            }
                        } else {
                            log_this(dqm_label, "Medium: 0", LOG_LEVEL_TRACE, 0);
                        }

                        // Fast queues
                        if (conn_config->queues.fast.start > 0) {
                            char* fast_descriptors[10];
                            int fast_count = 0;
                            for (int i = 0; i < db_queue->child_queue_count && fast_count < conn_config->queues.fast.start; i++) {
                                if (db_queue->child_queues[i] && strcmp(db_queue->child_queues[i]->queue_type, QUEUE_TYPE_FAST) == 0) {
                                    fast_descriptors[fast_count] = database_queue_generate_label(db_queue->child_queues[i]);
                                    fast_count++;
                                }
                            }
                            if (fast_count > 0) {
                                char descriptors_str[512] = "";
                                for (int i = 0; i < fast_count; i++) {
                                    if (i > 0) strcat(descriptors_str, ", ");
                                    strncat(descriptors_str, fast_descriptors[i], sizeof(descriptors_str) - strlen(descriptors_str) - 1);
                                    free(fast_descriptors[i]);
                                }
                                log_this(dqm_label, "Fast: %d (%s)", LOG_LEVEL_TRACE, 2, fast_count, descriptors_str);
                            }
                        } else {
                            log_this(dqm_label, "Fast: 0", LOG_LEVEL_TRACE, 0);
                        }

                        // Cache queues
                        if (conn_config->queues.cache.start > 0) {
                            char* cache_descriptors[10];
                            int cache_count = 0;
                            for (int i = 0; i < db_queue->child_queue_count && cache_count < conn_config->queues.cache.start; i++) {
                                if (db_queue->child_queues[i] && strcmp(db_queue->child_queues[i]->queue_type, QUEUE_TYPE_CACHE) == 0) {
                                    cache_descriptors[cache_count] = database_queue_generate_label(db_queue->child_queues[i]);
                                    cache_count++;
                                }
                            }
                            if (cache_count > 0) {
                                char descriptors_str[512] = "";
                                for (int i = 0; i < cache_count; i++) {
                                    if (i > 0) strcat(descriptors_str, ", ");
                                    strncat(descriptors_str, cache_descriptors[i], sizeof(descriptors_str) - strlen(descriptors_str) - 1);
                                    free(cache_descriptors[i]);
                                }
                                log_this(dqm_label, "Cache: %d (%s)", LOG_LEVEL_TRACE, 2, cache_count, descriptors_str);
                            }
                        } else {
                            log_this(dqm_label, "Cache: 0", LOG_LEVEL_TRACE, 0);
                        }
                        log_group_end();
                    }
                }
            }

            // Signal bootstrap completion for launch synchronization
            MutexResult bootstrap_lock_result = MUTEX_LOCK(&db_queue->bootstrap_lock, dqm_label);
            if (bootstrap_lock_result == MUTEX_SUCCESS) {
                db_queue->bootstrap_completed = true;
                pthread_cond_broadcast(&db_queue->bootstrap_cond);
                mutex_unlock(&db_queue->bootstrap_lock);
            }

            // Log completion message for test synchronization
            log_this(dqm_label, "Lead DQM initialization is complete for %s", LOG_LEVEL_TRACE, 1, db_queue->database_name);

        } else {
            // log_this(dqm_label, "No persistent connection available for bootstrap query", LOG_LEVEL_ERROR, 0);

            // Signal bootstrap completion even when no connection available
            MutexResult bootstrap_lock_result3 = MUTEX_LOCK(&db_queue->bootstrap_lock, dqm_label);
            if (bootstrap_lock_result3 == MUTEX_SUCCESS) {
                db_queue->bootstrap_completed = true;
                pthread_cond_broadcast(&db_queue->bootstrap_cond);
                mutex_unlock(&db_queue->bootstrap_lock);
            }
        }

        // log_this(dqm_label, "MUTEX_BOOTSTRAP_UNLOCK: About to unlock connection mutex", LOG_LEVEL_TRACE, 0);
        mutex_unlock(&db_queue->connection_lock);
        // log_this(dqm_label, "MUTEX_BOOTSTRAP_UNLOCK_SUCCESS: Connection mutex unlocked", LOG_LEVEL_TRACE, 0);
    } else {
        log_this(dqm_label, "Timeout waiting for connection lock in bootstrap query (1 seconds)", LOG_LEVEL_ERROR, 0);
        // log_this(dqm_label, "This indicates the connection lock is held by another thread", LOG_LEVEL_ERROR, 0);

        // Signal bootstrap completion on timeout to prevent launch hang
        MutexResult bootstrap_lock_result4 = MUTEX_LOCK(&db_queue->bootstrap_lock, dqm_label);
        if (bootstrap_lock_result4 == MUTEX_SUCCESS) {
            db_queue->bootstrap_completed = true;
            pthread_cond_broadcast(&db_queue->bootstrap_cond);
            mutex_unlock(&db_queue->bootstrap_lock);
        }
    }

cleanup:
    // Clean up
    if (request->query_id) free(request->query_id);
    if (request->sql_template) free(request->sql_template);
    if (request->parameters_json) free(request->parameters_json);
    free(request);

    if (result) {
        database_engine_cleanup_result(result);
    }

    free(dqm_label);
}