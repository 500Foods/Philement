/*
 * Database Migration Lua Integration
 *
 * Handles Lua script loading, execution, and database module setup for migrations.
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
 * Load database engine module into Lua state
 */
bool lua_load_engine_module(lua_State* L, const char* migration_name,
                           const char* engine_name, PayloadFile* payload_files,
                           size_t payload_count, const char* dqm_label) {
    // Find database_<engine>.lua file
    PayloadFile* engine_file = NULL;
    char engine_filename[256];
    snprintf(engine_filename, sizeof(engine_filename), "%s/database_%s.lua", migration_name, engine_name);

    for (size_t j = 0; j < payload_count; j++) {
        if (strcmp(payload_files[j].name, engine_filename) == 0) {
            engine_file = &payload_files[j];
            break;
        }
    }

    if (!engine_file) {
        log_this(dqm_label, "database_%s.lua not found in payload for migration", LOG_LEVEL_ERROR, 1, engine_name);
        return false;
    }

    log_this(dqm_label, "Found database_%s.lua in payload: %s (%'zu bytes)", LOG_LEVEL_TRACE, 3,
             engine_name, engine_filename, engine_file->size);

    // Load and execute database_<engine>.lua
    if (luaL_loadbuffer(L, (const char*)engine_file->data, engine_file->size, engine_filename) != LUA_OK) {
        const char* error = lua_tostring(L, -1);
        log_this(dqm_label, "Failed to load database_%s.lua: %s", LOG_LEVEL_ERROR, 2,
                 engine_name, error ? error : "Unknown error");
        return false;
    }

    if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
        const char* error = lua_tostring(L, -1);
        log_this(dqm_label, "Failed to execute database_%s.lua: %s", LOG_LEVEL_ERROR, 2,
                 engine_name, error ? error : "Unknown error");
        return false;
    }

    // Check that database_<engine>.lua returned a table
    if (!lua_istable(L, -1)) {
        log_this(dqm_label, "database_%s.lua did not return a table", LOG_LEVEL_ERROR, 1, engine_name);
        return false;
    }

    // Set the database engine module in package.loaded so require('database_<engine>') works
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "loaded");
    lua_pushvalue(L, -3);  // Push the database engine table
    char module_name[256];
    snprintf(module_name, sizeof(module_name), "database_%s", engine_name);
    lua_setfield(L, -2, module_name);
    lua_pop(L, 2);  // Pop package.loaded and package

    return true;
}

/*
 * Load and execute database.lua module
 */
bool lua_load_database_module(lua_State* L, const char* migration_name,
                             PayloadFile* payload_files, size_t payload_count,
                             const char* dqm_label) {
    // First load all database engine modules that database.lua will require
    const char* engines[] = {"sqlite", "postgresql", "mysql", "db2"};
    size_t engine_count = sizeof(engines) / sizeof(engines[0]);

    for (size_t i = 0; i < engine_count; i++) {
        if (!lua_load_engine_module(L, migration_name, engines[i], payload_files, payload_count, dqm_label)) {
            log_this(dqm_label, "Failed to load database engine module: %s", LOG_LEVEL_ERROR, 1, engines[i]);
            return false;
        }
    }

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

    log_this(dqm_label, "Found database.lua in payload: %s (%'zu bytes)", LOG_LEVEL_TRACE, 2, db_filename, db_file->size);

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

    // Check that migration file returned a function
    if (!lua_isfunction(L, -1)) {
        log_this(dqm_label, "Migration file did not return a function", LOG_LEVEL_ERROR, 0);
        return false;
    }

    return true;
}

/*
 * Execute migration function and extract queries table
 */
bool lua_execute_migration_function(lua_State* L, const char* engine_name,
                                   const char* migration_name, const char* schema_name,
                                   int* query_count, const char* dqm_label) {
    // The migration function should be at the top of the stack from lua_load_migration_file
    if (!lua_isfunction(L, -1)) {
        log_this(dqm_label, "Migration function not found on Lua stack", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Get the database table to pass engine config
    lua_getglobal(L, "database");
    if (!lua_istable(L, -1)) {
        log_this(dqm_label, "database table not found in Lua state", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Get the engine config from database.defaults[engine]
    lua_getfield(L, -1, "defaults");
    if (!lua_istable(L, -1)) {
        log_this(dqm_label, "database.defaults table not found", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Debug: Check what's in defaults table
    lua_pushnil(L);
    log_this(dqm_label, "Database defaults table contents:", LOG_LEVEL_DEBUG, 0);
    while (lua_next(L, -2) != 0) {
        const char* key = lua_tostring(L, -2);
        const char* type = lua_typename(L, lua_type(L, -1));
        log_this(dqm_label, "  %s: %s", LOG_LEVEL_DEBUG, 2, key ? key : "?", type);
        lua_pop(L, 1); // Remove value, keep key
    }

    lua_getfield(L, -1, engine_name);
    if (!lua_istable(L, -1)) {
        log_this(dqm_label, "Engine config not found for: %s", LOG_LEVEL_ERROR, 1, engine_name);
        return false;
    }
    log_this(dqm_label, "Engine config for %s found successfully", LOG_LEVEL_DEBUG, 1, engine_name);

    // Don't modify the config table - let replace_query handle it
    // The migration function expects the raw database.defaults[engine] table

    // Stack is now: [migration_func, database, defaults, cfg]
    // Call the migration function: migration_func(engine, design_name, schema_name, cfg)
    lua_pushvalue(L, -4);  // Push migration function (at position -4)
    lua_pushstring(L, engine_name);
    lua_pushstring(L, migration_name);
    lua_pushstring(L, schema_name);
    lua_pushvalue(L, -1);  // Push engine config table (copy of cfg at -1)

    int pcall_result = lua_pcall(L, 4, 1, 0);
    log_this(dqm_label, "Migration function pcall result: %d", LOG_LEVEL_DEBUG, 1, pcall_result);
    if (pcall_result != LUA_OK) {
        const char* error = lua_tostring(L, -1);
        log_this(dqm_label, "Failed to call migration function: %s", LOG_LEVEL_ERROR, 1, error ? error : "Unknown error");
        return false;
    }
    log_this(dqm_label, "Migration function called successfully", LOG_LEVEL_DEBUG, 0);

    // The migration function returns a queries table directly
    if (!lua_istable(L, -1)) {
        log_this(dqm_label, "Migration function did not return a queries table", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Count queries in the table and debug contents
    lua_pushnil(L);  // First key
    *query_count = 0;
    log_this(dqm_label, "Queries table contents:", LOG_LEVEL_DEBUG, 0);
    while (lua_next(L, -2) != 0) {
        (*query_count)++;
        // Debug: Check if this entry has sql field
        if (lua_istable(L, -1)) {
            lua_getfield(L, -1, "sql");
            if (lua_isstring(L, -1)) {
                size_t sql_len;
                const char* sql_content = lua_tolstring(L, -1, &sql_len);
                log_this(dqm_label, "  Query %d: sql field present (%zu bytes)", LOG_LEVEL_DEBUG, 2, *query_count, sql_len);
                (void)sql_content; // Suppress unused variable warning
            } else {
                log_this(dqm_label, "  Query %d: no sql field or not string", LOG_LEVEL_DEBUG, 1, *query_count);
            }
            lua_pop(L, 1); // Pop sql field
        } else {
            log_this(dqm_label, "  Query %d: not a table", LOG_LEVEL_DEBUG, 1, *query_count);
        }
        lua_pop(L, 1);  // Remove value, keep key for next iteration
    }
    log_this(dqm_label, "Total queries found: %d", LOG_LEVEL_DEBUG, 1, *query_count);

    return true;
}

/*
 * Execute run_migration function
 */
bool lua_execute_run_migration(lua_State* L, const char* engine_name,
                                 const char* migration_name, const char* schema_name,
                                 size_t* sql_length, const char** sql_result,
                                 const char* dqm_label) {
    // The queries table should be at the top of the stack from lua_execute_migration_function
    if (!lua_istable(L, -1)) {
        log_this(dqm_label, "Queries table not found on Lua stack", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Now call the run_migration function
    lua_getglobal(L, "database");
    if (!lua_istable(L, -1)) {
        log_this(dqm_label, "database table not found in Lua state", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Get run_migration function
    lua_getfield(L, -1, "run_migration");
    if (!lua_isfunction(L, -1)) {
        log_this(dqm_label, "run_migration function not found in database table (type: %s)", LOG_LEVEL_ERROR, 1, lua_typename(L, lua_type(L, -1)));
        return false;
    }

    // Push arguments for method call: self, queries, engine, design_name, schema_name
    lua_pushvalue(L, -2);  // Push database table as 'self'
    lua_pushvalue(L, -4);  // Push queries table (from before we got database)
    lua_pushstring(L, engine_name);
    lua_pushstring(L, migration_name);
    lua_pushstring(L, schema_name);

    // Call database:run_migration(queries, engine_name, design_name, schema_name)
    int run_migration_result = lua_pcall(L, 5, 1, 0);
    if (run_migration_result != LUA_OK) {
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
    log_this(dqm_label, "Final SQL result: length %zu", LOG_LEVEL_DEBUG, 1, *sql_length);
    if (*sql_length > 0 && *sql_result) {
        // Log first 500 characters for debugging
        char debug_preview[501];
        size_t copy_len = *sql_length > 500 ? 500 : *sql_length;
        memcpy(debug_preview, *sql_result, copy_len);
        debug_preview[copy_len] = '\0';
        log_this(dqm_label, "SQL result preview: %.500s%s", LOG_LEVEL_DEBUG, 2, debug_preview, *sql_length > 500 ? "..." : "");
    }

    *sql_result = lua_tolstring(L, -1, sql_length);

    // Debug: Log the actual SQL length and first few characters
    log_this(dqm_label, "Migration SQL result: %'zu bytes", LOG_LEVEL_DEBUG, 1, *sql_length);
    if (*sql_length > 0 && *sql_result) {
        char preview[101];
        size_t preview_len = *sql_length > 100 ? 100 : *sql_length;
        memcpy(preview, *sql_result, preview_len);
        preview[preview_len] = '\0';
        log_this(dqm_label, "SQL preview: %.100s%s", LOG_LEVEL_DEBUG, 2, preview, *sql_length > 100 ? "..." : "");
    }

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