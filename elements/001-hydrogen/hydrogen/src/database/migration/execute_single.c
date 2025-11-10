/*
 * Database Migration Execution - Single Migration
 *
 * Handles execution of individual migration files and lists of migrations.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>

// Third-Party includes
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

// Local includes
#include "migration.h"

/*
 * Execute a single migration file
 * Returns true if the migration executed successfully
 */
bool execute_single_migration(DatabaseHandle* connection, const char* migration_file,
                             const char* engine_name, const char* migration_name,
                             const char* schema_name, const char* dqm_label) {
    // Validate required parameters
    if (!connection || !migration_file || !engine_name || !migration_name || !dqm_label) {
        return false;
    }
    
    // Validate non-empty strings
    if (migration_file[0] == '\0' || engine_name[0] == '\0' || migration_name[0] == '\0') {
        return false;
    }
    
    // Validate schema_name if provided
    if (schema_name && schema_name[0] == '\0') {
        return false;
    }
    
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
    if (!lua_execute_migration_function(L, engine_name, migration_name, schema_name, &query_count, dqm_label)) {
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

    // Use helper function to finalize execution
    return finalize_migration_execution(connection, migration_file, sql_result, sql_length,
                                       query_count, L, payload_files, payload_count, dqm_label);
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