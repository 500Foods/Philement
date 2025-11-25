/*
 * Database Management Implementation
 *
 * Implements database addition and removal functionality
 * for the Hydrogen database subsystem.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/network/network.h> // Ping

// Local includes
#include "database.h"
#include "dbqueue/dbqueue.h"
#include "database_connstring.h"
#include "database_engine.h"

// Engine description function declarations
const char* postgresql_engine_get_description(void);
const char* sqlite_engine_get_description(void);
const char* mysql_engine_get_description(void);
const char* db2_engine_get_description(void);

// Global database subsystem instance
extern DatabaseSubsystem* database_subsystem;
extern pthread_mutex_t database_subsystem_mutex;

// Helper Functions for database_add_database

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

    // Create Lead queue (but don't start it yet)
    DatabaseQueue* db_queue = database_queue_create_lead(name, conn_str, conn_config->bootstrap_query);
    free(conn_str);

    if (!db_queue) {
        log_this(SR_DATABASE, "Failed to create Lead database queue", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // CRITICAL: Transfer prepared statement cache size BEFORE starting the worker thread
    db_queue->prepared_statement_cache_size = conn_config->prepared_statement_cache_size;
    log_this(SR_DATABASE, "Configured prepared statement cache size: %d", LOG_LEVEL_DEBUG, 1, 
             conn_config->prepared_statement_cache_size);

    // Now start the worker thread with the configuration already in place
    if (!database_queue_start_worker(db_queue)) {
        log_this(SR_DATABASE, "Failed to start Lead queue worker thread", LOG_LEVEL_ERROR, 0);
        database_queue_destroy(db_queue);
        return false;
    }
    
    // Register queue with global manager
    if (!database_register_queue(db_queue)) {
        return false;
    }

    // Launch complete - DQM is now independent and handles its own database work
    log_this(SR_DATABASE, "DQM launched successfully for %s", LOG_LEVEL_DEBUG, 1, name);

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

// Test database connectivity
bool database_test_connection(const char* database_name) {
    if (!database_subsystem || !database_name) {
        return false;
    }

    if (!global_queue_manager) {
        return false;
    }

    // Find the database queue
    MutexResult lock_result = MUTEX_LOCK(&global_queue_manager->manager_lock, SR_DATABASE);
    if (lock_result != MUTEX_SUCCESS) {
        return false;
    }

    DatabaseQueue* db_queue = NULL;
    for (size_t i = 0; i < global_queue_manager->database_count; i++) {
        if (global_queue_manager->databases[i] &&
            strcmp(global_queue_manager->databases[i]->database_name, database_name) == 0) {
            db_queue = global_queue_manager->databases[i];
            break;
        }
    }

    bool is_connected = false;
    if (db_queue && !db_queue->shutdown_requested) {
        // Check connection status
        MutexResult queue_lock_result = MUTEX_LOCK(&db_queue->connection_lock, SR_DATABASE);
        if (queue_lock_result == MUTEX_SUCCESS) {
            is_connected = db_queue->is_connected;
            mutex_unlock(&db_queue->connection_lock);
        }
    }

    mutex_unlock(&global_queue_manager->manager_lock);
    return is_connected;
}