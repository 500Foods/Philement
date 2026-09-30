// Database Subsystem Launch Implementation
// Comprehensive readiness checks and launch functionality

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/database/database_watchdog.h>
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/firebird/connection.h>
#include <src/queue/queue.h>

// Local includes
#include "launch.h"

volatile sig_atomic_t database_stopping = 0;

// Forward declarations
void validate_migration_config(const DatabaseConnection* conn, const char*** messages,
                              size_t* count, size_t* capacity, bool* migrations_go);

// Validate migration configuration for a database connection
void validate_migration_config(const DatabaseConnection* conn, const char*** messages,
                              size_t* count, size_t* capacity, bool* migrations_go) {
    *migrations_go = false;

    if (!conn->auto_migration) {
        // AutoMigration is false, no migration validation needed
        return;
    }

    if (!conn->migrations) {
        // AutoMigration is true but no Migrations specified
        char* msg = malloc(256);
        if (msg) {
            snprintf(msg, 256, "  No-Go:   Autoatic Migration enabled but no Migrations specified for %s",
                    conn->connection_name ? conn->connection_name : "Unknown");
            add_launch_message(messages, count, capacity, msg);
        }
        return;
    }

    // Check if migrations starts with PAYLOAD:
    if (strncmp(conn->migrations, "PAYLOAD:", 8) == 0) {
        // Extract migration name after PAYLOAD:
        const char* migration_name = conn->migrations + 8;
        if (strlen(migration_name) == 0) {
            char* msg = malloc(256);
            if (msg) {
                snprintf(msg, 256, "  No-Go:   Invalid PAYLOAD migration format for %s",
                        conn->connection_name ? conn->connection_name : "Unknown");
                add_launch_message(messages, count, capacity, msg);
            }
            return;
        }

        // Just note that we're using PAYLOAD migrations - actual validation happens at launch
        char* msg = malloc(512);
        if (msg) {
            snprintf(msg, 512, "  Go:      Using PAYLOAD:%s Migrations for %s", migration_name, conn->connection_name);
            add_launch_message(messages, count, capacity, msg);
        }
        *migrations_go = true;
    } else {
        // Path-based migration - just note the path, validation happens at launch
        char* msg = malloc(512);
        if (msg) {
            snprintf(msg, 512, "  Go:      Using %s Migrations for %s", conn->migrations, conn->connection_name);
            add_launch_message(messages, count, capacity, msg);
        }
        *migrations_go = true;
    }
}

// Validate individual database connections
bool validate_database_connections(const DatabaseConfig* db_config, const char*** messages,
                                   size_t* count, size_t* capacity) {
    bool connections_valid = true;
    for (int i = 0; i < db_config->connection_count; i++) {
        const DatabaseConnection* conn = &db_config->connections[i];

        if (!conn->name || strlen(conn->name) < 1 || strlen(conn->name) > 64) {
            char* name_msg = malloc(256);
            if (name_msg) {
                snprintf(name_msg, 256, "  No-Go:   Invalid database name for connection %d", i);
                add_launch_message(messages, count, capacity, name_msg);
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
                    add_launch_message(messages, count, capacity, type_msg);
                }
                conn_valid = false;
            }

            // Check required fields based on database type
            if (conn->type && strcmp(conn->type, "sqlite") == 0) {
                // SQLite only requires Database (filename)
                if (!conn->database) {
                    char* fields_msg = malloc(256);
                    if (fields_msg) {
                        snprintf(fields_msg, 256, "  No-Go:   Missing required field \"Database\" for SQLite connection %s", conn->name);
                        add_launch_message(messages, count, capacity, fields_msg);
                    }
                    conn_valid = false;
                } else {
                    // Check if SQLite database file exists
                    if (access(conn->database, F_OK) == 0) {
                        char* file_msg = malloc(256);
                        if (file_msg) {
                            snprintf(file_msg, 256, "  Go:      SQLite database file \"%s\" found", conn->database);
                            add_launch_message(messages, count, capacity, file_msg);
                        }
                    } else {
                        char* file_msg = malloc(256);
                        if (file_msg) {
                            snprintf(file_msg, 256, "  No-Go:   SQLite database file \"%s\" not found", conn->database);
                            add_launch_message(messages, count, capacity, file_msg);
                        }
                        conn_valid = false;
                    }
                }
            } else {
                // Other databases require database, host, port, user, pass
                bool missing_fields = false;
                char missing_list[256] = "";

                if (!conn->database) {
                    strcat(missing_list, "\"Database\"");
                    missing_fields = true;
                }
                if (!conn->host) {
                    if (missing_fields) strcat(missing_list, ", ");
                    strcat(missing_list, "\"Host\"");
                    missing_fields = true;
                }
                if (!conn->port) {
                    if (missing_fields) strcat(missing_list, ", ");
                    strcat(missing_list, "\"Port\"");
                    missing_fields = true;
                }
                if (!conn->user) {
                    if (missing_fields) strcat(missing_list, ", ");
                    strcat(missing_list, "\"User\"");
                    missing_fields = true;
                }
                if (!conn->pass) {
                    if (missing_fields) strcat(missing_list, ", ");
                    strcat(missing_list, "\"Pass\"");
                    missing_fields = true;
                }

                if (missing_fields) {
                    char* fields_msg = malloc(512);
                    if (fields_msg) {
                        snprintf(fields_msg, 512, "  No-Go:   Missing required fields %s for %s connection %s", missing_list, conn->type ? conn->type : "database", conn->name);
                        add_launch_message(messages, count, capacity, fields_msg);
                    }
                    conn_valid = false;
                }
            }

            // Queue configuration validation is handled during JSON parsing
            // Individual queue parameters are validated there

            if (conn_valid) {
                char* valid_msg = malloc(256);
                if (valid_msg) {
                    snprintf(valid_msg, 256, "  Go:      Database %s configuration valid (Prepared Statement Cache: %d)", 
                             conn->name, conn->prepared_statement_cache_size);
                    add_launch_message(messages, count, capacity, valid_msg);
                }
            } else {
                connections_valid = false;
            }
        } else {
            char* disabled_msg = malloc(256);
            if (disabled_msg) {
                snprintf(disabled_msg, 256, "  Go:      Database %s disabled", conn->name);
                add_launch_message(messages, count, capacity, disabled_msg);
            }
        }
    }
    return connections_valid;
}

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
    int postgres_count, mysql_count, sqlite_count, db2_count, firebird_count, mariadb_count, mssql_count;
    validate_database_configuration(&app_config->databases, &messages, &count, &capacity, &overall_readiness,
                                   &postgres_count, &mysql_count, &sqlite_count, &db2_count, &firebird_count, &mariadb_count, &mssql_count);

    int total_databases = postgres_count + mysql_count + sqlite_count + db2_count + firebird_count + mariadb_count + mssql_count;

    // For non-zero database count, check library dependencies
    if (total_databases > 0) {
        check_database_library_dependencies(&messages, &count, &capacity, &overall_readiness,
                                         postgres_count, mysql_count, sqlite_count, db2_count, firebird_count, mariadb_count, mssql_count);


    }

    // Validate individual database connections
    bool connections_valid = validate_database_connections(&app_config->databases, &messages, &count, &capacity);

    // Validate migration configurations
    bool migrations_overall_go = true;
    for (int i = 0; i < app_config->databases.connection_count; i++) {
        const DatabaseConnection* conn = &app_config->databases.connections[i];
        if (conn->enabled) {
            bool migrations_go = false;
            validate_migration_config(conn, &messages, &count, &capacity, &migrations_go);
            if (conn->auto_migration && !migrations_go) {
                migrations_overall_go = false;
            }
        }
    }

    // Check AutoMigration and TestMigration readiness
    if (migrations_overall_go) {
        for (int i = 0; i < app_config->databases.connection_count; i++) {
            const DatabaseConnection* conn = &app_config->databases.connections[i];
            if (conn->enabled && conn->auto_migration) {
                char* auto_msg = malloc(256);
                if (auto_msg) {
                    snprintf(auto_msg, 256, "  Go:      Automatic Migration enabled for %s",
                            conn->connection_name ? conn->connection_name : "Unknown");
                    add_launch_message(&messages, &count, &capacity, auto_msg);
                }

                if (conn->test_migration) {
                    char* test_msg = malloc(256);
                    if (test_msg) {
                        snprintf(test_msg, 256, "  Go:      Test Migration enabled for %s",
                                conn->connection_name ? conn->connection_name : "Unknown");
                        add_launch_message(&messages, &count, &capacity, test_msg);
                    }
                }
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

    log_this(SR_DATABASE, LOG_LINE_BREAK, LOG_LEVEL_DEBUG, 0);
    log_this(SR_DATABASE, "LAUNCH: " SR_DATABASE, LOG_LEVEL_DEBUG, 0);

    if (!app_config) {
        log_this(SR_DATABASE, "Configuration not loaded", LOG_LEVEL_ERROR, 0);
        log_this(SR_DATABASE, "LAUNCH: " SR_DATABASE " FAILED", LOG_LEVEL_DEBUG, 0);
        return 0;
    }

    // Get database configuration for logging
    const DatabaseConfig* db_config = &app_config->databases;
    log_this(SR_DATABASE, SR_DATABASE " connections configured", LOG_LEVEL_DEBUG, 0);

    // Log each configured database with name and engine type
    for (int i = 0; i < db_config->connection_count; i++) {
        const DatabaseConnection* conn = &db_config->connections[i];
        if (conn->enabled) {
            log_this(SR_DATABASE, "― %s (%s)", LOG_LEVEL_DEBUG, 2,
                     conn->name ? conn->name : "Unknown",
                     conn->type ? conn->type : "Unknown");
        }
    }

    // Initialize database engine registry
    log_this(SR_DATABASE, "Initializing " SR_DATABASE " engine registry", LOG_LEVEL_DEBUG, 0);
    if (!database_engine_init()) {
        log_this(SR_DATABASE, "Failed to initialize " SR_DATABASE " engine registry", LOG_LEVEL_ERROR, 0);
        return 0;
    }

    // Initialize database core
    log_this(SR_DATABASE, "Initializing " SR_DATABASE " core", LOG_LEVEL_DEBUG, 0);
    if (!database_subsystem_init()) {
        log_this(SR_DATABASE, "Failed to initialize " SR_DATABASE " core", LOG_LEVEL_DEBUG, 0);
        return 0;
    }

    // Initialize database query watchdog (detects client-side hangs)
    // The watchdog reads its clamp bounds from the Database config; if
    // app_config is unavailable (e.g. minimal configs), the compile-time
    // defaults in database_watchdog.h apply.
    log_this(SR_DATABASE, "Initializing " SR_DATABASE " query watchdog", LOG_LEVEL_DEBUG, 0);
    if (app_config) {
        database_watchdog_set_bounds(
            app_config->databases.watchdog_min_seconds,
            app_config->databases.watchdog_max_seconds,
            app_config->databases.bootstrap_timeout_seconds);
        if (database_subsystem) {
            database_subsystem->max_connections_per_database = app_config->databases.max_connections_per_database;
        }
    }
    if (!database_watchdog_init()) {
        log_this(SR_DATABASE, "Failed to initialize " SR_DATABASE " query watchdog", LOG_LEVEL_ERROR, 0);
        return 0;
    }

    // Initialize database queue system
    log_this(SR_DATABASE, "Initializing " SR_DATABASE " queue system", LOG_LEVEL_DEBUG, 0);
    if (!database_queue_system_init()) {
        log_this(SR_DATABASE, "Failed to initialize " SR_DATABASE " queue system", LOG_LEVEL_DEBUG, 0);
        return 0;
    }

    // Connect to configured databases and start queues
    int connected_databases = 0;
    int total_queues_started = 0;

    /* Before any engine thread runs. SQLite's extension load publishes
     * sha256_init from /usr/local/lib/crypto.so; Firebird's ChaCha plugin
     * must already be bound to libtomcrypt. */
    for (int i = 0; i < db_config->connection_count; i++) {
        const DatabaseConnection* conn = &db_config->connections[i];
        if (conn->enabled && conn->type && strcasecmp(conn->type, "firebird") == 0) {
            firebird_preload_wire_crypt();
            break;
        }
    }

    for (int i = 0; i < db_config->connection_count; i++) {
        const DatabaseConnection* conn = &db_config->connections[i];

        if (conn->enabled) {

            // With Lead queue architecture, we only start 1 Lead queue per database initially
            int queues_for_db = 1; 
            char queue_count_msg[64];
            snprintf(queue_count_msg, sizeof(queue_count_msg), "%d Lead queue", queues_for_db);

            if (database_add_database(conn->name, conn->type, NULL)) {
                connected_databases++;
                total_queues_started += queues_for_db;
            } else {
                log_this(SR_DATABASE, "Failed to add database", LOG_LEVEL_DEBUG, 0);
            }
        }
    }

    if (connected_databases == 0) {
        log_this(SR_DATABASE, "No databases were successfully connected", LOG_LEVEL_DEBUG, 0);
        return 0;
    }

    char total_queues_msg[128];
    snprintf(total_queues_msg, sizeof(total_queues_msg), "Total Lead queues started across all databases: %d", total_queues_started);
    log_this(SR_DATABASE, total_queues_msg, LOG_LEVEL_DEBUG, 0);

    // Get subsystem ID and update state
    int subsystem_id = get_subsystem_id_by_name(SR_DATABASE);
    if (subsystem_id >= 0) {
        log_this(SR_DATABASE, "Updating " SR_DATABASE " in " SR_REGISTRY, LOG_LEVEL_DEBUG, 0);
        update_subsystem_state(subsystem_id, SUBSYSTEM_RUNNING);
        log_this(SR_DATABASE, "LAUNCH: " SR_DATABASE " COMPLETE", LOG_LEVEL_DEBUG, 0);
        return 1;
    }

    log_this(SR_DATABASE, "LAUNCH: " SR_DATABASE " FAILED", LOG_LEVEL_DEBUG, 0);
    return 0;
}
