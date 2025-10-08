/*
 * Database Migration Execution
 *
 * Handles the execution of migration files using Lua scripts and database transactions.
 * Orchestrates the complete migration process from file discovery to execution.
 */

#include "../hydrogen.h"
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "database_queue.h"
#include "database.h"
#include "database_migrations.h"

/*
 * Execute auto migrations for the given database connection
 * This generates and executes SQL to populate the Queries table with migration information
 */
bool database_migrations_execute_auto(DatabaseQueue* db_queue, DatabaseHandle* connection) {
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
    const char* engine_name = conn_config->type;
    if (!engine_name) {
        log_this(dqm_label, "No database engine type specified", LOG_LEVEL_ERROR, 0);
        free(dqm_label);
        return false;
    }

    // Normalize engine names to match Lua expectations
    if (strcmp(engine_name, "postgresql") == 0 || strcmp(engine_name, "postgres") == 0) {
        engine_name = "postgresql";
    } else if (strcmp(engine_name, "mysql") == 0) {
        engine_name = "mysql";
    } else if (strcmp(engine_name, "sqlite") == 0) {
        engine_name = "sqlite";
    } else if (strcmp(engine_name, "db2") == 0) {
        engine_name = "db2";
    }

    // Get schema name (default to empty string if not specified)
    const char* schema_name = conn_config->schema ? conn_config->schema : "";

    // Extract migration name from PAYLOAD: prefix or path
    const char* migration_name = NULL;
    char* path_copy = NULL;  // Keep track of allocated path copy for cleanup

    if (strncmp(conn_config->migrations, "PAYLOAD:", 8) == 0) {
        migration_name = conn_config->migrations + 8;
    } else {
        // For path-based migrations, extract the basename
        path_copy = strdup(conn_config->migrations);
        if (path_copy) {
            migration_name = basename(path_copy);
        }
    }

    if (!migration_name) {
        log_this(dqm_label, "Invalid migration configuration", LOG_LEVEL_ERROR, 0);
        free(path_copy);
        free(dqm_label);
        return false;
    }

    // Discover all migration files in sorted order
    char** migration_files = NULL;
    size_t migration_count = 0;

    if (!database_migrations_discover_files(conn_config, &migration_files, &migration_count, dqm_label)) {
        free(path_copy);
        free(dqm_label);
        return false;
    }

    log_this(dqm_label, "Found %'zu migration files to execute", LOG_LEVEL_TRACE, 1, migration_count);

    // Execute each migration file using Lua C API
    bool all_success = true;
    for (size_t i = 0; i < migration_count; i++) {
        log_this(dqm_label, "Executing migration: %s (engine=%s, design_name=%s, schema_name=%s)", LOG_LEVEL_TRACE, 4,
                 migration_files[i],
                 engine_name,
                 migration_name,
                 schema_name ? schema_name : "(none)");

        // Set up Lua state
        lua_State* L = database_migrations_lua_setup(dqm_label);
        if (!L) {
            all_success = false;
            continue;
        }

        // Get all migration files from payload cache
        PayloadFile* payload_files = NULL;
        size_t payload_count = 0;
        size_t payload_capacity = 0;

        if (!get_payload_files_by_prefix(migration_name, &payload_files, &payload_count, &payload_capacity)) {
            log_this(dqm_label, "Failed to get payload files for migration: %s", LOG_LEVEL_ERROR, 1, migration_files[i]);
            lua_close(L);
            all_success = false;
            continue;
        }

        // Load database.lua module
        if (!database_migrations_lua_load_database_module(L, migration_name, payload_files, payload_count, dqm_label)) {
            // Free payload files
            for (size_t j = 0; j < payload_count; j++) {
                free(payload_files[j].name);
                free(payload_files[j].data);
            }
            free(payload_files);
            lua_close(L);
            all_success = false;
            continue;
        }

        // Find the specific migration file
        PayloadFile* mig_file = database_migrations_lua_find_migration_file(migration_files[i], payload_files, payload_count);

        if (!mig_file) {
            log_this(dqm_label, "Migration file not found in payload: %s", LOG_LEVEL_ERROR, 1, migration_files[i]);
            // Free payload files
            for (size_t j = 0; j < payload_count; j++) {
                free(payload_files[j].name);
                free(payload_files[j].data);
            }
            free(payload_files);
            lua_close(L);
            all_success = false;
            continue;
        }

        // Load and execute the migration file
        if (!database_migrations_lua_load_migration_file(L, mig_file, migration_files[i], dqm_label)) {
            // Free payload files
            for (size_t j = 0; j < payload_count; j++) {
                free(payload_files[j].name);
                free(payload_files[j].data);
            }
            free(payload_files);
            lua_close(L);
            all_success = false;
            continue;
        }

        // Extract queries table
        int query_count = 0;
        if (!database_migrations_lua_extract_queries_table(L, &query_count, dqm_label)) {
            // Free payload files
            for (size_t j = 0; j < payload_count; j++) {
                free(payload_files[j].name);
                free(payload_files[j].data);
            }
            free(payload_files);
            lua_close(L);
            all_success = false;
            continue;
        }

        // Execute run_migration function
        size_t sql_length = 0;
        const char* sql_result = NULL;
        if (!database_migrations_lua_execute_run_migration(L, engine_name, migration_name, schema_name,
                                                        &sql_length, &sql_result, dqm_label)) {
            // Free payload files
            for (size_t j = 0; j < payload_count; j++) {
                free(payload_files[j].name);
                free(payload_files[j].data);
            }
            free(payload_files);
            lua_close(L);
            all_success = false;
            continue;
        }

        // Count lines in SQL (approximate by counting newlines)
        int line_count = 1; // At least 1 line if there's content
        if (sql_result) {
            for (size_t j = 0; j < sql_length; j++) {
                if (sql_result[j] == '\n') line_count++;
            }
        }

        // Log execution summary
        database_migrations_lua_log_execution_summary(migration_files[i], sql_length, line_count, query_count, dqm_label);

        // Execute the generated SQL against the database as a transaction
        if (sql_result && sql_length > 0) {
            bool migration_success = database_migrations_execute_transaction(connection, sql_result, sql_length,
                                                                           migration_files[i], connection->engine_type, dqm_label);
            if (!migration_success) {
                all_success = false;
                // Stop processing further migrations on failure
                // Free payload files
                for (size_t j = 0; j < payload_count; j++) {
                    free(payload_files[j].name);
                    free(payload_files[j].data);
                }
                free(payload_files);
                lua_close(L);
                break;
            }
        } else {
            log_this(dqm_label, "No SQL generated for migration: %s", LOG_LEVEL_TRACE, 1, migration_files[i]);
            all_success = false;
        }

        // Free payload files
        for (size_t j = 0; j < payload_count; j++) {
            free(payload_files[j].name);
            free(payload_files[j].data);
        }
        free(payload_files);

        // Clean up Lua state
        database_migrations_lua_cleanup(L);
    }

    // Cleanup migration files list
    database_migrations_cleanup_files(migration_files, migration_count);

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