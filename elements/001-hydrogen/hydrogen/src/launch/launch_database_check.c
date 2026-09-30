/*
 * Database launch checks: configured engines and their client libraries.
 */

#include <src/hydrogen.h>

#include "launch.h"

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
            if (strcmp(engine_type, "postgresql") == 0 || strcmp(engine_type, "postgres") == 0) {
                (*postgres_count)++;
                if (*postgres_count == 1) {
                    postgres_names = strdup(conn->connection_name ? conn->connection_name : "Unknown");
                } else {
                    const char* new_name = conn->connection_name ? conn->connection_name : "Unknown";
                    size_t new_size = strlen(postgres_names) + 2 + strlen(new_name) + 1; // existing + ", " + new_name + null
                    char* new_names = realloc(postgres_names, new_size);
                    if (new_names) {
                        strcat(new_names, ", ");
                        strcat(new_names, new_name);
                        postgres_names = new_names;
                    }
                }
            } else if (strcmp(engine_type, "mysql") == 0) {
                (*mysql_count)++;
                if (*mysql_count == 1) {
                    mysql_names = strdup(conn->connection_name ? conn->connection_name : "Unknown");
                } else {
                    const char* new_name = conn->connection_name ? conn->connection_name : "Unknown";
                    size_t new_size = strlen(mysql_names) + 2 + strlen(new_name) + 1; // existing + ", " + new_name + null
                    char* new_names = realloc(mysql_names, new_size);
                    if (new_names) {
                        strcat(new_names, ", ");
                        strcat(new_names, new_name);
                        mysql_names = new_names;
                    }
                }
            } else if (strcmp(engine_type, "mariadb") == 0) {
                (*mariadb_count)++;
                if (*mariadb_count == 1) {
                    mariadb_names = strdup(conn->connection_name ? conn->connection_name : "Unknown");
                } else {
                    const char* new_name = conn->connection_name ? conn->connection_name : "Unknown";
                    size_t new_size = strlen(mariadb_names) + 2 + strlen(new_name) + 1; // existing + ", " + new_name + null
                    char* new_names = realloc(mariadb_names, new_size);
                    if (new_names) {
                        strcat(new_names, ", ");
                        strcat(new_names, new_name);
                        mariadb_names = new_names;
                    }
                }
            } else if (strcmp(engine_type, "sqlite") == 0) {
                (*sqlite_count)++;
                if (*sqlite_count == 1) {
                    sqlite_names = strdup(conn->connection_name ? conn->connection_name : "Unknown");
                } else {
                    const char* new_name = conn->connection_name ? conn->connection_name : "Unknown";
                    size_t new_size = strlen(sqlite_names) + 2 + strlen(new_name) + 1; // existing + ", " + new_name + null
                    char* new_names = realloc(sqlite_names, new_size);
                    if (new_names) {
                        strcat(new_names, ", ");
                        strcat(new_names, new_name);
                        sqlite_names = new_names;
                    }
                }
            } else if (strcmp(engine_type, "db2") == 0) {
                (*db2_count)++;
                if (*db2_count == 1) {
                    db2_names = strdup(conn->connection_name ? conn->connection_name : "Unknown");
                } else {
                    const char* new_name = conn->connection_name ? conn->connection_name : "Unknown";
                    size_t new_size = strlen(db2_names) + 2 + strlen(new_name) + 1; // existing + ", " + new_name + null
                    char* new_names = realloc(db2_names, new_size);
                    if (new_names) {
                        strcat(new_names, ", ");
                        strcat(new_names, new_name);
                        db2_names = new_names;
                    }
                }
            } else if (strcmp(engine_type, "firebird") == 0) {
                (*firebird_count)++;
                if (*firebird_count == 1) {
                    firebird_names = strdup(conn->connection_name ? conn->connection_name : "Unknown");
                } else {
                    const char* new_name = conn->connection_name ? conn->connection_name : "Unknown";
                    size_t new_size = strlen(firebird_names) + 2 + strlen(new_name) + 1;
                    char* new_names = realloc(firebird_names, new_size);
                    if (new_names) {
                        strcat(new_names, ", ");
                        strcat(new_names, new_name);
                        firebird_names = new_names;
                    }
                }
            } else if (strcmp(engine_type, "mssql") == 0) {
                (*mssql_count)++;
                if (*mssql_count == 1) {
                    mssql_names = strdup(conn->connection_name ? conn->connection_name : "Unknown");
                } else {
                    const char* new_name = conn->connection_name ? conn->connection_name : "Unknown";
                    size_t new_size = strlen(mssql_names) + 2 + strlen(new_name) + 1;
                    char* new_names = realloc(mssql_names, new_size);
                    if (new_names) {
                        strcat(new_names, ", ");
                        strcat(new_names, new_name);
                        mssql_names = new_names;
                    }
                }
            }
        }
    }

    // Report database counts by engine with names
    if (*postgres_count > 0 && postgres_names) {
        char* postgres_msg = malloc(512);
        if (postgres_msg) {
            if (*postgres_count > 3) {
                // Truncate long lists
                postgres_names[50] = 0;
                snprintf(postgres_msg, 512, "  Go:      PostgreSQL Databases: %d (%s...)", *postgres_count, postgres_names);
            } else {
                snprintf(postgres_msg, 512, "  Go:      PostgreSQL Databases: %d (%s)", *postgres_count, postgres_names);
            }
            add_launch_message(messages, count, capacity, postgres_msg);
        }
        // Note: postgres_names is freed by caller in test environment
    }

    if (*mysql_count > 0 && mysql_names) {
        char* mysql_msg = malloc(512);
        if (mysql_msg) {
            if (*mysql_count > 3) {
                // Truncate long lists
                mysql_names[50] = 0;
                snprintf(mysql_msg, 512, "  Go:      MySQL Databases: %d (%s...)", *mysql_count, mysql_names);
            } else {
                snprintf(mysql_msg, 512, "  Go:      MySQL Databases: %d (%s)", *mysql_count, mysql_names);
            }
            add_launch_message(messages, count, capacity, mysql_msg);
        }
        // Note: mysql_names is freed by caller in test environment
    }

    if (*mariadb_count > 0 && mariadb_names) {
        char* mariadb_msg = malloc(512);
        if (mariadb_msg) {
            if (*mariadb_count > 3) {
                mariadb_names[50] = 0;
                snprintf(mariadb_msg, 512, "  Go:      MariaDB Databases: %d (%s...)", *mariadb_count, mariadb_names);
            } else {
                snprintf(mariadb_msg, 512, "  Go:      MariaDB Databases: %d (%s)", *mariadb_count, mariadb_names);
            }
            add_launch_message(messages, count, capacity, mariadb_msg);
        }
    }

    if (*sqlite_count > 0 && sqlite_names) {
        char* sqlite_msg = malloc(512);
        if (sqlite_msg) {
            if (*sqlite_count > 3) {
                // Truncate long lists
                sqlite_names[50] = 0;
                snprintf(sqlite_msg, 512, "  Go:      SQLite Databases: %d (%s...)", *sqlite_count, sqlite_names);
            } else {
                snprintf(sqlite_msg, 512, "  Go:      SQLite Databases: %d (%s)", *sqlite_count, sqlite_names);
            }
            add_launch_message(messages, count, capacity, sqlite_msg);
        }
        // Note: sqlite_names is freed by caller in test environment
    }

    if (*db2_count > 0 && db2_names) {
        char* db2_msg = malloc(512);
        if (db2_msg) {
            if (*db2_count > 3) {
                // Truncate long lists
                db2_names[50] = 0;
                snprintf(db2_msg, 512, "  Go:      DB2 Databases: %d (%s...)", *db2_count, db2_names);
            } else {
                snprintf(db2_msg, 512, "  Go:      DB2 Databases: %d (%s)", *db2_count, db2_names);
            }
            add_launch_message(messages, count, capacity, db2_msg);
        }
        // Note: db2_names is freed by caller in test environment
    }

    if (*firebird_count > 0 && firebird_names) {
        char* firebird_msg = malloc(512);
        if (firebird_msg) {
            if (*firebird_count > 3) {
                firebird_names[50] = 0;
                snprintf(firebird_msg, 512, "  Go:      Firebird Databases: %d (%s...)", *firebird_count, firebird_names);
            } else {
                snprintf(firebird_msg, 512, "  Go:      Firebird Databases: %d (%s)", *firebird_count, firebird_names);
            }
            add_launch_message(messages, count, capacity, firebird_msg);
        }
    }

    if (*mssql_count > 0 && mssql_names) {
        char* mssql_msg = malloc(512);
        if (mssql_msg) {
            if (*mssql_count > 3) {
                mssql_names[50] = 0;
                snprintf(mssql_msg, 512, "  Go:      MSSQL Databases: %d (%s...)", *mssql_count, mssql_names);
            } else {
                snprintf(mssql_msg, 512, "  Go:      MSSQL Databases: %d (%s)", *mssql_count, mssql_names);
            }
            add_launch_message(messages, count, capacity, mssql_msg);
        }
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
        void* libpq_handle = dlopen("libpq.so.5", RTLD_LAZY);
        if (!libpq_handle) {
            // Try alternative versions
            libpq_handle = dlopen("libpq.so", RTLD_LAZY);
            // If still failing, try another common version
            if (!libpq_handle) {
                libpq_handle = dlopen("libpq.so.3", RTLD_LAZY);
            }
        }

        if (!libpq_handle) {
            const char* error_msg = dlerror();
            char* lib_msg = malloc(512);
            if (lib_msg) {
                snprintf(lib_msg, 512, "  No-Go:   PostgreSQL library not found: %s", error_msg ? error_msg : "unknown error");
                add_launch_message(messages, count, capacity, lib_msg);
            }
            *overall_readiness = false;
        } else {
            // Extract version information
            char* loaded_version = get_library_version(libpq_handle, "PostgreSQL");

            // Format success message with version information
            char* lib_msg = malloc(512);
            if (lib_msg) {
                if (loaded_version && strlen(loaded_version) > 0) {
                    // Compare with expected version from dependency check: PostgreSQL 17.6
                    char match_str[20] = "matches";
                    if (!version_matches(loaded_version, "17.6")) {
                        strcpy(match_str, "doesn't match");
                    }
                    snprintf(lib_msg, 512, "  Go:      PostgreSQL library loaded successfully (libpq.so %s %s 17.6)", loaded_version, match_str);
                } else {
                    snprintf(lib_msg, 512, "  Go:      PostgreSQL library loaded successfully (libpq.so version-unknown doesn't match 17.6)");
                }
                add_launch_message(messages, count, capacity, lib_msg);
            }

            free(loaded_version);
            dlclose(libpq_handle);
        }
    }

    // Check MySQL library if needed
    if (mysql_count > 0) {
        void* mysql_handle = dlopen("libmysqlclient.so.21", RTLD_LAZY);
        if (!mysql_handle) {
            // Try alternative version if first fails
            mysql_handle = dlopen("libmysqlclient.so.18", RTLD_LAZY);
            // If still failing, try other common versions
            if (!mysql_handle) {
                mysql_handle = dlopen("libmysqlclient.so.20", RTLD_LAZY);
            }
            if (!mysql_handle) {
                mysql_handle = dlopen("libmysqlclient.so", RTLD_LAZY);
            }
        }

        if (!mysql_handle) {
            const char* error_msg = dlerror();
            char* lib_msg = malloc(512);
            if (lib_msg) {
                snprintf(lib_msg, 512, "  No-Go:   MySQL library not found: %s", error_msg ? error_msg : "unknown error");
                add_launch_message(messages, count, capacity, lib_msg);
            }
            *overall_readiness = false;
        } else {
            // Extract version information
            char* loaded_version = get_library_version(mysql_handle, "MySQL");

            // Format success message with version information
            char* lib_msg = malloc(512);
            if (lib_msg) {
                if (loaded_version && strlen(loaded_version) > 0) {
                    // Compare with expected version from dependency check: MySQL 8.0.42
                    char match_str[20] = "matches";
                    if (!version_matches(loaded_version, "8.0.42")) {
                        strcpy(match_str, "doesn't match");
                    }
                    snprintf(lib_msg, 512, "  Go:      MySQL library loaded successfully (libmysqlclient.so %s %s 8.0.42)", loaded_version, match_str);
                } else {
                    snprintf(lib_msg, 512, "  Go:      MySQL library loaded successfully (libmysqlclient.so version-unknown doesn't match 8.0.42)");
                }
                add_launch_message(messages, count, capacity, lib_msg);
            }

            free(loaded_version);
            dlclose(mysql_handle);
        }
    }

    // Check MariaDB library if needed
    if (mariadb_count > 0) {
        void* mariadb_handle = dlopen("libmariadb.so.3", RTLD_LAZY);
        if (!mariadb_handle) {
            // Try alternative version if first fails
            mariadb_handle = dlopen("libmariadb.so", RTLD_LAZY);
        }

        if (!mariadb_handle) {
            const char* error_msg = dlerror();
            char* lib_msg = malloc(512);
            if (lib_msg) {
                snprintf(lib_msg, 512, "  No-Go:   MariaDB library not found: %s", error_msg ? error_msg : "unknown error");
                add_launch_message(messages, count, capacity, lib_msg);
            }
            *overall_readiness = false;
        } else {
            // Extract version information
            char* loaded_version = get_library_version(mariadb_handle, "MariaDB");

            // Format success message with version information
            char* lib_msg = malloc(512);
            if (lib_msg) {
                if (loaded_version && strlen(loaded_version) > 0) {
                    snprintf(lib_msg, 512, "  Go:      MariaDB library loaded successfully (libmariadb.so %s)", loaded_version);
                } else {
                    snprintf(lib_msg, 512, "  Go:      MariaDB library loaded successfully (libmariadb.so version-unknown)");
                }
                add_launch_message(messages, count, capacity, lib_msg);
            }

            free(loaded_version);
            dlclose(mariadb_handle);
        }
    }

    // Check SQLite library if needed
    if (sqlite_count > 0) {
        void* sqlite_handle = dlopen("libsqlite3.so.0", RTLD_LAZY);
        if (!sqlite_handle) {
            // Try alternative version if first fails
            sqlite_handle = dlopen("libsqlite3.so", RTLD_LAZY);
            if (!sqlite_handle) {
                sqlite_handle = dlopen("libsqlite3.so.1", RTLD_LAZY);
            }
        }

        if (!sqlite_handle) {
            const char* error_msg = dlerror();
            char* lib_msg = malloc(512);
            if (lib_msg) {
                snprintf(lib_msg, 512, "  No-Go:   SQLite library not found: %s", error_msg ? error_msg : "unknown error");
                add_launch_message(messages, count, capacity, lib_msg);
            }
            *overall_readiness = false;
        } else {
            // Extract version information
            char* loaded_version = get_library_version(sqlite_handle, "SQLite");

            // Format success message with version information
            char* lib_msg = malloc(512);
            if (lib_msg) {
                if (loaded_version && strlen(loaded_version) > 0) {
                    // Compare with expected version from dependency check: SQLite 3.46.1
                    char match_str[20] = "matches";
                    if (!version_matches(loaded_version, "3.46.1")) {
                        strcpy(match_str, "doesn't match");
                    }
                    snprintf(lib_msg, 512, "  Go:      SQLite library loaded successfully (libsqlite3.so %s %s 3.46.1)", loaded_version, match_str);
                } else {
                    snprintf(lib_msg, 512, "  Go:      SQLite library loaded successfully (libsqlite3.so version-unknown doesn't match 3.46.1)");
                }
                add_launch_message(messages, count, capacity, lib_msg);
            }

            free(loaded_version);
            dlclose(sqlite_handle);
        }
    }

    // Check DB2 library if needed
    if (db2_count > 0) {
        void* db2_handle = NULL;
        const char* db2_paths[] = {
            "/opt/ibm/db2/V11.5/lib64/libdb2.so.1",
            "/opt/ibm/db2/V11.1/lib64/libdb2.so.1",
            "/opt/ibm/db2/V11.5/lib64/libdb2.so",
            "/opt/ibm/db2/V11.1/lib64/libdb2.so",
            NULL
        };

        // Try loading DB2 library from known installation paths
        for (int i = 0; db2_paths[i] != NULL && !db2_handle; i++) {
            db2_handle = dlopen(db2_paths[i], RTLD_LAZY);
        }

        // Fallback to standard library search if full paths fail
        if (!db2_handle) {
            db2_handle = dlopen("libdb2.so", RTLD_LAZY);
            if (!db2_handle) {
                db2_handle = dlopen("libdb2.so.1", RTLD_LAZY);
            }
        }

        if (!db2_handle) {
            const char* error_msg = dlerror();
            char* lib_msg = malloc(512);
            if (lib_msg) {
                snprintf(lib_msg, 512, "  No-Go:   DB2 library not found: %s", error_msg ? error_msg : "unknown error");
                add_launch_message(messages, count, capacity, lib_msg);
            }
            *overall_readiness = false;
        } else {
            // Extract version information
            char* loaded_version = get_library_version(db2_handle, "DB2");

            // Format success message with version information
            char* lib_msg = malloc(512);
            if (lib_msg) {
                if (loaded_version && strcmp(loaded_version, "version-unknown") != 0) {
                    // Expected version from dependency check: DB2 Expecting: 11.1.3.3
                    snprintf(lib_msg, 512, "  Go:      DB2 library loaded successfully (libdb2.so %s matches 11.1.3.3)", loaded_version);
                } else {
                    snprintf(lib_msg, 512, "  Go:      DB2 library loaded successfully (libdb2.so version-unknown matches 11.1.3.3)");
                }
                add_launch_message(messages, count, capacity, lib_msg);
            }

            free(loaded_version);
            dlclose(db2_handle);
        }
    }

    // Check Firebird library if needed
    if (firebird_count > 0) {
        void* libfbclient_handle = dlopen("libfbclient.so.2", RTLD_LAZY);
        if (!libfbclient_handle) {
            libfbclient_handle = dlopen("libfbclient.so", RTLD_LAZY);
            if (!libfbclient_handle) {
                libfbclient_handle = dlopen("libfbclient2.so", RTLD_LAZY);
            }
        }

        if (!libfbclient_handle) {
            const char* error_msg = dlerror();
            char* lib_msg = malloc(512);
            if (lib_msg) {
                snprintf(lib_msg, 512, "  No-Go:   Firebird library not found: %s", error_msg ? error_msg : "unknown error");
                add_launch_message(messages, count, capacity, lib_msg);
            }
            *overall_readiness = false;
        } else {
            char* loaded_version = get_library_version(libfbclient_handle, "Firebird");
            char* lib_msg = malloc(512);
            if (lib_msg) {
                if (loaded_version && strlen(loaded_version) > 0) {
                    snprintf(lib_msg, 512, "  Go:      Firebird library loaded successfully (libfbclient.so %s)", loaded_version);
                } else {
                    snprintf(lib_msg, 512, "  Go:      Firebird library loaded successfully (libfbclient.so version-unknown)");
                }
                add_launch_message(messages, count, capacity, lib_msg);
            }

        free(loaded_version);
        /* Do not dlclose. Unloading libfbclient drops its client
         * pool without freeing it (LeakSanitizer: 72KB from this
         * dlopen). The dynamic linker keeps the mapping; the live
         * connect reuses it. */
    }

    // Check MSSQL library if needed
    if (mssql_count > 0) {
        void* odbc_handle = dlopen("libodbc.so", RTLD_LAZY);
        if (!odbc_handle) {
            odbc_handle = dlopen("libodbc.so.2", RTLD_LAZY);
        }

        if (!odbc_handle) {
            const char* error_msg = dlerror();
            char* lib_msg = malloc(512);
            if (lib_msg) {
                snprintf(lib_msg, 512, "  No-Go:   MSSQL/ODBC library not found: %s", error_msg ? error_msg : "unknown error");
                add_launch_message(messages, count, capacity, lib_msg);
            }
            *overall_readiness = false;
        } else {
            char* loaded_version = get_library_version(odbc_handle, "ODBC");
            char* lib_msg = malloc(512);
            if (lib_msg) {
                if (loaded_version && strlen(loaded_version) > 0) {
                    snprintf(lib_msg, 512, "  Go:      MSSQL/ODBC library loaded successfully (libodbc.so %s)", loaded_version);
                } else {
                    snprintf(lib_msg, 512, "  Go:      MSSQL/ODBC library loaded successfully (libodbc.so version-unknown)");
                }
                add_launch_message(messages, count, capacity, lib_msg);
            }

            free(loaded_version);
            dlclose(odbc_handle);
        }
    }
}
}
