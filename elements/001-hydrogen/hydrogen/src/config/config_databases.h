/*
 * Database configuration structures and defaults
 */

#ifndef CONFIG_DATABASES_H
#define CONFIG_DATABASES_H

#include <stdbool.h>
#include <jansson.h>

// Forward declarations
struct AppConfig;

// Structure for individual queue scaling configuration
typedef struct QueueScalingConfig {
    int start;             // Initial worker count
    int min;               // Minimum workers
    int max;               // Maximum workers
    int up;                // Queue length to trigger scale up
    int down;              // Queue length to trigger scale down
    int inactivity;        // Seconds of inactivity before scale down
} QueueScalingConfig;

// Structure for database queue configuration
typedef struct DatabaseQueues {
    QueueScalingConfig slow;     // Slow queue scaling config
    QueueScalingConfig medium;   // Medium queue scaling config
    QueueScalingConfig fast;     // Fast queue scaling config
    QueueScalingConfig cache;    // Cache queue scaling config
} DatabaseQueues;

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
    char* bootstrap_query; // Bootstrap query to run after connection (loads QTC)
    char* schema;          // Database schema name (e.g., "app")
    bool auto_migration;   // Whether to automatically run migrations
    bool test_migration;   // Whether to run migrations in test mode
    char* migrations;      // Migration source (PAYLOAD:name or path)
    DatabaseQueues queues; // Queue configuration for this connection
} DatabaseConnection;

// Structure for overall database configuration
typedef struct DatabaseConfig {
    DatabaseQueues default_queues;            // Default queue configuration
    int connection_count;                     // Number of configured connections (auto-calculated)
    DatabaseConnection connections[5];        // Array of database connections (max 5)
} DatabaseConfig;

// Initialize database configuration with defaults
void init_database_config(DatabaseConfig* config);

// Clean up a single database connection
void cleanup_database_connection(DatabaseConnection* conn);

// Clean up database configuration
void cleanup_database_config(DatabaseConfig* config);

void dump_database_config(const DatabaseConfig* config);

// Load database configuration from JSON
bool load_database_config(json_t* root, struct AppConfig* config);

#endif /* CONFIG_DATABASES_H */
