/*
 * Database Migration Management
 *
 * Implements migration validation and execution for Lead DQM queues.
 * Handles PAYLOAD: and path-based migration file discovery and validation.
 */

#include "../hydrogen.h"
#include <limits.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

// Local includes
#include "database_queue.h"
#include "database.h"
#include "database_migrations.h"

// DB2 includes for transaction control
#include "db2/types.h"
#include "db2/connection.h"
#include "db2/transaction.h"

// Lua debug hook for tracing execution
// static void lua_trace_hook(lua_State *L, lua_Debug *ar) {
//     if (ar->event == LUA_HOOKLINE) {
//         lua_getinfo(L, "S", ar);  // Get source info
//         const char* source = ar->short_src[0] ? ar->short_src : ar->source;
//         printf("TRACE: Line %d in %s\n", ar->currentline, source);

//         // Optional: Dump locals (can be slow)
//         lua_getinfo(L, "n", ar);
//         for (int i = 1; ; i++) {
//             const char *name = lua_getlocal(L, ar, i);
//             if (!name) break;
//             if (lua_isstring(L, -1)) {
//                 printf("  Local %d: %s = '%s'\n", i, name, lua_tostring(L, -1));
//             } else if (lua_isnumber(L, -1)) {
//                 printf("  Local %d: %s = %g\n", i, name, lua_tonumber(L, -1));
//             } else if (lua_istable(L, -1)) {
//                 printf("  Local %d: %s = table\n", i, name);
//             } else if (lua_isnil(L, -1)) {
//                 printf("  Local %d: %s = nil\n", i, name);
//             } else {
//                 printf("  Local %d: %s = %s\n", i, name, lua_typename(L, lua_type(L, -1)));
//             }
//             lua_pop(L, 1);
//         }
//     }
// }

/*
 * Validate migration files are available for the given database connection
 */
bool database_migrations_validate(DatabaseQueue* db_queue) {
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

    // Check if migrations are configured
    if (!conn_config->auto_migration || !conn_config->migrations) {
        log_this(dqm_label, "Migrations not configured or disabled", LOG_LEVEL_TRACE, 0);
        free(dqm_label);
        return true; // Not an error, just not configured
    }

    bool migrations_valid = false;

    // Check if migrations starts with PAYLOAD:
    if (strncmp(conn_config->migrations, "PAYLOAD:", 8) == 0) {
        // Extract migration name after PAYLOAD:
        const char* migration_name = conn_config->migrations + 8;
        if (strlen(migration_name) == 0) {
            log_this(dqm_label, "Invalid PAYLOAD migration format", LOG_LEVEL_ERROR, 0);
            free(dqm_label);
            return false;
        }

        // Find the first migration file that matches <migration>/<migration>_*.lua pattern
        PayloadFile* files = NULL;
        size_t num_files = 0;
        size_t capacity_files = 0;

        if (get_payload_files_by_prefix(migration_name, &files, &num_files, &capacity_files)) {
            // Find the migration file with the lowest number
            char* found_file = NULL;
            size_t file_size = 0;
            unsigned long lowest_number = ULONG_MAX;

            for (size_t i = 0; i < num_files; i++) {
                if (files[i].name) {
                    // Check if it matches the pattern <migration>/<migration>_XXXXX.lua
                    char expected_prefix[256];
                    snprintf(expected_prefix, sizeof(expected_prefix), "%s/%s_", migration_name, migration_name);

                    if (strncmp(files[i].name, expected_prefix, strlen(expected_prefix)) == 0) {
                        // Extract the number part
                        const char* number_start = files[i].name + strlen(expected_prefix);
                        const char* lua_ext = strstr(number_start, ".lua");
                        if (lua_ext) {
                            size_t number_len = (size_t)(lua_ext - number_start);
                            if (number_len >= 1 && number_len <= 6) {
                                // Valid number length, try to parse
                                char number_str[8];
                                strncpy(number_str, number_start, number_len);
                                number_str[number_len] = '\0';

                                unsigned long file_number = strtoul(number_str, NULL, 10);
                                if (file_number < lowest_number) {
                                    lowest_number = file_number;
                                    free(found_file);
                                    found_file = strdup(files[i].name);
                                    file_size = files[i].size;
                                }
                            }
                        }
                    }
                }
            }

            // Cleanup
            for (size_t i = 0; i < num_files; i++) {
                free(files[i].name);
            }
            free(files);

            if (found_file) {
                log_this(dqm_label, "Found first PAYLOAD migration file: %s (%'zu bytes)", LOG_LEVEL_TRACE, 2, found_file, file_size);
                free(found_file);
                migrations_valid = true;
            } else {
                log_this(dqm_label, "No migration files found in payload cache for: %s", LOG_LEVEL_ERROR, 1, migration_name);
            }
        } else {
            log_this(dqm_label, "Failed to access payload files for migration validation", LOG_LEVEL_ERROR, 0);
        }
    } else {
        // Path-based migration - find the first file matching <path>/<basename>_*.lua
        char* path_copy = strdup(conn_config->migrations);
        if (!path_copy) {
            log_this(dqm_label, "Memory allocation failed for migration path validation", LOG_LEVEL_ERROR, 0);
            free(dqm_label);
            return false;
        }

        // Make another copy for dirname since it modifies the string
        char* path_copy2 = strdup(conn_config->migrations);
        if (!path_copy2) {
            log_this(dqm_label, "Memory allocation failed for migration path validation", LOG_LEVEL_ERROR, 0);
            free(path_copy);
            free(dqm_label);
            return false;
        }

        const char* base_name = basename(path_copy);
        if (!base_name || strlen(base_name) == 0) {
            log_this(dqm_label, "Invalid migration path", LOG_LEVEL_ERROR, 0);
            free(path_copy);
            free(path_copy2);
            free(dqm_label);
            return false;
        }

        // Look for migration files in the directory
        char* dir_path = dirname(path_copy2);
        DIR* dir = opendir(dir_path);
        if (!dir) {
            log_this(dqm_label, "Cannot open migration directory: %s", LOG_LEVEL_ERROR, 1, dir_path);
            free(path_copy);
            free(path_copy2);
            free(dqm_label);
            return false;
        }

        // Find the migration file with the lowest number
        char* found_file = NULL;
        unsigned long lowest_number = ULONG_MAX;
        struct stat st;

        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            // Check if it matches the pattern <basename>_XXXXX.lua
            char expected_prefix[256];
            snprintf(expected_prefix, sizeof(expected_prefix), "%s_", base_name);

            if (strncmp(entry->d_name, expected_prefix, strlen(expected_prefix)) == 0) {
                // Extract the number part
                const char* number_start = entry->d_name + strlen(expected_prefix);
                const char* lua_ext = strstr(number_start, ".lua");
                if (lua_ext && lua_ext == strstr(entry->d_name, ".lua")) { // Ensure .lua is at the end
                    size_t number_len = (size_t)(lua_ext - number_start);
                    if (number_len >= 1 && number_len <= 6) {
                        // Valid number length, try to parse
                        char number_str[8];
                        strncpy(number_str, number_start, number_len);
                        number_str[number_len] = '\0';

                        unsigned long file_number = strtoul(number_str, NULL, 10);
                        if (file_number < lowest_number) {
                            lowest_number = file_number;

                            // Construct full path
                            char full_path[2048];
                            int written = snprintf(full_path, sizeof(full_path), "%s/%s", conn_config->migrations, entry->d_name);
                            if (written >= (int)sizeof(full_path)) {
                                continue; // Skip if path too long
                            }

                            free(found_file);
                            found_file = strdup(full_path);
                        }
                    }
                }
            }
        }

        closedir(dir);

        if (found_file && stat(found_file, &st) == 0) {
            log_this(dqm_label, "Found first migration file: %s (%lld bytes)", LOG_LEVEL_TRACE, 2, found_file, (long long)st.st_size);
            free(found_file);
            migrations_valid = true;
        } else {
            log_this(dqm_label, "No migration files found for: %s", LOG_LEVEL_ERROR, 1, conn_config->migrations);
            free(found_file);
        }

        free(path_copy);
        free(path_copy2);
    }

    free(dqm_label);
    return migrations_valid;
}

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
    bool is_payload = false;

    if (strncmp(conn_config->migrations, "PAYLOAD:", 8) == 0) {
        migration_name = conn_config->migrations + 8;
        is_payload = true;
    } else {
        // For path-based migrations, extract the basename
        char* path_copy = strdup(conn_config->migrations);
        if (path_copy) {
            migration_name = basename(path_copy);
            // Note: we don't free path_copy here as migration_name points into it
        }
    }

    if (!migration_name) {
        log_this(dqm_label, "Invalid migration configuration", LOG_LEVEL_ERROR, 0);
        free(dqm_label);
        return false;
    }

    // Find all migration files in sorted order
    char** migration_files = NULL;
    size_t migration_count = 0;
    size_t files_capacity = 0;

    if (is_payload) {
        // Get files from payload cache
        PayloadFile* payload_files = NULL;
        size_t payload_count = 0;
        size_t payload_capacity = 0;

        if (get_payload_files_by_prefix(migration_name, &payload_files, &payload_count, &payload_capacity)) {
            // Find all migration files and sort them by number
            for (size_t i = 0; i < payload_count; i++) {
                if (payload_files[i].name) {
                    // Check if it matches the pattern <migration>/<migration>_*.lua
                    char expected_prefix[256];
                    snprintf(expected_prefix, sizeof(expected_prefix), "%s/%s_", migration_name, migration_name);

                    if (strncmp(payload_files[i].name, expected_prefix, strlen(expected_prefix)) == 0) {
                        // Extract the number part
                        const char* number_start = payload_files[i].name + strlen(expected_prefix);
                        const char* lua_ext = strstr(number_start, ".lua");
                        if (lua_ext) {
                            size_t number_len = (size_t)(lua_ext - number_start);
                            if (number_len >= 1 && number_len <= 6) {
                                // Valid number length, add to our list
                                if (migration_count >= files_capacity) {
                                    files_capacity = files_capacity == 0 ? 10 : files_capacity * 2;
                                    char** new_files = realloc(migration_files, files_capacity * sizeof(char*));
                                    if (!new_files) {
                                        log_this(dqm_label, "Memory allocation failed for migration files", LOG_LEVEL_ERROR, 0);
                                        // Cleanup
                                        for (size_t j = 0; j < migration_count; j++) {
                                            free(migration_files[j]);
                                        }
                                        free(migration_files);
                                        for (size_t j = 0; j < payload_count; j++) {
                                            free(payload_files[j].name);
                                        }
                                        free(payload_files);
                                        free(dqm_label);
                                        return false;
                                    }
                                    migration_files = new_files;
                                }
                                migration_files[migration_count] = strdup(payload_files[i].name);
                                migration_count++;
                            }
                        }
                    }
                }
            }

            // Cleanup payload files
            for (size_t i = 0; i < payload_count; i++) {
                free(payload_files[i].name);
            }
            free(payload_files);
        }
    } else {
        // Path-based migration - scan directory
        char* path_copy = strdup(conn_config->migrations);
        if (!path_copy) {
            log_this(dqm_label, "Memory allocation failed for path copy", LOG_LEVEL_ERROR, 0);
            free(dqm_label);
            return false;
        }

        const char* base_name = basename(path_copy);
        char* dir_path = dirname(strdup(conn_config->migrations)); // dirname modifies string, so use a copy

        DIR* dir = opendir(dir_path);
        if (dir) {
            struct dirent* entry;
            while ((entry = readdir(dir)) != NULL) {
                // Check if it matches the pattern <basename>_*.lua
                char expected_prefix[256];
                snprintf(expected_prefix, sizeof(expected_prefix), "%s_", base_name);

                if (strncmp(entry->d_name, expected_prefix, strlen(expected_prefix)) == 0) {
                    // Extract the number part
                    const char* number_start = entry->d_name + strlen(expected_prefix);
                    const char* lua_ext = strstr(number_start, ".lua");
                    if (lua_ext && lua_ext == strstr(entry->d_name, ".lua")) { // Ensure .lua is at the end
                        size_t number_len = (size_t)(lua_ext - number_start);
                        if (number_len >= 1 && number_len <= 6) {
                            // Valid number length, add to our list
                            if (migration_count >= files_capacity) {
                                files_capacity = files_capacity == 0 ? 10 : files_capacity * 2;
                                char** new_files = realloc(migration_files, files_capacity * sizeof(char*));
                                if (!new_files) {
                                    log_this(dqm_label, "Memory allocation failed for migration files", LOG_LEVEL_ERROR, 0);
                                    // Cleanup
                                    for (size_t j = 0; j < migration_count; j++) {
                                        free(migration_files[j]);
                                    }
                                    free(migration_files);
                                    closedir(dir);
                                    free(path_copy);
                                    free(dqm_label);
                                    return false;
                                }
                                migration_files = new_files;
                            }

                            // Construct full path
                            char full_path[2048];
                            int written = snprintf(full_path, sizeof(full_path), "%s/%s", conn_config->migrations, entry->d_name);
                            if (written >= (int)sizeof(full_path)) {
                                continue; // Skip if path too long
                            }
                            migration_files[migration_count] = strdup(full_path);
                            migration_count++;
                        }
                    }
                }
            }
            closedir(dir);
        }
        free(path_copy);
    }

    // Sort migration files by number (ascending)
    if (migration_count > 1) {
        // Simple bubble sort by extracting numbers from filenames
        for (size_t i = 0; i < migration_count - 1; i++) {
            for (size_t j = 0; j < migration_count - i - 1; j++) {
                // Extract numbers from filenames for comparison
                unsigned long num1 = 0, num2 = 0;

                // Parse number from migration_files[j]
                const char* name1 = strrchr(migration_files[j], '_');
                if (name1) {
                    num1 = strtoul(name1 + 1, NULL, 10);
                }

                // Parse number from migration_files[j+1]
                const char* name2 = strrchr(migration_files[j+1], '_');
                if (name2) {
                    num2 = strtoul(name2 + 1, NULL, 10);
                }

                if (num1 > num2) {
                    // Swap
                    char* temp = migration_files[j];
                    migration_files[j] = migration_files[j+1];
                    migration_files[j+1] = temp;
                }
            }
        }
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

        // Create a new Lua state for this migration
        lua_State* L = luaL_newstate();
        if (!L) {
            log_this(dqm_label, "Failed to create Lua state for migration: %s", LOG_LEVEL_ERROR, 1, migration_files[i]);
            all_success = false;
            continue;
        }

        // Open standard Lua libraries
        luaL_openlibs(L);

        // Open debug library for tracing
        luaopen_debug(L);
        lua_setglobal(L, "debug");  // Make it available as a global

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
            log_this(dqm_label, "database.lua not found in payload for migration: %s", LOG_LEVEL_ERROR, 1, migration_files[i]);
            // Debug: List all available payload files
            log_this(dqm_label, "Available payload files for prefix '%s':", LOG_LEVEL_DEBUG, 1, migration_name);
            for (size_t j = 0; j < payload_count; j++) {
                log_this(dqm_label, "  %s (%zu bytes)", LOG_LEVEL_DEBUG, 2, payload_files[j].name, payload_files[j].size);
            }
            log_this(dqm_label, "Looking for: %s", LOG_LEVEL_DEBUG, 1, db_filename);
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

        log_this(dqm_label, "Found database.lua in payload: %s (%zu bytes)", LOG_LEVEL_DEBUG, 2, db_filename, db_file->size);

        // Load and execute database.lua and make it available as a module
        if (luaL_loadbuffer(L, (const char*)db_file->data, db_file->size, "database.lua") != LUA_OK) {
            const char* error = lua_tostring(L, -1);
            log_this(dqm_label, "Failed to load database.lua: %s", LOG_LEVEL_ERROR, 1, error ? error : "Unknown error");
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

        if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
            const char* error = lua_tostring(L, -1);
            log_this(dqm_label, "Failed to execute database.lua: %s", LOG_LEVEL_ERROR, 1, error ? error : "Unknown error");
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

        // Set the database module in package.loaded so require('database') works
        lua_getglobal(L, "package");
        lua_getfield(L, -1, "loaded");
        lua_pushvalue(L, -3);  // Push the database table
        lua_setfield(L, -2, "database");
        lua_pop(L, 2);  // Pop package.loaded and package

        // Also set it as a global variable for direct access
        lua_setglobal(L, "database");

        // Find the specific migration file
        PayloadFile* mig_file = NULL;
        for (size_t j = 0; j < payload_count; j++) {
            if (strcmp(payload_files[j].name, migration_files[i]) == 0) {
                mig_file = &payload_files[j];
                break;
            }
        }

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
        if (luaL_loadbuffer(L, (const char*)mig_file->data, mig_file->size, migration_files[i]) != LUA_OK) {
            const char* error = lua_tostring(L, -1);
            log_this(dqm_label, "Failed to load migration file: %s", LOG_LEVEL_ERROR, 1, error ? error : "Unknown error");
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

        if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
            const char* error = lua_tostring(L, -1);
            log_this(dqm_label, "Failed to execute migration file: %s", LOG_LEVEL_ERROR, 1, error ? error : "Unknown error");
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

        // Free payload files
        for (size_t j = 0; j < payload_count; j++) {
            free(payload_files[j].name);
            free(payload_files[j].data);
        }
        free(payload_files);

        // Extract the queries table from the migration result
        // The migration file returns { queries = { ... } }
        lua_getfield(L, -1, "queries");
        if (!lua_istable(L, -1)) {
            log_this(dqm_label, "queries table not found in migration result", LOG_LEVEL_ERROR, 0);
            lua_close(L);
            all_success = false;
            continue;
        }

        // Count queries in the table
        lua_pushnil(L);  // First key
        int query_count = 0;
        while (lua_next(L, -2) != 0) {
            query_count++;
            lua_pop(L, 1);  // Remove value, keep key for next iteration
        }

        // Now call the run_migration function
        lua_getglobal(L, "database");
        if (!lua_istable(L, -1)) {
            log_this(dqm_label, "database table not found in Lua state", LOG_LEVEL_ERROR, 0);
            lua_close(L);
            all_success = false;
            continue;
        }
        // Get run_migration function
        lua_getfield(L, -1, "run_migration");
        if (!lua_isfunction(L, -1)) {
            log_this(dqm_label, "run_migration function not found in database table", LOG_LEVEL_ERROR, 0);
            lua_close(L);
            all_success = false;
            continue;
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
            lua_close(L);
            all_success = false;
            continue;
        }

        // Get the SQL output from the return value (run_migration returns the SQL string)
        if (!lua_isstring(L, -1)) {
            log_this(dqm_label, "run_migration did not return a string (type: %s)", LOG_LEVEL_ERROR, 1,
                     lua_typename(L, lua_type(L, -1)));
            lua_close(L);
            all_success = false;
            continue;
        }

        size_t sql_length = 0;
        const char* sql_result = lua_tolstring(L, -1, &sql_length);

        // Count lines in SQL (approximate by counting newlines)
        int line_count = 1; // At least 1 line if there's content
        if (sql_result) {
            for (size_t j = 0; j < sql_length; j++) {
                if (sql_result[j] == '\n') line_count++;
            }
        }

        // Log concise summary
        log_this(dqm_label, "Migration %s returned %'zu bytes in %'d lines containing %'d queries", LOG_LEVEL_TRACE, 4, 
                migration_files[i], 
                sql_length, 
                line_count, 
                query_count
            );

        // Execute the generated SQL against the database as a transaction
        if (sql_result && sql_length > 0) {
            log_this(dqm_label, "Executing migration %s as transaction", LOG_LEVEL_TRACE, 1, migration_files[i]);

            // For DB2, we need to handle transactions differently using ODBC auto-commit control

            if (connection->engine_type == DB_ENGINE_DB2) {
                // For DB2: Parse multi-statement SQL and execute each statement individually
                // within a transaction controlled by auto-commit settings

                // Parse the SQL into individual statements using the -- QUERY DELIMITER
                char* sql_copy = strdup(sql_result);
                if (!sql_copy) {
                    log_this(dqm_label, "Failed to allocate memory for SQL parsing", LOG_LEVEL_ERROR, 0);
                    lua_close(L);
                    all_success = false;
                    continue;
                }

                // Split on "-- QUERY DELIMITER\n" manually (strtok doesn't work well with multi-char delimiters)
                const char* delimiter = "-- QUERY DELIMITER\n";
                size_t delimiter_len = strlen(delimiter);
                char* sql_ptr = sql_copy;
                bool db2_transaction_success = true;

                // Collect all statements first
                char** statements = NULL;
                size_t statement_count = 0;
                size_t statements_capacity = 0;

                while (*sql_ptr != '\0') {
                    // Find the next delimiter
                    char* delimiter_pos = strstr(sql_ptr, delimiter);

                    // If delimiter found, temporarily null-terminate it
                    if (delimiter_pos) {
                        *delimiter_pos = '\0';
                    }

                    // Process the current statement
                    char* stmt = sql_ptr;

                    // Trim whitespace
                    while (*stmt && isspace((unsigned char)*stmt)) stmt++;
                    char* end = stmt + strlen(stmt) - 1;
                    while (end > stmt && isspace((unsigned char)*end)) {
                        *end = '\0';
                        end--;
                    }

                    // Skip empty statements
                    if (strlen(stmt) > 0) {
                        // Add to statements array
                        if (statement_count >= statements_capacity) {
                            statements_capacity = statements_capacity == 0 ? 10 : statements_capacity * 2;
                            char** new_statements = realloc(statements, statements_capacity * sizeof(char*));
                            if (!new_statements) {
                                log_this(dqm_label, "Failed to allocate memory for statements array", LOG_LEVEL_ERROR, 0);
                                db2_transaction_success = false;
                                break;
                            }
                            statements = new_statements;
                        }
                        statements[statement_count] = strdup(stmt);
                        statement_count++;
                    }

                    // Move to next statement
                    if (delimiter_pos) {
                        // Restore the delimiter and move past it
                        *delimiter_pos = '-';  // Restore first char of delimiter
                        sql_ptr = delimiter_pos + delimiter_len;
                    } else {
                        // No more delimiters, we're done
                        break;
                    }
                }

                free(sql_copy);

                if (!db2_transaction_success) {
                    // Cleanup statements
                    for (size_t j = 0; j < statement_count; j++) {
                        free(statements[j]);
                    }
                    free(statements);
                    log_this(dqm_label, "Migration %s failed - memory allocation error", LOG_LEVEL_ERROR, 1, migration_files[i]);
                    lua_close(L);
                    all_success = false;
                    break;
                }

                // Now execute all statements within a transaction using proper DB2 transaction functions
                Transaction* db2_transaction = NULL;
                if (!db2_begin_transaction(connection, DB_ISOLATION_READ_COMMITTED, &db2_transaction)) {
                    log_this(dqm_label, "Failed to begin DB2 transaction for migration %s", LOG_LEVEL_ERROR, 1, migration_files[i]);
                    // Cleanup statements
                    for (size_t j = 0; j < statement_count; j++) {
                        free(statements[j]);
                    }
                    free(statements);
                    lua_close(L);
                    all_success = false;
                    continue;
                }

                log_this(dqm_label, "Started DB2 transaction for migration %s (%zu statements)", LOG_LEVEL_TRACE, 2, migration_files[i], statement_count);

                // Execute all statements
                for (size_t j = 0; j < statement_count && db2_transaction_success; j++) {
                    // log_this(dqm_label, "Executing DB2 statement %zu/%zu: %.100s%s", LOG_LEVEL_TRACE, 4,
                    //         j + 1, 
                    //         statement_count, 
                    //         statements[j], 
                    //         strlen(statements[j]) > 100 ? "..." : "");

                    QueryRequest* stmt_request = calloc(1, sizeof(QueryRequest));
                    if (!stmt_request) {
                        log_this(dqm_label, "Failed to allocate statement request", LOG_LEVEL_ERROR, 0);
                        db2_transaction_success = false;
                        break;
                    }

                    stmt_request->query_id = strdup("migration_statement");
                    stmt_request->sql_template = strdup(statements[j]);
                    stmt_request->parameters_json = strdup("{}");
                    stmt_request->timeout_seconds = 30;
                    stmt_request->isolation_level = DB_ISOLATION_READ_COMMITTED;
                    stmt_request->use_prepared_statement = false;

                    QueryResult* stmt_result = NULL;
                    bool stmt_success = database_engine_execute(connection, stmt_request, &stmt_result);

                    // Clean up request
                    if (stmt_request->query_id) free(stmt_request->query_id);
                    if (stmt_request->sql_template) free(stmt_request->sql_template);
                    if (stmt_request->parameters_json) free(stmt_request->parameters_json);
                    free(stmt_request);

                    if (stmt_success && stmt_result && stmt_result->success) {
                        log_this(dqm_label, "Statement %zu executed successfully: affected %d rows", LOG_LEVEL_TRACE, 2, j + 1, stmt_result->affected_rows);
                    } else {
                        const char* error_msg = "Unknown error";
                        if (stmt_result && stmt_result->error_message) {
                            error_msg = stmt_result->error_message;
                        }
                        // log_this(dqm_label, "Statement %zu execution failed: %s", LOG_LEVEL_TRACE, 2, j + 1, error_msg);
                        db2_transaction_success = false;
                    }

                    if (stmt_result) {
                        database_engine_cleanup_result(stmt_result);
                    }
                }

                // Commit or rollback based on success
                if (db2_transaction_success) {
                    // Commit the transaction
                    if (!db2_commit_transaction(connection, db2_transaction)) {
                        log_this(dqm_label, "Failed to commit migration %s", LOG_LEVEL_ERROR, 1, migration_files[i]);
                        db2_transaction_success = false;
                    } else {
                        log_this(dqm_label, "Migration %s committed successfully", LOG_LEVEL_TRACE, 1, migration_files[i]);
                    }
                } else {
                    // Rollback the transaction
                    if (!db2_rollback_transaction(connection, db2_transaction)) {
                        log_this(dqm_label, "Failed to rollback migration %s", LOG_LEVEL_ERROR, 1, migration_files[i]);
                    } else {
                        log_this(dqm_label, "Migration %s rolled back due to errors", LOG_LEVEL_TRACE, 1, migration_files[i]);
                    }
                }

                // Clean up transaction structure
                if (db2_transaction) {
                    if (db2_transaction->transaction_id) free(db2_transaction->transaction_id);
                    free(db2_transaction);
                }

                // Cleanup statements
                for (size_t j = 0; j < statement_count; j++) {
                    free(statements[j]);
                }
                free(statements);

                if (!db2_transaction_success) {
                    log_this(dqm_label, "Migration %s failed - transaction rolled back", LOG_LEVEL_TRACE, 1, migration_files[i]);
                    lua_close(L);
                    all_success = false;
                    break; // Stop processing further migrations
                }

                log_this(dqm_label, "Migration %s completed successfully", LOG_LEVEL_TRACE, 1, migration_files[i]);
            }

        } else {
            log_this(dqm_label, "No SQL generated for migration: %s", LOG_LEVEL_TRACE, 1, migration_files[i]);
            all_success = false;
        }

        lua_close(L);
    }

    // Cleanup migration files list
    for (size_t i = 0; i < migration_count; i++) {
        free(migration_files[i]);
    }
    free(migration_files);

    if (all_success) {
        log_this(dqm_label, "Test migration completed successfully - executed %zu migrations", LOG_LEVEL_TRACE, 1, migration_count);
    } else {
        log_this(dqm_label, "Test migration failed - some migrations did not execute successfully", LOG_LEVEL_TRACE, 0);
    }

    free(dqm_label);
    return all_success;
}