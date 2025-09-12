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
    log_this(dqm_label, "Heartbeat monitoring started: state DISCONNECTED", LOG_LEVEL_STATE, 0);

    // Attempt database connection
    log_this(dqm_label, "Attempting database connection", LOG_LEVEL_STATE, 0);

    // Perform immediate connection check and log result
    bool is_connected = database_queue_check_connection(db_queue);

    log_this(dqm_label, "Connection attempt: %s", LOG_LEVEL_STATE, 1, is_connected ? "SUCCESS" : "FAILED");

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
        // Connection successful - store the persistent connection
        MutexResult result = MUTEX_LOCK(&db_queue->connection_lock, SR_DATABASE);
        if (result == MUTEX_SUCCESS) {
            // Clean up any existing connection first
            if (db_queue->persistent_connection) {
                database_engine_cleanup_connection(db_queue->persistent_connection);
            }
            db_queue->persistent_connection = db_handle;
            db_queue->is_connected = true;

            // CRITICAL: Validate the stored connection's mutex before unlocking
            if (db_handle && (uintptr_t)&db_handle->connection_lock >= 0x1000) {
                mutex_unlock(&db_queue->connection_lock);
            } else {
                log_this(SR_DATABASE, "CRITICAL ERROR: Stored connection has corrupted mutex! Not unlocking queue mutex", LOG_LEVEL_ERROR, 0);
                // Don't unlock - this will cause the timeout detection to trigger
                return false;
            }
        } else {
            // Clean up the handle since we can't store it
            database_engine_cleanup_connection(db_handle);
            db_queue->is_connected = false;
        }

        // Execute bootstrap query if configured (only for Lead queues)
        if (db_queue->is_lead_queue) {
            database_queue_execute_bootstrap_query(db_queue);
        }
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

    // Check connection status using persistent connection
    bool was_connected = db_queue->is_connected;
    bool is_connected = false;

    // Use persistent connection for health check if available
    log_this(dqm_label, "MUTEX_HEARTBEAT_LOCK: About to lock queue connection mutex", LOG_LEVEL_DEBUG, 0);
    MutexResult result = MUTEX_LOCK(&db_queue->connection_lock, SR_DATABASE);
    log_this(dqm_label, "MUTEX_HEARTBEAT_LOCK_RESULT: Lock result %s", LOG_LEVEL_DEBUG, 1, mutex_result_to_string(result));

    if (result == MUTEX_SUCCESS) {
        if (db_queue->persistent_connection) {
            // CRITICAL: Validate persistent connection integrity before use
            if ((uintptr_t)&db_queue->persistent_connection->connection_lock >= 0x1000) {
                is_connected = database_engine_health_check(db_queue->persistent_connection);
                db_queue->is_connected = is_connected;
            } else {
                log_this(dqm_label, "CRITICAL ERROR: Persistent connection has corrupted mutex! Address: %p", LOG_LEVEL_ERROR, 1, (void*)&db_queue->persistent_connection->connection_lock);
                log_this(dqm_label, "Discarding corrupted connection and attempting reconnection", LOG_LEVEL_ERROR, 0);

                // Clean up corrupted connection
                database_engine_cleanup_connection(db_queue->persistent_connection);
                db_queue->persistent_connection = NULL;

                // Attempt to establish a new connection
                is_connected = database_queue_check_connection(db_queue);
            }
        } else {
            // No persistent connection, attempt to establish one
            is_connected = database_queue_check_connection(db_queue);
        }
        log_this(dqm_label, "MUTEX_HEARTBEAT_UNLOCK: About to unlock queue connection mutex", LOG_LEVEL_DEBUG, 0);
        mutex_unlock(&db_queue->connection_lock);
        log_this(dqm_label, "MUTEX_HEARTBEAT_UNLOCK: Unlock completed", LOG_LEVEL_DEBUG, 0);
    } else {
        is_connected = false;
        db_queue->is_connected = false;
    }

    // Always log heartbeat activity to show the DQM is alive
    log_this(dqm_label, "Heartbeat: connection %s, queue depth: %zu", LOG_LEVEL_STATE, 2, 
        is_connected ? "OK" : "FAILED",
        database_queue_get_depth(db_queue));

    // Log connection status changes
    if (was_connected != is_connected) {
        if (is_connected) {
            log_this(dqm_label, "Database connection established", LOG_LEVEL_STATE, 0);
        } else {
            log_this(dqm_label, "Database connection lost - will retry", LOG_LEVEL_ALERT, 0);
        }
    }

    free(dqm_label);

    // If Lead queue, manage child queues
    if (db_queue->is_lead_queue) {
        database_queue_manage_child_queues(db_queue);
    }
}

/*
 * Execute bootstrap query after successful Lead DQM connection
 * This loads the Query Table Cache (QTC) and confirms connection health
 */
void database_queue_execute_bootstrap_query(DatabaseQueue* db_queue) {
    if (!db_queue || !db_queue->is_lead_queue) {
        return;
    }

    char* dqm_label = database_queue_generate_label(db_queue);
    log_this(dqm_label, "Executing bootstrap query", LOG_LEVEL_STATE, 0);

    // Use configured bootstrap query or fallback to safe default
    const char* bootstrap_query = db_queue->bootstrap_query ? db_queue->bootstrap_query : "SELECT 42 as test_value";
    log_this(dqm_label, "Bootstrap query SQL: %s", LOG_LEVEL_DEBUG, 1, bootstrap_query);

    // Create query request
    QueryRequest* request = calloc(1, sizeof(QueryRequest));
    if (!request) {
        log_this(dqm_label, "Failed to allocate query request for bootstrap", LOG_LEVEL_ERROR, 0);
        free(dqm_label);
        return;
    }

    request->query_id = strdup("bootstrap_query");
    request->sql_template = strdup(bootstrap_query);
    request->parameters_json = strdup("{}");
    request->timeout_seconds = 10; // Shorter timeout for bootstrap
    request->isolation_level = DB_ISOLATION_READ_COMMITTED;
    request->use_prepared_statement = false;

    log_this(dqm_label, "Bootstrap query request created - timeout: %d seconds", LOG_LEVEL_DEBUG, 1, request->timeout_seconds);

    // Execute query using persistent connection
    QueryResult* result = NULL;
    bool query_success = false;

    // Add timeout protection
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += 5; // 5 second timeout for the entire operation

    log_this(dqm_label, "Attempting to acquire connection lock with 5 second timeout", LOG_LEVEL_DEBUG, 0);

    log_this(dqm_label, "MUTEX_BOOTSTRAP_LOCK: About to lock connection mutex for bootstrap", LOG_LEVEL_DEBUG, 0);
    if (pthread_mutex_timedlock(&db_queue->connection_lock, &timeout) == 0) {
        log_this(dqm_label, "MUTEX_BOOTSTRAP_LOCK_SUCCESS: Connection lock acquired successfully", LOG_LEVEL_DEBUG, 0);

        if (db_queue->persistent_connection) {
            log_this(dqm_label, "Persistent connection available, executing query", LOG_LEVEL_DEBUG, 0);

            // Debug the connection handle before calling database_engine_execute
            log_this(dqm_label, "Connection handle: %p", LOG_LEVEL_DEBUG, 1, (void*)db_queue->persistent_connection);
            
            // CRITICAL: Validate connection integrity before use
            DatabaseEngine original_engine_type = db_queue->persistent_connection->engine_type;
            log_this(dqm_label, "Connection engine_type: %d", LOG_LEVEL_DEBUG, 1, (int)original_engine_type);
            
            // Add memory corruption detection
            if (original_engine_type != DB_ENGINE_POSTGRESQL) {
                log_this(dqm_label, "CRITICAL ERROR: Connection engine_type corrupted! Expected 0 (PostgreSQL), got %d", LOG_LEVEL_ERROR, 1, (int)original_engine_type);
                log_this(dqm_label, "Aborting bootstrap query to prevent hang", LOG_LEVEL_ERROR, 0);
                pthread_mutex_unlock(&db_queue->connection_lock);
                goto cleanup;
            }
            
            log_this(dqm_label, "Request: %p, query: '%s'", LOG_LEVEL_DEBUG, 2, (void*)request, request->sql_template ? request->sql_template : "NULL");

            log_this(dqm_label, "About to call database_engine_execute - this is where it might hang", LOG_LEVEL_DEBUG, 0);
            
            // Force log output before the hanging call
            fflush(stdout);
            fflush(stderr);

            time_t query_start_time = time(NULL);
            
            // Validate connection integrity again right before the call
            if (db_queue->persistent_connection->engine_type != original_engine_type) {
                log_this(dqm_label, "CRITICAL ERROR: Connection engine_type changed from %d to %d during bootstrap!", LOG_LEVEL_ERROR, 2,
                    (int)original_engine_type, 
                    (int)db_queue->persistent_connection->engine_type);
                log_this(dqm_label, "Memory corruption detected - aborting bootstrap", LOG_LEVEL_ERROR, 0);
                pthread_mutex_unlock(&db_queue->connection_lock);
                goto cleanup;
            }
            
            // Simple direct call with extensive debugging
            log_this(dqm_label, "BOOTSTRAP HANG DEBUG: About to call database_engine_execute", LOG_LEVEL_ERROR, 0);
            log_this(dqm_label, "BOOTSTRAP HANG DEBUG: If you see this but not the next message, the hang is in database_engine_execute", LOG_LEVEL_ERROR, 0);
            
            // CRITICAL: Stack corruption detection
            DatabaseHandle* connection_to_use = db_queue->persistent_connection;
            
            // Simple logging without complex format strings to avoid buffer overflow
            log_this(dqm_label, "Stack validation: connection_to_use assigned", LOG_LEVEL_ERROR, 0);
            log_this(dqm_label, "Stack validation: checking pointer validity", LOG_LEVEL_ERROR, 0);
            
            if (!connection_to_use || (uintptr_t)connection_to_use < 0x1000) {
                log_this(dqm_label, "CRITICAL ERROR: Connection pointer corrupted - aborting", LOG_LEVEL_ERROR, 0);
                pthread_mutex_unlock(&db_queue->connection_lock);
                goto cleanup;
            }
            
            log_this(dqm_label, "Stack validation: checking engine type", LOG_LEVEL_ERROR, 0);
            
            if (connection_to_use->engine_type != DB_ENGINE_POSTGRESQL) {
                log_this(dqm_label, "CRITICAL ERROR: Connection engine_type corrupted - aborting", LOG_LEVEL_ERROR, 0);
                pthread_mutex_unlock(&db_queue->connection_lock);
                goto cleanup;
            }
            
            log_this(dqm_label, "Stack validation: about to call function", LOG_LEVEL_ERROR, 0);
            
            // CRITICAL: Dump connection BEFORE the call to see if it's valid
            debug_dump_connection("BEFORE", connection_to_use, dqm_label);
            
            // Call with minimal parameters to reduce stack corruption risk
            query_success = database_engine_execute(connection_to_use, request, &result);
            
            // CRITICAL: Dump connection AFTER the call to see what changed
            debug_dump_connection("AFTER", connection_to_use, dqm_label);
            
            log_this(dqm_label, "BOOTSTRAP HANG DEBUG: database_engine_execute call completed", LOG_LEVEL_ERROR, 0);
            
            time_t query_end_time = time(NULL);

            log_this(dqm_label, "Query execution completed (direct method)", LOG_LEVEL_DEBUG, 0);

            log_this(dqm_label, "Query execution completed in %ld seconds", LOG_LEVEL_DEBUG, 1, query_end_time - query_start_time);

            if (query_success && result && result->success) {
                log_this(dqm_label, "Bootstrap query successful: %zu records returned", LOG_LEVEL_STATE, 1, result->row_count);

                // Log detailed result information
                if (result->row_count > 0 && result->column_count > 0 && result->data_json) {
                    log_this(dqm_label, "Bootstrap query result data: %s", LOG_LEVEL_DEBUG, 1, result->data_json);
                }

                if (result->affected_rows > 0) {
                    log_this(dqm_label, "Bootstrap query affected %d rows", LOG_LEVEL_DEBUG, 1, result->affected_rows);
                }

                log_this(dqm_label, "Bootstrap query completed successfully - continuing with heartbeat", LOG_LEVEL_STATE, 0);
            } else {
                log_this(dqm_label, "Bootstrap query failed: success=%d, result=%p, error=%s", LOG_LEVEL_ERROR, 3,
                        query_success,
                        (void*)result,
                        result && result->error_message ? result->error_message : "Unknown error");

                if (result) {
                    log_this(dqm_label, "Result details: row_count=%zu, column_count=%zu, affected_rows=%d", LOG_LEVEL_DEBUG, 3,
                            result->row_count,
                            result->column_count,
                            result->affected_rows);
                }
            }
        } else {
            log_this(dqm_label, "No persistent connection available for bootstrap query", LOG_LEVEL_ERROR, 0);
        }

        log_this(dqm_label, "MUTEX_BOOTSTRAP_UNLOCK: About to unlock connection mutex", LOG_LEVEL_DEBUG, 0);
        mutex_unlock(&db_queue->connection_lock);
        log_this(dqm_label, "MUTEX_BOOTSTRAP_UNLOCK_SUCCESS: Connection mutex unlocked", LOG_LEVEL_DEBUG, 0);
    } else {
        log_this(dqm_label, "Timeout waiting for connection lock in bootstrap query (5 seconds)", LOG_LEVEL_ERROR, 0);
        log_this(dqm_label, "This indicates the connection lock is held by another thread", LOG_LEVEL_ERROR, 0);
    }

cleanup:
    // Clean up
    if (request->query_id) free(request->query_id);
    if (request->sql_template) free(request->sql_template);
    if (request->parameters_json) free(request->parameters_json);
    free(request);

    if (result) {
        database_engine_cleanup_result(result);
    }

    free(dqm_label);
}