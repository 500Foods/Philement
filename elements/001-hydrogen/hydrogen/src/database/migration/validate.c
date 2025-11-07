/*
 * Database Migration Validation
 *
 * Handles validation of migration file availability and configuration.
 * Supports both PAYLOAD: and path-based migration sources.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/database/dbqueue/dbqueue.h>

// Local includes
#include "migration.h"

/*
 * Validate PAYLOAD-based migration files
 */
bool validate_payload_migrations(const DatabaseConnection* conn_config, const char* dqm_label) {
    if (!conn_config || !conn_config->migrations) {
        log_this(dqm_label, "Invalid database connection configuration", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Extract migration name after PAYLOAD:
    const char* migration_name = conn_config->migrations + 8;
    if (strlen(migration_name) == 0) {
        log_this(dqm_label, "Invalid PAYLOAD migration format", LOG_LEVEL_ERROR, 0);
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
            free(files[i].data);
        }
        free(files);

        if (found_file) {
            log_this(dqm_label, "Found first PAYLOAD migration file: %s (%'zu bytes)", LOG_LEVEL_TRACE, 2, found_file, file_size);
            free(found_file);
            return true;
        } else {
            log_this(dqm_label, "No migration files found in payload cache for: %s", LOG_LEVEL_ERROR, 1, migration_name);
            return false;
        }
    } else {
        log_this(dqm_label, "Failed to access payload files for migration validation", LOG_LEVEL_ERROR, 0);
        return false;
    }
}

/*
 * Validate path-based migration files
 */
bool validate_path_migrations(const DatabaseConnection* conn_config, const char* dqm_label) {
    if (!conn_config || !conn_config->migrations) {
        log_this(dqm_label, "Invalid database connection configuration", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Path-based migration - find the first file matching <path>/<basename>_*.lua
    char* path_copy = strdup(conn_config->migrations);
    if (!path_copy) {
        log_this(dqm_label, "Memory allocation failed for migration path validation", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Make another copy for dirname since it modifies the string
    char* path_copy2 = strdup(conn_config->migrations);
    if (!path_copy2) {
        log_this(dqm_label, "Memory allocation failed for migration path validation", LOG_LEVEL_ERROR, 0);
        free(path_copy);
        return false;
    }

    const char* base_name = basename(path_copy);
    if (!base_name || strlen(base_name) == 0) {
        log_this(dqm_label, "Invalid migration path", LOG_LEVEL_ERROR, 0);
        free(path_copy);
        free(path_copy2);
        return false;
    }

    // Look for migration files in the directory
    char* dir_path = dirname(path_copy2);
    DIR* dir = opendir(dir_path);
    if (!dir) {
        log_this(dqm_label, "Cannot open migration directory: %s", LOG_LEVEL_ERROR, 1, dir_path);
        free(path_copy);
        free(path_copy2);
        return false;
    }

    // Find both the lowest and highest migration file numbers
    char* found_file = NULL;
    char* latest_file = NULL;
    unsigned long lowest_number = ULONG_MAX;
    unsigned long highest_number = 0;
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

                    // Track lowest number (first file)
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

                    // Track highest number (latest available)
                    if (file_number > highest_number) {
                        highest_number = file_number;
                        free(latest_file);
                        latest_file = strdup(entry->d_name);
                    }
                }
            }
        }
    }


    closedir(dir);

    if (found_file && stat(found_file, &st) == 0) {
        log_this(dqm_label, "Found first migration file: %s (%lld bytes)", LOG_LEVEL_TRACE, 2, found_file, (long long)st.st_size);
        if (latest_file && strcmp(found_file, latest_file) != 0) {
            log_this(dqm_label, "Found latest migration file: %s (version %lu)", LOG_LEVEL_TRACE, 2, latest_file, highest_number);
        }
        free(found_file);
        free(latest_file);
        free(path_copy);
        free(path_copy2);
        return true;
    } else {
        log_this(dqm_label, "No migration files found for: %s", LOG_LEVEL_ERROR, 1, conn_config->migrations);
        free(found_file);
        free(latest_file);
        free(path_copy);
        free(path_copy2);
        return false;
    }
}

/*
 * Validate migration files are available for the given database connection
 */
bool validate(DatabaseQueue* db_queue) {
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
        migrations_valid = validate_payload_migrations(conn_config, dqm_label);
    } else {
        migrations_valid = validate_path_migrations(conn_config, dqm_label);
    }

    // Update the latest available migration version from payload files
    if (migrations_valid && strncmp(conn_config->migrations, "PAYLOAD:", 8) == 0) {
        long long latest_version = find_latest_available_migration(db_queue);
        if (latest_version > 0) {
            db_queue->latest_available_migration = latest_version;
        }
    }

    free(dqm_label);
    return migrations_valid;
}

/*
 * Find the latest available migration version from payload files
 */
long long find_latest_available_migration(const DatabaseQueue* db_queue) {
    const DatabaseConnection* conn_config = NULL;
    if (app_config) {
        for (int i = 0; i < app_config->databases.connection_count; i++) {
            if (strcmp(app_config->databases.connections[i].name, db_queue->database_name) == 0) {
                conn_config = &app_config->databases.connections[i];
                break;
            }
        }
    }

    if (!conn_config || !conn_config->migrations) {
        return -1;
    }

    // Extract migration name after PAYLOAD:
    const char* migration_name = conn_config->migrations + 8;
    if (strlen(migration_name) == 0) {
        return -1;
    }

    // Find all migration files that match <migration>/<migration>_*.lua pattern
    PayloadFile* files = NULL;
    size_t num_files = 0;
    size_t capacity_files = 0;
    long long highest_version = -1;

    if (get_payload_files_by_prefix(migration_name, &files, &num_files, &capacity_files)) {
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
                            if ((long long)file_number > highest_version) {
                                highest_version = (long long)file_number;
                            }
                        }
                    }
                }
            }
        }

        // Cleanup
        for (size_t i = 0; i < num_files; i++) {
            free(files[i].name);
            free(files[i].data);
        }
        free(files);
    }

    return highest_version;
}