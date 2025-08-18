/*
 * Database Configuration Implementation
 *
 * Implements the configuration handlers for database operations,
 * including JSON parsing and environment variable handling.
 * Note: Validation moved to launch readiness checks
 */

#include <stdlib.h>
#include <string.h>
#include "config_databases.h"
#include "config.h"
#include "config_utils.h"
#include "../logging/logging.h"

// Function declarations
void cleanup_database_connection(DatabaseConnection* conn);
void dump_database_config(const DatabaseConfig* config);

// Load database configuration from JSON
bool load_database_config(json_t* root, AppConfig* config) {
    bool success = true;
    DatabaseConfig* db_config = &config->databases;

    // Initialize database config with defaults
    memset(db_config, 0, sizeof(DatabaseConfig));
    db_config->default_workers = 1;

    // Count actual databases in config
    json_t* connections = json_object_get(json_object_get(root, "Databases"), "Connections");
    size_t db_count = json_is_object(connections) ? json_object_size(connections) : 1;  // At least Acuranzo
    db_config->connection_count = (int)db_count;

    // Initialize all database connections with minimal defaults
    for (size_t i = 0; i < db_count; i++) {
        DatabaseConnection* conn = &db_config->connections[i];
        memset(conn, 0, sizeof(DatabaseConnection));
        conn->enabled = (i == 0);  // Only first one enabled by default
        conn->workers = 1;
    }

    // Process Databases section and DefaultWorkers
    success = PROCESS_SECTION(root, "Databases");
    success = success && PROCESS_INT(root, db_config, default_workers, "Databases.DefaultWorkers", "Databases");

    // Process Connections section
    success = success && PROCESS_SECTION(root, "Databases.Connections");

    // Set up Acuranzo defaults (first database)
    DatabaseConnection* acuranzo = &db_config->connections[0];
    acuranzo->name = strdup("Acuranzo");
    acuranzo->connection_name = strdup("Acuranzo");

    // Process Acuranzo section and settings
    success = success && PROCESS_SECTION(root, "Databases.Connections.Acuranzo");
    acuranzo->database = strdup("${env.ACURANZO_DATABASE}");
    acuranzo->type = strdup("${env.ACURANZO_DB_TYPE}");
    acuranzo->host = strdup("${env.ACURANZO_DB_HOST}");
    acuranzo->port = strdup("${env.ACURANZO_DB_PORT}");
    acuranzo->user = strdup("${env.ACURANZO_DB_USER}");
    acuranzo->pass = strdup("${env.ACURANZO_DB_PASS}");
    acuranzo->workers = db_config->default_workers;
    success = success && PROCESS_BOOL(root, acuranzo, enabled, "Databases.Connections.Acuranzo.Enabled", "Databases");
    success = success && PROCESS_STRING(root, acuranzo, type, "Databases.Connections.Acuranzo.Type", "Databases");
    success = success && PROCESS_STRING(root, acuranzo, database, "Databases.Connections.Acuranzo.Database", "Databases");
    success = success && PROCESS_STRING(root, acuranzo, host, "Databases.Connections.Acuranzo.Host", "Databases");
    success = success && PROCESS_STRING(root, acuranzo, port, "Databases.Connections.Acuranzo.Port", "Databases");
    success = success && PROCESS_STRING(root, acuranzo, user, "Databases.Connections.Acuranzo.User", "Databases");
    success = success && PROCESS_SENSITIVE(root, acuranzo, pass, "Databases.Connections.Acuranzo.Pass", "Databases");
    success = success && PROCESS_INT(root, acuranzo, workers, "Databases.Connections.Acuranzo.Workers", "Databases");

    // Process additional databases
    if (json_is_object(connections)) {  // Use existing connections variable
        const char* key;
        json_t* value;
        size_t i = 1;  // Start after Acuranzo
        json_object_foreach(connections, key, value) {
            if (i >= db_count) break;  // Safety check
            if (strcmp(key, "Acuranzo") == 0) continue;  // Skip Acuranzo as it's handled separately

            DatabaseConnection* conn = &db_config->connections[i];
            char path[256];

            // Add section header for this database
            snprintf(path, sizeof(path), "Databases.Connections.%s", key);
            success = success && PROCESS_SECTION(root, path);

            // Store the actual connection name from JSON
            free(conn->name);  // Update both name and connection_name
            free(conn->connection_name);
            conn->name = strdup(key);
            conn->connection_name = strdup(key);

            snprintf(path, sizeof(path), "Databases.Connections.%s.Enabled", key);
            success = success && PROCESS_BOOL(root, conn, enabled, path, "Databases");

            if (success && conn->enabled) {
                snprintf(path, sizeof(path), "Databases.Connections.%s.Type", key);
                success = success && PROCESS_STRING(root, conn, type, path, "Databases");

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
            }
            i++;  // Move to next database
        }
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
        log_this("Config", "Cannot dump NULL database config", LOG_LEVEL_TRACE);
        return;
    }

    // Dump global settings
    DUMP_INT("―― DefaultWorkers", config->default_workers);
    DUMP_INT("―― Connections", config->connection_count);

    // Dump each connection
    for (int i = 0; i < config->connection_count; i++) {
        const DatabaseConnection* conn = &config->connections[i];
        
        // Create section header for each database
        DUMP_TEXT("――", conn->connection_name);
        
        // Dump connection details
        DUMP_BOOL("―――― Enabled", conn->enabled);
        if (conn->enabled) {
            DUMP_STRING("―――― Type", conn->type);
            DUMP_STRING("―――― Database", conn->database);
            DUMP_STRING("―――― Host", conn->host);
            DUMP_STRING("―――― Port", conn->port);
            DUMP_STRING("―――― User", conn->user);
            DUMP_SECRET("―――― Pass", conn->pass);
            DUMP_INT("―――― Workers", conn->workers);
        }
    }
}
