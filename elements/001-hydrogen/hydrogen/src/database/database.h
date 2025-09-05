/*
 * Hydrogen Database Subsystem Main Header
 *
 * This header provides the main interface for the Hydrogen database subsystem.
 * It implements the architecture described in DATABASE_PLAN.md.
 *
 * Phase 1 (Current): Queue-based processing with multi-queue system
 * Phase 2 (Next): Multi-engine interface layer (PostgreSQL, SQLite, MySQL, DB2, AI)
 * Phase 3: Query caching and bootstrap system
 * Phase 4: Acuranzo integration and testing
 */

#ifndef DATABASE_H
#define DATABASE_H

// Standard includes
#include <stdbool.h>
#include <time.h>

// Project includes
#include "../hydrogen.h"
#include "database_queue.h"

// Subsystem name for logging
#define SR_DATABASE "DATABASE"

// Note: land_database_subsystem() is already declared in landing.h

// Database connection status
typedef enum {
    DB_CONNECTION_DISCONNECTED = 0,
    DB_CONNECTION_CONNECTING,
    DB_CONNECTION_CONNECTED,
    DB_CONNECTION_ERROR,
    DB_CONNECTION_SHUTTING_DOWN
} DatabaseConnectionStatus;

// Database engine types (for Phase 2 expansion)
typedef enum {
    DB_ENGINE_POSTGRESQL = 0,
    DB_ENGINE_SQLITE,
    DB_ENGINE_MYSQL,
    DB_ENGINE_DB2,
    DB_ENGINE_AI,         // For future AI/ML query processing
    DB_ENGINE_MAX
} DatabaseEngine;

// Query result status
typedef enum {
    DB_QUERY_SUCCESS = 0,
    DB_QUERY_ERROR,
    DB_QUERY_TIMEOUT,
    DB_QUERY_CANCELLED,
    DB_QUERY_CONNECTION_LOST
} DatabaseQueryStatus;

// Forward declarations
typedef struct DatabaseConnection DatabaseConnection;
typedef struct DatabaseEngineInterface DatabaseEngineInterface;

// Main database subsystem structure
typedef struct {
    // Global state
    bool initialized;
    bool shutdown_requested;

    // Connection management
    DatabaseQueueManager* queue_manager;

    // Engine registry (for Phase 2)
    DatabaseEngineInterface* engines[DB_ENGINE_MAX];

    // Global statistics
    time_t start_time;
    volatile long long total_queries_processed;
    volatile long long successful_queries;
    volatile long long failed_queries;
    volatile long long timeout_queries;

    // Configuration
    int max_connections_per_database;
    int default_worker_threads;
    int query_timeout_seconds;

} DatabaseSubsystem;

// Global database subsystem instance
extern DatabaseSubsystem* database_subsystem;

/*
 * Database Subsystem Core API
 */

// Initialize the database subsystem
bool database_subsystem_init(void);

// Shut down the database subsystem
void database_subsystem_shutdown(void);

// Add a database configuration
bool database_add_database(const char* name, const char* engine, const char* connection_string);

// Remove a database
bool database_remove_database(const char* name);

// Get database statistics
void database_get_stats(char* buffer, size_t buffer_size);

// Health check for entire subsystem
bool database_health_check(void);

/*
 * Query Processing API (Phase 2 integration points)
 */

// Submit a query to the database subsystem
bool database_submit_query(const char* database_name,
                          const char* query_id,
                          const char* query_template,
                          const char* parameters_json,
                          int queue_type_hint);

// Check query result status
DatabaseQueryStatus database_query_status(const char* query_id);

// Get query result
bool database_get_result(const char* query_id, char* result_buffer, size_t buffer_size);

// Cancel a running query
bool database_cancel_query(const char* query_id);

/*
 * Configuration and Maintenance API
 */

// Reload database configurations
bool database_reload_config(void);

// Test database connectivity
bool database_test_connection(const char* database_name);

// Get supported database engines
void database_get_supported_engines(char* buffer, size_t buffer_size);

/*
 * Integration points for other subsystems
 */

// For API subsystem integration
bool database_process_api_query(const char* database, const char* query_path,
                               const char* parameters, char* result_buffer, size_t buffer_size);

// For launch/landing subsystem integration
int launch_database_subsystem(void);     // Already in launch_database.c
// Note: land_database_subsystem() already declared in landing.h

/*
 * Utility Functions
 */

// Validate query template
bool database_validate_query(const char* query_template);

// Escape query parameters
char* database_escape_parameter(const char* parameter);

// Get query processing time
time_t database_get_query_age(const char* query_id);

// Cleanup old query results
void database_cleanup_old_results(time_t max_age_seconds);

#endif // DATABASE_H
