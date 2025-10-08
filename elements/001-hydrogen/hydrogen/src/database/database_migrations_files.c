/*
 * Database Migration File Discovery
 *
 * Handles discovery and sorting of migration files from PAYLOAD or filesystem sources.
 */

#include "../hydrogen.h"
#include <limits.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "database_queue.h"
#include "database.h"
#include "database_migrations.h"

/*
 * Sort migration files by number (ascending)
 */
static void sort_migration_files(char** migration_files, size_t migration_count) {
    if (migration_count <= 1) {
        return;
    }

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

/*
 * Discover PAYLOAD-based migration files
 */
static bool discover_payload_migration_files(const char* migration_name, char*** migration_files,
                                           size_t* migration_count, size_t* files_capacity,
                                           const char* dqm_label) {
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
                            if (*migration_count >= *files_capacity) {
                                *files_capacity = *files_capacity == 0 ? 10 : *files_capacity * 2;
                                char** new_files = realloc(*migration_files, *files_capacity * sizeof(char*));
                                if (!new_files) {
                                    log_this(dqm_label, "Memory allocation failed for migration files", LOG_LEVEL_ERROR, 0);
                                    // Cleanup
                                    for (size_t j = 0; j < *migration_count; j++) {
                                        free((*migration_files)[j]);
                                    }
                                    free(*migration_files);
                                    for (size_t j = 0; j < payload_count; j++) {
                                        free(payload_files[j].name);
                                    }
                                    free(payload_files);
                                    return false;
                                }
                                *migration_files = new_files;
                            }
                            (*migration_files)[*migration_count] = strdup(payload_files[i].name);
                            (*migration_count)++;
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
        return true;
    }
    return false;
}

/*
 * Discover path-based migration files
 */
static bool discover_path_migration_files(const DatabaseConnection* conn_config, char*** migration_files,
                                        size_t* migration_count, size_t* files_capacity,
                                        const char* dqm_label) {
    // Path-based migration - scan directory
    char* path_copy = strdup(conn_config->migrations);
    if (!path_copy) {
        log_this(dqm_label, "Memory allocation failed for path copy", LOG_LEVEL_ERROR, 0);
        return false;
    }

    const char* base_name = basename(path_copy);
    if (!base_name || strlen(base_name) == 0 || strcmp(base_name, "/") == 0) {
        log_this(dqm_label, "Invalid migration path", LOG_LEVEL_ERROR, 0);
        free(path_copy);
        return false;
    }

    char* dir_path = dirname(strdup(conn_config->migrations)); // dirname modifies string, so use a copy

    DIR* dir = opendir(dir_path);
    if (!dir) {
        log_this(dqm_label, "Cannot open migration directory: %s", LOG_LEVEL_ERROR, 1, dir_path);
        free(path_copy);
        return false;
    }

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
                    if (*migration_count >= *files_capacity) {
                        *files_capacity = *files_capacity == 0 ? 10 : *files_capacity * 2;
                        char** new_files = realloc(*migration_files, *files_capacity * sizeof(char*));
                        if (!new_files) {
                            log_this(dqm_label, "Memory allocation failed for migration files", LOG_LEVEL_ERROR, 0);
                            // Cleanup
                            for (size_t j = 0; j < *migration_count; j++) {
                                free((*migration_files)[j]);
                            }
                            free(*migration_files);
                            closedir(dir);
                            free(path_copy);
                            return false;
                        }
                        *migration_files = new_files;
                    }

                    // Construct full path
                    char full_path[2048];
                    int written = snprintf(full_path, sizeof(full_path), "%s/%s", conn_config->migrations, entry->d_name);
                    if (written >= (int)sizeof(full_path)) {
                        continue; // Skip if path too long
                    }
                    (*migration_files)[*migration_count] = strdup(full_path);
                    (*migration_count)++;
                }
            }
        }
    }
    closedir(dir);
    free(path_copy);
    return true;
}

/*
 * Discover and sort all migration files for the given configuration
 */
bool database_migrations_discover_files(const DatabaseConnection* conn_config, char*** migration_files,
                                      size_t* migration_count, const char* dqm_label) {
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
        return false;
    }

    size_t files_capacity = 0;
    *migration_count = 0;
    *migration_files = NULL;

    bool success = false;
    if (is_payload) {
        success = discover_payload_migration_files(migration_name, migration_files, migration_count, &files_capacity, dqm_label);
    } else {
        success = discover_path_migration_files(conn_config, migration_files, migration_count, &files_capacity, dqm_label);
    }

    if (success && *migration_count > 0) {
        // Sort migration files by number (ascending)
        sort_migration_files(*migration_files, *migration_count);
    }

    return success;
}

/*
 * Clean up migration files list
 */
void database_migrations_cleanup_files(char** migration_files, size_t migration_count) {
    if (migration_files) {
        for (size_t i = 0; i < migration_count; i++) {
            free(migration_files[i]);
        }
        free(migration_files);
    }
}