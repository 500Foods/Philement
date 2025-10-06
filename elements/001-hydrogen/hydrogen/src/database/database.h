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

// Project includes
#include "../hydrogen.h"
#include "database_types.h"
#include "database_queue.h"

// Subsystem name for logging (defined in globals.h)

// Note: land_database_subsystem() is already declared in landing.h

// Forward declarations
typedef struct ConnectionConfig ConnectionConfig;
typedef struct QueryRequest QueryRequest;
typedef struct QueryResult QueryResult;
typedef struct PreparedStatement PreparedStatement;
typedef struct Transaction Transaction;
typedef struct DatabaseHandle DatabaseHandle;
typedef struct DatabaseEngineInterface DatabaseEngineInterface;

// Database connection status
typedef enum {
    DB_CONNECTION_DISCONNECTED = 0,
    DB_CONNECTION_CONNECTING,
    DB_CONNECTION_CONNECTED,
    DB_CONNECTION_ERROR,
    DB_CONNECTION_SHUTTING_DOWN
} DatabaseConnectionStatus;

// Query result status
typedef enum {
    DB_QUERY_SUCCESS = 0,
    DB_QUERY_ERROR,
    DB_QUERY_TIMEOUT,
    DB_QUERY_CANCELLED,
    DB_QUERY_CONNECTION_LOST
} DatabaseQueryStatus;

// Transaction isolation levels
typedef enum {
    DB_ISOLATION_READ_UNCOMMITTED = 0,
    DB_ISOLATION_READ_COMMITTED,
    DB_ISOLATION_REPEATABLE_READ,
    DB_ISOLATION_SERIALIZABLE
} DatabaseIsolationLevel;

// Connection configuration
struct ConnectionConfig {
    char* host;
    int port;
    char* database;
    char* username;
    char* password;
    char* connection_string;  // Alternative to individual fields
    int timeout_seconds;
    bool ssl_enabled;
    char* ssl_cert_path;
    char* ssl_key_path;
    char* ssl_ca_path;
};

// Query request structure
struct QueryRequest {
    char* query_id;
    char* sql_template;
    char* parameters_json;
    int timeout_seconds;
    DatabaseIsolationLevel isolation_level;
    bool use_prepared_statement;
    char* prepared_statement_name;
};

// Query result structure
struct QueryResult {
    bool success;
    char* data_json;          // JSON result data
    size_t row_count;
    size_t column_count;
    char** column_names;
    char* error_message;
    time_t execution_time_ms;
    int affected_rows;
};

// Prepared statement structure
struct PreparedStatement {
    char* name;
    char* sql_template;
    void* engine_specific_handle;  // Engine-specific prepared statement handle
    time_t created_at;
    volatile int usage_count;
};

// Transaction structure
struct Transaction {
    char* transaction_id;
    DatabaseIsolationLevel isolation_level;
    time_t started_at;
    bool active;
    void* engine_specific_handle;  // Engine-specific transaction handle
};

// Database connection handle structure
struct DatabaseHandle {
    DatabaseEngine engine_type;
    void* connection_handle;              // Engine-specific handle (PGconn, sqlite3, etc.)
    ConnectionConfig* config;
    char* designator;                     // DQM designator for logging (e.g., "DQM-ACZ-00-SMFC")
    DatabaseConnectionStatus status;
    time_t connected_since;
    Transaction* current_transaction;
    PreparedStatement** prepared_statements;  // Array of prepared statements
    size_t prepared_statement_count;
    pthread_mutex_t connection_lock;      // Thread safety for connection operations
    volatile bool in_use;                 // Connection pool management
    time_t last_health_check;
    int consecutive_failures;
};

// Database engine interface structure
struct DatabaseEngineInterface {
    DatabaseEngine engine_type;
    char* name;                           // Engine identifier ("postgresql", "sqlite", etc.)

    // Core connection management
    bool (*connect)(ConnectionConfig* config, DatabaseHandle** connection, const char* designator);
    bool (*disconnect)(DatabaseHandle* connection);
    bool (*health_check)(DatabaseHandle* connection);
    bool (*reset_connection)(DatabaseHandle* connection);

    // Query execution
    bool (*execute_query)(DatabaseHandle* connection, QueryRequest* request, QueryResult** result);
    bool (*execute_prepared)(DatabaseHandle* connection, const PreparedStatement* stmt, QueryRequest* request, QueryResult** result);

    // Transaction management
    bool (*begin_transaction)(DatabaseHandle* connection, DatabaseIsolationLevel level, Transaction** transaction);
    bool (*commit_transaction)(DatabaseHandle* connection, Transaction* transaction);
    bool (*rollback_transaction)(DatabaseHandle* connection, Transaction* transaction);

    // Prepared statement management
    bool (*prepare_statement)(DatabaseHandle* connection, const char* name, const char* sql, PreparedStatement** stmt);
    bool (*unprepare_statement)(DatabaseHandle* connection, PreparedStatement* stmt);

    // Engine-specific utilities
    char* (*get_connection_string)(const ConnectionConfig* config);
    bool (*validate_connection_string)(const char* connection_string);
    char* (*escape_string)(const DatabaseHandle* connection, const char* input);
};



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
bool database_get_result(const char* query_id, const char* result_buffer, size_t buffer_size);

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
                               const char* parameters, const char* result_buffer, size_t buffer_size);

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

/*
 * Database Engine Interface Management (Phase 2)
 */

// Initialize database engine registry
bool database_engine_init(void);

// Register a database engine
bool database_engine_register(DatabaseEngineInterface* engine);

// Get engine interface by type
DatabaseEngineInterface* database_engine_get(DatabaseEngine engine_type);
DatabaseEngineInterface* database_engine_get_with_designator(DatabaseEngine engine_type, const char* designator);

// Get engine interface by name
DatabaseEngineInterface* database_engine_get_by_name(const char* name);

// Create database connection using engine
bool database_engine_connect(DatabaseEngine engine_type, ConnectionConfig* config, DatabaseHandle** connection);
bool database_engine_connect_with_designator(DatabaseEngine engine_type, ConnectionConfig* config, DatabaseHandle** connection, const char* designator);

// Execute query using engine abstraction
bool database_engine_execute(DatabaseHandle* connection, QueryRequest* request, QueryResult** result);

// Health check using engine abstraction
bool database_engine_health_check(DatabaseHandle* connection);

// Transaction management using engine abstraction
bool database_engine_begin_transaction(DatabaseHandle* connection, DatabaseIsolationLevel level, Transaction** transaction);
bool database_engine_commit_transaction(DatabaseHandle* connection, Transaction* transaction);
bool database_engine_rollback_transaction(DatabaseHandle* connection, Transaction* transaction);

// Connection string utilities
char* database_engine_build_connection_string(DatabaseEngine engine_type, ConnectionConfig* config);
bool database_engine_validate_connection_string(DatabaseEngine engine_type, const char* connection_string);

// Cleanup functions
void database_engine_cleanup_connection(DatabaseHandle* connection);
void database_engine_cleanup_result(QueryResult* result);
void database_engine_cleanup_transaction(Transaction* transaction);

#endif // DATABASE_H
