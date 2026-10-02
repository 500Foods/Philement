/*
 * Database launch checks: configured engines and their client libraries.
 *
 * Refactored to eliminate the 7x duplication of per-engine counting/reporting
 * logic into two reusable helpers:
 *   - add_connection_name: appends a connection name to a dynamically grown
 *     string list (first occurrence seeds the buffer; subsequent appends use
 *     realloc + strcat). Returns the (possibly reallocated) buffer.
 *   - report_database_count: formats and appends a "Go: <Engine> Databases: N (names)"
 *     or "Go: <Engine> Databases: 0" message.
 *   - check_database_library: dlopen + optional get_library_version + message logic
 *     shared by every engine's library check.
 */

#include <src/hydrogen.h>

#include "launch.h"

// Append a connection name to a dynamically-grown name list.
// On the first call (*names is NULL) the buffer is seeded via strdup; on
// subsequent calls the buffer is realloc'd and ", " + new_name appended.
// Returns the (possibly new) buffer pointer, or NULL on allocation failure.
char* add_connection_name(char* names, const char* new_name) {
    if (names == NULL) {
        return strdup(new_name ? new_name : "Unknown");
    }
    const char* src = new_name ? new_name : "Unknown";
    size_t new_size = strlen(names) + 2 + strlen(src) + 1;
    char* new_names = realloc(names, new_size);
    if (new_names) {
        strcat(new_names, ", ");
        strcat(new_names, src);
        return new_names;
    }
    return names;
}

// Format and add a database-count message.  If count > 3 the names list is
// truncated to 50 chars to keep the output readable.
void report_database_count(const char*** messages, size_t* count, size_t* capacity,
                           const char* engine_label, int db_count, char* db_names) {
    char* db_msg = malloc(512);
    if (db_msg) {
        if (db_count > 3 && db_names) {
            db_names[50] = 0;
            snprintf(db_msg, 512, "  Go:      %s: %d (%s...)", engine_label, db_count, db_names);
        } else if (db_names) {
            snprintf(db_msg, 512, "  Go:      %s: %d (%s)", engine_label, db_count, db_names);
        } else {
            snprintf(db_msg, 512, "  Go:      %s: %d", engine_label, db_count);
        }
        add_launch_message(messages, count, capacity, db_msg);
    }
}

// Shared library dependency check for a single database engine.
// dlopens the provided library paths (in order, stopping at first success),
// optionally extracts a version string via get_library_version, and adds a
// Go/No-Go message.  Returns true if the library loaded successfully.
//
// Parameters:
//   lib_name: human-readable name for messages (e.g. "PostgreSQL", "MSSQL/ODBC")
//   lib_paths: array of dlopen paths to try, in priority order
//   num_paths: number of entries in lib_paths
//   found_pattern: the pattern string to show in success messages
//                   (e.g. "libpq.so", "libmysqlclient.so")
//   expected_version: version string to compare against, or NULL for no comparison
//   match_mode: "minor" for major.minor comparison, "exact" for full-string compare,
//               or NULL for no comparison (just report version)
//   keep_open: if true, do not dlclose the handle (Firebird/MSSQL/ODBC keep
//              the mapping for reuse)
bool check_database_library(const char*** messages, size_t* count, size_t* capacity,
                            bool* overall_readiness,
                            const char* lib_name, const char* const* lib_paths,
                            int num_paths, const char* found_pattern,
                            const char* expected_version, const char* match_mode,
                            bool keep_open) {
    void* handle = NULL;
    for (int i = 0; i < num_paths && !handle; i++) {
        handle = dlopen(lib_paths[i], RTLD_LAZY);
    }

    if (!handle) {
        const char* error_msg = dlerror();
        char* lib_msg = malloc(512);
        if (lib_msg) {
            snprintf(lib_msg, 512, "  No-Go:   %s library not found: %s",
                     lib_name, error_msg ? error_msg : "unknown error");
            add_launch_message(messages, count, capacity, lib_msg);
        }
        *overall_readiness = false;
        return false;
    }

    // Library loaded successfully — extract version information
    char* loaded_version = get_library_version(handle, lib_name);
    char* lib_msg = malloc(512);
    if (lib_msg) {
        if (loaded_version && strlen(loaded_version) > 0) {
            if (expected_version) {
                char match_str[20] = "matches";
                if (strcmp(match_mode, "exact") == 0) {
                    if (strcmp(loaded_version, expected_version) != 0) {
                        strcpy(match_str, "doesn't match");
                    }
                } else {
                    if (!version_matches(loaded_version, expected_version)) {
                        strcpy(match_str, "doesn't match");
                    }
                }
                snprintf(lib_msg, 512, "  Go:      %s library loaded successfully (%s %s %s %s)",
                         lib_name, found_pattern, loaded_version, match_str, expected_version);
            } else {
                snprintf(lib_msg, 512, "  Go:      %s library loaded successfully (%s %s)",
                         lib_name, found_pattern, loaded_version);
            }
            add_launch_message(messages, count, capacity, lib_msg);
        } else {
            if (expected_version) {
                snprintf(lib_msg, 512, "  Go:      %s library loaded successfully (%s version-unknown doesn't match %s)",
                         lib_name, found_pattern, expected_version);
            } else {
                snprintf(lib_msg, 512, "  Go:      %s library loaded successfully (%s version-unknown)",
                         lib_name, found_pattern);
            }
            add_launch_message(messages, count, capacity, lib_msg);
        }
    }

    free(loaded_version);

    if (!keep_open) {
        dlclose(handle);
    }
    return true;
}

// Validate database configuration and count databases by type
void validate_database_configuration(const DatabaseConfig* db_config, const char*** messages,
                                     size_t* count, size_t* capacity, bool* overall_readiness,
                                     int* postgres_count, int* mysql_count, int* sqlite_count, int* db2_count, int* firebird_count, int* mariadb_count, int* mssql_count) {
    // Queue configuration is validated during JSON parsing
    add_launch_message(messages, count, capacity, strdup("  Go:      Queue configuration validated"));

    char* postgres_names = NULL;
    char* mysql_names = NULL;
    char* sqlite_names = NULL;
    char* db2_names = NULL;
    char* firebird_names = NULL;
    char* mariadb_names = NULL;
    char* mssql_names = NULL;

    // Initialize counters
    *postgres_count = 0;
    *mysql_count = 0;
    *sqlite_count = 0;
    *db2_count = 0;
    *firebird_count = 0;
    *mariadb_count = 0;
    *mssql_count = 0;

    for (int i = 0; i < db_config->connection_count; i++) {
        const DatabaseConnection* conn = &db_config->connections[i];
        if (conn->enabled && conn->type) {
            const char* engine_type = conn->type;
            const char* new_name = conn->connection_name ? conn->connection_name : "Unknown";

            if (strcmp(engine_type, "postgresql") == 0 || strcmp(engine_type, "postgres") == 0) {
                (*postgres_count)++;
                if (*postgres_count == 1) {
                    postgres_names = strdup(new_name);
                } else {
                    postgres_names = add_connection_name(postgres_names, new_name);
                }
            } else if (strcmp(engine_type, "mysql") == 0) {
                (*mysql_count)++;
                if (*mysql_count == 1) {
                    mysql_names = strdup(new_name);
                } else {
                    mysql_names = add_connection_name(mysql_names, new_name);
                }
            } else if (strcmp(engine_type, "mariadb") == 0) {
                (*mariadb_count)++;
                if (*mariadb_count == 1) {
                    mariadb_names = strdup(new_name);
                } else {
                    mariadb_names = add_connection_name(mariadb_names, new_name);
                }
            } else if (strcmp(engine_type, "sqlite") == 0) {
                (*sqlite_count)++;
                if (*sqlite_count == 1) {
                    sqlite_names = strdup(new_name);
                } else {
                    sqlite_names = add_connection_name(sqlite_names, new_name);
                }
            } else if (strcmp(engine_type, "db2") == 0) {
                (*db2_count)++;
                if (*db2_count == 1) {
                    db2_names = strdup(new_name);
                } else {
                    db2_names = add_connection_name(db2_names, new_name);
                }
            } else if (strcmp(engine_type, "firebird") == 0) {
                (*firebird_count)++;
                if (*firebird_count == 1) {
                    firebird_names = strdup(new_name);
                } else {
                    firebird_names = add_connection_name(firebird_names, new_name);
                }
            } else if (strcmp(engine_type, "mssql") == 0) {
                (*mssql_count)++;
                if (*mssql_count == 1) {
                    mssql_names = strdup(new_name);
                } else {
                    mssql_names = add_connection_name(mssql_names, new_name);
                }
            }
        }
    }

    // Report database counts by engine with names
    if (*postgres_count > 0) {
        report_database_count(messages, count, capacity, "PostgreSQL Databases", *postgres_count, postgres_names);
    }
    if (*mysql_count > 0) {
        report_database_count(messages, count, capacity, "MySQL Databases", *mysql_count, mysql_names);
    }
    if (*mariadb_count > 0) {
        report_database_count(messages, count, capacity, "MariaDB Databases", *mariadb_count, mariadb_names);
    }
    if (*sqlite_count > 0) {
        report_database_count(messages, count, capacity, "SQLite Databases", *sqlite_count, sqlite_names);
    }
    if (*db2_count > 0) {
        report_database_count(messages, count, capacity, "DB2 Databases", *db2_count, db2_names);
    }
    if (*firebird_count > 0) {
        report_database_count(messages, count, capacity, "Firebird Databases", *firebird_count, firebird_names);
    }
    if (*mssql_count > 0) {
        report_database_count(messages, count, capacity, "MSSQL Databases", *mssql_count, mssql_names);
    }

    // Handle cases with 0 databases
    if (*postgres_count == 0) {
        add_launch_message(messages, count, capacity, strdup("  Go:      PostgreSQL Databases: 0"));
    }
    if (*mysql_count == 0) {
        add_launch_message(messages, count, capacity, strdup("  Go:      MySQL Databases: 0"));
    }
    if (*mariadb_count == 0) {
        add_launch_message(messages, count, capacity, strdup("  Go:      MariaDB Databases: 0"));
    }
    if (*sqlite_count == 0) {
        add_launch_message(messages, count, capacity, strdup("  Go:      SQLite Databases: 0"));
    }
    if (*db2_count == 0) {
        add_launch_message(messages, count, capacity, strdup("  Go:      DB2 Databases: 0"));
    }
    if (*firebird_count == 0) {
        add_launch_message(messages, count, capacity, strdup("  Go:      Firebird Databases: 0"));
    }
    if (*mssql_count == 0) {
        add_launch_message(messages, count, capacity, strdup("  Go:      MSSQL Databases: 0"));
    }

    // Total database count check
    int total_databases = *postgres_count + *mysql_count + *sqlite_count + *db2_count + *firebird_count + *mariadb_count + *mssql_count;
    if (total_databases == 0) {
        add_launch_message(messages, count, capacity, strdup("  No-Go:   No databases configured - database subsystem not needed"));
        *overall_readiness = false;
    } else {
        char* total_msg = malloc(256);
        if (total_msg) {
            snprintf(total_msg, 256, "  Go:      Total databases configured: %d", total_databases);
            add_launch_message(messages, count, capacity, total_msg);
        }
    }

    // Free allocated name strings
    free(postgres_names);
    free(mysql_names);
    free(mariadb_names);
    free(sqlite_names);
    free(db2_names);
    free(firebird_names);
    free(mssql_names);
}

// Check library dependencies for database engines
void check_database_library_dependencies(const char*** messages, size_t* count, size_t* capacity, bool* overall_readiness,
                                         int postgres_count, int mysql_count, int sqlite_count, int db2_count, int firebird_count, int mariadb_count, int mssql_count) {
    // Check PostgreSQL library if needed
    if (postgres_count > 0) {
        const char* pg_paths[] = {"libpq.so.5", "libpq.so", "libpq.so.3"};
        check_database_library(messages, count, capacity, overall_readiness,
                               "PostgreSQL", pg_paths, 3, "libpq.so", "17.6", "minor", false);
    }

    // Check MySQL library if needed
    if (mysql_count > 0) {
        const char* mysql_paths[] = {"libmysqlclient.so.21", "libmysqlclient.so.18", "libmysqlclient.so.20", "libmysqlclient.so"};
        check_database_library(messages, count, capacity, overall_readiness,
                               "MySQL", mysql_paths, 4, "libmysqlclient.so", "8.0.42", "minor", false);
    }

    // Check MariaDB library if needed
    if (mariadb_count > 0) {
        const char* maria_paths[] = {"libmariadb.so.3", "libmariadb.so"};
        check_database_library(messages, count, capacity, overall_readiness,
                               "MariaDB", maria_paths, 2, "libmariadb.so", NULL, NULL, false);
    }

    // Check SQLite library if needed
    if (sqlite_count > 0) {
        const char* sqlite_paths[] = {"libsqlite3.so.0", "libsqlite3.so", "libsqlite3.so.1"};
        check_database_library(messages, count, capacity, overall_readiness,
                               "SQLite", sqlite_paths, 3, "libsqlite3.so", "3.46.1", "minor", false);
    }

    // Check DB2 library if needed
    if (db2_count > 0) {
        const char* db2_paths[] = {
            "/opt/ibm/db2/V11.5/lib64/libdb2.so.1",
            "/opt/ibm/db2/V11.1/lib64/libdb2.so.1",
            "/opt/ibm/db2/V11.5/lib64/libdb2.so",
            "/opt/ibm/db2/V11.1/lib64/libdb2.so",
            "libdb2.so",
            "libdb2.so.1",
        };
        check_database_library(messages, count, capacity, overall_readiness,
                               "DB2", db2_paths, 6, "libdb2.so", "11.1.3.3", "exact", false);
    }

    // Check Firebird library if needed
    if (firebird_count > 0) {
        const char* fb_paths[] = {"libfbclient.so.2", "libfbclient.so", "libfbclient2.so"};
        check_database_library(messages, count, capacity, overall_readiness,
                               "Firebird", fb_paths, 3, "libfbclient.so", NULL, NULL, true);
    }

    // Check MSSQL/ODBC library if needed
    if (mssql_count > 0) {
        const char* odbc_paths[] = {"libodbc.so", "libodbc.so.2"};
        check_database_library(messages, count, capacity, overall_readiness,
                               "MSSQL/ODBC", odbc_paths, 2, "libodbc.so", NULL, NULL, true);
    }
}
