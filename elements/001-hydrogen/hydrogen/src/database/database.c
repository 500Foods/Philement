/*
 * Database Subsystem Core Implementation
 *
 * Implements the core database subsystem functionality including
 * subsystem initialization, database management, and API functions.
 */

#include "../hydrogen.h"
#include "database.h"
#include "database_queue.h"

// Global database subsystem instance
DatabaseSubsystem* database_subsystem = NULL;
static pthread_mutex_t database_subsystem_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * Database Subsystem Core API
 */

// Initialize the database subsystem
bool database_subsystem_init(void) {
    // log_this(SR_DATABASE, "Starting database subsystem initialization", LOG_LEVEL_STATE, 0);

    MutexResult result = MUTEX_LOCK(&database_subsystem_mutex, SR_DATABASE);
    if (result != MUTEX_SUCCESS) {
        return false;
    }

    if (database_subsystem) {
        mutex_unlock(&database_subsystem_mutex);
        return true; // Already initialized
    }

    database_subsystem = calloc(1, sizeof(DatabaseSubsystem));
    if (!database_subsystem) {
        log_this(SR_DATABASE, "Failed to allocate database subsystem", LOG_LEVEL_ERROR, 0);
        mutex_unlock(&database_subsystem_mutex);
        return false;
    }

    // Initialize subsystem
    database_subsystem->initialized = false;
    database_subsystem->shutdown_requested = false;
    database_subsystem->queue_manager = NULL;
    database_subsystem->start_time = time(NULL);
    database_subsystem->total_queries_processed = 0;
    database_subsystem->successful_queries = 0;
    database_subsystem->failed_queries = 0;
    database_subsystem->timeout_queries = 0;
    database_subsystem->max_connections_per_database = 16;
    database_subsystem->default_worker_threads = 2;
    database_subsystem->query_timeout_seconds = 30;

    // Initialize engine registry
    memset(database_subsystem->engines, 0, sizeof(database_subsystem->engines));

    database_subsystem->initialized = true;
    mutex_unlock(&database_subsystem_mutex);

    // Initialize database thread tracking
    init_service_threads(&database_threads, SR_DATABASE);

    // log_this(SR_DATABASE, "Database subsystem initialization completed successfully", LOG_LEVEL_STATE, 0);
    return true;
}

// Shut down the database subsystem
void database_subsystem_shutdown(void) {
    MutexResult result = MUTEX_LOCK(&database_subsystem_mutex, SR_DATABASE);
    if (result != MUTEX_SUCCESS) {
        return;
    }

    if (!database_subsystem) {
        mutex_unlock(&database_subsystem_mutex);
        return;
    }

    database_subsystem->shutdown_requested = true;

    // TODO: Clean shutdown of queue manager, connections, etc.

    free(database_subsystem);
    database_subsystem = NULL;

    mutex_unlock(&database_subsystem_mutex);

    log_this(SR_DATABASE, "Database subsystem shutdown complete", LOG_LEVEL_STATE, 0);
}

// Add a database configuration
bool database_add_database(const char* name, const char* engine, const char* connection_string __attribute__((unused))) {
    log_this(SR_DATABASE, "Starting database: %s", LOG_LEVEL_STATE, 1, name);

    if (!database_subsystem || !name || !engine) {
        log_this(SR_DATABASE, "Invalid parameters for database", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Validate that we can get the engine interface
    DatabaseEngineInterface* engine_interface = NULL;

    if (strcmp(engine, "postgresql") == 0 || strcmp(engine, "postgres") == 0) {
        engine_interface = database_engine_get(DB_ENGINE_POSTGRESQL);
    } else if (strcmp(engine, "sqlite") == 0) {
        engine_interface = database_engine_get(DB_ENGINE_SQLITE);
    } else if (strcmp(engine, "mysql") == 0) {
        engine_interface = database_engine_get(DB_ENGINE_MYSQL);
    } else if (strcmp(engine, "db2") == 0) {
        engine_interface = database_engine_get(DB_ENGINE_DB2);
    }

    if (!engine_interface) {
        log_this(SR_DATABASE, "Database engine not available", LOG_LEVEL_ERROR, 0);
        log_this(SR_DATABASE, engine, LOG_LEVEL_ERROR, 0);
        return false;
    }

    // log_this(SR_DATABASE, "Database engine interface found", LOG_LEVEL_DEBUG, 0);

    // Get queue configuration from app config
    const DatabaseConfig* db_config = &app_config->databases;
    const DatabaseConnection* conn_config = NULL;

    // Find the connection configuration for this database
    for (int i = 0; i < db_config->connection_count; i++) {
        if (strcmp(db_config->connections[i].name, name) == 0) {
            conn_config = &db_config->connections[i];
            break;
        }
    }

    if (!conn_config) {
        log_this(SR_DATABASE, "Database configuration not found", LOG_LEVEL_ERROR, 0);
        log_this(SR_DATABASE, name, LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Create database queue with connection string
    char* conn_str = NULL;
    if (engine_interface && engine_interface->get_connection_string) {
        // Use the engine's connection string builder with engine-specific defaults
        int default_port = 5432; // PostgreSQL default
        if (strcmp(engine, "mysql") == 0) {
            default_port = 3306;
        } else if (strcmp(engine, "db2") == 0) {
            default_port = 50000; // DB2 default port
        }

        ConnectionConfig temp_config = {
            .host = conn_config->host,
            .port = conn_config->port ? atoi(conn_config->port) : default_port,
            .database = conn_config->database,
            .username = conn_config->user,
            .password = conn_config->pass,
            .connection_string = NULL,  // Not available in DatabaseConnection
            .timeout_seconds = 30
        };
        conn_str = engine_interface->get_connection_string(&temp_config);
    } else {
        // Fallback connection string building
        if (strcmp(engine, "sqlite") == 0) {
            // For SQLite, use the database path directly
            conn_str = strdup(conn_config->database ? conn_config->database : ":memory:");
        } else {
            // For other engines, build connection string from config
            size_t conn_str_len = 256;
            conn_str = malloc(conn_str_len);
            if (conn_str) {
                if (strcmp(engine, "mysql") == 0) {
                    snprintf(conn_str, conn_str_len, "mysql://%s:%s@%s:%s/%s",
                            conn_config->user ? conn_config->user : "",
                            conn_config->pass ? conn_config->pass : "",
                            conn_config->host ? conn_config->host : "localhost",
                            conn_config->port ? conn_config->port : "3306",
                            conn_config->database ? conn_config->database : "");
                } else if (strcmp(engine, "db2") == 0) {
                    // DB2 uses database name as DSN
                    conn_str = strdup(conn_config->database ? conn_config->database : "SAMPLE");
                } else {
                    // Default PostgreSQL-style
                    snprintf(conn_str, conn_str_len, "%s://%s:%s@%s:%s/%s",
                            engine,
                            conn_config->user ? conn_config->user : "",
                            conn_config->pass ? conn_config->pass : "",
                            conn_config->host ? conn_config->host : "localhost",
                            conn_config->port ? conn_config->port : "5432",
                            conn_config->database ? conn_config->database : "test");
                }
            }
        }
    }

    if (!conn_str) {
        log_this(SR_DATABASE, "Failed to create connection string", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Create Lead queue for this database instead of multiple queues
    DatabaseQueue* db_queue = database_queue_create_lead(name, conn_str, conn_config->bootstrap_query);
    free(conn_str);

    if (!db_queue) {
        log_this(SR_DATABASE, "Failed to create Lead database queue", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Start the Lead queue worker thread
    if (!database_queue_start_worker(db_queue)) {
        log_this(SR_DATABASE, "Failed to start Lead queue worker thread", LOG_LEVEL_ERROR, 0);
        database_queue_destroy(db_queue);
        return false;
    }

    // Add to global queue manager - launch responsibility ends here
    if (!global_queue_manager) {
        log_this(SR_DATABASE, "Global queue manager not initialized", LOG_LEVEL_ERROR, 0);
        database_queue_destroy(db_queue);
        return false;
    }

    if (!database_queue_manager_add_database(global_queue_manager, db_queue)) {
        log_this(SR_DATABASE, "Failed to add DQM to queue manager", LOG_LEVEL_ERROR, 0);
        database_queue_destroy(db_queue);
        return false;
    }

    // Store reference to global queue manager in subsystem
    database_subsystem->queue_manager = global_queue_manager;

    // Launch complete - DQM is now independent and handles its own database work
    log_this(SR_DATABASE, "DQM launched successfully for %s", LOG_LEVEL_STATE, 1, name);

    return true;
}

// Remove a database
bool database_remove_database(const char* name) {
    if (!database_subsystem || !name) {
        return false;
    }

    // TODO: Implement database removal logic
    log_this(SR_DATABASE, "Database removal not yet implemented", LOG_LEVEL_DEBUG, 0);
    return false;
}

// Get database statistics
void database_get_stats(char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) {
        return;
    }

    if (!database_subsystem) {
        snprintf(buffer, buffer_size, "Database subsystem not initialized");
        return;
    }

    snprintf(buffer, buffer_size,
             "Database Stats: Total=%lld, Success=%lld, Failed=%lld, Timeout=%lld",
             database_subsystem->total_queries_processed,
             database_subsystem->successful_queries,
             database_subsystem->failed_queries,
             database_subsystem->timeout_queries);
}

// Health check for entire subsystem
bool database_health_check(void) {
    if (!database_subsystem) {
        return false;
    }

    // TODO: Implement comprehensive health check
    return database_subsystem->initialized && !database_subsystem->shutdown_requested;
}

/*
 * Query Processing API (Phase 2 integration points)
 */

// Submit a query to the database subsystem
bool database_submit_query(const char* database_name __attribute__((unused)),
                          const char* query_id __attribute__((unused)),
                          const char* query_template __attribute__((unused)),
                          const char* parameters_json __attribute__((unused)),
                          int queue_type_hint __attribute__((unused))) {
    if (!database_subsystem || !database_name || !query_template) {
        return false;
    }

    // TODO: Implement query submission to queue system
    log_this(SR_DATABASE, "Query submission not yet implemented", LOG_LEVEL_DEBUG, 0);
    return false;
}

// Check query result status
DatabaseQueryStatus database_query_status(const char* query_id) {
    if (!database_subsystem || !query_id) {
        return DB_QUERY_ERROR;
    }

    // TODO: Implement query status checking
    return DB_QUERY_ERROR;
}

// Get query result
bool database_get_result(const char* query_id, const char* result_buffer, size_t buffer_size) {
    if (!database_subsystem || !query_id || !result_buffer || buffer_size == 0) {
        return false;
    }

    // TODO: Implement result retrieval
    return false;
}

// Cancel a running query
bool database_cancel_query(const char* query_id) {
    if (!database_subsystem || !query_id) {
        return false;
    }

    // TODO: Implement query cancellation
    return false;
}

/*
 * Configuration and Maintenance API
 */

// Reload database configurations
bool database_reload_config(void) {
    if (!database_subsystem) {
        return false;
    }

    // TODO: Implement configuration reload
    log_this(SR_DATABASE, "Configuration reload not yet implemented", LOG_LEVEL_DEBUG, 0);
    return false;
}

// Test database connectivity
bool database_test_connection(const char* database_name) {
    if (!database_subsystem || !database_name) {
        return false;
    }

    // TODO: Implement connection testing
    log_this(SR_DATABASE, "Connection testing not yet implemented", LOG_LEVEL_DEBUG, 0);
    return false;
}

// Get supported database engines
void database_get_supported_engines(char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) {
        return;
    }

    if (!database_subsystem) {
        snprintf(buffer, buffer_size, "Database subsystem not initialized");
        return;
    }

    // List supported engines
    const char* engines = "PostgreSQL, SQLite, MySQL, DB2";
    strncpy(buffer, engines, buffer_size - 1);
    buffer[buffer_size - 1] = '\0';
}

/*
 * Integration points for other subsystems
 */

// For API subsystem integration
bool database_process_api_query(const char* database __attribute__((unused)), const char* query_path __attribute__((unused)),
                               const char* parameters __attribute__((unused)), const char* result_buffer __attribute__((unused)), size_t buffer_size __attribute__((unused))) {
    if (!database_subsystem || !database || !query_path || !result_buffer || buffer_size == 0) {
        return false;
    }

    // TODO: Implement API query processing
    log_this(SR_DATABASE, "API query processing not yet implemented", LOG_LEVEL_DEBUG, 0);
    return false;
}

/*
 * Utility Functions
 */

// Validate query template
bool database_validate_query(const char* query_template) {
    if (!query_template) {
        return false;
    }

    // Basic validation - check for SQL injection patterns
    // TODO: Implement more comprehensive validation
    return strlen(query_template) > 0;
}

// Escape query parameters
char* database_escape_parameter(const char* parameter) {
    if (!parameter) {
        return NULL;
    }

    // TODO: Implement parameter escaping
    return strdup(parameter);
}

// Get query processing time
time_t database_get_query_age(const char* query_id) {
    if (!database_subsystem || !query_id) {
        return 0;
    }

    // TODO: Implement query age tracking
    return 0;
}

// Cleanup old query results
void database_cleanup_old_results(time_t max_age_seconds __attribute__((unused))) {
    if (!database_subsystem) {
        return;
    }

    // TODO: Implement result cleanup
    log_this(SR_DATABASE, "Result cleanup not yet implemented", LOG_LEVEL_DEBUG, 0);
}