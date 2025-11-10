/*
 * Database Migration Execution - Auto Migration
 *
 * Handles automatic migration execution for database connections.
 * This generates and executes SQL to populate the Queries table with migration information.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/database/dbqueue/dbqueue.h>

// Local includes
#include "migration.h"

/*
 * Execute auto migrations for the given database connection
 * This generates and executes SQL to populate the Queries table with migration information
 *
 * NOTE: This function currently does both LOAD and APPLY phases.
 * For proper separation, LOAD phase should only populate metadata (type = 1000)
 * and APPLY phase should execute the stored queries through normal pipeline.
 */
bool execute_auto(DatabaseQueue* db_queue, DatabaseHandle* connection) {
    if (!db_queue || !db_queue->is_lead_queue) {
        return false;
    }

    char* dqm_label = database_queue_generate_label(db_queue);

    // Find the database configuration
    const DatabaseConnection* conn_config = NULL;
    if (app_config) {
        for (int i = 0; i < app_config->databases.connection_count; i++) {
            if (strcmp(app_config->databases.connections[i].name, db_queue->database_name) == 0) {
                conn_config = &app_config->databases.connections[i];
                break;
            }
        }
    }

    if (!conn_config) {
        log_this(dqm_label, "No configuration found for database", LOG_LEVEL_ERROR, 0);
        free(dqm_label);
        return false;
    }

    // Check if test migration is enabled
    if (!conn_config->test_migration) {
        log_this(dqm_label, "Test migration not enabled", LOG_LEVEL_TRACE, 0);
        free(dqm_label);
        return true; // Not an error, just not enabled
    }

    log_this(dqm_label, "Test migration execution started", LOG_LEVEL_TRACE, 0);

    // First validate that migrations are configured
    if (!conn_config->migrations) {
        log_this(dqm_label, "No migrations configured", LOG_LEVEL_ERROR, 0);
        free(dqm_label);
        return false;
    }

    // Determine database engine type
    const char* engine_name = normalize_engine_name(conn_config->type);
    if (!engine_name) {
        log_this(dqm_label, "No database engine type specified", LOG_LEVEL_ERROR, 0);
        free(dqm_label);
        return false;
    }

    // Get schema name (default to empty string if not specified)
    const char* schema_name = conn_config->schema ? conn_config->schema : "";

    // Extract migration name from PAYLOAD: prefix or path
    char* path_copy = NULL;
    const char* migration_name = extract_migration_name(conn_config->migrations, &path_copy);

    if (!migration_name) {
        log_this(dqm_label, "Invalid migration configuration", LOG_LEVEL_ERROR, 0);
        free(path_copy);
        free(dqm_label);
        return false;
    }

    // Discover all migration files in sorted order
    char** migration_files = NULL;
    size_t migration_count = 0;

    if (!discover_files(conn_config, &migration_files, &migration_count, dqm_label)) {
        free(path_copy);
        free(dqm_label);
        return false;
    }

    log_this(dqm_label, "Found %'zu migration files to execute", LOG_LEVEL_TRACE, 1, migration_count);

    // Execute each migration file
    bool all_success = execute_migration_files(connection, migration_files, migration_count,
                                              engine_name, migration_name, schema_name, dqm_label);

    // Cleanup migration files list
    cleanup_files(migration_files, migration_count);

    if (all_success) {
        log_this(dqm_label, "Test migration completed successfully - executed %zu migrations", LOG_LEVEL_TRACE, 1, migration_count);
    } else {
        log_this(dqm_label, "Test migration failed - some migrations did not execute successfully", LOG_LEVEL_TRACE, 0);
    }

    // Clean up allocated path copy
    free(path_copy);

    free(dqm_label);
    return all_success;
}