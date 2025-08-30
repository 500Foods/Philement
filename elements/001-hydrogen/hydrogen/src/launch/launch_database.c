/*
 * Database Subsystem Launch Implementation
 */

// Global includes 
#include "../hydrogen.h"

// Local includes
#include "launch.h"

volatile sig_atomic_t database_stopping = 0;



// Check database subsystem launch readiness
LaunchReadiness check_database_launch_readiness(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool overall_readiness = false;

    // First message is subsystem name
    add_launch_message(&messages, &count, &capacity, strdup(SR_DATABASE));

    // Early return cases
    if (server_stopping) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   System shutdown in progress"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_DATABASE, .ready = false, .messages = messages };
    }

    if (!server_starting && !server_running) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   System not in startup or running state"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_DATABASE, .ready = false, .messages = messages };
    }

    if (!app_config) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Configuration not loaded"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_DATABASE, .ready = false, .messages = messages };
    }

    // Validate database configuration
    const DatabaseConfig* db_config = &app_config->databases;

    // Check default workers
    if (db_config->default_workers < 1 || db_config->default_workers > 32) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Invalid default worker count"));
        overall_readiness = false;
    } else {
        char* worker_msg = malloc(256);
        if (worker_msg) {
            snprintf(worker_msg, 256, "  Go:      Default worker count valid: %d", db_config->default_workers);
            add_launch_message(&messages, &count, &capacity, worker_msg);
        }
    }

    char* count_msg = malloc(256);
    if (count_msg) {
        snprintf(count_msg, 256, "  Go:      Connection count: %d", db_config->connection_count);
        add_launch_message(&messages, &count, &capacity, count_msg);
    }

    // Validate each enabled connection
    bool connections_valid = true;
    for (int i = 0; i < db_config->connection_count; i++) {
        const DatabaseConnection* conn = &db_config->connections[i];

        if (!conn->name || strlen(conn->name) < 1 || strlen(conn->name) > 64) {
            char* name_msg = malloc(256);
            if (name_msg) {
                snprintf(name_msg, 256, "  No-Go:   Invalid database name for connection %d", i);
                add_launch_message(&messages, &count, &capacity, name_msg);
            }
            connections_valid = false;
            continue;
        }

        if (conn->enabled) {
            bool conn_valid = true;

            if (!conn->type || strlen(conn->type) < 1 || strlen(conn->type) > 32) {
                char* type_msg = malloc(256);
                if (type_msg) {
                    snprintf(type_msg, 256, "  No-Go:   Invalid database type for %s", conn->name);
                    add_launch_message(&messages, &count, &capacity, type_msg);
                }
                conn_valid = false;
            }

            if (!conn->database || !conn->host || !conn->port || !conn->user || !conn->pass) {
                char* fields_msg = malloc(256);
                if (fields_msg) {
                    snprintf(fields_msg, 256, "  No-Go:   Missing required fields for %s", conn->name);
                    add_launch_message(&messages, &count, &capacity, fields_msg);
                }
                conn_valid = false;
            }

            if (conn->workers < 1 || conn->workers > 32) {
                char* worker_count_msg = malloc(256);
                if (worker_count_msg) {
                    snprintf(worker_count_msg, 256, "  No-Go:   Invalid worker count for %s", conn->name);
                    add_launch_message(&messages, &count, &capacity, worker_count_msg);
                }
                conn_valid = false;
            }

            if (conn_valid) {
                char* valid_msg = malloc(256);
                if (valid_msg) {
                    snprintf(valid_msg, 256, "  Go:      Database %s configuration valid", conn->name);
                    add_launch_message(&messages, &count, &capacity, valid_msg);
                }
            } else {
                connections_valid = false;
            }
        } else {
            char* disabled_msg = malloc(256);
            if (disabled_msg) {
                snprintf(disabled_msg, 256, "  Go:      Database %s disabled", conn->name);
                add_launch_message(&messages, &count, &capacity, disabled_msg);
            }
        }
    }

    // Basic readiness check - verify subsystem registration and config validity
    int subsystem_id = get_subsystem_id_by_name(SR_DATABASE);
    if (subsystem_id >= 0 && connections_valid) {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Database subsystem registered"));
        add_launch_message(&messages, &count, &capacity, strdup("  Decide:  Go For Launch of Database Subsystem"));
        overall_readiness = true;
    } else {
        if (subsystem_id < 0) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Database subsystem not registered"));
        }
        add_launch_message(&messages, &count, &capacity, strdup("  Decide:  No-Go For Launch of Database Subsystem"));
        overall_readiness = false;
    }

    finalize_launch_messages(&messages, &count, &capacity);

    return (LaunchReadiness){
        .subsystem = SR_DATABASE,
        .ready = overall_readiness,
        .messages = messages
    };
}

// Launch the database subsystem
int launch_database_subsystem(void) {
    // Reset shutdown flag
    database_stopping = 0;
    
    log_this(SR_DATABASE, "Initializing database subsystem", LOG_LEVEL_STATE);
    
    // Get subsystem ID and update state
    int subsystem_id = get_subsystem_id_by_name(SR_DATABASE);
    if (subsystem_id >= 0) {
        update_subsystem_state(subsystem_id, SUBSYSTEM_RUNNING);
        log_this(SR_DATABASE, "Database subsystem initialized", LOG_LEVEL_STATE);
        return 1;
    }
    
    log_this(SR_DATABASE, "Failed to initialize database subsystem", LOG_LEVEL_ERROR);
    return 0;
}

// Shutdown handler - defined in launch_database.h, implemented here
void shutdown_database(void) {
    if (!database_stopping) {
        database_stopping = 1;
        log_this(SR_DATABASE, "Database subsystem shutting down", LOG_LEVEL_STATE);
    }
}
