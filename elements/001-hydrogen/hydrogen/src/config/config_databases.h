/*
 * Database configuration structures and defaults
 */

#ifndef CONFIG_DATABASES_H
#define CONFIG_DATABASES_H

#include <stdbool.h>

// Maximum number of supported databases
#define MAX_DATABASES 5

// Known database names
extern const char* const KNOWN_DATABASES[MAX_DATABASES];

// Default values
#define DEFAULT_DB_WORKERS 1
#define DEFAULT_DB_TYPE "postgres"

// Structure for individual database connection
typedef struct DatabaseConnection {
    char* name;             // Database name (e.g., "Acuranzo", "OIDC")
    bool enabled;           // Whether this database is enabled
    char* type;            // Database type (e.g., "postgres")
    char* database;        // Database name
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
    DatabaseConnection connections[MAX_DATABASES];  // Array of database connections
} DatabaseConfig;

// Initialize database configuration with defaults
void init_database_config(DatabaseConfig* config);

// Clean up database configuration
void cleanup_database_config(DatabaseConfig* config);

#endif /* CONFIG_DATABASES_H */
