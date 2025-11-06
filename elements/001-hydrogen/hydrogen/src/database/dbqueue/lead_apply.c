/*
 * Database Queue Lead - Migration APPLY Phase
 *
 * APPLY Phase: Process loaded migrations through normal query pipeline
 * - Get list of loaded-but-not-applied migrations from bootstrap query results
 * - Handle multi-statement queries for all engines (DB2 requirement becomes universal)
 * - Re-run bootstrap query between each migration to maintain current state
 * - Execute migrations within transactions with proper rollback on failure
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
 * Find the next migration to apply from the loaded migrations
 * Returns migration ID if found, 0 if no more migrations to apply
 *
 * This function uses the bootstrap query data already loaded into the QTC
 * to find the next migration with ref=(APPLY+1) AND type=1000 (forward migration).
 */
long long database_queue_find_next_migration_to_apply(DatabaseQueue* lead_queue) {
    if (!lead_queue) {
        return 0;
    }

    char* dqm_label = database_queue_generate_label(lead_queue);

    long long next_migration_id = lead_queue->latest_applied_migration + 1;
    log_this(dqm_label, "Looking for next migration to apply from QTC (ref=%lld, type=1000)",
             LOG_LEVEL_DEBUG, 1, next_migration_id);

    // LOAD = highest migration with type 1000 (loaded forward migrations)
    // APPLY = highest migration with type 1003 (applied migrations)
    // We want the next migration: ref=(APPLY+1) AND type=1000

    // Look for entry with specific ref AND type=1000 (forward migration)
    // Multiple entries may share same ref but different types (1000=forward, 1001=reverse, 1002=diagram, 1003=applied)
    if (lead_queue->query_cache) {
        const QueryCacheEntry* entry = query_cache_lookup_by_ref_and_type(
            lead_queue->query_cache, (int)next_migration_id, 1000, dqm_label);

        if (entry && entry->sql_template) {
            log_this(dqm_label, "Found next migration to apply: ref=%lld, type=1000 (from QTC)",
                     LOG_LEVEL_DEBUG, 1, next_migration_id);
            free(dqm_label);
            return next_migration_id;
        } else {
            log_this(dqm_label, "No forward migration found for ref=%lld (type=1000) - APPLY phase complete",
                     LOG_LEVEL_DEBUG, 1, next_migration_id);
            free(dqm_label);
            return 0;
        }
    } else {
        log_this(dqm_label, "No query cache available for migration lookup", LOG_LEVEL_ERROR, 0);
        free(dqm_label);
        return 0;
    }
}

/*
 * Apply a single migration through the normal query processing pipeline
 * This retrieves the migration SQL from the database (via bootstrap query results) and executes it as a normal query
 * NOTE: Assumes connection_lock is already held by caller (migration process)
 */
bool database_queue_apply_single_migration(DatabaseQueue* lead_queue, long long migration_id, const char* dqm_label) {
    log_this(dqm_label, "Applying migration %lld through normal query pipeline", LOG_LEVEL_DEBUG, 1, migration_id);

    // Step 1: Retrieve migration SQL from QTC using ref AND type
    // QTC uses its own read lock for thread safety
    char* migration_sql = NULL;

    if (lead_queue->query_cache) {
        const QueryCacheEntry* entry = query_cache_lookup_by_ref_and_type(lead_queue->query_cache, (int)migration_id, 1000, dqm_label);
        if (entry && entry->sql_template) {
            migration_sql = strdup(entry->sql_template);
            if (!migration_sql) {
                log_this(dqm_label, "Failed to duplicate SQL template for migration %lld", LOG_LEVEL_ERROR, 1, migration_id);
                return false;
            }
            log_this(dqm_label, "Retrieved SQL for migration %lld from QTC (%zu bytes)", LOG_LEVEL_DEBUG, 2, migration_id, strlen(migration_sql));
        } else {
            log_this(dqm_label, "Migration %lld (type=1000) not found in query cache", LOG_LEVEL_ERROR, 1, migration_id);
            return false;
        }
    } else {
        log_this(dqm_label, "No query cache available for migration %lld", LOG_LEVEL_ERROR, 1, migration_id);
        return false;
    }

    // Step 2: Split multi-statement SQL for all engines (DB2 requirement becomes universal)
    char** statements = NULL;
    size_t statement_count = 0;
    size_t statements_capacity = 0;

    // Use the existing parse_sql_statements function from transaction.c with SUBQUERY delimiter for APPLY phase
    if (!parse_sql_statements(migration_sql, strlen(migration_sql), &statements, &statement_count, &statements_capacity, "-- SUBQUERY DELIMITER\n", dqm_label)) {
        log_this(dqm_label, "Failed to split migration SQL for migration %lld", LOG_LEVEL_ERROR, 1, migration_id);
        free(migration_sql);
        return false;
    }

    free(migration_sql); // No longer needed after splitting

    // Step 3: Execute all statements within a single transaction
    // Begin transaction for the entire migration
    Transaction* migration_transaction = NULL;
    if (!database_engine_begin_transaction(lead_queue->persistent_connection, DB_ISOLATION_READ_COMMITTED, &migration_transaction)) {
        log_this(dqm_label, "Failed to begin transaction for migration %lld", LOG_LEVEL_ERROR, 1, migration_id);
        return false;
    }

    log_this(dqm_label, "Started transaction for migration %lld (%zu statements)", LOG_LEVEL_TRACE, 2, migration_id, statement_count);

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

        stmt_request->query_id = strdup("migration_statement");
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
            log_this(dqm_label, "Statement %zu executed successfully (hash: %s): affected %lld rows",
                     LOG_LEVEL_TRACE, 3, i + 1, stmt_hash, stmt_result->affected_rows);
        } else {
            log_this(dqm_label, "Statement %zu failed (hash: %s)", LOG_LEVEL_ERROR, 2, i + 1, stmt_hash);
            success = false;
        }

        if (stmt_result) {
            database_engine_cleanup_result(stmt_result);
        }
    }

    // Commit or rollback the entire migration transaction
    if (success) {
        // Commit the transaction
        if (!database_engine_commit_transaction(lead_queue->persistent_connection, migration_transaction)) {
            log_this(dqm_label, "Failed to commit migration %lld", LOG_LEVEL_ERROR, 1, migration_id);
            success = false;
        } else {
            log_this(dqm_label, "Migration %lld committed successfully", LOG_LEVEL_TRACE, 1, migration_id);
        }
    } else {
        // Rollback the transaction
        if (!database_engine_rollback_transaction(lead_queue->persistent_connection, migration_transaction)) {
            log_this(dqm_label, "Failed to rollback migration %lld", LOG_LEVEL_ERROR, 1, migration_id);
        } else {
            log_this(dqm_label, "Migration %lld rolled back due to errors", LOG_LEVEL_TRACE, 1, migration_id);
        }
    }

    // Clean up transaction structure
    database_engine_cleanup_transaction(migration_transaction);

    // NOTE: We do NOT clear prepared statements here
    // The defensive checks in find_prepared_statement() handle corrupted entries gracefully
    // Clearing them during execution causes more problems than it solves
    
    // Migration has been successfully applied through the normal query pipeline
    // The migration SQL itself should have updated its own status to applied (1003)
    // No additional status update is needed here
    if (success) {
        log_this(dqm_label, "Migration %lld successfully applied through query pipeline", LOG_LEVEL_DEBUG, 1, migration_id);
    }

    // Cleanup
    for (size_t i = 0; i < statement_count; i++) {
        free(statements[i]);
    }
    free(statements);

    if (success) {
        log_this(dqm_label, "Successfully applied migration %lld (%zu statements)", LOG_LEVEL_DEBUG, 2, migration_id, statement_count);
    }

    return success;
}

/*
 * Execute migration apply phase
 *
 * APPLY Phase: Process loaded migrations through normal query pipeline
 * - Get list of loaded-but-not-applied migrations from bootstrap query results
 * - Handle multi-statement queries for all engines (DB2 requirement becomes universal)
 * - Re-run bootstrap query between each migration to maintain current state
 */
bool database_queue_lead_execute_migration_apply(DatabaseQueue* lead_queue) {
    char* dqm_label = database_queue_generate_label(lead_queue);
    log_this(dqm_label, "Starting migration APPLY phase", LOG_LEVEL_DEBUG, 0);

    bool overall_success = true;
    bool more_work = true;
    int applied_count = 0;

    // Track previous APPLY value to detect stalls (migration didn't actually apply)
    long long previous_apply = lead_queue->latest_applied_migration;
    
    // Continue applying migrations until no more work or error
    while (more_work && overall_success) {
        // Re-run bootstrap query to get current migration state
        // Bootstrap query ALWAYS populates QTC and updates migration info (AVAIL/LOAD/APPLY)
        database_queue_execute_bootstrap_query(lead_queue);

        // Find next migration to apply (loaded but not applied)
        // Look for migrations with type = 1000 (loaded) that haven't been applied yet
        long long next_migration_id = database_queue_find_next_migration_to_apply(lead_queue);

        if (next_migration_id == 0) {
            // No more migrations to apply
            more_work = false;
            log_this(dqm_label, "No more migrations to apply - APPLY phase complete", LOG_LEVEL_DEBUG, 0);
            break;
        }

        log_this(dqm_label, "Applying migration ID: %lld", LOG_LEVEL_DEBUG, 1, next_migration_id);

        // Apply the migration through normal query pipeline
        if (database_queue_apply_single_migration(lead_queue, next_migration_id, dqm_label)) {
            applied_count++;
            log_this(dqm_label, "Successfully applied migration %lld (total applied: %d)", LOG_LEVEL_DEBUG, 2, next_migration_id, applied_count);

            // Re-run bootstrap query to update state after successful application
            database_queue_execute_bootstrap_query(lead_queue);
            
            // Check if APPLY value actually changed - if not, migration didn't take effect
            if (lead_queue->latest_applied_migration == previous_apply) {
                log_this(dqm_label, "Migration %lld applied but APPLY value unchanged (%lld) - stopping to prevent infinite loop",
                         LOG_LEVEL_ERROR, 2, next_migration_id, previous_apply);
                overall_success = false;
                break;
            }
            
            // Update for next iteration
            previous_apply = lead_queue->latest_applied_migration;
        } else {
            log_this(dqm_label, "Failed to apply migration %lld - stopping APPLY phase", LOG_LEVEL_ERROR, 1, next_migration_id);
            overall_success = false;
            break;
        }
    }

    // Variable 'more_work' is no longer used after the loop
    (void)more_work;

    if (overall_success) {
        log_this(dqm_label, "Migration APPLY phase completed successfully - applied %d migrations", LOG_LEVEL_DEBUG, 1, applied_count);
    } else {
        log_this(dqm_label, "Migration APPLY phase failed after applying %d migrations", LOG_LEVEL_ERROR, 1, applied_count);
    }

    free(dqm_label);
    return overall_success;
}