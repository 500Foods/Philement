/*
 * Database Queue Heartbeat Functions
 *
 * Implements heartbeat monitoring and connection management for the Hydrogen database subsystem.
 * Split from database_queue.c for better maintainability.
 */

#include "../hydrogen.h"
#include <assert.h>

// Local includes
#include "database_queue.h"
#include "database.h"

/*
 * Parse connection string into ConnectionConfig
 * Supports formats like: postgresql://user:pass@host:port/database
 */
static ConnectionConfig* parse_connection_string(const char* conn_string) {
    if (!conn_string) return NULL;

    ConnectionConfig* config = calloc(1, sizeof(ConnectionConfig));
    if (!config) return NULL;

    // Copy the connection string
    config->connection_string = strdup(conn_string);
    if (!config->connection_string) {
        free(config);
        return NULL;
    }

    // Parse PostgreSQL connection string format: postgresql://user:pass@host:port/database
    if (strncmp(conn_string, "postgresql://", 13) == 0) {
        const char* after_proto = conn_string + 13; // Skip "postgresql://"

        // Find @ symbol to separate user:pass from host:port/database
        const char* at_pos = strchr(after_proto, '@');
        if (at_pos) {
            // Parse user:pass part
            char user_pass[256] = {0};
            size_t user_pass_len = (size_t)(at_pos - after_proto);
            if (user_pass_len < sizeof(user_pass) && user_pass_len > 0) {
                strncpy(user_pass, after_proto, user_pass_len);
                user_pass[user_pass_len] = '\0';

                // Split user:pass
                char* colon_pos = strchr(user_pass, ':');
                if (colon_pos) {
                    *colon_pos = '\0';
                    config->username = strdup(user_pass);
                    if (!config->username) goto cleanup;
                    config->password = strdup(colon_pos + 1);
                    if (!config->password) goto cleanup;
                } else {
                    config->username = strdup(user_pass);
                    if (!config->username) goto cleanup;
                }
            }

            // Parse host:port/database part
            const char* host_start = at_pos + 1;
            const char* slash_pos = strchr(host_start, '/');
            if (slash_pos) {
                // Parse host:port
                char host_port[256] = {0};
                size_t host_port_len = (size_t)(slash_pos - host_start);
                if (host_port_len < sizeof(host_port) && host_port_len > 0) {
                    strncpy(host_port, host_start, host_port_len);
                    host_port[host_port_len] = '\0';

                    // Split host:port
                    char* colon_pos = strchr(host_port, ':');
                    if (colon_pos) {
                        *colon_pos = '\0';
                        config->host = strdup(host_port);
                        if (!config->host) goto cleanup;
                        config->port = atoi(colon_pos + 1);
                    } else {
                        config->host = strdup(host_port);
                        if (!config->host) goto cleanup;
                        config->port = 5432; // Default PostgreSQL port
                    }
                }

                // Parse database
                config->database = strdup(slash_pos + 1);
                if (!config->database) goto cleanup;
            }
        }
    }

    // Set defaults if not specified - with proper error handling
    if (!config->host) {
        config->host = strdup("localhost");
        if (!config->host) goto cleanup;
    }
    if (!config->port) config->port = 5432;
    if (!config->database) {
        config->database = strdup("postgres");
        if (!config->database) goto cleanup;
    }
    if (!config->username) {
        config->username = strdup("");
        if (!config->username) goto cleanup;
    }
    if (!config->password) {
        config->password = strdup("");
        if (!config->password) goto cleanup;
    }

    config->timeout_seconds = 30; // Default timeout
    config->ssl_enabled = false;  // Default SSL setting

    return config;

cleanup:
    // Clean up any allocated memory on failure
    if (config->host) free(config->host);
    if (config->database) free(config->database);
    if (config->username) free(config->username);
    if (config->password) free(config->password);
    if (config->connection_string) free(config->connection_string);
    free(config);
    return NULL;
}

/*
 * Free ConnectionConfig and all its allocated strings
 */
static void free_connection_config(ConnectionConfig* config) {
    if (!config) return;

    // Only free allocated string fields with additional safety checks
    if (config->host && (uintptr_t)config->host > 0x1000) free(config->host);
    if (config->database && (uintptr_t)config->database > 0x1000) free(config->database);
    if (config->username && (uintptr_t)config->username > 0x1000) free(config->username);
    if (config->password && (uintptr_t)config->password > 0x1000) free(config->password);
    if (config->connection_string && (uintptr_t)config->connection_string > 0x1000) free(config->connection_string);

    // SSL fields are not allocated in parse_connection_string, so don't free them
    // if (config->ssl_cert_path) free(config->ssl_cert_path);
    // if (config->ssl_key_path) free(config->ssl_key_path);
    // if (config->ssl_ca_path) free(config->ssl_ca_path);

    free(config);
}

/*
 * Start heartbeat monitoring for a database queue
 */
void database_queue_start_heartbeat(DatabaseQueue* db_queue) {
    if (!db_queue) return;

    db_queue->last_heartbeat = time(NULL);
    db_queue->last_connection_attempt = time(NULL);

    char* dqm_label = database_queue_generate_label(db_queue);

    // Log initial disconnected state
    log_this(dqm_label, "Heartbeat monitoring started: initial state DISCONNECTED",
             LOG_LEVEL_STATE);

    // Attempt initial connection
    log_this(dqm_label, "Attempting initial database connection", LOG_LEVEL_STATE);

    // Perform immediate connection check and log result
    bool is_connected = database_queue_check_connection(db_queue);

    log_this(dqm_label, "Initial connection attempt: %s",
             LOG_LEVEL_STATE, is_connected ? "SUCCESS" : "FAILED");

    free(dqm_label);
}

/*
 * Check database connection and update status
 */
bool database_queue_check_connection(DatabaseQueue* db_queue) {
    if (!db_queue || !db_queue->connection_string) return false;

    // Parse connection string into config
    ConnectionConfig* config = parse_connection_string(db_queue->connection_string);
    if (!config) {
        db_queue->is_connected = false;
        db_queue->last_connection_attempt = time(NULL);
        return false;
    }

    // Determine database engine type from connection string
    DatabaseEngine engine_type = DB_ENGINE_POSTGRESQL; // Default to PostgreSQL
    if (strncmp(db_queue->connection_string, "postgresql://", 13) == 0) {
        engine_type = DB_ENGINE_POSTGRESQL;
    } else if (strncmp(db_queue->connection_string, "mysql://", 8) == 0) {
        engine_type = DB_ENGINE_MYSQL;
    } else if (strncmp(db_queue->connection_string, "sqlite:", 7) == 0) {
        engine_type = DB_ENGINE_SQLITE;
    }

    // Initialize database engine system if not already done
    if (!database_engine_init()) {
        free_connection_config(config);
        db_queue->is_connected = false;
        db_queue->last_connection_attempt = time(NULL);
        return false;
    }

    // Attempt real database connection with DQM designator
    DatabaseHandle* db_handle = NULL;
    char* dqm_designator = database_queue_generate_label(db_queue);
    bool connection_success = database_engine_connect_with_designator(engine_type, config, &db_handle, dqm_designator);
    free(dqm_designator);

    if (connection_success && db_handle) {
        // Connection successful - clean up the handle since we don't need to keep it
        database_engine_cleanup_connection(db_handle);
        db_queue->is_connected = true;
    } else {
        // Connection failed
        db_queue->is_connected = false;
    }

    db_queue->last_connection_attempt = time(NULL);

    // Clean up config
    free_connection_config(config);

    return db_queue->is_connected;
}

/*
 * Perform heartbeat operations
 */
void database_queue_perform_heartbeat(DatabaseQueue* db_queue) {
    if (!db_queue) return;

    time_t current_time = time(NULL);
    db_queue->last_heartbeat = current_time;

    char* dqm_label = database_queue_generate_label(db_queue);

    // Check connection status
    bool was_connected = db_queue->is_connected;
    bool is_connected = database_queue_check_connection(db_queue);

    // Always log heartbeat activity to show the DQM is alive
    log_this(dqm_label, "Heartbeat: connection %s, queue depth: %zu",
             LOG_LEVEL_STATE, is_connected ? "OK" : "FAILED",
             database_queue_get_depth(db_queue));

    // Log connection status changes
    if (was_connected != is_connected) {
        if (is_connected) {
            log_this(dqm_label, "Database connection established", LOG_LEVEL_STATE);
        } else {
            log_this(dqm_label, "Database connection lost - will retry", LOG_LEVEL_ALERT);
        }
    }

    free(dqm_label);

    // If Lead queue, manage child queues
    if (db_queue->is_lead_queue) {
        database_queue_manage_child_queues(db_queue);
    }
}