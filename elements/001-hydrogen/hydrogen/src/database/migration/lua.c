/*
 * Database Migration Lua Integration
 *
 * Handles Lua script loading, execution, and database module setup for migrations.
 */

#include "../../hydrogen.h"
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "../queue/database_queue.h"
#include "../database.h"
#include "migration.h"

/*
 * Set up Lua state for migration execution
 */
lua_State* lua_setup(const char* dqm_label) {
    // Create a new Lua state for this migration
    lua_State* L = luaL_newstate();
    if (!L) {
        log_this(dqm_label, "Failed to create Lua state for migration", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // Open standard Lua libraries
    luaL_openlibs(L);

    // Open debug library for tracing
    luaopen_debug(L);
    lua_setglobal(L, "debug");  // Make it available as a global

    return L;
}

/*
 * Load and execute database.lua module
 */
bool lua_load_database_module(lua_State* L, const char* migration_name,
                             PayloadFile* payload_files, size_t payload_count,
                             const char* dqm_label) {
    // Find database.lua file
    PayloadFile* db_file = NULL;
    char db_filename[256];
    snprintf(db_filename, sizeof(db_filename), "%s/database.lua", migration_name);

    for (size_t j = 0; j < payload_count; j++) {
        if (strcmp(payload_files[j].name, db_filename) == 0) {
            db_file = &payload_files[j];
            break;
        }
    }

    if (!db_file) {
        log_this(dqm_label, "database.lua not found in payload for migration", LOG_LEVEL_ERROR, 0);
        // Debug: List all available payload files
        log_this(dqm_label, "Available payload files for prefix '%s':", LOG_LEVEL_DEBUG, 1, migration_name);
        for (size_t j = 0; j < payload_count; j++) {
            log_this(dqm_label, "  %s (%zu bytes)", LOG_LEVEL_DEBUG, 2, payload_files[j].name, payload_files[j].size);
        }
        log_this(dqm_label, "Looking for: %s", LOG_LEVEL_DEBUG, 1, db_filename);
        return false;
    }

    log_this(dqm_label, "Found database.lua in payload: %s (%zu bytes)", LOG_LEVEL_TRACE, 2, db_filename, db_file->size);

    // Load and execute database.lua and make it available as a module
    if (luaL_loadbuffer(L, (const char*)db_file->data, db_file->size, "database.lua") != LUA_OK) {
        const char* error = lua_tostring(L, -1);
        log_this(dqm_label, "Failed to load database.lua: %s", LOG_LEVEL_ERROR, 1, error ? error : "Unknown error");
        return false;
    }

    if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
        const char* error = lua_tostring(L, -1);
        log_this(dqm_label, "Failed to execute database.lua: %s", LOG_LEVEL_ERROR, 1, error ? error : "Unknown error");
        return false;
    }

    // Check that database.lua returned a table
    if (!lua_istable(L, -1)) {
        log_this(dqm_label, "database.lua did not return a table", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Set the database module in package.loaded so require('database') works
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "loaded");
    lua_pushvalue(L, -3);  // Push the database table
    lua_setfield(L, -2, "database");
    lua_pop(L, 2);  // Pop package.loaded and package

    // Also set it as a global variable for direct access
    lua_setglobal(L, "database");

    return true;
}

/*
 * Find specific migration file in payload
 */
PayloadFile* lua_find_migration_file(const char* migration_file_path,
                                   PayloadFile* payload_files, size_t payload_count) {
    for (size_t j = 0; j < payload_count; j++) {
        if (strcmp(payload_files[j].name, migration_file_path) == 0) {
            return &payload_files[j];
        }
    }
    return NULL;
}

/*
 * Load and execute migration file
 */
bool lua_load_migration_file(lua_State* L, PayloadFile* mig_file,
                            const char* migration_file_path, const char* dqm_label) {
    // Load and execute the migration file
    if (luaL_loadbuffer(L, (const char*)mig_file->data, mig_file->size, migration_file_path) != LUA_OK) {
        const char* error = lua_tostring(L, -1);
        log_this(dqm_label, "Failed to load migration file: %s", LOG_LEVEL_ERROR, 1, error ? error : "Unknown error");
        return false;
    }

    if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
        const char* error = lua_tostring(L, -1);
        log_this(dqm_label, "Failed to execute migration file: %s", LOG_LEVEL_ERROR, 1, error ? error : "Unknown error");
        return false;
    }

    return true;
}

/*
 * Extract queries table from migration result
 */
bool lua_extract_queries_table(lua_State* L, int* query_count, const char* dqm_label) {
    // Extract the queries table from the migration result
    // The migration file returns { queries = { ... } }
    lua_getfield(L, -1, "queries");
    if (!lua_istable(L, -1)) {
        log_this(dqm_label, "queries table not found in migration result", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Count queries in the table
    lua_pushnil(L);  // First key
    *query_count = 0;
    while (lua_next(L, -2) != 0) {
        (*query_count)++;
        lua_pop(L, 1);  // Remove value, keep key for next iteration
    }

    return true;
}

/*
 * Execute run_migration function
 */
bool lua_execute_run_migration(lua_State* L, const char* engine_name,
                              const char* migration_name, const char* schema_name,
                              size_t* sql_length, const char** sql_result,
                              const char* dqm_label) {
    // Now call the run_migration function
    lua_getglobal(L, "database");
    if (!lua_istable(L, -1)) {
        log_this(dqm_label, "database table not found in Lua state", LOG_LEVEL_ERROR, 0);
        return false;
    }
    // Get run_migration function
    lua_getfield(L, -1, "run_migration");
    if (!lua_isfunction(L, -1)) {
        log_this(dqm_label, "run_migration function not found in database table", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Push arguments for method call: self, queries, engine, design_name, schema_name
    lua_pushvalue(L, -2);  // Push database table as 'self'
    lua_pushvalue(L, -4);  // Push queries table
    lua_pushstring(L, engine_name);
    lua_pushstring(L, migration_name);
    lua_pushstring(L, schema_name);

    // Call database:run_migration(queries, engine, design_name, schema_name)
    if (lua_pcall(L, 5, 1, 0) != LUA_OK) {
        const char* error = lua_tostring(L, -1);
        log_this(dqm_label, "Failed to call run_migration: %s", LOG_LEVEL_ERROR, 1, error ? error : "Unknown error");
        return false;
    }

    // Get the SQL output from the return value (run_migration returns the SQL string)
    if (!lua_isstring(L, -1)) {
        log_this(dqm_label, "run_migration did not return a string (type: %s)", LOG_LEVEL_ERROR, 1,
                 lua_typename(L, lua_type(L, -1)));
        return false;
    }

    *sql_result = lua_tolstring(L, -1, sql_length);
    return true;
}

/*
 * Log migration execution summary
 */
void lua_log_execution_summary(const char* migration_file_path, size_t sql_length,
                              int line_count, int query_count, const char* dqm_label) {
    // Log concise summary
    log_this(dqm_label, "Migration %s returned %'zu bytes in %'d lines containing %'d queries", LOG_LEVEL_TRACE, 4,
             migration_file_path,
             sql_length,
             line_count,
             query_count
            );
}

/*
 * Clean up Lua state
 */
void lua_cleanup(lua_State* L) {
    if (L) {
        lua_close(L);
    }
}