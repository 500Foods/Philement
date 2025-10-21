/*
 * Database Bootstrap Query Implementation
 *
 * Implements bootstrap query execution after database connection establishment
 * for Lead queues to initialize the Query Table Cache (QTC).
 */

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
#include "database_cache.h"
#include "migration/migration.h"

/*
 * Execute bootstrap query with QTC population control
 * This loads migration information and optionally populates QTC based on populate_qtc flag
 */
void database_queue_execute_bootstrap_query_full(DatabaseQueue* db_queue, bool populate_qtc) {
    if (!db_queue || !db_queue->is_lead_queue) {
        return;
    }

    char* dqm_label = database_queue_generate_label(db_queue);

    // Use the configured bootstrap query from database configuration
    // This will fail on empty databases (expected), then succeed after migrations create the queries table
    // Updated to include Conduit QTC fields: sql_template, description, queue_type, timeout_seconds
    const char* bootstrap_query = db_queue->bootstrap_query ? db_queue->bootstrap_query :
        "SELECT query_id, query_ref, query_status_a27 query_status, query_type_lua_28 query_type, query_dialect_lua_30 query_engine, query_queue_lua_58 query_queue, query_timeout, query_name, query_code query FROM test.queries WHERE (query_status_lua_27 = 1) ORDER BY query_type_lua_28 desc;";

    // Create query request
    QueryRequest* request = calloc(1, sizeof(QueryRequest));
    if (!request) {
        log_this(dqm_label, "Failed to allocate query request for bootstrap", LOG_LEVEL_ERROR, 0);
        free(dqm_label);
        return;
    }

    // At this point request is guaranteed to be non-NULL

    request->query_id = strdup("bootstrap_query");
    request->sql_template = strdup(bootstrap_query);
    request->parameters_json = strdup("{}");
    request->timeout_seconds = 1;
    request->isolation_level = DB_ISOLATION_READ_COMMITTED;
    request->use_prepared_statement = false;

    // Execute query using persistent connection
    log_this(dqm_label, "Bootstrap query submitted", LOG_LEVEL_TRACE, 0);

    // Declare result variable in function scope for cleanup
    QueryResult* result = NULL;

    MutexResult lock_result = MUTEX_LOCK(&db_queue->connection_lock, dqm_label);
    if (lock_result == MUTEX_SUCCESS) {
        if (db_queue->persistent_connection) {
            bool query_success = database_engine_execute(db_queue->persistent_connection, request, &result);

            // Parse bootstrap query results for migration information
            long long latest_available_migration = -1;
            long long latest_installed_migration = -1;
            bool empty_database = true;

            if (query_success && result && result->success) {
                log_this(dqm_label, "Bootstrap query succeeded: %zu rows, %zu columns", LOG_LEVEL_DEBUG, 2, result->row_count, result->column_count);

                // Initialize QTC if requested and not already done
                if (populate_qtc && !db_queue->query_cache) {
                    db_queue->query_cache = query_cache_create();
                    if (!db_queue->query_cache) {
                        log_this(dqm_label, "Failed to create query cache", LOG_LEVEL_ERROR, true, true, true);
                    }
                }

                // Parse bootstrap query results for migration information and optionally QTC
                if (result->row_count > 0 && result->data_json) {
                    empty_database = false;

                    json_t* root;
                    json_error_t error;
                    root = json_loads(result->data_json, 0, &error);

                    if (root && json_is_array(root)) {
                          size_t row_count = json_array_size(root);
                          if (populate_qtc) {
                              log_this(dqm_label, "Processing %zu bootstrap query rows for QTC and migrations", LOG_LEVEL_DEBUG, 1, row_count);
                          } else {
                              log_this(dqm_label, "Processing %zu bootstrap query rows for migration information", LOG_LEVEL_DEBUG, 1, row_count);
                          }

                          for (size_t i = 0; i < row_count; i++) {
                              json_t* row = json_array_get(root, i);
                              if (json_is_object(row)) {
                                 // Extract QTC fields (Conduit) if populating QTC
                                 if (populate_qtc) {
                                     json_t* query_ref_obj = json_object_get(row, "query_ref");
                                     json_t* sql_template_obj = json_object_get(row, "sql_template");
                                     json_t* description_obj = json_object_get(row, "description");
                                     json_t* queue_type_obj = json_object_get(row, "queue_type");
                                     json_t* timeout_seconds_obj = json_object_get(row, "timeout_seconds");

                                     // Process QTC entry if all required fields present
                                     if (query_ref_obj && json_is_integer(query_ref_obj) &&
                                         sql_template_obj && json_is_string(sql_template_obj) &&
                                         description_obj && json_is_string(description_obj) &&
                                         queue_type_obj && json_is_string(queue_type_obj) &&
                                         timeout_seconds_obj && json_is_integer(timeout_seconds_obj)) {

                                         int query_ref = (int)json_integer_value(query_ref_obj);
                                         const char* sql_template = json_string_value(sql_template_obj);
                                         const char* description = json_string_value(description_obj);
                                         const char* queue_type = json_string_value(queue_type_obj);
                                         int timeout_seconds = (int)json_integer_value(timeout_seconds_obj);

                                         // Create and add QTC entry
                                         QueryCacheEntry* entry = query_cache_entry_create(
                                             query_ref, sql_template, description, queue_type, timeout_seconds);

                                         if (entry && db_queue->query_cache) {
                                             if (query_cache_add_entry(db_queue->query_cache, entry)) {
                                                 log_this(dqm_label, "Added QTC entry: query_ref=%d, queue_type=%s",
                                                         LOG_LEVEL_DEBUG, 2, query_ref, queue_type);
                                             } else {
                                                 log_this(dqm_label, "Failed to add QTC entry: query_ref=%d", LOG_LEVEL_ERROR, 1, query_ref);
                                                 query_cache_entry_destroy(entry);
                                             }
                                         } else {
                                             log_this(dqm_label, "Failed to create QTC entry: query_ref=%d", LOG_LEVEL_ERROR, 1, query_ref);
                                         }
                                     }
                                 }

                                 // Extract migration fields (existing)
                                 json_t* query_type_obj = json_object_get(row, "query_type");
                                 json_t* query_ref_obj = json_object_get(row, "query_ref");

                                 // Process migration information (existing logic)
                                 if (query_type_obj && query_ref_obj && json_is_integer(query_ref_obj)) {
                                     long long query_type = json_integer_value(query_type_obj);
                                     long long query_ref = json_integer_value(query_ref_obj);

                                     if (query_type == 1000) {
                                          // Track the highest version found for available migrations (query_type = 1000)
                                          if (query_ref > latest_available_migration) {
                                              latest_available_migration = query_ref;
                                          }
                                      } else if (query_type == 1003) {
                                          // Track the highest version found for installed migrations (query_type = 1003)
                                          if (query_ref > latest_installed_migration) {
                                              latest_installed_migration = query_ref;
                                          }
                                      }
                                 }
                             }
                         }
                         json_decref(root);

                         // Log QTC population completion if QTC was populated
                         if (populate_qtc && db_queue->query_cache) {
                             size_t qtc_count = query_cache_get_entry_count(db_queue->query_cache);
                             log_this(dqm_label, "QTC population completed: %zu queries loaded", LOG_LEVEL_STATE, 1, qtc_count);
                         }
                    }
                }
            } else {
                // Bootstrap failed - expected for empty databases
                log_this(dqm_label, "Bootstrap query failed (expected for empty DB): %s", LOG_LEVEL_DEBUG, 1,
                         result && result->error_message ? result->error_message : "Unknown error");
                empty_database = true;
                latest_available_migration = 0;
                latest_installed_migration = 0;
            }

            // Store migration information in the queue structure
            db_queue->latest_available_migration = latest_available_migration;
            db_queue->latest_installed_migration = latest_installed_migration;
            db_queue->empty_database = empty_database;

            // Log clear decision
            if (empty_database) {
                log_this(dqm_label, "DECISION: Empty database detected - will run migrations", LOG_LEVEL_DEBUG, 0);
            } else {
                log_this(dqm_label, "DECISION: Database has queries - available=%lld, installed=%lld",
                         LOG_LEVEL_DEBUG, 2, latest_available_migration, latest_installed_migration);
            }

            // Signal bootstrap completion
            MutexResult bootstrap_lock_result = MUTEX_LOCK(&db_queue->bootstrap_lock, dqm_label);
            if (bootstrap_lock_result == MUTEX_SUCCESS) {
                db_queue->bootstrap_completed = true;
                pthread_cond_broadcast(&db_queue->bootstrap_cond);
                mutex_unlock(&db_queue->bootstrap_lock);
            }

            // DECISION: Comment out verbose bootstrap completion logging - keep only essential decision messages
            // log_this(dqm_label, "Bootstrap complete for %s - migration decision made", LOG_LEVEL_TRACE, 1, db_queue->database_name);
        }

        mutex_unlock(&db_queue->connection_lock);
    }

    // Clean up (request is guaranteed to be non-NULL at this point)
    if (request->query_id) free(request->query_id);
    if (request->sql_template) free(request->sql_template);
    if (request->parameters_json) free(request->parameters_json);
    free(request);

    if (result) {
        database_engine_cleanup_result(result);
    }

    free(dqm_label);
}

/*
 * Execute bootstrap query after successful Lead DQM connection
 * This loads migration information only (QTC population controlled separately)
 */
void database_queue_execute_bootstrap_query(DatabaseQueue* db_queue) {
    database_queue_execute_bootstrap_query_full(db_queue, false);
}

/*
 * Execute bootstrap query and populate QTC
 * This loads both migration information and populates the Query Table Cache
 */
void database_queue_execute_bootstrap_query_with_qtc(DatabaseQueue* db_queue) {
    database_queue_execute_bootstrap_query_full(db_queue, true);
}