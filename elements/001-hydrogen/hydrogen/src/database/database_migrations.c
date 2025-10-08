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
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

// Local includes
#include "database_queue.h"
#include "database.h"
#include "database_migrations.h"

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
            log_this(dqm_label, "Available payload files:", LOG_LEVEL_DEBUG, 0);
            for (size_t j = 0; j < payload_count; j++) {
                log_this(dqm_label, "  %s (%zu bytes)", LOG_LEVEL_DEBUG, 2, payload_files[j].name, payload_files[j].size);
            }
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

        // log_this(dqm_label, "Found database.lua in payload: %s (%zu bytes)", LOG_LEVEL_DEBUG, 2, db_filename, db_file->size);

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

        // Execute the generated SQL against the database
        if (sql_result && sql_length > 0) {
            // Create a query request for the migration SQL
            QueryRequest* migration_request = calloc(1, sizeof(QueryRequest));
            if (!migration_request) {
                log_this(dqm_label, "Failed to allocate query request for migration SQL", LOG_LEVEL_ERROR, 0);
                lua_close(L);
                all_success = false;
                continue;
            }

            migration_request->query_id = strdup("migration_sql");
            migration_request->sql_template = strdup(sql_result);
            migration_request->parameters_json = strdup("{}");
            migration_request->timeout_seconds = 30; // Longer timeout for migrations
            migration_request->isolation_level = DB_ISOLATION_READ_COMMITTED;
            migration_request->use_prepared_statement = false;

            // Execute the migration SQL using the provided connection (already locked)
            QueryResult* migration_result = NULL;
            bool migration_query_success = database_engine_execute(connection, migration_request, &migration_result);

            if (migration_query_success && migration_result && migration_result->success) {
                log_this(dqm_label, "Migration SQL executed successfully: affected %d rows",
                        LOG_LEVEL_TRACE, 1, migration_result->affected_rows);
            } else {
                log_this(dqm_label, "Migration SQL execution failed: %s",
                        LOG_LEVEL_ERROR, 1,
                        migration_result && migration_result->error_message ?
                        migration_result->error_message : "Unknown error");
                all_success = false;
            }

            // Clean up migration request and result
            if (migration_request->query_id) free(migration_request->query_id);
            if (migration_request->sql_template) free(migration_request->sql_template);
            if (migration_request->parameters_json) free(migration_request->parameters_json);
            free(migration_request);

            if (migration_result) {
                database_engine_cleanup_result(migration_result);
            }
        } else {
            log_this(dqm_label, "No SQL generated for migration: %s", LOG_LEVEL_ERROR, 1, migration_files[i]);
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