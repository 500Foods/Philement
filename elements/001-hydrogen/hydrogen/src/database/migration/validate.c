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

    char** designs = NULL;
    size_t design_count = 0;
    size_t d;
    bool ok;

    ok = migration_payload_designs(conn_config->migrations, &designs, &design_count, dqm_label);
    for (d = 0; ok && d < design_count; d++) {
        char* found_file = NULL;
        size_t file_size = 0;
        long long highest = -1;

        if (!migration_design_span(designs[d], &found_file, &file_size, &highest, dqm_label)) {
            ok = false;
            break;
        }
        if (!found_file) {
            log_this(dqm_label, "No migration files found in payload cache for: %s", LOG_LEVEL_ERROR, 1, designs[d]);
            ok = false;
            break;
        }
        log_this(dqm_label, "Found first PAYLOAD migration file: %s (%'zu bytes)", LOG_LEVEL_TRACE, 2, found_file, file_size);
        free(found_file);
    }
    migration_payload_designs_free(designs, design_count);
    return ok;
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

    /* Payload migrations track one AVAIL/LOAD/APPLY per thousand that the
     * payload actually ships. Path-based migrations leave the band list empty. */
    if (migrations_valid && strncmp(conn_config->migrations, "PAYLOAD:", 8) == 0) {
        if (!migration_ranges_load_from_payload(db_queue, conn_config->migrations, dqm_label)) {
            migrations_valid = false;
        } else {
            long long latest_version = find_latest_available_migration(db_queue);
            if (latest_version > 0) {
                db_queue->latest_available_migration = latest_version;
            }
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

    if (db_queue && db_queue->migration_range_count > 0) {
        size_t range_index;
        long long highest = -1;

        for (range_index = 0; range_index < db_queue->migration_range_count; range_index++) {
            if (db_queue->migration_ranges[range_index].available > highest) {
                highest = db_queue->migration_ranges[range_index].available;
            }
        }
        return highest;
    }
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

    char** designs = NULL;
    size_t design_count = 0;
    size_t d;
    long long highest_version = -1;

    if (!migration_payload_designs(conn_config->migrations, &designs, &design_count, "Migration")) {
        return -1;
    }

    for (d = 0; d < design_count; d++) {
        char* found_file = NULL;
        size_t file_size = 0;
        long long highest = -1;

        if (!migration_design_span(designs[d], &found_file, &file_size, &highest, "Migration")) {
            migration_payload_designs_free(designs, design_count);
            return -1;
        }
        free(found_file);
        if (highest > highest_version) {
            highest_version = highest;
        }
    }

    migration_payload_designs_free(designs, design_count);
    return highest_version;
}