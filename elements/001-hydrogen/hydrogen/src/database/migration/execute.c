/*
 * Database Migration Execution
 *
 * Handles the execution of migration files using Lua scripts and database transactions.
 * Orchestrates the complete migration process from file discovery to execution.
 */

#include "../../hydrogen.h"
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "../queue/database_queue.h"
#include "../database.h"
#include "migration.h"

/*
 * Normalize database engine name to match Lua expectations
 * Returns the normalized engine name or NULL if unsupported
 */
const char* normalize_engine_name(const char* engine_name) {
    if (!engine_name) {
        return NULL;
    }

    if (strcmp(engine_name, "postgresql") == 0 || strcmp(engine_name, "postgres") == 0) {
        return "postgresql";
    } else if (strcmp(engine_name, "mysql") == 0) {
        return "mysql";
    } else if (strcmp(engine_name, "sqlite") == 0) {
        return "sqlite";
    } else if (strcmp(engine_name, "db2") == 0) {
        return "db2";
    }

    return NULL; // Unsupported engine
}

/*
 * Extract migration name from configuration
 * For PAYLOAD: prefix, returns the part after prefix
 * For path-based, returns basename of the path
 * Returns NULL on error
 */
const char* extract_migration_name(const char* migrations_config, char** path_copy_out) {
    if (!migrations_config) {
        return NULL;
    }

    if (strncmp(migrations_config, "PAYLOAD:", 8) == 0) {
        *path_copy_out = NULL;
        return migrations_config + 8;
    } else {
        // For path-based migrations, extract the basename
        *path_copy_out = strdup(migrations_config);
        if (*path_copy_out) {
            const char* migration_name = basename(*path_copy_out);
            return migration_name;
        }
        return NULL;
    }
}

/*
 * Free payload files array
 */
void free_payload_files(PayloadFile* payload_files, size_t payload_count) {
    if (payload_files) {
        for (size_t j = 0; j < payload_count; j++) {
            free(payload_files[j].name);
            free(payload_files[j].data);
        }
        free(payload_files);
    }
}

/*
 * Execute a single migration file
 * Returns true if the migration executed successfully
 */
bool execute_single_migration(DatabaseHandle* connection, const char* migration_file,
                             const char* engine_name, const char* migration_name,
                             const char* schema_name, const char* dqm_label) {
    log_this(dqm_label, "Executing migration: %s (engine=%s, design_name=%s, schema_name=%s)", LOG_LEVEL_TRACE, 4,
             migration_file,
             engine_name,
             migration_name,
             schema_name ? schema_name : "(none)");

    // Set up Lua state
    lua_State* L = lua_setup(dqm_label);
    if (!L) {
        return false;
    }

    // Get all migration files from payload cache
    PayloadFile* payload_files = NULL;
    size_t payload_count = 0;
    size_t payload_capacity = 0;

    if (!get_payload_files_by_prefix(migration_name, &payload_files, &payload_count, &payload_capacity)) {
        log_this(dqm_label, "Failed to get payload files for migration: %s", LOG_LEVEL_ERROR, 1, migration_file);
        lua_close(L);
        return false;
    }

    // Load database.lua module
    if (!lua_load_database_module(L, migration_name, payload_files, payload_count, dqm_label)) {
        // Free payload files
        free_payload_files(payload_files, payload_count);
        lua_close(L);
        return false;
    }

    // Find the specific migration file
    PayloadFile* mig_file = lua_find_migration_file(migration_file, payload_files, payload_count);

    if (!mig_file) {
        log_this(dqm_label, "Migration file not found in payload: %s", LOG_LEVEL_ERROR, 1, migration_file);
        // Free payload files
        free_payload_files(payload_files, payload_count);
        lua_close(L);
        return false;
    }

    // Load and execute the migration file
    if (!lua_load_migration_file(L, mig_file, migration_file, dqm_label)) {
        // Free payload files
        free_payload_files(payload_files, payload_count);
        lua_close(L);
        return false;
    }

    // Extract queries table
    int query_count = 0;
    if (!lua_extract_queries_table(L, &query_count, dqm_label)) {
        // Free payload files
        free_payload_files(payload_files, payload_count);
        lua_close(L);
        return false;
    }

    // Execute run_migration function
    size_t sql_length = 0;
    const char* sql_result = NULL;
    if (!lua_execute_run_migration(L, engine_name, migration_name, schema_name,
                                  &sql_length, &sql_result, dqm_label)) {
        // Free payload files
        free_payload_files(payload_files, payload_count);
        lua_close(L);
        return false;
    }

    // Count lines in SQL (approximate by counting newlines)
    int line_count = 1; // At least 1 line if there's content
    if (sql_result) {
        for (size_t j = 0; j < sql_length; j++) {
            if (sql_result[j] == '\n') line_count++;
        }
    }

    // Log execution summary
    lua_log_execution_summary(migration_file, sql_length, line_count, query_count, dqm_label);

    // Execute the generated SQL against the database as a transaction
    bool success = true;
    if (sql_result && sql_length > 0) {
        success = execute_transaction(connection, sql_result, sql_length,
                                     migration_file, connection->engine_type, dqm_label);
    } else {
        log_this(dqm_label, "No SQL generated for migration: %s", LOG_LEVEL_TRACE, 1, migration_file);
        success = false;
    }

    // Free payload files
    free_payload_files(payload_files, payload_count);

    // Clean up Lua state
    lua_cleanup(L);

    return success;
}

/*
 * Execute a list of migration files
 * Returns true if all migrations executed successfully
 */
bool execute_migration_files(DatabaseHandle* connection, char** migration_files,
                            size_t migration_count, const char* engine_name,
                            const char* migration_name, const char* schema_name,
                            const char* dqm_label) {
    if (!migration_files && migration_count > 0) {
        return false;  // Invalid parameters
    }

    bool all_success = true;

    for (size_t i = 0; i < migration_count; i++) {
        bool migration_success = execute_single_migration(connection, migration_files[i],
                                                         engine_name, migration_name, schema_name, dqm_label);
        if (!migration_success) {
            all_success = false;
            break;  // Stop processing further migrations on failure
        }
    }

    return all_success;
}

/*
 * Execute auto migrations for the given database connection
 * This generates and executes SQL to populate the Queries table with migration information
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