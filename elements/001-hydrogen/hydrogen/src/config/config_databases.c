/*
 * Database Configuration Implementation
 *
 * Implements the configuration handlers for database operations,
 * including JSON parsing and environment variable handling.
 * Note: Validation moved to launch readiness checks
 */

// Global includes 
#include "hydrogen.h"

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
    db_config->default_workers = 2;  // Default worker count only

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

    // Process DefaultWorkers
    json_t* default_workers_obj = json_object_get(databases_obj, "DefaultWorkers");
    if (default_workers_obj && json_is_integer(default_workers_obj)) {
        db_config->default_workers = (int)json_integer_value(default_workers_obj);
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
    success = success && PROCESS_INT(root, db_config, default_workers, "Databases.DefaultWorkers", "Databases");
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
                conn->name = strdup(json_string_value(name_obj));
                conn->connection_name = strdup(json_string_value(name_obj));
            }

            json_t* enabled_obj = json_object_get(conn_obj, "Enabled");
            conn->enabled = (enabled_obj && json_is_boolean(enabled_obj)) ?
                            json_boolean_value(enabled_obj) : false;

            json_t* workers_obj = json_object_get(conn_obj, "Workers");
            conn->workers = (workers_obj && json_is_integer(workers_obj)) ?
                           (int)json_integer_value(workers_obj) : db_config->default_workers;

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
                conn->database = strdup(json_string_value(database_obj));
            }

            // Only extract network fields for non-SQLite databases
            if (conn->type && strcmp(conn->type, "sqlite") != 0) {
                json_t* host_obj = json_object_get(conn_obj, "Host");
                if (host_obj && json_is_string(host_obj)) {
                    conn->host = strdup(json_string_value(host_obj));
                }

                json_t* port_obj = json_object_get(conn_obj, "Port");
                if (port_obj && json_is_string(port_obj)) {
                    conn->port = strdup(json_string_value(port_obj));
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

                // Workers (int)
                snprintf(kpath, sizeof(kpath), "%s.Workers", cpath);
                PROCESS_INT(root, conn, workers, kpath, "Databases");
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
            if (db_index >= 5) break;  // Safety limit

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

                snprintf(path, sizeof(path), "Databases.Connections.%s.Workers", key);
                success = success && PROCESS_INT(root, conn, workers, path, "Databases");

                db_index++;
            }
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
        log_this(SR_CONFIG_CURRENT, "Cannot dump NULL database config", LOG_LEVEL_TRACE);
        return;
    }

    // Dump global settings
    log_this(SR_CONFIG_CURRENT, "――― DefaultWorkers: %d", LOG_LEVEL_STATE, config->default_workers);
    log_this(SR_CONFIG_CURRENT, "――― ConnectionCount: %d", LOG_LEVEL_STATE, config->connection_count);

    // Dump connections header
    log_this(SR_CONFIG_CURRENT, "――― Connections", LOG_LEVEL_STATE);

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
            log_this(SR_CONFIG_CURRENT, "――― PostgreSQL Databases: %d", LOG_LEVEL_STATE, postgres_count);
        } else {
            log_this(SR_CONFIG_CURRENT, "――― PostgreSQL Databases: %d", LOG_LEVEL_STATE, postgres_count);
        }
    } else {
        log_this(SR_CONFIG_CURRENT, "――― PostgreSQL Databases: 0", LOG_LEVEL_STATE);
    }

    if (mysql_count > 0) {
        log_this(SR_CONFIG_CURRENT, "――― MySQL Databases: %d", LOG_LEVEL_STATE, mysql_count);
    } else {
        log_this(SR_CONFIG_CURRENT, "――― MySQL Databases: 0", LOG_LEVEL_STATE);
    }

    if (sqlite_count > 0) {
        log_this(SR_CONFIG_CURRENT, "――― SQLite Databases: %d", LOG_LEVEL_STATE, sqlite_count);
    } else {
        log_this(SR_CONFIG_CURRENT, "――― SQLite Databases: 0", LOG_LEVEL_STATE);
    }

    if (db2_count > 0) {
        log_this(SR_CONFIG_CURRENT, "――― DB2 Databases: %d", LOG_LEVEL_STATE, db2_count);
    } else {
        log_this(SR_CONFIG_CURRENT, "――― DB2 Databases: 0", LOG_LEVEL_STATE);
    }

    int total_databases = postgres_count + mysql_count + sqlite_count + db2_count;
    log_this(SR_CONFIG_CURRENT, "――― Total databases configured: %d", LOG_LEVEL_STATE, total_databases);

    // Dump each connection details
    for (int i = 0; i < config->connection_count; i++) {
        const DatabaseConnection* conn = &config->connections[i];

        // Create section header for each database
        log_this(SR_CONFIG_CURRENT, "――― %s", LOG_LEVEL_STATE,
                 conn->connection_name ? conn->connection_name : "Unknown");

        // Dump connection details
        log_this(SR_CONFIG_CURRENT, "――― Enabled: %s", LOG_LEVEL_STATE,
                 conn->enabled ? "true" : "false");
        if (conn->enabled) {
            log_this(SR_CONFIG_CURRENT, "――― Type: %s", LOG_LEVEL_STATE,
                     conn->type ? conn->type : "(not set)");
            log_this(SR_CONFIG_CURRENT, "――― Database: %s", LOG_LEVEL_STATE,
                     conn->database ? conn->database : "(not set)");
            log_this(SR_CONFIG_CURRENT, "――― Host: %s", LOG_LEVEL_STATE,
                     conn->host ? conn->host : "(not set)");
            log_this(SR_CONFIG_CURRENT, "――― Port: %s", LOG_LEVEL_STATE,
                     conn->port ? conn->port : "(not set)");
            log_this(SR_CONFIG_CURRENT, "――― User: %s", LOG_LEVEL_STATE,
                     conn->user ? conn->user : "(not set)");
            log_this(SR_CONFIG_CURRENT, "――― Pass: %s", LOG_LEVEL_STATE,
                     conn->pass ? "****" : "(not set)");
            log_this(SR_CONFIG_CURRENT, "――― Workers: %d", LOG_LEVEL_STATE,
                     conn->workers);
        }
    }
}
