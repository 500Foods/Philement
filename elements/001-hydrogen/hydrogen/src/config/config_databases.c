/*
 * Database Configuration Implementation
 *
 * Implements the configuration handlers for database operations,
 * including JSON parsing, environment variable handling, and validation.
 */

#include <stdlib.h>
#include <string.h>
#include "config_databases.h"
#include "config.h"
#include "config_utils.h"
#include "../logging/logging.h"

// Known database names in fixed order
const char* const KNOWN_DATABASES[MAX_DATABASES] = {
    "Acuranzo",
    "OIDC",
    "Log",
    "Canvas",
    "Helium"
};

// Forward declarations for internal functions
static void init_database_connection(DatabaseConnection* conn, const char* name, bool enabled);
static void cleanup_database_connection(DatabaseConnection* conn);
static int validate_database_connection(const DatabaseConnection* conn);

// Public function declarations
int config_database_validate(const DatabaseConfig* config);

// Load database configuration from JSON
bool load_database_config(json_t* root, AppConfig* config) {
    // Initialize with defaults
    init_database_config(&config->databases);

    // Process all config items in sequence
    bool success = true;

    // Process Databases section if present
    success = PROCESS_SECTION(root, "Databases");
    if (success) {
        // Process DefaultWorkers
        success = success && PROCESS_INT(root, &config->databases, default_workers, "Databases.DefaultWorkers", "Databases");

        // Process Connections section
        json_t* connections = json_object_get(json_object_get(root, "Databases"), "Connections");
        if (json_is_object(connections)) {
            log_config_item("Connections", "Configured", false);

            // Process each known database
            for (int i = 0; i < MAX_DATABASES; i++) {
                json_t* conn_obj = json_object_get(connections, KNOWN_DATABASES[i]);
                DatabaseConnection* conn = &config->databases.connections[i];

                if (json_is_object(conn_obj)) {
                    log_config_item(KNOWN_DATABASES[i], "", false);

                    // Process connection settings
                    success = success && PROCESS_BOOL(conn_obj, conn, enabled, "Enabled", KNOWN_DATABASES[i]);
                    
                    if (conn->enabled) {
                        success = success && PROCESS_STRING(conn_obj, conn, type, "Type", KNOWN_DATABASES[i]);
                        success = success && PROCESS_INT(conn_obj, conn, workers, "Workers", KNOWN_DATABASES[i]);

                        // Special handling for Acuranzo database
                        if (strcmp(KNOWN_DATABASES[i], "Acuranzo") == 0) {
                            success = success && PROCESS_STRING(conn_obj, conn, database, "Database", KNOWN_DATABASES[i]);
                            if (success && !conn->database) conn->database = strdup("${env.ACURANZO_DATABASE}");

                            success = success && PROCESS_STRING(conn_obj, conn, host, "Host", KNOWN_DATABASES[i]);
                            if (success && !conn->host) conn->host = strdup("${env.ACURANZO_DB_HOST}");

                            success = success && PROCESS_STRING(conn_obj, conn, port, "Port", KNOWN_DATABASES[i]);
                            if (success && !conn->port) conn->port = strdup("${env.ACURANZO_DB_PORT}");

                            success = success && PROCESS_SENSITIVE(conn_obj, conn, user, "User", KNOWN_DATABASES[i]);
                            if (success && !conn->user) conn->user = strdup("${env.ACURANZO_DB_USER}");

                            success = success && PROCESS_SENSITIVE(conn_obj, conn, pass, "Pass", KNOWN_DATABASES[i]);
                            if (success && !conn->pass) conn->pass = strdup("${env.ACURANZO_DB_PASS}");
                        } else {
                            success = success && PROCESS_STRING(conn_obj, conn, database, "Database", KNOWN_DATABASES[i]);
                            success = success && PROCESS_STRING(conn_obj, conn, host, "Host", KNOWN_DATABASES[i]);
                            success = success && PROCESS_STRING(conn_obj, conn, port, "Port", KNOWN_DATABASES[i]);
                            success = success && PROCESS_SENSITIVE(conn_obj, conn, user, "User", KNOWN_DATABASES[i]);
                            success = success && PROCESS_SENSITIVE(conn_obj, conn, pass, "Pass", KNOWN_DATABASES[i]);
                        }
                    }
                } else {
                    log_config_item(KNOWN_DATABASES[i], "Using defaults", true);
                }
            }
        } else {
            log_config_item("Connections", "Using defaults", true);
        }
    }

    // Validate the configuration if loaded successfully
    if (success) {
        success = (config_database_validate(&config->databases) == 0);
    }

    // Clean up on failure
    if (!success) {
        cleanup_database_config(&config->databases);
        init_database_config(&config->databases);
    }

    return success;
}

// Initialize a single database connection with defaults
static void init_database_connection(DatabaseConnection* conn, const char* name, bool enabled) {
    if (!conn || !name) return;

    conn->name = strdup(name);
    conn->enabled = enabled;
    conn->type = strdup("postgres");  // Default type
    conn->database = NULL;
    conn->host = NULL;
    conn->port = NULL;
    conn->user = NULL;
    conn->pass = NULL;
    conn->workers = 1;  // Default workers
}

// Clean up a single database connection
static void cleanup_database_connection(DatabaseConnection* conn) {
    if (!conn) return;

    free(conn->name);
    free(conn->type);
    free(conn->database);
    free(conn->host);
    free(conn->port);
    free(conn->user);
    free(conn->pass);

    // Zero out the structure
    memset(conn, 0, sizeof(DatabaseConnection));
}

// Initialize database configuration with defaults
void init_database_config(DatabaseConfig* config) {
    if (!config) return;

    config->default_workers = 1;  // Default worker count
    config->connection_count = MAX_DATABASES;

    // Initialize each known database
    for (int i = 0; i < MAX_DATABASES; i++) {
        // Acuranzo is enabled by default, others disabled
        bool enabled = (strcmp(KNOWN_DATABASES[i], "Acuranzo") == 0);
        init_database_connection(&config->connections[i], KNOWN_DATABASES[i], enabled);

        // Set Acuranzo environment variable defaults
        if (enabled) {
            DatabaseConnection* acuranzo = &config->connections[i];
            free(acuranzo->type);  // Free default type
            acuranzo->type = strdup("${env.ACURANZO_DB_TYPE}");
            acuranzo->database = strdup("${env.ACURANZO_DATABASE}");
            acuranzo->host = strdup("${env.ACURANZO_DB_HOST}");
            acuranzo->port = strdup("${env.ACURANZO_DB_PORT}");
            acuranzo->user = strdup("${env.ACURANZO_DB_USER}");
            acuranzo->pass = strdup("${env.ACURANZO_DB_PASS}");
        }
    }
}

// Clean up database configuration
void cleanup_database_config(DatabaseConfig* config) {
    if (!config) return;

    for (int i = 0; i < config->connection_count; i++) {
        cleanup_database_connection(&config->connections[i]);
    }

    // Zero out the structure
    memset(config, 0, sizeof(DatabaseConfig));
}

// Validate a single database connection
static int validate_database_connection(const DatabaseConnection* conn) {
    if (!conn) {
        log_this("Config", "Database connection pointer is NULL", LOG_LEVEL_ERROR);
        return -1;
    }

    if (!conn->name || strlen(conn->name) < 1 || strlen(conn->name) > 64) {
        log_this("Config", "Invalid database name", LOG_LEVEL_ERROR);
        return -1;
    }

    if (conn->enabled) {
        // Required fields for enabled databases
        if (!conn->type || strlen(conn->type) < 1 || strlen(conn->type) > 32) {
            log_this("Config", "Invalid database type for %s", LOG_LEVEL_ERROR, conn->name);
            return -1;
        }

        if (!conn->database || !conn->host || !conn->port || !conn->user || !conn->pass) {
            log_this("Config", "Missing required fields for enabled database %s", LOG_LEVEL_ERROR, conn->name);
            return -1;
        }

        if (conn->workers < 1 || conn->workers > 32) {
            log_this("Config", "Invalid worker count for %s", LOG_LEVEL_ERROR, conn->name);
            return -1;
        }
    }

    return 0;
}

// Validate database configuration values
int config_database_validate(const DatabaseConfig* config) {
    if (!config) {
        log_this("Config", "Database config pointer is NULL", LOG_LEVEL_ERROR);
        return -1;
    }

    // Validate default workers
    if (config->default_workers < 1 || config->default_workers > 32) {
        log_this("Config", "Invalid default worker count", LOG_LEVEL_ERROR);
        return -1;
    }

    // Validate connection count
    if (config->connection_count != MAX_DATABASES) {
        log_this("Config", "Invalid connection count", LOG_LEVEL_ERROR);
        return -1;
    }

    // Validate each connection
    for (int i = 0; i < config->connection_count; i++) {
        if (validate_database_connection(&config->connections[i]) != 0) {
            return -1;
        }
    }

    return 0;
}
