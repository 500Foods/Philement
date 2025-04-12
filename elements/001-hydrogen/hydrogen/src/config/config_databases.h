/*
 * Database configuration structures and defaults
 */

#ifndef CONFIG_DATABASES_H
#define CONFIG_DATABASES_H

#include <stdbool.h>

// Structure for individual database connection
typedef struct DatabaseConnection {
    char* name;             // Database name from KNOWN_DATABASES
    char* connection_name;  // Connection name from JSON (e.g., "Acuranzo", "OIDC")
    bool enabled;           // Whether this database is enabled
    char* type;            // Database type (e.g., "postgres")
    char* database;        // Database name to connect to
    char* host;            // Database host
    char* port;            // Database port
    char* user;            // Database user
    char* pass;            // Database password
    int workers;           // Number of worker threads
} DatabaseConnection;

// Structure for overall database configuration
typedef struct DatabaseConfig {
    int default_workers;                      // Default number of worker threads
    int connection_count;                     // Number of configured connections
    DatabaseConnection connections[5];        // Array of database connections (max 5)
} DatabaseConfig;

// Initialize database configuration with defaults
void init_database_config(DatabaseConfig* config);

// Clean up a single database connection
void cleanup_database_connection(DatabaseConnection* conn);

// Clean up database configuration
void cleanup_database_config(DatabaseConfig* config);

void dump_database_config(const DatabaseConfig* config);

#endif /* CONFIG_DATABASES_H */
