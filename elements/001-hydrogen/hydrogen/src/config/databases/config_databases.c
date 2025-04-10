/*
 * Database configuration implementation
 */

#include <stdlib.h>
#include <string.h>
#include "config_databases.h"

// Known database names in fixed order
const char* const KNOWN_DATABASES[MAX_DATABASES] = {
    "Acuranzo",
    "OIDC",
    "Log",
    "Canvas",
    "Helium"
};

// Initialize a single database connection with defaults
static void init_database_connection(DatabaseConnection* conn, const char* name, bool enabled) {
    conn->name = strdup(name);
    conn->enabled = enabled;
    conn->type = strdup(DEFAULT_DB_TYPE);
    conn->database = NULL;
    conn->host = NULL;
    conn->port = NULL;
    conn->user = NULL;
    conn->pass = NULL;
    conn->workers = DEFAULT_DB_WORKERS;
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
}

// Initialize database configuration with defaults
void init_database_config(DatabaseConfig* config) {
    if (!config) return;

    config->default_workers = DEFAULT_DB_WORKERS;
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
}