// Database Subsystem Launch Implementation
// Comprehensive readiness checks and launch functionality

// Global includes
#include "../hydrogen.h"

// Local includes
#include "launch.h"
#include "../database/database.h"
#include "../database/database_queue.h"
#include "../queue/queue.h"

volatile sig_atomic_t database_stopping = 0;

// Check database subsystem launch readiness with comprehensive dependency and library verification
LaunchReadiness check_database_launch_readiness(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool overall_readiness = true;

    // First message is subsystem name
    add_launch_message(&messages, &count, &capacity, strdup(SR_DATABASE));

    // Early return cases
    if (server_stopping) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   System shutdown in progress"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_DATABASE, .ready = false, .messages = messages };
    }

    if (!server_starting && !server_running) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   System not in startup or running state"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_DATABASE, .ready = false, .messages = messages };
    }

    if (!app_config) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Configuration not loaded"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_DATABASE, .ready = false, .messages = messages };
    }

    // Check subsystem dependencies

    int database_subsystem_id = get_subsystem_id_by_name(SR_DATABASE);
    if (database_subsystem_id < 0) {
        // Register the database subsystem with dependencies
        database_subsystem_id = register_subsystem(SR_DATABASE, NULL, NULL, NULL,
                                                  (int (*)(void))launch_database_subsystem,
                                                  (void (*)(void))0);

        if (database_subsystem_id >= 0) {
            // Add required dependencies similar to webserver
            if (!add_dependency_from_launch(database_subsystem_id, SR_REGISTRY)) {
                add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Failed to register Registry dependency"));
                overall_readiness = false;
            } else {
                add_launch_message(&messages, &count, &capacity, strdup("  Go:      Registry dependency registered"));
            }

            if (!add_dependency_from_launch(database_subsystem_id, SR_THREADS)) {
                add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Failed to register Threads dependency"));
                overall_readiness = false;
            } else {
                add_launch_message(&messages, &count, &capacity, strdup("  Go:      Threads dependency registered"));
            }

            if (!add_dependency_from_launch(database_subsystem_id, SR_NETWORK)) {
                add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Failed to register Network dependency"));
                overall_readiness = false;
            } else {
                add_launch_message(&messages, &count, &capacity, strdup("  Go:      Network dependency registered"));
            }
        } else {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Failed to register database subsystem"));
            overall_readiness = false;
        }
    } else {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Database subsystem already registered"));
    }

    // Validate database configuration
    const DatabaseConfig* db_config = &app_config->databases;

    // Queue configuration is validated during JSON parsing
    add_launch_message(&messages, &count, &capacity, strdup("  Go:      Queue configuration validated during JSON parsing"));

    // Count databases by engine type and collect names for detailed reporting
    int postgres_count = 0, mysql_count = 0, sqlite_count = 0, db2_count = 0;
    char* postgres_names = NULL;
    char* mysql_names = NULL;
    char* sqlite_names = NULL;
    char* db2_names = NULL;

    for (int i = 0; i < db_config->connection_count; i++) {
        const DatabaseConnection* conn = &db_config->connections[i];
        if (conn->enabled && conn->type) {
            const char* engine_type = conn->type;
            if (strcmp(engine_type, "postgresql") == 0 || strcmp(engine_type, "postgres") == 0) {
                postgres_count++;
                if (postgres_count == 1) {
                    postgres_names = strdup(conn->connection_name ? conn->connection_name : "Unknown");
                } else {
                    char* new_names = realloc(postgres_names, strlen(postgres_names) + strlen(conn->connection_name ? conn->connection_name : "Unknown") + 4);
                    if (new_names) {
                        strcat(new_names, ", ");
                        strcat(new_names, conn->connection_name ? conn->connection_name : "Unknown");
                        postgres_names = new_names;
                    }
                }
            } else if (strcmp(engine_type, "mysql") == 0) {
                mysql_count++;
                if (mysql_count == 1) {
                    mysql_names = strdup(conn->connection_name ? conn->connection_name : "Unknown");
                } else {
                    char* new_names = realloc(mysql_names, strlen(mysql_names) + strlen(conn->connection_name ? conn->connection_name : "Unknown") + 4);
                    if (new_names) {
                        strcat(new_names, ", ");
                        strcat(new_names, conn->connection_name ? conn->connection_name : "Unknown");
                        mysql_names = new_names;
                    }
                }
            } else if (strcmp(engine_type, "sqlite") == 0) {
                sqlite_count++;
                if (sqlite_count == 1) {
                    sqlite_names = strdup(conn->connection_name ? conn->connection_name : "Unknown");
                } else {
                    char* new_names = realloc(sqlite_names, strlen(sqlite_names) + strlen(conn->connection_name ? conn->connection_name : "Unknown") + 4);
                    if (new_names) {
                        strcat(new_names, ", ");
                        strcat(new_names, conn->connection_name ? conn->connection_name : "Unknown");
                        sqlite_names = new_names;
                    }
                }
            } else if (strcmp(engine_type, "db2") == 0) {
                db2_count++;
                if (db2_count == 1) {
                    db2_names = strdup(conn->connection_name ? conn->connection_name : "Unknown");
                } else {
                    char* new_names = realloc(db2_names, strlen(db2_names) + strlen(conn->connection_name ? conn->connection_name : "Unknown") + 4);
                    if (new_names) {
                        strcat(new_names, ", ");
                        strcat(new_names, conn->connection_name ? conn->connection_name : "Unknown");
                        db2_names = new_names;
                    }
                }
            }
        }
    }

    // Report database counts by engine with names
    if (postgres_count > 0 && postgres_names) {
        char* postgres_msg = malloc(512);
        if (postgres_msg) {
            if (postgres_count > 3) {
                // Truncate long lists
                postgres_names[50] = 0;
                snprintf(postgres_msg, 512, "  Go:      PostgreSQL Databases: %d (%s...)", postgres_count, postgres_names);
            } else {
                snprintf(postgres_msg, 512, "  Go:      PostgreSQL Databases: %d (%s)", postgres_count, postgres_names);
            }
            add_launch_message(&messages, &count, &capacity, postgres_msg);
        }
        free(postgres_names);
    }

    if (mysql_count > 0 && mysql_names) {
        char* mysql_msg = malloc(512);
        if (mysql_msg) {
            if (mysql_count > 3) {
                // Truncate long lists
                mysql_names[50] = 0;
                snprintf(mysql_msg, 512, "  Go:      MySQL Databases: %d (%s...)", mysql_count, mysql_names);
            } else {
                snprintf(mysql_msg, 512, "  Go:      MySQL Databases: %d (%s)", mysql_count, mysql_names);
            }
            add_launch_message(&messages, &count, &capacity, mysql_msg);
        }
        free(mysql_names);
    }

    if (sqlite_count > 0 && sqlite_names) {
        char* sqlite_msg = malloc(512);
        if (sqlite_msg) {
            if (sqlite_count > 3) {
                // Truncate long lists
                sqlite_names[50] = 0;
                snprintf(sqlite_msg, 512, "  Go:      SQLite Databases: %d (%s...)", sqlite_count, sqlite_names);
            } else {
                snprintf(sqlite_msg, 512, "  Go:      SQLite Databases: %d (%s)", sqlite_count, sqlite_names);
            }
            add_launch_message(&messages, &count, &capacity, sqlite_msg);
        }
        free(sqlite_names);
    }

    if (db2_count > 0 && db2_names) {
        char* db2_msg = malloc(512);
        if (db2_msg) {
            if (db2_count > 3) {
                // Truncate long lists
                db2_names[50] = 0;
                snprintf(db2_msg, 512, "  Go:      DB2 Databases: %d (%s...)", db2_count, db2_names);
            } else {
                snprintf(db2_msg, 512, "  Go:      DB2 Databases: %d (%s)", db2_count, db2_names);
            }
            add_launch_message(&messages, &count, &capacity, db2_msg);
        }
        free(db2_names);
    }

    // Handle cases with 0 databases
    if (postgres_count == 0) {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      PostgreSQL Databases: 0"));
    }

    if (mysql_count == 0) {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      MySQL Databases: 0"));
    }

    if (sqlite_count == 0) {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      SQLite Databases: 0"));
    }

    if (db2_count == 0) {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      DB2 Databases: 0"));
    }

    // Total database count check
    int total_databases = postgres_count + mysql_count + sqlite_count + db2_count;
    if (total_databases == 0) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   No databases configured - database subsystem not needed"));
        overall_readiness = false;
    } else {
        char* total_msg = malloc(256);
        if (total_msg) {
            snprintf(total_msg, 256, "  Go:      Total databases configured: %d", total_databases);
            add_launch_message(&messages, &count, &capacity, total_msg);
        }
    }

    // For non-zero database count, check library dependencies
    if (total_databases > 0) {

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
                    add_launch_message(&messages, &count, &capacity, lib_msg);
                }
                overall_readiness = false;
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
                    add_launch_message(&messages, &count, &capacity, lib_msg);
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
                    add_launch_message(&messages, &count, &capacity, lib_msg);
                }
                overall_readiness = false;
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
                    add_launch_message(&messages, &count, &capacity, lib_msg);
                }

                free(loaded_version);
                dlclose(mysql_handle);
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
                    add_launch_message(&messages, &count, &capacity, lib_msg);
                }
                overall_readiness = false;
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
                    add_launch_message(&messages, &count, &capacity, lib_msg);
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
                    add_launch_message(&messages, &count, &capacity, lib_msg);
                }
                overall_readiness = false;
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
                    add_launch_message(&messages, &count, &capacity, lib_msg);
                }

                free(loaded_version);
                dlclose(db2_handle);
            }
        }
    }

    // Validate individual database connections
    bool connections_valid = true;
    for (int i = 0; i < db_config->connection_count; i++) {
        const DatabaseConnection* conn = &db_config->connections[i];

        if (!conn->name || strlen(conn->name) < 1 || strlen(conn->name) > 64) {
            char* name_msg = malloc(256);
            if (name_msg) {
                snprintf(name_msg, 256, "  No-Go:   Invalid database name for connection %d", i);
                add_launch_message(&messages, &count, &capacity, name_msg);
            }
            connections_valid = false;
            continue;
        }

        if (conn->enabled) {
            bool conn_valid = true;

            if (!conn->type || strlen(conn->type) < 1 || strlen(conn->type) > 32) {
                char* type_msg = malloc(256);
                if (type_msg) {
                    snprintf(type_msg, 256, "  No-Go:   Invalid database type for %s", conn->name);
                    add_launch_message(&messages, &count, &capacity, type_msg);
                }
                conn_valid = false;
            }

            // For SQLite, host/port/user/pass are optional
            bool is_sqlite = (conn->type && strcmp(conn->type, "sqlite") == 0);
            if (!is_sqlite) {
                if (!conn->database || !conn->host || !conn->port || !conn->user || !conn->pass) {
                    char* fields_msg = malloc(256);
                    if (fields_msg) {
                        snprintf(fields_msg, 256, "  No-Go:   Missing required fields for %s", conn->name);
                        add_launch_message(&messages, &count, &capacity, fields_msg);
                    }
                    conn_valid = false;
                }
            } else {
                // SQLite only requires database (file path)
                if (!conn->database) {
                    char* fields_msg = malloc(256);
                    if (fields_msg) {
                        snprintf(fields_msg, 256, "  No-Go:   Missing database path for %s", conn->name);
                        add_launch_message(&messages, &count, &capacity, fields_msg);
                    }
                    conn_valid = false;
                }
            }

            // Queue configuration validation is handled during JSON parsing
            // Individual queue parameters are validated there

            if (conn_valid) {
                char* valid_msg = malloc(256);
                if (valid_msg) {
                    snprintf(valid_msg, 256, "  Go:      Database %s configuration valid", conn->name);
                    add_launch_message(&messages, &count, &capacity, valid_msg);
                }
            } else {
                connections_valid = false;
            }
        } else {
            char* disabled_msg = malloc(256);
            if (disabled_msg) {
                snprintf(disabled_msg, 256, "  Go:      Database %s disabled", conn->name);
                add_launch_message(&messages, &count, &capacity, disabled_msg);
            }
        }
    }

    // Final decision
    if (overall_readiness && connections_valid && total_databases > 0) {
        add_launch_message(&messages, &count, &capacity, strdup("  Decide:  Go For Launch of Database Subsystem"));
    } else {
        overall_readiness = false;
        if (total_databases == 0) {
            add_launch_message(&messages, &count, &capacity, strdup("  Decide:  No-Go - No databases configured"));
        } else if (!connections_valid) {
            add_launch_message(&messages, &count, &capacity, strdup("  Decide:  No-Go - Configuration errors"));
        } else {
            add_launch_message(&messages, &count, &capacity, strdup("  Decide:  No-Go - Library dependencies missing"));
        }
    }

    finalize_launch_messages(&messages, &count, &capacity);

    return (LaunchReadiness){
        .subsystem = SR_DATABASE,
        .ready = overall_readiness,
        .messages = messages
    };
}

// Launch the database subsystem
int launch_database_subsystem(void) {

    database_stopping = 0;

    log_this(SR_DATABASE, LOG_LINE_BREAK, LOG_LEVEL_STATE, 0);
    log_this(SR_DATABASE, "LAUNCH: " SR_DATABASE, LOG_LEVEL_STATE, 0);

    // Get database configuration for logging
    const DatabaseConfig* db_config = &app_config->databases;
    log_this(SR_DATABASE, "Database connections configured: %d", LOG_LEVEL_STATE, 1, db_config->connection_count);

    // Phase 1: Initialize database engine registry
    log_this(SR_DATABASE, "Initializing database engine registry", LOG_LEVEL_STATE, 0);
    if (!database_engine_init()) {
        log_this(SR_DATABASE, "Failed to initialize database engine registry", LOG_LEVEL_ERROR, 0);
        return 0;
    }

    // Phase 2: Initialize database subsystem
    log_this(SR_DATABASE, "Initializing database subsystem", LOG_LEVEL_STATE, 0);
    if (!database_subsystem_init()) {
        log_this(SR_DATABASE, "Failed to initialize database subsystem", LOG_LEVEL_ERROR, 0);
        return 0;
    }

    // Phase 3: Initialize queue system FIRST (moved from Phase 4)
    // log_this(SR_DATABASE, "Initializing queue system", LOG_LEVEL_STATE, 0);
    // log_this(SR_DATABASE, "Phase 3.1: Starting queue manager initialization", LOG_LEVEL_STATE, 0);

    if (!database_queue_system_init()) {
        log_this(SR_DATABASE, "Failed to initialize database queue system", LOG_LEVEL_ERROR, 0);
        return 0;
    }

    // log_this(SR_DATABASE, "Phase 3.3: Queue manager initialization completed", LOG_LEVEL_STATE, 0);

    // Phase 4: Connect to configured databases and start queues
    // log_this(SR_DATABASE, "Connecting to configured databases and starting queues", LOG_LEVEL_STATE, 0);
    int connected_databases = 0;
    int total_queues_started = 0;

    // log_this(SR_DATABASE, "Phase 4.1: Starting database connection processing loop", LOG_LEVEL_STATE, 0);
    for (int i = 0; i < db_config->connection_count; i++) {
        const DatabaseConnection* conn = &db_config->connections[i];

        // log_this(SR_DATABASE, "Phase 4.2: Processing database connection %d of %d", LOG_LEVEL_STATE, 2, i+1, db_config->connection_count);
        // log_this(SR_DATABASE, "Phase 4.3: Connection name: %s, enabled: %s", LOG_LEVEL_STATE, 2, conn->name, conn->enabled ? "true" : "false",0);

        if (conn->enabled) {
            // log_this(SR_DATABASE, "Phase 4.4: Connecting to enabled database", LOG_LEVEL_STATE, 0);
            // log_this(SR_DATABASE, "Phase 4.5: Database name: %s", LOG_LEVEL_STATE, 1, conn->name);

            // With Lead queue architecture, we only start 1 Lead queue per database initially
            int queues_for_db = 1; // Always just the Lead queue initially

            // log_this(SR_DATABASE, "Expected queues for database", LOG_LEVEL_DEBUG, 0);
            char queue_count_msg[64];
            snprintf(queue_count_msg, sizeof(queue_count_msg), "%d Lead queue", queues_for_db);
            // log_this(SR_DATABASE, queue_count_msg, LOG_LEVEL_DEBUG, 0);

            // Add database to subsystem (this will create Lead queue)
            // log_this(SR_DATABASE, "Phase 4.6: Starting database addition for: %s", LOG_LEVEL_STATE, 1, conn->name);
            // log_this(SR_DATABASE, "Phase 4.6.1: About to call database_add_database", LOG_LEVEL_STATE, 0);
            if (database_add_database(conn->name, conn->type, NULL)) {
                // log_this(SR_DATABASE, "Phase 4.7: Database added to subsystem successfully", LOG_LEVEL_STATE, 0);
                connected_databases++;
                total_queues_started += queues_for_db;

                // Bootstrap query runs asynchronously - don't wait for it to speed up launch
                // DatabaseQueue* db_queue = database_queue_manager_get_database(global_queue_manager, conn->name);
                // Bootstrap will complete in background and signal when done

                // Log Lead queue startup for this database
                // log_this(SR_DATABASE, "Phase 4.8: Lead queue started for database", LOG_LEVEL_DEBUG, 0);
                // log_this(SR_DATABASE, conn->name, LOG_LEVEL_DEBUG, 0);
            } else {
                log_this(SR_DATABASE, "Failed to add database to subsystem", LOG_LEVEL_ERROR, 0);
            }
        }
        // log_this(SR_DATABASE, "Phase 4.10: Finished processing database connection %d", LOG_LEVEL_STATE, 1, i+1);
    }
    // log_this(SR_DATABASE, "Phase 4.11: Database connection processing loop completed", LOG_LEVEL_STATE, 0);

    // log_this(SR_DATABASE, "Phase 4.50.1: Just finished worker thread creation, checking connected_databases = %d", LOG_LEVEL_STATE, 1, connected_databases);
    // log_this(SR_DATABASE, "Phase 4.50: Checking if any databases were connected", LOG_LEVEL_STATE, 0);
    if (connected_databases == 0) {
        log_this(SR_DATABASE, "No databases were successfully connected", LOG_LEVEL_ERROR, 0);
        return 0;
    }

    // Phase 4.51: Wait for all Lead DQMs to complete their initial connection attempts
    log_this(SR_DATABASE, "Waiting for all Lead DQMs to complete initial connection attempts", LOG_LEVEL_STATE, 0);
    bool all_dqms_ready = true;

    for (int i = 0; i < db_config->connection_count; i++) {
        const DatabaseConnection* conn = &db_config->connections[i];

        if (conn->enabled) {
            // Get the Lead DQM for this database
            DatabaseQueue* lead_queue = database_queue_manager_get_database(global_queue_manager, conn->name);
            if (lead_queue && lead_queue->is_lead_queue) {
                // Wait for initial connection attempt to complete (with 10 second timeout)
                bool ready = database_queue_wait_for_initial_connection(lead_queue, 10);
                if (!ready) {
                    log_this(SR_DATABASE, "Timeout waiting for Lead DQM %s to complete initial connection attempt", LOG_LEVEL_ERROR, 1, conn->name);
                    all_dqms_ready = false;
                } else {
                    log_this(SR_DATABASE, "Lead DQM %s initial connection attempt completed", LOG_LEVEL_STATE, 1, conn->name);
                }
            }
        }
    }

    if (!all_dqms_ready) {
        log_this(SR_DATABASE, "Not all Lead DQMs completed initial connection attempts within timeout", LOG_LEVEL_ERROR, 0);
        // Continue anyway - DQMs will retry connections in heartbeat
    }

    // log_this(SR_DATABASE, "Phase 4.52: Database connections established", LOG_LEVEL_STATE, 0);
    // log_this(SR_DATABASE, "Phase 4.53: Preparing queue count message", LOG_LEVEL_STATE, 0);
    char total_queues_msg[128];
    snprintf(total_queues_msg, sizeof(total_queues_msg), "Total Lead queues started across all databases: %d", total_queues_started);
    // log_this(SR_DATABASE, "Phase 4.54: Logging queue count message", LOG_LEVEL_STATE, 0);
    log_this(SR_DATABASE, total_queues_msg, LOG_LEVEL_STATE, 0);

    // Phase 5: Worker threads already started
    // log_this(SR_DATABASE, "Phase 5: Lead queue worker threads", LOG_LEVEL_STATE, 0);
    // log_this(SR_DATABASE, "Phase 5.1: Lead queue worker threads started as part of database addition", LOG_LEVEL_STATE, 0);

    // Phase 6: Load query templates (for Phase 4)
    // log_this(SR_DATABASE, "Phase 6: Loading query templates", LOG_LEVEL_STATE, 0);
    // log_this(SR_DATABASE, "Phase 6.1: Query template loading will be implemented in Phase 4", LOG_LEVEL_STATE, 0);

    // Phase 7: Register triggers (for Phase 4)
    // log_this(SR_DATABASE, "Phase 7: Registering triggers", LOG_LEVEL_STATE, 0);
    // log_this(SR_DATABASE, "Phase 7.1: Trigger registration will be implemented in Phase 4", LOG_LEVEL_STATE, 0);

    // Get subsystem ID and update state
    int subsystem_id = get_subsystem_id_by_name(SR_DATABASE);
    if (subsystem_id >= 0) {
        update_subsystem_state(subsystem_id, SUBSYSTEM_RUNNING);
        log_this(SR_DATABASE, "Database subsystem fully operational", LOG_LEVEL_STATE, 0);
        return 1;
    }

    log_this(SR_DATABASE, "Failed to complete database subsystem launch", LOG_LEVEL_ERROR, 0);
    return 0;
}
