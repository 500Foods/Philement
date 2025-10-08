/*
 * Database Configuration Implementation
 *
 * Implements the configuration handlers for database operations,
 * including JSON parsing and environment variable handling.
 * Note: Validation moved to launch readiness checks
 */

// Global includes 
#include "../hydrogen.h"

// Local includes
#include "config_databases.h"

// Function declarations
void cleanup_database_connection(DatabaseConnection* conn);
void dump_database_config(const DatabaseConfig* config);

// Load database configuration from JSON
bool load_database_config(json_t* root, AppConfig* config) {
    bool success = true;
    DatabaseConfig* db_config = &config->databases;

    // Initialize database config with no defaults
    memset(db_config, 0, sizeof(DatabaseConfig));

    // Set default queue scaling configurations
    // Slow queue: conservative scaling
    db_config->default_queues.slow.start = 0;
    db_config->default_queues.slow.min = 1;
    db_config->default_queues.slow.max = 4;
    db_config->default_queues.slow.up = 10;
    db_config->default_queues.slow.down = 2;
    db_config->default_queues.slow.inactivity = 300;

    // Medium queue: moderate scaling
    db_config->default_queues.medium.start = 0;
    db_config->default_queues.medium.min = 1;
    db_config->default_queues.medium.max = 8;
    db_config->default_queues.medium.up = 15;
    db_config->default_queues.medium.down = 3;
    db_config->default_queues.medium.inactivity = 240;

    // Fast queue: aggressive scaling
    db_config->default_queues.fast.start = 0;
    db_config->default_queues.fast.min = 2;
    db_config->default_queues.fast.max = 16;
    db_config->default_queues.fast.up = 20;
    db_config->default_queues.fast.down = 5;
    db_config->default_queues.fast.inactivity = 180;

    // Cache queue: minimal scaling
    db_config->default_queues.cache.start = 0;
    db_config->default_queues.cache.min = 1;
    db_config->default_queues.cache.max = 4;
    db_config->default_queues.cache.up = 5;
    db_config->default_queues.cache.down = 1;
    db_config->default_queues.cache.inactivity = 600;

    // Get Databases section
    json_t* databases_obj = json_object_get(root, "Databases");
    if (!databases_obj || !json_is_object(databases_obj)) {
        // No databases section found - use empty config
        db_config->connection_count = 0;
        return true;
    }

    // Get ConnectionCount
    json_t* connection_count_obj = json_object_get(databases_obj, "ConnectionCount");
    if (connection_count_obj && json_is_integer(connection_count_obj)) {
        db_config->connection_count = (int)json_integer_value(connection_count_obj);
    } else {
        db_config->connection_count = 5;  // Default to max supported
    }


    // Get Connections section
    json_t* connections_obj = json_object_get(databases_obj, "Connections");
    if (!connections_obj) {
        // No connections section - no databases
        db_config->connection_count = 0;
        return true;
    }

    success = PROCESS_SECTION(root, "Databases");
    success = success && PROCESS_SECTION(root, "Databases.Connections");

    // Log top-level database settings during config load (consistent with other sections)
    success = success && PROCESS_INT(root, db_config, connection_count, "Databases.ConnectionCount", "Databases");

    // Handle array-based connections (the format you're using)
    if (json_is_array(connections_obj)) {
        size_t array_size = json_array_size(connections_obj);
        if ((int)array_size > db_config->connection_count) {
            array_size = (size_t)db_config->connection_count;
        }

        int valid_connections = 0;
        for (size_t i = 0; i < array_size; i++) {
            json_t* conn_obj = json_array_get(connections_obj, i);
            if (!json_is_object(conn_obj)) continue;

            DatabaseConnection* conn = &db_config->connections[i];
            memset(conn, 0, sizeof(DatabaseConnection));

            // Extract database details
            json_t* name_obj = json_object_get(conn_obj, "Name");
            if (name_obj && json_is_string(name_obj)) {
                const char* name_str = json_string_value(name_obj);
                char* resolved_name = process_env_variable_string(name_str);
                if (resolved_name) {
                    conn->name = resolved_name;
                    conn->connection_name = strdup(resolved_name);
                } else {
                    conn->name = strdup(name_str);
                    conn->connection_name = strdup(name_str);
                }
            }

            json_t* enabled_obj = json_object_get(conn_obj, "Enabled");
            conn->enabled = (enabled_obj && json_is_boolean(enabled_obj)) ?
                            json_boolean_value(enabled_obj) : false;


            // Try Engine first, then Type
            json_t* engine_obj = json_object_get(conn_obj, "Engine");
            if (engine_obj && json_is_string(engine_obj)) {
                conn->type = strdup(json_string_value(engine_obj));
            } else {
                json_t* type_obj = json_object_get(conn_obj, "Type");
                if (type_obj && json_is_string(type_obj)) {
                    conn->type = strdup(json_string_value(type_obj));
                }
            }

            // Extract connection details (SQLite doesn't need host/port/user/pass)
            json_t* database_obj = json_object_get(conn_obj, "Database");
            if (database_obj && json_is_string(database_obj)) {
                const char* database_str = json_string_value(database_obj);
                char* resolved_database = process_env_variable_string(database_str);
                if (resolved_database) {
                    conn->database = resolved_database;
                } else {
                    conn->database = strdup(database_str);
                }
            } else {
                log_this(SR_CONFIG_CURRENT, "Database config: WARNING - Database field not found or not a string", LOG_LEVEL_ERROR, 0);
                if (database_obj) {
                    log_this(SR_CONFIG_CURRENT, "Database config: Database object type: %d", LOG_LEVEL_DEBUG, 1, json_typeof(database_obj));
                } else {
                    log_this(SR_CONFIG_CURRENT, "Database config: Database object is NULL", LOG_LEVEL_DEBUG, 0);
                }
            }

            // Extract Bootstrap query
            json_t* bootstrap_obj = json_object_get(conn_obj, "Bootstrap");
            if (bootstrap_obj && json_is_string(bootstrap_obj)) {
                conn->bootstrap_query = strdup(json_string_value(bootstrap_obj));
            }

            // Extract Schema
            json_t* schema_obj = json_object_get(conn_obj, "Schema");
            if (schema_obj && json_is_string(schema_obj)) {
                conn->schema = strdup(json_string_value(schema_obj));
            }

            // Extract migration settings
            json_t* auto_migration_obj = json_object_get(conn_obj, "AutoMigration");
            conn->auto_migration = (auto_migration_obj && json_is_boolean(auto_migration_obj)) ?
                                   json_boolean_value(auto_migration_obj) : false;

            json_t* test_migration_obj = json_object_get(conn_obj, "TestMigration");
            conn->test_migration = (test_migration_obj && json_is_boolean(test_migration_obj)) ?
                                   json_boolean_value(test_migration_obj) : false;

            json_t* migrations_obj = json_object_get(conn_obj, "Migrations");
            if (migrations_obj && json_is_string(migrations_obj)) {
                const char* migrations_str = json_string_value(migrations_obj);
                char* resolved_migrations = process_env_variable_string(migrations_str);
                if (resolved_migrations) {
                    conn->migrations = resolved_migrations;
                } else {
                    conn->migrations = strdup(migrations_str);
                }
                // log_this(SR_CONFIG_CURRENT, "Database config: Loaded migrations: %s", LOG_LEVEL_DEBUG, 1, conn->migrations);
            }

            // Only extract network fields for non-SQLite databases
            if (conn->type && strcmp(conn->type, "sqlite") != 0) {
                json_t* host_obj = json_object_get(conn_obj, "Host");
                if (host_obj && json_is_string(host_obj)) {
                    conn->host = strdup(json_string_value(host_obj));
                }

                json_t* port_obj = json_object_get(conn_obj, "Port");
                if (port_obj) {
                    if (json_is_string(port_obj)) {
                        conn->port = strdup(json_string_value(port_obj));
                    } else if (json_is_integer(port_obj)) {
                        // Convert integer to string
                        char port_str[16];
                        snprintf(port_str, sizeof(port_str), "%lld", (long long)json_integer_value(port_obj));
                        conn->port = strdup(port_str);
                    }
                }

                json_t* user_obj = json_object_get(conn_obj, "User");
                if (user_obj && json_is_string(user_obj)) {
                    conn->user = strdup(json_string_value(user_obj));
                }

                json_t* pass_obj = json_object_get(conn_obj, "Pass");
                if (pass_obj && json_is_string(pass_obj)) {
                    conn->pass = strdup(json_string_value(pass_obj));
                }
            }

            // Process queue configuration with defaults
            json_t* queues_obj = json_object_get(conn_obj, "Queues");
            if (queues_obj && json_is_object(queues_obj)) {
                // Process each queue type
                json_t* slow_obj = json_object_get(queues_obj, "Slow");
                if (slow_obj && json_is_object(slow_obj)) {
                    json_t* start_obj = json_object_get(slow_obj, "start");
                    conn->queues.slow.start = (start_obj && json_is_integer(start_obj)) ?
                                             (int)json_integer_value(start_obj) : db_config->default_queues.slow.start;
                    json_t* min_obj = json_object_get(slow_obj, "min");
                    conn->queues.slow.min = (min_obj && json_is_integer(min_obj)) ?
                                           (int)json_integer_value(min_obj) : db_config->default_queues.slow.min;
                    json_t* max_obj = json_object_get(slow_obj, "max");
                    conn->queues.slow.max = (max_obj && json_is_integer(max_obj)) ?
                                           (int)json_integer_value(max_obj) : db_config->default_queues.slow.max;
                    json_t* up_obj = json_object_get(slow_obj, "up");
                    conn->queues.slow.up = (up_obj && json_is_integer(up_obj)) ?
                                          (int)json_integer_value(up_obj) : db_config->default_queues.slow.up;
                    json_t* down_obj = json_object_get(slow_obj, "down");
                    conn->queues.slow.down = (down_obj && json_is_integer(down_obj)) ?
                                            (int)json_integer_value(down_obj) : db_config->default_queues.slow.down;
                    json_t* inactivity_obj = json_object_get(slow_obj, "inactivity");
                    conn->queues.slow.inactivity = (inactivity_obj && json_is_integer(inactivity_obj)) ?
                                                  (int)json_integer_value(inactivity_obj) : db_config->default_queues.slow.inactivity;
                } else {
                    // Use defaults
                    conn->queues.slow = db_config->default_queues.slow;
                }

                // Process Medium queue
                json_t* medium_obj = json_object_get(queues_obj, "Medium");
                if (medium_obj && json_is_object(medium_obj)) {
                    json_t* start_obj = json_object_get(medium_obj, "start");
                    conn->queues.medium.start = (start_obj && json_is_integer(start_obj)) ?
                                               (int)json_integer_value(start_obj) : db_config->default_queues.medium.start;
                    json_t* min_obj = json_object_get(medium_obj, "min");
                    conn->queues.medium.min = (min_obj && json_is_integer(min_obj)) ?
                                             (int)json_integer_value(min_obj) : db_config->default_queues.medium.min;
                    json_t* max_obj = json_object_get(medium_obj, "max");
                    conn->queues.medium.max = (max_obj && json_is_integer(max_obj)) ?
                                             (int)json_integer_value(max_obj) : db_config->default_queues.medium.max;
                    json_t* up_obj = json_object_get(medium_obj, "up");
                    conn->queues.medium.up = (up_obj && json_is_integer(up_obj)) ?
                                            (int)json_integer_value(up_obj) : db_config->default_queues.medium.up;
                    json_t* down_obj = json_object_get(medium_obj, "down");
                    conn->queues.medium.down = (down_obj && json_is_integer(down_obj)) ?
                                              (int)json_integer_value(down_obj) : db_config->default_queues.medium.down;
                    json_t* inactivity_obj = json_object_get(medium_obj, "inactivity");
                    conn->queues.medium.inactivity = (inactivity_obj && json_is_integer(inactivity_obj)) ?
                                                    (int)json_integer_value(inactivity_obj) : db_config->default_queues.medium.inactivity;
                } else {
                    conn->queues.medium = db_config->default_queues.medium;
                }

                // Process Fast queue
                json_t* fast_obj = json_object_get(queues_obj, "Fast");
                if (fast_obj && json_is_object(fast_obj)) {
                    json_t* start_obj = json_object_get(fast_obj, "start");
                    conn->queues.fast.start = (start_obj && json_is_integer(start_obj)) ?
                                             (int)json_integer_value(start_obj) : db_config->default_queues.fast.start;
                    json_t* min_obj = json_object_get(fast_obj, "min");
                    conn->queues.fast.min = (min_obj && json_is_integer(min_obj)) ?
                                           (int)json_integer_value(min_obj) : db_config->default_queues.fast.min;
                    json_t* max_obj = json_object_get(fast_obj, "max");
                    conn->queues.fast.max = (max_obj && json_is_integer(max_obj)) ?
                                           (int)json_integer_value(max_obj) : db_config->default_queues.fast.max;
                    json_t* up_obj = json_object_get(fast_obj, "up");
                    conn->queues.fast.up = (up_obj && json_is_integer(up_obj)) ?
                                          (int)json_integer_value(up_obj) : db_config->default_queues.fast.up;
                    json_t* down_obj = json_object_get(fast_obj, "down");
                    conn->queues.fast.down = (down_obj && json_is_integer(down_obj)) ?
                                            (int)json_integer_value(down_obj) : db_config->default_queues.fast.down;
                    json_t* inactivity_obj = json_object_get(fast_obj, "inactivity");
                    conn->queues.fast.inactivity = (inactivity_obj && json_is_integer(inactivity_obj)) ?
                                                  (int)json_integer_value(inactivity_obj) : db_config->default_queues.fast.inactivity;
                } else {
                    conn->queues.fast = db_config->default_queues.fast;
                }

                // Process Cache queue
                json_t* cache_obj = json_object_get(queues_obj, "Cache");
                if (cache_obj && json_is_object(cache_obj)) {
                    json_t* start_obj = json_object_get(cache_obj, "start");
                    conn->queues.cache.start = (start_obj && json_is_integer(start_obj)) ?
                                              (int)json_integer_value(start_obj) : db_config->default_queues.cache.start;
                    json_t* min_obj = json_object_get(cache_obj, "min");
                    conn->queues.cache.min = (min_obj && json_is_integer(min_obj)) ?
                                            (int)json_integer_value(min_obj) : db_config->default_queues.cache.min;
                    json_t* max_obj = json_object_get(cache_obj, "max");
                    conn->queues.cache.max = (max_obj && json_is_integer(max_obj)) ?
                                            (int)json_integer_value(max_obj) : db_config->default_queues.cache.max;
                    json_t* up_obj = json_object_get(cache_obj, "up");
                    conn->queues.cache.up = (up_obj && json_is_integer(up_obj)) ?
                                           (int)json_integer_value(up_obj) : db_config->default_queues.cache.up;
                    json_t* down_obj = json_object_get(cache_obj, "down");
                    conn->queues.cache.down = (down_obj && json_is_integer(down_obj)) ?
                                             (int)json_integer_value(down_obj) : db_config->default_queues.cache.down;
                    json_t* inactivity_obj = json_object_get(cache_obj, "inactivity");
                    conn->queues.cache.inactivity = (inactivity_obj && json_is_integer(inactivity_obj)) ?
                                                   (int)json_integer_value(inactivity_obj) : db_config->default_queues.cache.inactivity;
                } else {
                    conn->queues.cache = db_config->default_queues.cache;
                }
            } else {
                // No queue configuration provided, use all defaults
                conn->queues = db_config->default_queues;
            }

            // Emit configuration logging for this connection (array-based format)
            do {
                const char* cname = (conn->connection_name && strlen(conn->connection_name) > 0)
                                    ? conn->connection_name : "Unknown";
                char cpath[256];
                snprintf(cpath, sizeof(cpath), "Databases.Connections.%s", cname);

                // Subsection header for this connection
                success = success && PROCESS_SECTION(root, cpath);

                // Build per-key paths under this connection
                char kpath[320];

                // Enabled (bool)
                snprintf(kpath, sizeof(kpath), "%s.Enabled", cpath);
                PROCESS_BOOL(root, conn, enabled, kpath, "Databases");

                // Type/Engine
                snprintf(kpath, sizeof(kpath), "%s.Type", cpath);
                PROCESS_STRING(NULL, conn, type, kpath, "Databases");

                // Database (file or DB name)
                snprintf(kpath, sizeof(kpath), "%s.Database", cpath);
                PROCESS_STRING(NULL, conn, database, kpath, "Databases");

                // Host
                snprintf(kpath, sizeof(kpath), "%s.Host", cpath);
                PROCESS_STRING(NULL, conn, host, kpath, "Databases");

                // Port
                snprintf(kpath, sizeof(kpath), "%s.Port", cpath);
                PROCESS_STRING(NULL, conn, port, kpath, "Databases");

                // User
                snprintf(kpath, sizeof(kpath), "%s.User", cpath);
                PROCESS_STRING(NULL, conn, user, kpath, "Databases");

                // Pass (sensitive)
                snprintf(kpath, sizeof(kpath), "%s.Pass", cpath);
                PROCESS_SENSITIVE(NULL, conn, pass, kpath, "Databases");

                // Bootstrap query
                snprintf(kpath, sizeof(kpath), "%s.Bootstrap", cpath);
                PROCESS_STRING(NULL, conn, bootstrap_query, kpath, "Databases");

                // Schema
                snprintf(kpath, sizeof(kpath), "%s.Schema", cpath);
                PROCESS_STRING(NULL, conn, schema, kpath, "Databases");

                // AutoMigration
                snprintf(kpath, sizeof(kpath), "%s.AutoMigration", cpath);
                PROCESS_BOOL(root, conn, auto_migration, kpath, "Databases");

                // TestMigration
                snprintf(kpath, sizeof(kpath), "%s.TestMigration", cpath);
                PROCESS_BOOL(root, conn, test_migration, kpath, "Databases");

                // Migrations
                snprintf(kpath, sizeof(kpath), "%s.Migrations", cpath);
                PROCESS_STRING(NULL, conn, migrations, kpath, "Databases");

            } while (0);

            valid_connections++;
        }

        // Update connection count to match actually loaded connections
        db_config->connection_count = valid_connections;
    }
    // Handle object-based connections (legacy format)
    else if (json_is_object(connections_obj)) {
        db_config->connection_count = 0;  // Set to 0 initially, will count as we process
        const char* key;
        json_t* value;
        int db_index = 0;

        json_object_foreach(connections_obj, key, value) {
            // cppcheck-suppress[knownConditionTrueFalse] - cppcheck incorrectly assumes db_index never exceeds 4, but it does when there are more than 5 connections
            if (db_index > 4) break;  // Safety limit

            DatabaseConnection* conn = &db_config->connections[db_index];
            memset(conn, 0, sizeof(DatabaseConnection));

            conn->name = strdup(key);
            conn->connection_name = strdup(key);

            // Process this database's configuration
            char path[256];
            snprintf(path, sizeof(path), "Databases.Connections.%s", key);
            success = success && PROCESS_SECTION(root, path);

            // Process enabled status
            snprintf(path, sizeof(path), "Databases.Connections.%s.Enabled", key);
            success = success && PROCESS_BOOL(root, conn, enabled, path, "Databases");

            if (conn->enabled) {
                // Try Engine first, then Type
                char engine_path[256], type_path[256];
                snprintf(engine_path, sizeof(engine_path), "Databases.Connections.%s.Engine", key);
                snprintf(type_path, sizeof(type_path), "Databases.Connections.%s.Type", key);
                success = success && (PROCESS_STRING(root, conn, type, engine_path, "Databases") ||
                                      PROCESS_STRING(root, conn, type, type_path, "Databases"));

                // Process other database fields
                snprintf(path, sizeof(path), "Databases.Connections.%s.Database", key);
                success = success && PROCESS_STRING(root, conn, database, path, "Databases");

                snprintf(path, sizeof(path), "Databases.Connections.%s.Host", key);
                success = success && PROCESS_STRING(root, conn, host, path, "Databases");

                snprintf(path, sizeof(path), "Databases.Connections.%s.Port", key);
                success = success && PROCESS_STRING(root, conn, port, path, "Databases");

                snprintf(path, sizeof(path), "Databases.Connections.%s.User", key);
                success = success && PROCESS_STRING(root, conn, user, path, "Databases");

                snprintf(path, sizeof(path), "Databases.Connections.%s.Pass", key);
                success = success && PROCESS_SENSITIVE(root, conn, pass, path, "Databases");

                // Process migration settings
                char auto_migration_path[256], test_migration_path[256], migrations_path[256];
                snprintf(auto_migration_path, sizeof(auto_migration_path), "Databases.Connections.%s.AutoMigration", key);
                success = success && PROCESS_BOOL(root, conn, auto_migration, auto_migration_path, "Databases");

                snprintf(test_migration_path, sizeof(test_migration_path), "Databases.Connections.%s.TestMigration", key);
                success = success && PROCESS_BOOL(root, conn, test_migration, test_migration_path, "Databases");

                snprintf(migrations_path, sizeof(migrations_path), "Databases.Connections.%s.Migrations", key);
                success = success && PROCESS_STRING(root, conn, migrations, migrations_path, "Databases");
            }

            db_index++;
        }
        db_config->connection_count = db_index;
    }

    // Clean up and return on failure
    if (!success) {
        cleanup_database_config(&config->databases);
        return false;
    }

    return success;
}

// Clean up a single database connection
void cleanup_database_connection(DatabaseConnection* conn) {
    if (!conn) return;

    free(conn->name);
    free(conn->connection_name);
    free(conn->type);
    free(conn->database);
    free(conn->host);
    free(conn->port);
    free(conn->user);
    free(conn->pass);
    free(conn->bootstrap_query);
    free(conn->schema);
    free(conn->migrations);

    // Zero out the structure
    memset(conn, 0, sizeof(DatabaseConnection));
}

// Clean up database configuration
void cleanup_database_config(DatabaseConfig* config) {
    if (!config) return;

    // Clean up each connection
    for (int i = 0; i < config->connection_count; i++) {
        cleanup_database_connection(&config->connections[i]);
    }

    // Zero out the structure
    memset(config, 0, sizeof(DatabaseConfig));
}

void dump_database_config(const DatabaseConfig* config) {
    if (!config) {
        log_this(SR_CONFIG_CURRENT, "Cannot dump NULL database config", LOG_LEVEL_ERROR, 0);
        return;
    }

    // Dump global settings
    log_this(SR_CONFIG_CURRENT, "――― ConnectionCount: %d", LOG_LEVEL_STATE, 1, config->connection_count);

    // Dump connections header
    log_this(SR_CONFIG_CURRENT, "――― Connections", LOG_LEVEL_STATE, 0);

    // Count databases by type for summary
    int postgres_count = 0, mysql_count = 0, sqlite_count = 0, db2_count = 0;
    char postgres_name[64] = "", mysql_name[64] = "", sqlite_name[64] = "", db2_name[64] = "";

    for (int i = 0; i < config->connection_count; i++) {
        const DatabaseConnection* conn = &config->connections[i];
        if (conn->enabled && conn->type) {
            if (strcmp(conn->type, "postgresql") == 0 || strcmp(conn->type, "postgres") == 0) {
                postgres_count++;
                if (postgres_count == 1 && conn->connection_name) {
                    strcpy(postgres_name, conn->connection_name);
                }
            } else if (strcmp(conn->type, "mysql") == 0) {
                mysql_count++;
                if (mysql_count == 1 && conn->connection_name) {
                    strcpy(mysql_name, conn->connection_name);
                }
            } else if (strcmp(conn->type, "sqlite") == 0) {
                sqlite_count++;
                if (sqlite_count == 1 && conn->connection_name) {
                    strcpy(sqlite_name, conn->connection_name);
                }
            } else if (strcmp(conn->type, "db2") == 0) {
                db2_count++;
                if (db2_count == 1 && conn->connection_name) {
                    strcpy(db2_name, conn->connection_name);
                }
            }
        }
    }

    // Dump database counts with names
    if (postgres_count > 0) {
        if (strlen(postgres_name) > 0) {
            log_this(SR_CONFIG_CURRENT, "――― PostgreSQL Databases: %d", LOG_LEVEL_STATE, 1, postgres_count);
        } else {
            log_this(SR_CONFIG_CURRENT, "――― PostgreSQL Databases: %d", LOG_LEVEL_STATE, 1, postgres_count);
        }
    } else {
        log_this(SR_CONFIG_CURRENT, "――― PostgreSQL Databases: 0", LOG_LEVEL_STATE, 0);
    }

    if (mysql_count > 0) {
        log_this(SR_CONFIG_CURRENT, "――― MySQL Databases: %d", LOG_LEVEL_STATE, 1, mysql_count);
    } else {
        log_this(SR_CONFIG_CURRENT, "――― MySQL Databases: 0", LOG_LEVEL_STATE, 0);
    }

    if (sqlite_count > 0) {
        log_this(SR_CONFIG_CURRENT, "――― SQLite Databases: %d", LOG_LEVEL_STATE, 1, sqlite_count);
    } else {
        log_this(SR_CONFIG_CURRENT, "――― SQLite Databases: 0", LOG_LEVEL_STATE, 0);
    }

    if (db2_count > 0) {
        log_this(SR_CONFIG_CURRENT, "――― DB2 Databases: %d", LOG_LEVEL_STATE, 1, db2_count);
    } else {
        log_this(SR_CONFIG_CURRENT, "――― DB2 Databases: 0", LOG_LEVEL_STATE, 0);
    }

    int total_databases = postgres_count + mysql_count + sqlite_count + db2_count;
    log_this(SR_CONFIG_CURRENT, "――― Total databases configured: %d", LOG_LEVEL_STATE, 1, total_databases);

    // Dump each connection details
    for (int i = 0; i < config->connection_count; i++) {
        const DatabaseConnection* conn = &config->connections[i];

        // Create section header for each database
        log_this(SR_CONFIG_CURRENT, "――― %s", LOG_LEVEL_STATE, 1, conn->connection_name ? conn->connection_name : "Unknown");

        // Dump connection details
        log_this(SR_CONFIG_CURRENT, "――― Enabled: %s", LOG_LEVEL_STATE, 1, conn->enabled ? "true" : "false");
        if (conn->enabled) {
            log_this(SR_CONFIG_CURRENT, "――― Type: %s", LOG_LEVEL_STATE, 1, conn->type ? conn->type : "(not set)");
            log_this(SR_CONFIG_CURRENT, "――― Database: %s", LOG_LEVEL_STATE, 1, conn->database ? conn->database : "(not set)");
            log_this(SR_CONFIG_CURRENT, "――― Host: %s", LOG_LEVEL_STATE, 1, conn->host ? conn->host : "(not set)");
            log_this(SR_CONFIG_CURRENT, "――― Port: %s", LOG_LEVEL_STATE, 1, conn->port ? conn->port : "(not set)");
            log_this(SR_CONFIG_CURRENT, "――― User: %s", LOG_LEVEL_STATE, 1, conn->user ? conn->user : "(not set)");
            log_this(SR_CONFIG_CURRENT, "――― Pass: %s", LOG_LEVEL_STATE, 1, conn->pass ? "****" : "(not set)");
            log_this(SR_CONFIG_CURRENT, "――― Bootstrap: %s", LOG_LEVEL_STATE,1, conn->bootstrap_query ? conn->bootstrap_query : "(not set)");
            log_this(SR_CONFIG_CURRENT, "――― AutoMigration: %s", LOG_LEVEL_STATE, 1, conn->auto_migration ? "true" : "false");
            log_this(SR_CONFIG_CURRENT, "――― TestMigration: %s", LOG_LEVEL_STATE, 1, conn->test_migration ? "true" : "false");
            log_this(SR_CONFIG_CURRENT, "――― Migrations: %s", LOG_LEVEL_STATE, 1, conn->migrations ? conn->migrations : "(not set)");

            // Dump queue configurations
            log_this(SR_CONFIG_CURRENT, "――― Queues", LOG_LEVEL_STATE, 0);

            // Slow queue
            log_this(SR_CONFIG_CURRENT, "―――   Slow: start=%d, min=%d, max=%d, up=%d, down=%d, inactivity=%d", LOG_LEVEL_STATE, 6,
                     conn->queues.slow.start, 
                     conn->queues.slow.min,
                     conn->queues.slow.max, 
                     conn->queues.slow.up, 
                     conn->queues.slow.down,
                     conn->queues.slow.inactivity);

            // Medium queue
            log_this(SR_CONFIG_CURRENT, "―――   Medium: start=%d, min=%d, max=%d, up=%d, down=%d, inactivity=%d", LOG_LEVEL_STATE, 6,
                     conn->queues.medium.start, 
                     conn->queues.medium.min,
                     conn->queues.medium.max, 
                     conn->queues.medium.up, 
                     conn->queues.medium.down,
                     conn->queues.medium.inactivity);

            // Fast queue
            log_this(SR_CONFIG_CURRENT, "―――   Fast: start=%d, min=%d, max=%d, up=%d, down=%d, inactivity=%d", LOG_LEVEL_STATE, 6,
                     conn->queues.fast.start, 
                     conn->queues.fast.min,
                     conn->queues.fast.max, 
                     conn->queues.fast.up, 
                     conn->queues.fast.down,
                     conn->queues.fast.inactivity);

            // Cache queue
            log_this(SR_CONFIG_CURRENT, "―――   Cache: start=%d, min=%d, max=%d, up=%d, down=%d, inactivity=%d", LOG_LEVEL_STATE, 6,
                     conn->queues.cache.start, 
                     conn->queues.cache.min,
                     conn->queues.cache.max, 
                     conn->queues.cache.up, 
                     conn->queues.cache.down,
                     conn->queues.cache.inactivity);
        }
    }
}
