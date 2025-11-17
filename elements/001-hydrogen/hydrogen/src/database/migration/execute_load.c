/*
 * Database Migration Execution - Load Phase
 *
 * Handles LOAD phase migrations which populate the Queries table with migration metadata.
 * NO database schema changes occur in this phase - only metadata is populated.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/database/dbqueue/dbqueue.h>

// Third-Party includes
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

// Local includes
#include "migration.h"

/*
 * Helper function to validate migration configuration and extract parameters
 * Returns true if configuration is valid, false otherwise
 * Extracts engine_name, schema_name, and migration_name on success
 */
static bool validate_migration_config(const DatabaseConnection* conn_config,
                                     const char** engine_name_out,
                                     const char** schema_name_out,
                                     const char** migration_name_out,
                                     char** path_copy_out,
                                     const char* dqm_label) {
    // Check if test migration is enabled
    if (!conn_config->test_migration) {
        log_this(dqm_label, "Test migration not enabled", LOG_LEVEL_TRACE, 0);
        return false; // Not an error, just not enabled
    }

    // Validate that migrations are configured
    if (!conn_config->migrations) {
        log_this(dqm_label, "No migrations configured", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Determine database engine type
    const char* engine_name = normalize_engine_name(conn_config->type);
    if (!engine_name) {
        log_this(dqm_label, "No database engine type specified", LOG_LEVEL_ERROR, 0);
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
        return false;
    }

    // Output parameters
    *engine_name_out = engine_name;
    *schema_name_out = schema_name;
    *migration_name_out = migration_name;
    *path_copy_out = path_copy;

    return true;
}



/*
 * Execute LOAD phase migrations for the given database connection
 * This generates SQL to populate the Queries table with migration metadata (type = 1000)
 * NO database schema changes occur in this phase
 */
bool execute_load_migrations(DatabaseQueue* db_queue, DatabaseHandle* connection) {
    if (!db_queue || !db_queue->is_lead_queue) {
        return false;
    }

    char* dqm_label = database_queue_generate_label(db_queue);
    log_this(dqm_label, "Starting LOAD phase - populating Queries table metadata only", LOG_LEVEL_DEBUG, 0);

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

    // Check if test migration is enabled - return success if disabled
    if (!conn_config->test_migration) {
        log_this(dqm_label, "Test migration not enabled", LOG_LEVEL_TRACE, 0);
        free(dqm_label);
        return true; // Disabled is not an error - return success
    }

    // Validate migration configuration and extract parameters
    const char* engine_name = NULL;
    const char* schema_name = NULL;
    const char* migration_name = NULL;
    char* path_copy = NULL;

    if (!validate_migration_config(conn_config, &engine_name, &schema_name, &migration_name, &path_copy, dqm_label)) {
        free(dqm_label);
        return false; // validate_migration_config handles logging and cleanup
    }

    // Discover all migration files in sorted order
    char** migration_files = NULL;
    size_t migration_count = 0;

    if (!discover_files(conn_config, &migration_files, &migration_count, dqm_label)) {
        free(path_copy);
        free(dqm_label);
        return false;
    }

    log_this(dqm_label, "Found %'zu migration files for LOAD phase", LOG_LEVEL_TRACE, 1, migration_count);

    // LOAD PHASE: Execute each migration file to populate Queries table metadata
    // This should generate INSERT statements for the Queries table with type = 1000
    bool all_success = execute_migration_files_load_only(connection, migration_files, migration_count,
                                                        engine_name, migration_name, schema_name, dqm_label);

    // Cleanup migration files list
    cleanup_files(migration_files, migration_count);

    if (all_success) {
        log_this(dqm_label, "LOAD phase completed successfully - populated Queries table with %zu migration metadata entries", LOG_LEVEL_DEBUG, 1, migration_count);
    } else {
        log_this(dqm_label, "LOAD phase failed - could not populate Queries table metadata", LOG_LEVEL_ERROR, 0);
    }

    // Clean up allocated path copy
    free(path_copy);

    free(dqm_label);
    return all_success;
}

/*
 * Execute LOAD phase for a list of migration files
 * This generates INSERT statements for the Queries table (type = 1000) only
 * NO schema changes are executed in this phase
 *
 * SIMPLE APPROACH: Fresh Lua state per migration, shared payload files
 * Lua's parser internal state accumulates corruption across compilations.
 * Fresh state per migration is the most reliable approach.
 */
bool execute_migration_files_load_only(DatabaseHandle* connection, char** migration_files,
                                      size_t migration_count, const char* engine_name,
                                      const char* migration_name, const char* schema_name,
                                      const char* dqm_label) {
    if (!migration_files && migration_count > 0) {
        return false;  // Invalid parameters
    }

    // CRITICAL: Get payload files ONCE - this is the expensive allocation (100KB+ total)
    // Payload files are read-only data and can be safely shared across all migrations
    PayloadFile* payload_files = NULL;
    size_t payload_count = 0;
    size_t payload_capacity = 0;

    if (!get_payload_files_by_prefix(migration_name, &payload_files, &payload_count, &payload_capacity)) {
        log_this(dqm_label, "Failed to get payload files for migrations", LOG_LEVEL_ERROR, 0);
        return false;
    }

    bool all_success = true;

    // Process each migration with a FRESH Lua state
    // Passing NULL for L forces execute_single_migration_load_only_with_state to create its own
    for (size_t i = 0; i < migration_count; i++) {
        bool migration_success = execute_single_migration_load_only_with_state(
            connection, migration_files[i], engine_name, migration_name, schema_name,
            dqm_label, NULL, payload_files, payload_count);
        
        if (!migration_success) {
            all_success = false;
            break;  // Stop on first failure
        }
    }

    // Clean up payload files ONCE at the end
    free_payload_files(payload_files, payload_count);

    return all_success;
}

/*
 * Execute LOAD phase for a single migration file (LEGACY - not used in batch processing)
 * This generates INSERT statements for the Queries table (type = 1000) only
 * NO schema changes are executed in this phase
 *
 * NOTE: This function creates its own Lua state and should NOT be used
 * for batch processing. Use execute_migration_files_load_only() instead.
 */
bool execute_single_migration_load_only(DatabaseHandle* connection, const char* migration_file,
                                       const char* engine_name, const char* migration_name,
                                       const char* schema_name, const char* dqm_label) {
    log_this(dqm_label, "LOAD PHASE: Processing migration metadata: %s (engine=%s, design_name=%s, schema_name=%s)",
             LOG_LEVEL_TRACE, 4, migration_file, engine_name, migration_name, schema_name ? schema_name : "(none)");

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
        free_payload_files(payload_files, payload_count);
        lua_close(L);
        return false;
    }

    // Use the shared state version (pass L so it doesn't create its own)
    bool result = execute_single_migration_load_only_with_state(connection, migration_file,
                                                                engine_name, migration_name, schema_name,
                                                                dqm_label, L, payload_files, payload_count);

    // Clean up Lua state and payload files (helper function doesn't clean up provided L)
    lua_cleanup(L);
    free_payload_files(payload_files, payload_count);

    return result;
}

/*
 * Execute LOAD phase for a single migration file with shared payload files
 * This generates INSERT statements for the Queries table (type = 1000) only
 * NO schema changes are executed in this phase
 *
 * CRITICAL: Creates a FRESH Lua state if L is NULL (recommended for batch processing)
 * Reusing Lua states across migrations causes internal memory corruption
 */
bool execute_single_migration_load_only_with_state(DatabaseHandle* connection, const char* migration_file,
                                                   const char* engine_name, const char* migration_name,
                                                   const char* schema_name, const char* dqm_label,
                                                   lua_State* L, PayloadFile* payload_files, size_t payload_count) {
    log_this(dqm_label, "LOAD PHASE: Processing migration metadata: %s (engine=%s, design_name=%s, schema_name=%s)",
             LOG_LEVEL_TRACE, 4, migration_file, engine_name, migration_name, schema_name ? schema_name : "(none)");

    // Create a fresh Lua state if one wasn't provided
    bool own_lua_state = (L == NULL);
    if (own_lua_state) {
        L = lua_setup(dqm_label);
        if (!L) {
            return false;
        }

        // Load database.lua module (this loads all 4 engine modules)
        if (!lua_load_database_module(L, migration_name, payload_files, payload_count, dqm_label)) {
            lua_cleanup(L);
            return false;
        }
    }

    // Find the specific migration file
    PayloadFile* mig_file = lua_find_migration_file(migration_file, payload_files, payload_count);

    if (!mig_file) {
        log_this(dqm_label, "Migration file not found in payload: %s", LOG_LEVEL_ERROR, 1, migration_file);
        if (own_lua_state) {
            lua_cleanup(L);
        }
        return false;
    }

    // Load and execute the migration file
    if (!lua_load_migration_file(L, mig_file, migration_file, dqm_label)) {
        if (own_lua_state) {
            lua_cleanup(L);
        }
        return false;
    }

    // Extract queries table
    int query_count = 0;
    if (!lua_execute_migration_function(L, engine_name, migration_name, schema_name, &query_count, dqm_label)) {
        if (own_lua_state) {
            lua_cleanup(L);
        }
        return false;
    }

    // LOAD PHASE: Generate metadata INSERT statements for Queries table
    // This should create INSERT statements with type = 1000 (loaded status)
    size_t sql_length = 0;
    const char* sql_result = NULL;
    if (!lua_execute_load_metadata(L, engine_name, migration_name, schema_name,
                                  &sql_length, &sql_result, dqm_label)) {
        if (own_lua_state) {
            lua_cleanup(L);
        }
        return false;
    }

    // CRITICAL: Copy SQL string from Lua's internal buffer to C-owned memory
    // lua_tolstring() returns pointer to Lua's internal string buffer
    // When we lua_close(), that buffer is freed, potentially causing corruption
    // We MUST copy the string before closing Lua state
    char* sql_copy = NULL;
    if (sql_result && sql_length > 0) {
        sql_copy = malloc(sql_length + 1);
        if (!sql_copy) {
            log_this(dqm_label, "Failed to allocate memory for SQL copy", LOG_LEVEL_ERROR, 0);
            if (own_lua_state) {
                lua_cleanup(L);
            }
            return false;
        }
        memcpy(sql_copy, sql_result, sql_length);
        sql_copy[sql_length] = '\0';
    }

    // Count lines in SQL (approximate by counting newlines)
    int line_count = 1; // At least 1 line if there's content
    if (sql_copy) {
        for (size_t j = 0; j < sql_length; j++) {
            if (sql_copy[j] == '\n') line_count++;
        }
    }

    // Log execution summary
    lua_log_execution_summary(migration_file, sql_length, line_count, query_count, dqm_label);

    // CRITICAL: Clean up migration-specific state from Lua stack
    // When reusing Lua state across migrations, we must clean up after each migration
    // The stack currently has: [migration_func, queries_table, sql_string, ...]
    // We need to pop everything back to the base state (just database modules loaded)
    if (own_lua_state) {
        // If we created our own state, close it completely
        lua_cleanup(L);
    } else {
        // If we're sharing the state, clean up this migration's data
        // Pop all values added by this migration execution
        // After lua_execute_load_metadata we have: sql_result on top, queries below it, etc.
        // Clear the stack back to initial state
        lua_settop(L, 0);  // Clear the entire stack back to base
        
        // Force a garbage collection cycle to clean up temporary objects
        lua_gc(L, LUA_GCCOLLECT, 0);
    }

    // LOAD PHASE: Execute the generated metadata INSERT statements using our copy
    bool success = true;
    if (sql_copy && sql_length > 0) {
        success = execute_transaction(connection, sql_copy, sql_length,
                                     migration_file, connection->engine_type, dqm_label);
    } else {
        log_this(dqm_label, "No metadata SQL generated for migration: %s", LOG_LEVEL_TRACE, 1, migration_file);
        success = false;
    }

    // Free the SQL copy
    free(sql_copy);

    return success;
}