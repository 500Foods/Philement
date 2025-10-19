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

    // Use the configured bootstrap query from database configuration
    // This will fail on empty databases (expected), then succeed after migrations create the queries table
    const char* bootstrap_query = db_queue->bootstrap_query ? db_queue->bootstrap_query :
        "SELECT query_id, query_ref, query_type_lua_28, query_dialect_lua_30, name, summary, code, collection FROM queries ORDER BY query_ref DESC";

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

                // Parse migration information from results
                if (result->row_count > 0 && result->data_json) {
                    empty_database = false;

                    json_t* root;
                    json_error_t error;
                    root = json_loads(result->data_json, 0, &error);

                    if (root && json_is_array(root)) {
                         size_t row_count = json_array_size(root);
                         // DECISION: Comment out verbose row parsing logging - keep only essential decision messages
                         // log_this(dqm_label, "Parsing %zu bootstrap query rows for migration info", LOG_LEVEL_TRACE, 1, row_count);

                         for (size_t i = 0; i < row_count; i++) {
                             json_t* row = json_array_get(root, i);
                             if (json_is_object(row)) {
                                 // DECISION: Comment out verbose row parsing logging - keep only essential decision messages
                                 // log_this(dqm_label, "Parsing %zu bootstrap query rows for migration info", LOG_LEVEL_TRACE, 1, row_count);
                                // DECISION: Comment out verbose field-by-field logging - keep only essential decision messages
                                // Debug: Log all field names and values in this row
                                // const char* key;
                                // json_t* value;
                                // log_this(dqm_label, "Row %zu fields:", LOG_LEVEL_TRACE, 1, i);
                                // json_object_foreach(row, key, value) {
                                //     if (json_is_integer(value)) {
                                //          log_this(dqm_label, "  %s = %lld", LOG_LEVEL_TRACE, 2, key, json_integer_value(value));
                                //      } else if (json_is_string(value)) {
                                //          log_this(dqm_label, "  %s = '%s'", LOG_LEVEL_TRACE, 2, key, json_string_value(value));
                                //      }
                                // }

                                // Case-insensitive field lookup
                                json_t* query_type_obj = NULL;
                                json_t* query_ref_obj = NULL;

                                // Convert field names to lowercase and find matches
                                const char* lowercase_key;
                                json_t* field_value;
                                json_object_foreach(row, lowercase_key, field_value) {
                                    // Convert to lowercase for comparison
                                    char lower_key[256];
                                    size_t j;
                                    for (j = 0; j < sizeof(lower_key) - 1 && lowercase_key[j]; j++) {
                                        lower_key[j] = (char)tolower(lowercase_key[j]);
                                    }
                                    lower_key[j] = '\0';

                                    if (strcmp(lower_key, "query_type_lua_28") == 0) {
                                        query_type_obj = field_value;
                                    } else if (strcmp(lower_key, "query_ref") == 0) {
                                        query_ref_obj = field_value;
                                    }
                                }

                                if ((query_type_obj && json_is_integer(query_type_obj)) &&
                                    (query_ref_obj && json_is_integer(query_ref_obj))) {

                                    long long query_type = json_integer_value(query_type_obj);
                                    long long query_ref = json_integer_value(query_ref_obj);

                                    // DECISION: Comment out verbose migration data logging - keep only essential decision messages
                                    // log_this(dqm_label, "Found migration data (integer values): type=%lld, ref=%lld", LOG_LEVEL_TRACE, 2, query_type, query_ref);

                                    // Process the migration data (same logic for both integer and string cases)
                                    if (query_type == 1000) {
                                         // Track the highest version found for available migrations (query_type = 1000)
                                         if (query_ref > latest_available_migration) {
                                             latest_available_migration = query_ref;
                                             // log_this(dqm_label, "Updated available migration to: %lld", LOG_LEVEL_TRACE, 1, query_ref);
                                         }
                                     } else if (query_type == 1003) {
                                         // Track the highest version found for installed migrations (query_type = 1003)
                                         if (query_ref > latest_installed_migration) {
                                             latest_installed_migration = query_ref;
                                             // log_this(dqm_label, "Updated installed migration to: %lld", LOG_LEVEL_TRACE, 1, query_ref);
                                         }
                                     }
                                } else if ((query_type_obj && json_is_string(query_type_obj)) &&
                                           (query_ref_obj && json_is_string(query_ref_obj))) {

                                    // Handle string values - convert to integers
                                    long long query_type = strtoll(json_string_value(query_type_obj), NULL, 10);
                                    long long query_ref = strtoll(json_string_value(query_ref_obj), NULL, 10);

                                    // DECISION: Comment out verbose migration data logging - keep only essential decision messages
                                    // log_this(dqm_label, "Found migration data (string values): type=%lld, ref=%lld", LOG_LEVEL_TRACE, 2, query_type, query_ref);

                                    // Process the migration data (same logic for both integer and string cases)
                                    if (query_type == 1000) {
                                         // Track the highest version found for available migrations (query_type = 1000)
                                         if (query_ref > latest_available_migration) {
                                             latest_available_migration = query_ref;
                                             // log_this(dqm_label, "Updated available migration to: %lld", LOG_LEVEL_TRACE, 1, query_ref);
                                         }
                                     } else if (query_type == 1003) {
                                         // Track the highest version found for installed migrations (query_type = 1003)
                                         if (query_ref > latest_installed_migration) {
                                             latest_installed_migration = query_ref;
                                             // log_this(dqm_label, "Updated installed migration to: %lld", LOG_LEVEL_TRACE, 1, query_ref);
                                         }
                                     }
                                } else {
                                     // DECISION: Comment out verbose missing fields logging - keep only essential decision messages
                                     // log_this(dqm_label, "Row %zu missing required migration fields", LOG_LEVEL_TRACE, 1, i);
                                 }
                            }
                        }
                        json_decref(root);
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