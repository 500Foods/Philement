/*
 * Database Queue Lead - Migration TEST Phase
 *
 * TEST Phase: Reverse applied migrations for testing
 * - Apply reverse migrations (type=1001) to undo forward migrations
 * - Start with highest APPLY value and work backwards
 * - Re-run bootstrap query between each reverse migration
 * - Stop when APPLY reaches 0 or no more reverse migrations found
 */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/database/database_bootstrap.h>
#include <src/database/migration/migration.h>
#include <src/database/database_engine.h>

// Local includes
#include "dbqueue.h"

/*
 * Find the next reverse migration to apply for test migration
 * Returns migration ID if found, 0 if no more reverse migrations to apply
 *
 * This function uses the bootstrap query data already loaded into the QTC
 * to find the next reverse migration with ref=(APPLY) AND type=1001 (reverse migration).
 * For test migration, we start with the highest APPLY value and work backwards.
 */
long long database_queue_find_next_reverse_migration_to_apply(DatabaseQueue* lead_queue) {
    if (!lead_queue) {
        return 0;
    }

    char* dqm_label = database_queue_generate_label(lead_queue);

    long long current_apply = lead_queue->latest_applied_migration;
    log_this(dqm_label, "Looking for next reverse migration to apply from QTC (ref=%lld, type=1001)",
             LOG_LEVEL_DEBUG, 1, current_apply);

    // For test migration, we want to reverse the most recently applied migration
    // Look for entry with ref=(current APPLY) AND type=1001 (reverse migration)
    if (lead_queue->query_cache) {
        const QueryCacheEntry* entry = query_cache_lookup_by_ref_and_type(
            lead_queue->query_cache, (int)current_apply, 1001, dqm_label);

        if (entry && entry->sql_template) {
            log_this(dqm_label, "Found next reverse migration to apply: ref=%lld, type=1001 (from QTC)",
                     LOG_LEVEL_DEBUG, 1, current_apply);
            free(dqm_label);
            return current_apply;
        } else {
            log_this(dqm_label, "No reverse migration found for ref=%lld (type=1001) - TEST phase complete",
                     LOG_LEVEL_DEBUG, 1, current_apply);
            free(dqm_label);
            return 0;
        }
    } else {
        log_this(dqm_label, "No query cache available for reverse migration lookup", LOG_LEVEL_ERROR, 0);
        free(dqm_label);
        return 0;
    }
}

/*
 * Apply a single reverse migration through the normal query processing pipeline
 * This retrieves the reverse migration SQL from the database (via bootstrap query results) and executes it as a normal query
 * NOTE: Assumes connection_lock is already held by caller (migration process)
 */
bool database_queue_apply_single_reverse_migration(DatabaseQueue* lead_queue, long long migration_id, const char* dqm_label) {
    log_this(dqm_label, "Applying reverse migration %lld through normal query pipeline", LOG_LEVEL_DEBUG, 1, migration_id);

    // Step 1: Retrieve reverse migration SQL from QTC using ref AND type=1001
    // QTC uses its own read lock for thread safety
    char* migration_sql = NULL;

    if (lead_queue->query_cache) {
        const QueryCacheEntry* entry = query_cache_lookup_by_ref_and_type(lead_queue->query_cache, (int)migration_id, 1001, dqm_label);
        if (entry && entry->sql_template) {
            migration_sql = strdup(entry->sql_template);
            if (!migration_sql) {
                log_this(dqm_label, "Failed to duplicate SQL template for reverse migration %lld", LOG_LEVEL_ERROR, 1, migration_id);
                return false;
            }
            log_this(dqm_label, "Retrieved SQL for reverse migration %lld from QTC (%zu bytes)", LOG_LEVEL_DEBUG, 2, migration_id, strlen(migration_sql));
        } else {
            log_this(dqm_label, "Reverse migration %lld (type=1001) not found in query cache", LOG_LEVEL_ERROR, 1, migration_id);
            return false;
        }
    } else {
        log_this(dqm_label, "No query cache available for reverse migration %lld", LOG_LEVEL_ERROR, 1, migration_id);
        return false;
    }

    // Step 2: Split multi-statement SQL for all engines (DB2 requirement becomes universal)
    char** statements = NULL;
    size_t statement_count = 0;
    size_t statements_capacity = 0;

    // Use the existing parse_sql_statements function from transaction.c with SUBQUERY delimiter for APPLY phase
    if (!parse_sql_statements(migration_sql, strlen(migration_sql), &statements, &statement_count, &statements_capacity, "-- SUBQUERY DELIMITER\n", dqm_label)) {
        log_this(dqm_label, "Failed to split reverse migration SQL for migration %lld", LOG_LEVEL_ERROR, 1, migration_id);
        free(migration_sql);
        return false;
    }

    free(migration_sql); // No longer needed after splitting

    // Step 3: Execute all statements within a single transaction
    // Begin transaction for the entire reverse migration
    Transaction* migration_transaction = NULL;
    if (!database_engine_begin_transaction(lead_queue->persistent_connection, DB_ISOLATION_READ_COMMITTED, &migration_transaction)) {
        log_this(dqm_label, "Failed to begin transaction for reverse migration %lld", LOG_LEVEL_ERROR, 1, migration_id);
        return false;
    }

    log_this(dqm_label, "Started transaction for reverse migration %lld (%zu statements)", LOG_LEVEL_TRACE, 2, migration_id, statement_count);

    bool success = true;
    for (size_t i = 0; i < statement_count && success; i++) {
        // Create QueryRequest for each statement
        QueryRequest* stmt_request = calloc(1, sizeof(QueryRequest));
        if (!stmt_request) {
            log_this(dqm_label, "Failed to allocate statement request", LOG_LEVEL_ERROR, 0);
            success = false;
            break;
        }

        // Generate hash for prepared statement caching
        char stmt_hash[64];
        get_stmt_hash("MPSC", statements[i], 16, stmt_hash);

        stmt_request->query_id = strdup("reverse_migration_statement");
        stmt_request->sql_template = strdup(statements[i]);
        stmt_request->parameters_json = strdup("{}");
        stmt_request->timeout_seconds = 30;
        stmt_request->isolation_level = DB_ISOLATION_READ_COMMITTED;
        stmt_request->use_prepared_statement = true;
        stmt_request->prepared_statement_name = strdup(stmt_hash);

        if (!stmt_request->query_id || !stmt_request->sql_template ||
            !stmt_request->parameters_json || !stmt_request->prepared_statement_name) {
            log_this(dqm_label, "Failed to allocate request fields", LOG_LEVEL_ERROR, 0);
            success = false;
            if (stmt_request->query_id) free(stmt_request->query_id);
            if (stmt_request->sql_template) free(stmt_request->sql_template);
            if (stmt_request->parameters_json) free(stmt_request->parameters_json);
            if (stmt_request->prepared_statement_name) free(stmt_request->prepared_statement_name);
            free(stmt_request);
            break;
        }

        // Execute statement within the transaction - connection_lock already held by caller, no additional locking needed
        QueryResult* stmt_result = NULL;
        bool stmt_success = false;

        if (lead_queue->persistent_connection) {
            stmt_success = database_engine_execute(lead_queue->persistent_connection, stmt_request, &stmt_result);
        } else {
            log_this(dqm_label, "No persistent connection available", LOG_LEVEL_ERROR, 0);
        }

        // Clean up request
        if (stmt_request->query_id) free(stmt_request->query_id);
        if (stmt_request->sql_template) free(stmt_request->sql_template);
        if (stmt_request->parameters_json) free(stmt_request->parameters_json);
        if (stmt_request->prepared_statement_name) free(stmt_request->prepared_statement_name);
        free(stmt_request);

        if (stmt_success && stmt_result && stmt_result->success) {
            log_this(dqm_label, "Reverse migration statement %zu executed successfully (hash: %s): affected %lld rows",
                     LOG_LEVEL_TRACE, 3, i + 1, stmt_hash, stmt_result->affected_rows);
        } else {
            log_this(dqm_label, "Reverse migration statement %zu failed (hash: %s)", LOG_LEVEL_ERROR, 2, i + 1, stmt_hash);
            success = false;
        }

        if (stmt_result) {
            database_engine_cleanup_result(stmt_result);
        }
    }

    // Commit or rollback the entire reverse migration transaction
    if (success) {
        // Commit the transaction
        if (!database_engine_commit_transaction(lead_queue->persistent_connection, migration_transaction)) {
            log_this(dqm_label, "Failed to commit reverse migration %lld", LOG_LEVEL_ERROR, 1, migration_id);
            success = false;
        } else {
            log_this(dqm_label, "Reverse migration %lld committed successfully", LOG_LEVEL_TRACE, 1, migration_id);
        }
    } else {
        // Rollback the transaction
        if (!database_engine_rollback_transaction(lead_queue->persistent_connection, migration_transaction)) {
            log_this(dqm_label, "Failed to rollback reverse migration %lld", LOG_LEVEL_ERROR, 1, migration_id);
        } else {
            log_this(dqm_label, "Reverse migration %lld rolled back due to errors", LOG_LEVEL_TRACE, 1, migration_id);
        }
    }

    // Clean up transaction structure
    database_engine_cleanup_transaction(migration_transaction);

    // NOTE: We do NOT clear prepared statements here
    // The defensive checks in find_prepared_statement() handle corrupted entries gracefully
    // Clearing them during execution causes more problems than it solves

    // Reverse migration has been successfully applied through the normal query pipeline
    if (success) {
        log_this(dqm_label, "Reverse migration %lld successfully applied through query pipeline", LOG_LEVEL_DEBUG, 1, migration_id);
    }

    // Cleanup
    for (size_t i = 0; i < statement_count; i++) {
        free(statements[i]);
    }
    free(statements);

    if (success) {
        log_this(dqm_label, "Successfully applied reverse migration %lld (%zu statements)", LOG_LEVEL_DEBUG, 2, migration_id, statement_count);
    }

    return success;
}

/*
 * Execute migration test process according to the documented algorithm
 *
 * TestMigration Workflow:
 * 1. Check if test_migration is enabled in config
 * 2. If enabled, start a loop that checks for APPLY > 0
 * 3. For each iteration: find reverse migration (type=1001) for current APPLY value
 * 4. Apply the reverse migration through normal query pipeline
 * 5. Re-run bootstrap query to update migration state
 * 6. Check if APPLY value decremented - if so, continue loop
 * 7. Stop when no more reverse migrations or error occurs
 */
bool database_queue_lead_execute_migration_test_process(DatabaseQueue* lead_queue, const char* dqm_label) {
    log_this(dqm_label, "Starting TestMigration process - reversing applied migrations", LOG_LEVEL_DEBUG, 0);

    bool overall_success = true;
    bool more_work = true;
    int reversed_count = 0;

    // Track previous APPLY value to detect stalls (reverse migration didn't actually reverse)
    long long previous_apply = lead_queue->latest_applied_migration;

    // Continue applying reverse migrations until no more work or error
    while (more_work && overall_success) {
        // Check if there are any applied migrations to reverse (APPLY > 0)
        if (lead_queue->latest_applied_migration <= 0) {
            more_work = false;
            log_this(dqm_label, "No applied migrations to reverse - TestMigration complete", LOG_LEVEL_DEBUG, 0);
            break;
        }

        // Find next reverse migration to apply (for current APPLY value)
        long long next_reverse_migration_id = database_queue_find_next_reverse_migration_to_apply(lead_queue);

        if (next_reverse_migration_id == 0) {
            // No reverse migration found for current APPLY value
            more_work = false;
            log_this(dqm_label, "No reverse migration found for APPLY=%lld - TestMigration complete", LOG_LEVEL_DEBUG, 1, lead_queue->latest_applied_migration);
            break;
        }

        log_this(dqm_label, "Applying reverse migration for ref=%lld", LOG_LEVEL_DEBUG, 1, next_reverse_migration_id);

        // Apply the reverse migration through normal query pipeline
        if (database_queue_apply_single_reverse_migration(lead_queue, next_reverse_migration_id, dqm_label)) {
            reversed_count++;
            log_this(dqm_label, "Successfully applied reverse migration %lld (total reversed: %d)", LOG_LEVEL_DEBUG, 2, next_reverse_migration_id, reversed_count);

            // Re-run bootstrap query to update state after successful reverse application
            database_queue_execute_bootstrap_query(lead_queue);

            // Check if APPLY value actually decremented - if not, reverse migration didn't take effect
            if (lead_queue->latest_applied_migration == previous_apply) {
                log_this(dqm_label, "Reverse migration %lld applied but APPLY value unchanged (%lld) - stopping to prevent infinite loop",
                         LOG_LEVEL_ERROR, 2, next_reverse_migration_id, previous_apply);
                overall_success = false;
                break;
            }

            // Update for next iteration
            previous_apply = lead_queue->latest_applied_migration;
        } else {
            log_this(dqm_label, "Failed to apply reverse migration %lld - stopping TestMigration", LOG_LEVEL_ERROR, 1, next_reverse_migration_id);
            overall_success = false;
            break;
        }
    }

    // Variable 'more_work' is no longer used after the loop
    (void)more_work;

    if (overall_success) {
        log_this(dqm_label, "TestMigration process completed successfully - reversed %d migrations", LOG_LEVEL_DEBUG, 1, reversed_count);
    } else {
        log_this(dqm_label, "TestMigration process failed after reversing %d migrations", LOG_LEVEL_ERROR, 1, reversed_count);
    }

    return overall_success;
}