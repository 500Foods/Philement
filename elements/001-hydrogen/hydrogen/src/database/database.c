/*
 * Database Subsystem Core Implementation
 *
 * Implements the core database subsystem functionality including
 * subsystem initialization, database management, and API functions.
 */

#include "../hydrogen.h"
#include "database.h"
#include "database_queue.h"
#include "database_connstring.h"

// Network includes for ping functionality
#include "../network/network.h"

// Engine description function declarations
const char* postgresql_engine_get_description(void);
const char* sqlite_engine_get_description(void);
const char* mysql_engine_get_description(void);
const char* db2_engine_get_description(void);

// Global database subsystem instance
DatabaseSubsystem* database_subsystem = NULL;
static pthread_mutex_t database_subsystem_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * Helper Functions for database_add_database
 */

// Get database engine interface by name
DatabaseEngineInterface* database_get_engine_interface(const char* engine) {
    if (!engine) return NULL;

    if (strcmp(engine, "postgresql") == 0 || strcmp(engine, "postgres") == 0) {
        return database_engine_get(DB_ENGINE_POSTGRESQL);
    } else if (strcmp(engine, "sqlite") == 0) {
        return database_engine_get(DB_ENGINE_SQLITE);
    } else if (strcmp(engine, "mysql") == 0) {
        return database_engine_get(DB_ENGINE_MYSQL);
    } else if (strcmp(engine, "db2") == 0) {
        return database_engine_get(DB_ENGINE_DB2);
    }

    return NULL;
}

// Find database connection configuration
const DatabaseConnection* database_find_connection_config(const char* name) {
    if (!database_subsystem || !app_config || !name) return NULL;

    const DatabaseConfig* db_config = &app_config->databases;

    for (int i = 0; i < db_config->connection_count; i++) {
        if (strcmp(db_config->connections[i].name, name) == 0) {
            return &db_config->connections[i];
        }
    }

    return NULL;
}

// Build connection string for database
char* database_build_connection_string(const char* engine, const DatabaseConnection* conn_config) {
    if (!engine || !conn_config) return NULL;

    DatabaseEngineInterface* engine_interface = database_get_engine_interface(engine);
    if (!engine_interface) return NULL;

    if (engine_interface->get_connection_string) {
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

        return engine_interface->get_connection_string(&temp_config);
    } else {
        // Fallback connection string building
        if (strcmp(engine, "sqlite") == 0) {
            // For SQLite, use the database path directly
            return strdup(conn_config->database ? conn_config->database : ":memory:");
        } else {
            // For other engines, build connection string from config
            size_t conn_str_len = 256;
            char* conn_str = malloc(conn_str_len);
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
                    free(conn_str); // Free the previously allocated buffer
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
            return conn_str;
        }
    }
}

// Create and start database queue
DatabaseQueue* database_create_and_start_queue(const char* name, const char* conn_str, const char* bootstrap_query) {
    if (!name || !conn_str) return NULL;

    // Create Lead queue for this database
    DatabaseQueue* db_queue = database_queue_create_lead(name, conn_str, bootstrap_query);
    if (!db_queue) {
        log_this(SR_DATABASE, "Failed to create Lead database queue", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // Start the Lead queue worker thread
    if (!database_queue_start_worker(db_queue)) {
        log_this(SR_DATABASE, "Failed to start Lead queue worker thread", LOG_LEVEL_ERROR, 0);
        database_queue_destroy(db_queue);
        return NULL;
    }

    return db_queue;
}

// Register queue with global manager
bool database_register_queue(DatabaseQueue* db_queue) {
    if (!db_queue) return false;

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

    return true;
}

/*
 * Database Subsystem Core API
 */

// Initialize the database subsystem
bool database_subsystem_init(void) {
    // log_this(SR_DATABASE, "Starting database subsystem initialization", LOG_LEVEL_DEBUG, 0);

    MutexResult result = MUTEX_LOCK(&database_subsystem_mutex, SR_DATABASE);
    if (result != MUTEX_SUCCESS) {
        return false;
    }

    if (database_subsystem) {
        MUTEX_UNLOCK(&database_subsystem_mutex, SR_DATABASE);
        return true; // Already initialized
    }

    database_subsystem = calloc(1, sizeof(DatabaseSubsystem));
    if (!database_subsystem) {
        log_this(SR_DATABASE, "Failed to allocate database subsystem", LOG_LEVEL_ERROR, 0);
        MUTEX_UNLOCK(&database_subsystem_mutex, SR_DATABASE);
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
    MUTEX_UNLOCK(&database_subsystem_mutex, SR_DATABASE);

    // Initialize database thread tracking
    init_service_threads(&database_threads, SR_DATABASE);

    // log_this(SR_DATABASE, "Database subsystem initialization completed successfully", LOG_LEVEL_DEBUG, 0);
    return true;
}

// Shut down the database subsystem
void database_subsystem_shutdown(void) {
    MutexResult result = MUTEX_LOCK(&database_subsystem_mutex, SR_DATABASE);
    if (result != MUTEX_SUCCESS) {
        return;
    }

    if (!database_subsystem) {
        MUTEX_UNLOCK(&database_subsystem_mutex, SR_DATABASE);
        return;
    }

    database_subsystem->shutdown_requested = true;

    // TODO: Clean shutdown of queue manager, connections, etc.

    free(database_subsystem);
    database_subsystem = NULL;

    MUTEX_UNLOCK(&database_subsystem_mutex, SR_DATABASE);

    log_this(SR_DATABASE, "Database subsystem shutdown complete", LOG_LEVEL_DEBUG, 0);
}

// Add a database configuration
bool database_add_database(const char* name, const char* engine, const char* connection_string __attribute__((unused))) {
    log_this(SR_DATABASE, "Starting database: %s", LOG_LEVEL_DEBUG, 1, name);

    if (!database_subsystem || !name || !engine) {
        log_this(SR_DATABASE, "Invalid parameters for database", LOG_LEVEL_TRACE, 0);
        return false;
    }

    // Validate that we can get the engine interface
    const DatabaseEngineInterface* engine_interface = database_get_engine_interface(engine);
    if (!engine_interface) {
        log_this(SR_DATABASE, "Database engine not available", LOG_LEVEL_ERROR, 0);
        log_this(SR_DATABASE, engine, LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Find the connection configuration for this database
    const DatabaseConnection* conn_config = database_find_connection_config(name);
    if (!conn_config) {
        log_this(SR_DATABASE, "Database configuration not found: %s", LOG_LEVEL_ERROR, 1, name);
        return false;
    }

    // Build connection string
    char* conn_str = database_build_connection_string(engine, conn_config);
    if (!conn_str) {
        log_this(SR_DATABASE, "Failed to create connection string", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Output engine description and ping host if applicable
    DatabaseEngine engine_type = DB_ENGINE_SQLITE; // Default
    if (strcmp(engine, "postgresql") == 0 || strcmp(engine, "postgres") == 0) {
        engine_type = DB_ENGINE_POSTGRESQL;
    } else if (strcmp(engine, "mysql") == 0) {
        engine_type = DB_ENGINE_MYSQL;
    } else if (strcmp(engine, "db2") == 0) {
        engine_type = DB_ENGINE_DB2;
    } else if (strcmp(engine, "sqlite") == 0) {
        engine_type = DB_ENGINE_SQLITE;
    }

    const char* description = NULL;
    switch (engine_type) {
        case DB_ENGINE_POSTGRESQL:
            description = postgresql_engine_get_description();
            break;
        case DB_ENGINE_SQLITE:
            description = sqlite_engine_get_description();
            break;
        case DB_ENGINE_MYSQL:
            description = mysql_engine_get_description();
            break;
        case DB_ENGINE_DB2:
            description = db2_engine_get_description();
            break;
        case DB_ENGINE_AI:
        case DB_ENGINE_MAX:
        default:
            description = "Unknown database engine";
            break;
    }
    if (description) {
        log_this(SR_DATABASE, "Engine description: %s", LOG_LEVEL_DEBUG, 1, description);
    }

    // Ping host if connection string involves an IP and host
    ConnectionConfig* parsed_config = parse_connection_string(conn_str);
    if (parsed_config && parsed_config->host) {
        // log_this(SR_DATABASE, "Parsed host: %s", LOG_LEVEL_DEBUG, 1, parsed_config->host);
        // Check if host is not localhost or 127.0.0.1
        bool should_ping = true;
        if (engine_type == DB_ENGINE_SQLITE) {
            should_ping = false;
        }
        // if (strcmp(parsed_config->host, "localhost") == 0 ||
        //     strcmp(parsed_config->host, "127.0.0.1") == 0 ||
        //     strcmp(parsed_config->host, "::1") == 0) {
        //     should_ping = false;
        //     log_this(SR_DATABASE, "Skipping ping for localhost", LOG_LEVEL_DEBUG, 0);
        // }

        if (should_ping) {
            double ping_time = interface_time(parsed_config->host);
            if (ping_time > 0) {
                log_this(SR_DATABASE, "Host (%s) ping time: %.6fms", LOG_LEVEL_DEBUG, 2, parsed_config->host, ping_time);
            } else {
                log_this(SR_DATABASE, "Host (%s) ping not measurable", LOG_LEVEL_DEBUG, 1, parsed_config->host);
            }
        }
    } else {
        log_this(SR_DATABASE, "No host found in connection string", LOG_LEVEL_DEBUG, 0);
    }
    if (parsed_config) {
        free_connection_config(parsed_config);
    }

    // Create and start database queue
    DatabaseQueue* db_queue = database_create_and_start_queue(name, conn_str, conn_config->bootstrap_query);
    free(conn_str);

    if (!db_queue) {
        return false;
    }

    // Register queue with global manager
    if (!database_register_queue(db_queue)) {
        return false;
    }

    // Launch complete - DQM is now independent and handles its own database work
    log_this(SR_DATABASE, "DQM launched successfully for %s", LOG_LEVEL_TRACE, 1, name);

    return true;
}

// Remove a database
bool database_remove_database(const char* name) {
    if (!database_subsystem || !name) {
        return false;
    }

    // TODO: Implement database removal logic
    log_this(SR_DATABASE, "Database removal not yet implemented", LOG_LEVEL_TRACE, 0);
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
    log_this(SR_DATABASE, "Query submission not yet implemented", LOG_LEVEL_TRACE, 0);
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
    log_this(SR_DATABASE, "Configuration reload not yet implemented", LOG_LEVEL_TRACE, 0);
    return false;
}

// Test database connectivity
bool database_test_connection(const char* database_name) {
    if (!database_subsystem || !database_name) {
        return false;
    }

    // TODO: Implement connection testing
    log_this(SR_DATABASE, "Connection testing not yet implemented", LOG_LEVEL_TRACE, 0);
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
    log_this(SR_DATABASE, "API query processing not yet implemented", LOG_LEVEL_TRACE, 0);
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
    log_this(SR_DATABASE, "Result cleanup not yet implemented", LOG_LEVEL_TRACE, 0);
}

// Get total number of database queues (lead + child queues)
int database_get_total_queue_count(void) {
    if (!global_queue_manager) {
        return 0;
    }

    MutexResult lock_result = MUTEX_LOCK(&global_queue_manager->manager_lock, SR_DATABASE);
    if (lock_result != MUTEX_SUCCESS) {
        return 0;
    }

    int total_queues = 0;
    for (size_t i = 0; i < global_queue_manager->database_count; i++) {
        const DatabaseQueue* db_queue = global_queue_manager->databases[i];
        if (db_queue) {
            // Count the lead queue
            total_queues++;
            // Count child queues
            total_queues += db_queue->child_queue_count;
        }
    }

    mutex_unlock(&global_queue_manager->manager_lock);
    return total_queues;
}

// Get queue counts by type
void database_get_queue_counts_by_type(int* lead_count, int* slow_count, int* medium_count, int* fast_count, int* cache_count) {
    if (!global_queue_manager) {
        *lead_count = 0;
        *slow_count = 0;
        *medium_count = 0;
        *fast_count = 0;
        *cache_count = 0;
        return;
    }

    MutexResult lock_result = MUTEX_LOCK(&global_queue_manager->manager_lock, SR_DATABASE);
    if (lock_result != MUTEX_SUCCESS) {
        *lead_count = 0;
        *slow_count = 0;
        *medium_count = 0;
        *fast_count = 0;
        *cache_count = 0;
        return;
    }

    *lead_count = 0;
    *slow_count = 0;
    *medium_count = 0;
    *fast_count = 0;
    *cache_count = 0;

    for (size_t i = 0; i < global_queue_manager->database_count; i++) {
        DatabaseQueue* db_queue = global_queue_manager->databases[i];
        if (db_queue) {
            if (db_queue->is_lead_queue) {
                (*lead_count)++;
            }

            // Count child queues by type
            for (int j = 0; j < db_queue->child_queue_count; j++) {
                const DatabaseQueue* child_queue = db_queue->child_queues[j];
                if (child_queue) {
                    if (strcmp(child_queue->queue_type, "slow") == 0) {
                        (*slow_count)++;
                    } else if (strcmp(child_queue->queue_type, "medium") == 0) {
                        (*medium_count)++;
                    } else if (strcmp(child_queue->queue_type, "fast") == 0) {
                        (*fast_count)++;
                    } else if (strcmp(child_queue->queue_type, "cache") == 0) {
                        (*cache_count)++;
                    }
                }
            }
        }
    }

    mutex_unlock(&global_queue_manager->manager_lock);
}

// Get database counts by type
void database_get_counts_by_type(int* postgres_count, int* mysql_count, int* sqlite_count, int* db2_count) {
    if (!app_config) {
        *postgres_count = 0;
        *mysql_count = 0;
        *sqlite_count = 0;
        *db2_count = 0;
        return;
    }

    const DatabaseConfig* db_config = &app_config->databases;

    *postgres_count = 0;
    *mysql_count = 0;
    *sqlite_count = 0;
    *db2_count = 0;

    for (int i = 0; i < db_config->connection_count; i++) {
        const DatabaseConnection* conn = &db_config->connections[i];
        if (conn->enabled && conn->type) {
            if (strcmp(conn->type, "postgresql") == 0 || strcmp(conn->type, "postgres") == 0) {
                (*postgres_count)++;
            } else if (strcmp(conn->type, "mysql") == 0) {
                (*mysql_count)++;
            } else if (strcmp(conn->type, "sqlite") == 0) {
                (*sqlite_count)++;
            } else if (strcmp(conn->type, "db2") == 0) {
                (*db2_count)++;
            }
        }
    }
}