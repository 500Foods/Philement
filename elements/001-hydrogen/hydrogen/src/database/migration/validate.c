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
 * Look up the DatabaseConnection matching a queue's database_name
 * in the global app_config. Returns NULL if not found.
 */
const DatabaseConnection* find_conn_config_for_queue(const DatabaseQueue* db_queue) {
    if (!db_queue || !db_queue->database_name || !app_config) {
        return NULL;
    }

    for (int i = 0; i < app_config->databases.connection_count; i++) {
        if (strcmp(app_config->databases.connections[i].name, db_queue->database_name) == 0) {
            return &app_config->databases.connections[i];
        }
    }

    return NULL;
}

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
 * Scan a migration directory for <basename>_XXXX.lua files and return
 * the first (lowest number) and latest (highest number) file names.
 *
 * On success returns true and sets *first_file and *latest_file
 * (both newly allocated strings that the caller must free).  On failure
 * returns false; the output pointers are left NULL.
 */
bool scan_migration_directory(const DatabaseConnection* conn_config, char** first_file, char** latest_file, const char* dqm_label) {
    if (first_file) *first_file = NULL;
    if (latest_file) *latest_file = NULL;

    if (!conn_config || !conn_config->migrations) {
        log_this(dqm_label, "Invalid database connection configuration", LOG_LEVEL_ERROR, 0);
        return false;
    }

    char* path_copy = strdup(conn_config->migrations);
    if (!path_copy) {
        log_this(dqm_label, "Memory allocation failed for migration path validation", LOG_LEVEL_ERROR, 0);
        return false;
    }

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

    char* dir_path = dirname(path_copy2);
    char* saved_dir = strdup(dir_path);
    DIR* dir = opendir(dir_path);
    if (!dir) {
        log_this(dqm_label, "Cannot open migration directory: %s", LOG_LEVEL_ERROR, 1, dir_path);
        free(path_copy);
        free(path_copy2);
        free(saved_dir);
        return false;
    }

    char* found_file = NULL;
    char* scan_latest_file = NULL;
    unsigned long lowest_number = ULONG_MAX;
    unsigned long highest_number = 0;
    struct stat st;

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        char expected_prefix[256];
        snprintf(expected_prefix, sizeof(expected_prefix), "%s_", base_name);

        if (strncmp(entry->d_name, expected_prefix, strlen(expected_prefix)) == 0) {
            const char* number_start = entry->d_name + strlen(expected_prefix);
            const char* lua_ext = strstr(number_start, ".lua");
            if (lua_ext && lua_ext == strstr(entry->d_name, ".lua")) {
                size_t number_len = (size_t)(lua_ext - number_start);
                if (number_len >= 1 && number_len <= 6) {
                    char number_str[8];
                    strncpy(number_str, number_start, number_len);
                    number_str[number_len] = '\0';

                    unsigned long file_number = strtoul(number_str, NULL, 10);

                    if (file_number < lowest_number) {
                        lowest_number = file_number;

                        char full_path[2048];
                        int written = snprintf(full_path, sizeof(full_path), "%s/%s", saved_dir, entry->d_name);
                        if (written >= (int)sizeof(full_path)) {
                            continue;
                        }

                        free(found_file);
                        found_file = strdup(full_path);
                    }

                    if (file_number > highest_number) {
                        highest_number = file_number;
                        free(scan_latest_file);
                        scan_latest_file = strdup(entry->d_name);
                    }
                }
            }
        }
    }

    closedir(dir);
    free(path_copy);
    free(path_copy2);
    free(saved_dir);

    if (found_file && stat(found_file, &st) == 0) {
        log_this(dqm_label, "Found first migration file: %s (%lld bytes)", LOG_LEVEL_TRACE, 2, found_file, (long long)st.st_size);
        if (scan_latest_file && strcmp(found_file, scan_latest_file) != 0) {
            log_this(dqm_label, "Found latest migration file: %s (version %lu)", LOG_LEVEL_TRACE, 2, scan_latest_file, highest_number);
        }
        if (first_file) *first_file = found_file;
        if (latest_file) *latest_file = scan_latest_file;
        else free(scan_latest_file);
        return true;
    } else {
        log_this(dqm_label, "No migration files found for: %s", LOG_LEVEL_ERROR, 1, conn_config->migrations);
        free(found_file);
        free(scan_latest_file);
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

    char* found_file = NULL;
    char* latest_file = NULL;
    struct stat st;

    if (!scan_migration_directory(conn_config, &found_file, &latest_file, dqm_label)) {
        return false;
    }

    if (found_file && stat(found_file, &st) == 0) {
        log_this(dqm_label, "Found first migration file: %s (%lld bytes)", LOG_LEVEL_TRACE, 2, found_file, (long long)st.st_size);
        if (latest_file && strcmp(found_file, latest_file) != 0) {
            log_this(dqm_label, "Found latest migration file: %s", LOG_LEVEL_TRACE, 1, latest_file);
        }
    }

    free(found_file);
    free(latest_file);
    return true;
}

/*
 * Validate migration files are available for the given database connection
 */
bool validate(DatabaseQueue* db_queue) {
    if (!db_queue || !db_queue->is_lead_queue) {
        return false;
    }

    char* dqm_label = database_queue_generate_label(db_queue);

    const DatabaseConnection* conn_config = find_conn_config_for_queue(db_queue);

    if (!conn_config) {
        log_this(dqm_label, "No configuration found for database", LOG_LEVEL_ERROR, 0);
        free(dqm_label);
        return false;
    }

    if (!conn_config->auto_migration || !conn_config->migrations) {
        log_this(dqm_label, "Migrations not configured or disabled", LOG_LEVEL_TRACE, 0);
        free(dqm_label);
        return true;
    }

    bool migrations_valid = false;
    bool is_payload = (strncmp(conn_config->migrations, "PAYLOAD:", 8) == 0);

    if (is_payload) {
        migrations_valid = validate_payload_migrations(conn_config, dqm_label);
    } else {
        migrations_valid = validate_path_migrations(conn_config, dqm_label);
    }

    if (migrations_valid && is_payload) {
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

    const DatabaseConnection* conn_config = find_conn_config_for_queue(db_queue);

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
