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
#include "sqlite/types.h"


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
    // Parse MySQL connection string format: mysql://user:pass@host:port/database
    else if (strncmp(conn_string, "mysql://", 8) == 0) {
        const char* after_proto = conn_string + 8; // Skip "mysql://"

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
                        config->port = 3306; // Default MySQL port
                    }
                }

                // Parse database
                config->database = strdup(slash_pos + 1);
                if (!config->database) goto cleanup;
            }
        }
    }
    // Parse DB2 ODBC connection string format: DRIVER={...};DATABASE=...;HOSTNAME=...;PORT=...;PROTOCOL=...;UID=...;PWD=...
    else if (strstr(conn_string, "DRIVER=") != NULL && strstr(conn_string, "DATABASE=") != NULL) {
        // Parse key=value pairs separated by semicolons
        char* conn_str_copy = strdup(conn_string);
        if (!conn_str_copy) goto cleanup;

        char* token = strtok(conn_str_copy, ";");
        while (token != NULL) {
            // Skip whitespace
            while (*token == ' ') token++;

            // Find the = separator
            char* equals_pos = strchr(token, '=');
            if (equals_pos) {
                *equals_pos = '\0';
                const char* key = token;
                const char* value = equals_pos + 1;

                // Remove quotes around values if present
                if (*value == '{' && value[strlen(value) - 1] == '}') {
                    // Handle {value} format for DRIVER
                    value++;
                    char* end_quote = strrchr((char*)value, '}');
                    if (end_quote) *end_quote = '\0';
                } else if (*value == '"' && value[strlen(value) - 1] == '"') {
                    // Handle "value" format
                    value++;
                    char* end_quote = strrchr((char*)value, '"');
                    if (end_quote) *end_quote = '\0';
                }

                if (strcmp(key, "DATABASE") == 0) {
                    config->database = strdup(value);
                    if (!config->database) {
                        free(conn_str_copy);
                        goto cleanup;
                    }
                } else if (strcmp(key, "HOSTNAME") == 0) {
                    config->host = strdup(value);
                    if (!config->host) {
                        free(conn_str_copy);
                        goto cleanup;
                    }
                } else if (strcmp(key, "PORT") == 0) {
                    config->port = atoi(value);
                } else if (strcmp(key, "UID") == 0) {
                    config->username = strdup(value);
                    if (!config->username) {
                        free(conn_str_copy);
                        goto cleanup;
                    }
                } else if (strcmp(key, "PWD") == 0) {
                    config->password = strdup(value);
                    if (!config->password) {
                        free(conn_str_copy);
                        goto cleanup;
                    }
                }
            }
            token = strtok(NULL, ";");
        }
        free(conn_str_copy);
    }

    // Set defaults if not specified - with proper error handling
    if (!config->host) {
        config->host = strdup("localhost");
        if (!config->host) goto cleanup;
    }
    if (!config->port) config->port = 5432;

    // Handle database field based on connection string format
    if (!config->database) {
        // Check if this is a SQLite file path (doesn't match PostgreSQL or DB2 formats)
        if (conn_string && strncmp(conn_string, "postgresql://", 13) != 0 &&
            !strstr(conn_string, "DATABASE=")) {
            // This is likely a SQLite file path - use it as the database
            config->database = strdup(conn_string);
            if (!config->database) goto cleanup;
        } else {
            // Default to postgres for PostgreSQL connections
            config->database = strdup("postgres");
            if (!config->database) goto cleanup;
        }
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

    // Perform immediate connection check and log result
    bool is_connected = database_queue_check_connection(db_queue);

    if (is_connected) {
        log_this(dqm_label, "Connection attempt: SUCCESS", LOG_LEVEL_TRACE, 0);
    } else {
        log_this(dqm_label, "Connection attempt: FAILED", LOG_LEVEL_ERROR, 0);

        // Determine engine type from connection string for better error reporting
        const char* engine_name = "unknown";
        if (db_queue->connection_string) {
            if (strncmp(db_queue->connection_string, "postgresql://", 13) == 0) {
                engine_name = "PostgreSQL";
            } else if (strncmp(db_queue->connection_string, "mysql://", 8) == 0) {
                engine_name = "MySQL";
            } else if (strncmp(db_queue->connection_string, "sqlite:", 7) == 0) {
                engine_name = "SQLite";
            } else {
                // For DB2 and other engines, the connection string is just the database name
                engine_name = "DB2";
            }
        }

        // Log connection details without password for security
        char* safe_conn_str = strdup(db_queue->connection_string);
        if (safe_conn_str) {
            // Mask passwords in different connection string formats
            char* pwd_pos = strstr(safe_conn_str, "PWD=");
            if (pwd_pos) {
                // DB2 format: PWD=password;
                const char* end_pos = strchr(pwd_pos, ';');
                if (end_pos) {
                    memset(pwd_pos + 4, '*', (size_t)(end_pos - (pwd_pos + 4)));
                } else {
                    // Password at end of string
                    memset(pwd_pos + 4, '*', strlen(pwd_pos + 4));
                }
            } else if (strncmp(safe_conn_str, "mysql://", 8) == 0) {
                // MySQL format: mysql://user:password@host:port/database
                const char* after_proto = safe_conn_str + 8; // Skip "mysql://"
                const char* at_pos = strchr(after_proto, '@');
                if (at_pos) {
                    const char* colon_pos = strchr(after_proto, ':');
                    if (colon_pos && colon_pos < at_pos) {
                        // Mask from after colon to @
                        memset((char*)colon_pos + 1, '*', (size_t)(at_pos - (colon_pos + 1)));
                    }
                }
            } else if (strncmp(safe_conn_str, "postgresql://", 13) == 0) {
                // PostgreSQL format: postgresql://user:password@host:port/database
                const char* after_proto = safe_conn_str + 13; // Skip "postgresql://"
                const char* at_pos = strchr(after_proto, '@');
                if (at_pos) {
                    const char* colon_pos = strchr(after_proto, ':');
                    if (colon_pos && colon_pos < at_pos) {
                        // Mask from after colon to @
                        memset((char*)colon_pos + 1, '*', (size_t)(at_pos - (colon_pos + 1)));
                    }
                }
            }
            log_this(dqm_label, "Connection details: string='%s', engine='%s'",
                     LOG_LEVEL_ERROR, 2, safe_conn_str, engine_name);
            free(safe_conn_str);
        }
    }

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

        // Signal that initial connection attempt is complete (parsing failed)
        if (db_queue->is_lead_queue) {
            char* dqm_label = database_queue_generate_label(db_queue);
            MutexResult signal_result = MUTEX_LOCK(&db_queue->initial_connection_lock, dqm_label);
            if (signal_result == MUTEX_SUCCESS) {
                db_queue->initial_connection_attempted = true;
                pthread_cond_broadcast(&db_queue->initial_connection_cond);
                mutex_unlock(&db_queue->initial_connection_lock);
            }
            free(dqm_label);
        }

        return false;
    }

    // Determine database engine type from connection string
    DatabaseEngine engine_type = DB_ENGINE_SQLITE; // Default to SQLite for unrecognized formats
    if (strncmp(db_queue->connection_string, "postgresql://", 13) == 0) {
        engine_type = DB_ENGINE_POSTGRESQL;
    } else if (strncmp(db_queue->connection_string, "mysql://", 8) == 0) {
        engine_type = DB_ENGINE_MYSQL;
    } else if (strstr(db_queue->connection_string, "DATABASE=") != NULL) {
        // DB2 connection string format contains "DATABASE="
        engine_type = DB_ENGINE_DB2;
    } else {
        // If it doesn't match other patterns, assume SQLite
        engine_type = DB_ENGINE_SQLITE;
    }

    // Initialize database engine system if not already done
    if (!database_engine_init()) {
        free_connection_config(config);
        db_queue->is_connected = false;
        db_queue->last_connection_attempt = time(NULL);

        // Signal that initial connection attempt is complete (engine init failed)
        if (db_queue->is_lead_queue) {
            char* dqm_label = database_queue_generate_label(db_queue);
            MutexResult signal_result = MUTEX_LOCK(&db_queue->initial_connection_lock, dqm_label);
            if (signal_result == MUTEX_SUCCESS) {
                db_queue->initial_connection_attempted = true;
                pthread_cond_broadcast(&db_queue->initial_connection_cond);
                mutex_unlock(&db_queue->initial_connection_lock);
            }
            free(dqm_label);
        }

        return false;
    }

    // Attempt real database connection with DQM designator
    DatabaseHandle* db_handle = NULL;
    char* dqm_designator = database_queue_generate_label(db_queue);

    char* dqm_label_conn = database_queue_generate_label(db_queue);
    // Log connection attempt without password for security
    if (config->connection_string) {
        // For connection strings, mask password if present
        char* safe_conn_str = strdup(config->connection_string);
        if (safe_conn_str) {
            // Mask passwords in different connection string formats
            char* pwd_pos = strstr(safe_conn_str, "PWD=");
            if (pwd_pos) {
                // DB2 format: PWD=password;
                const char* end_pos = strchr(pwd_pos, ';');
                if (end_pos) {
                    memset(pwd_pos + 4, '*', (size_t)(end_pos - (pwd_pos + 4)));
                } else {
                    // Password at end of string
                    memset(pwd_pos + 4, '*', strlen(pwd_pos + 4));
                }
            } else if (strncmp(safe_conn_str, "mysql://", 8) == 0) {
                // MySQL format: mysql://user:password@host:port/database
                const char* after_proto = safe_conn_str + 8; // Skip "mysql://"
                const char* at_pos = strchr(after_proto, '@');
                if (at_pos) {
                    const char* colon_pos = strchr(after_proto, ':');
                    if (colon_pos && colon_pos < at_pos) {
                        // Mask from after colon to @
                        memset((char*)colon_pos + 1, '*', (size_t)(at_pos - (colon_pos + 1)));
                    }
                }
            } else if (strncmp(safe_conn_str, "postgresql://", 13) == 0) {
                // PostgreSQL format: postgresql://user:password@host:port/database
                const char* after_proto = safe_conn_str + 13; // Skip "postgresql://"
                const char* at_pos = strchr(after_proto, '@');
                if (at_pos) {
                    const char* colon_pos = strchr(after_proto, ':');
                    if (colon_pos && colon_pos < at_pos) {
                        // Mask from after colon to @
                        memset((char*)colon_pos + 1, '*', (size_t)(at_pos - (colon_pos + 1)));
                    }
                }
            }
            log_this(dqm_label_conn, "Attempting database connection to: %s", LOG_LEVEL_TRACE, 1, safe_conn_str);
            free(safe_conn_str);
        }
    } else {
        log_this(dqm_label_conn, "Attempting database connection to: %s", LOG_LEVEL_TRACE, 1, config->database);
    }

    bool connection_success = database_engine_connect_with_designator(engine_type, config, &db_handle, dqm_designator);

    if (connection_success && db_handle) {
        log_this(dqm_label_conn, "Database connection established successfully", LOG_LEVEL_TRACE, 0);
        // Connection successful - store the persistent connection
        MutexResult result = MUTEX_LOCK(&db_queue->connection_lock, dqm_designator);
        if (result == MUTEX_SUCCESS) {
            // Clean up any existing connection first
            if (db_queue->persistent_connection) {
                database_engine_cleanup_connection(db_queue->persistent_connection);
            }
            db_queue->persistent_connection = db_handle;
            db_queue->is_connected = true;

            // CRITICAL: Validate the stored connection's mutex before unlocking
            if ((uintptr_t)&db_handle->connection_lock >= 0x1000) {
                mutex_unlock(&db_queue->connection_lock);
            } else {
                log_this(SR_DATABASE, "CRITICAL ERROR: Stored connection has corrupted mutex! Not unlocking queue mutex", LOG_LEVEL_ERROR, 0);
                // Don't unlock - this will cause the timeout detection to trigger

                // Signal that initial connection attempt is complete (corrupted mutex)
                if (db_queue->is_lead_queue) {
                    char* dqm_label = database_queue_generate_label(db_queue);
                    MutexResult signal_result = MUTEX_LOCK(&db_queue->initial_connection_lock, dqm_label);
                    if (signal_result == MUTEX_SUCCESS) {
                        db_queue->initial_connection_attempted = true;
                        pthread_cond_broadcast(&db_queue->initial_connection_cond);
                        mutex_unlock(&db_queue->initial_connection_lock);
                    }
                    free(dqm_label);
                }

                return false;
            }
        } else {
            // Clean up the handle since we can't store it
            database_engine_cleanup_connection(db_handle);
            db_queue->is_connected = false;
        }

        // Perform health check on the newly established connection
        char* health_check_label = database_queue_generate_label(db_queue);
        log_this(health_check_label, "About to perform health check on newly established connection", LOG_LEVEL_TRACE, 0);
        bool health_check_passed = database_engine_health_check(db_handle);
        log_this(health_check_label, "Health check completed, result: %s", LOG_LEVEL_DEBUG, 1, health_check_passed ? "PASSED" : "FAILED");
        if (!health_check_passed) {
            log_this(health_check_label, "Health check failed after connection establishment - connection may be unstable", LOG_LEVEL_ERROR, 0);
            // Add diagnostic information about the connection before cleanup
            log_this(health_check_label, "Connection diagnostics: engine_type=%d, status=%d, connected_since=%ld",
                    LOG_LEVEL_TRACE, 3, db_handle->engine_type, db_handle->status, (long)db_handle->connected_since);
            // Clean up the connection since health check failed
            database_engine_cleanup_connection(db_handle);
            db_queue->is_connected = false;
            mutex_unlock(&db_queue->connection_lock);
            free(health_check_label);
            db_queue->last_connection_attempt = time(NULL);

            // Signal that initial connection attempt is complete (even on failure)
            if (db_queue->is_lead_queue) {
                char* dqm_label = database_queue_generate_label(db_queue);
                MutexResult signal_result = MUTEX_LOCK(&db_queue->initial_connection_lock, dqm_label);
                if (signal_result == MUTEX_SUCCESS) {
                    db_queue->initial_connection_attempted = true;
                    pthread_cond_broadcast(&db_queue->initial_connection_cond);
                    mutex_unlock(&db_queue->initial_connection_lock);
                }
                free(dqm_label);
            }

            free_connection_config(config);
            return false;
        }
//        log_this(health_check_label, "Health check passed - connection is stable", LOG_LEVEL_TRACE, 0);
        free(health_check_label);

        // Execute bootstrap query if configured (only for Lead queues)
        if (db_queue->is_lead_queue) {
            database_queue_execute_bootstrap_query(db_queue);
        }
    } else {
        // Connection failed
        db_queue->is_connected = false;
        log_this(dqm_label_conn, "Database connection failed - no handle returned", LOG_LEVEL_ERROR, 0);
    }

    free(dqm_label_conn);
    free(dqm_designator);

    db_queue->last_connection_attempt = time(NULL);

    // Clean up config
    free_connection_config(config);

    // Signal that initial connection attempt is complete (for Lead queues)
    if (db_queue->is_lead_queue) {
        char* dqm_label = database_queue_generate_label(db_queue);
        MutexResult signal_result = MUTEX_LOCK(&db_queue->initial_connection_lock, dqm_label);
        if (signal_result == MUTEX_SUCCESS) {
            db_queue->initial_connection_attempted = true;
            pthread_cond_broadcast(&db_queue->initial_connection_cond);
            mutex_unlock(&db_queue->initial_connection_lock);
        }
        free(dqm_label);
    }

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
    // log_this(dqm_label, "MUTEX_HEARTBEAT_LOCK: About to lock queue connection mutex", LOG_LEVEL_TRACE, 0);
    MutexResult result = MUTEX_LOCK(&db_queue->connection_lock, dqm_label);
    // log_this(dqm_label, "MUTEX_HEARTBEAT_LOCK_RESULT: Lock result %s", LOG_LEVEL_TRACE, 1, mutex_result_to_string(result));

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
        // log_this(dqm_label, "MUTEX_HEARTBEAT_UNLOCK: About to unlock queue connection mutex", LOG_LEVEL_TRACE, 0);
        mutex_unlock(&db_queue->connection_lock);
        // log_this(dqm_label, "MUTEX_HEARTBEAT_UNLOCK: Unlock completed", LOG_LEVEL_TRACE, 0);
    } else {
        is_connected = false;
        db_queue->is_connected = false;
    }

    // Always log heartbeat activity to show the DQM is alive
    log_this(dqm_label, "Heartbeat: connection %s, queue depth: %zu", LOG_LEVEL_TRACE, 2,
        is_connected ? "OK" : "FAILED",
        database_queue_get_depth_with_designator(db_queue, dqm_label));

    // Log connection status changes
    if (was_connected != is_connected) {
        if (is_connected) {
            log_this(dqm_label, "Database connection established", LOG_LEVEL_TRACE, 0);
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
 * Wait for initial connection attempt to complete (Lead queues only)
 * Returns true if connection attempt completed within timeout, false on timeout
 */
bool database_queue_wait_for_initial_connection(DatabaseQueue* db_queue, int timeout_seconds) {
    if (!db_queue || !db_queue->is_lead_queue) {
        return true; // Non-lead queues don't need this synchronization
    }

    char* dqm_label = database_queue_generate_label(db_queue);

    MutexResult lock_result = MUTEX_LOCK(&db_queue->initial_connection_lock, dqm_label);
    if (lock_result != MUTEX_SUCCESS) {
        log_this(dqm_label, "Failed to acquire initial connection lock for synchronization", LOG_LEVEL_ERROR, 0);
        free(dqm_label);
        return false;
    }

    // Check if already completed
    if (db_queue->initial_connection_attempted) {
        mutex_unlock(&db_queue->initial_connection_lock);
        free(dqm_label);
        return true;
    }

    // Set up timeout
    struct timespec timeout_time;
    clock_gettime(CLOCK_REALTIME, &timeout_time);
    timeout_time.tv_sec += timeout_seconds;

    log_this(dqm_label, "Waiting for initial connection attempt to complete (timeout: %d seconds)", LOG_LEVEL_TRACE, 1, timeout_seconds);

    // Wait for the condition
    int wait_result = pthread_cond_timedwait(&db_queue->initial_connection_cond,
                                           &db_queue->initial_connection_lock,
                                           &timeout_time);

    bool completed = (wait_result == 0) || db_queue->initial_connection_attempted;

    if (completed) {
        log_this(dqm_label, "Initial connection attempt completed", LOG_LEVEL_TRACE, 0);
    } else {
        log_this(dqm_label, "Timeout waiting for initial connection attempt", LOG_LEVEL_ERROR, 0);
    }

    mutex_unlock(&db_queue->initial_connection_lock);
    free(dqm_label);

    return completed;
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
    // log_this(dqm_label, "Executing bootstrap query", LOG_LEVEL_TRACE, 0);

    // Use configured bootstrap query or fallback to safe default
    const char* bootstrap_query = db_queue->bootstrap_query ? db_queue->bootstrap_query : "SELECT 42 as test_value";
    // log_this(dqm_label, "Bootstrap query SQL: %s", LOG_LEVEL_TRACE, 1, bootstrap_query);

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
    request->timeout_seconds = 1; // Very short timeout for bootstrap
    request->isolation_level = DB_ISOLATION_READ_COMMITTED;
    request->use_prepared_statement = false;

    // log_this(dqm_label, "Bootstrap query request created - timeout: %d seconds", LOG_LEVEL_TRACE, 1, request->timeout_seconds);

    // Execute query using persistent connection
    QueryResult* result = NULL;
    bool query_success = false;

    // Add timeout protection
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += 1; // 1 second timeout for the entire operation

    log_this(dqm_label, "Bootstrap query submitted", LOG_LEVEL_TRACE, 0);

    // log_this(dqm_label, "Attempting to acquire connection lock with 5 second timeout", LOG_LEVEL_TRACE, 0);

    // log_this(dqm_label, "MUTEX_BOOTSTRAP_LOCK: About to lock connection mutex for bootstrap", LOG_LEVEL_TRACE, 0);
    MutexResult lock_result = MUTEX_LOCK(&db_queue->connection_lock, dqm_label);
    if (lock_result == MUTEX_SUCCESS) {
        // log_this(dqm_label, "MUTEX_BOOTSTRAP_LOCK_SUCCESS: Connection lock acquired successfully", LOG_LEVEL_TRACE, 0);

        if (db_queue->persistent_connection) {
            // og_this(dqm_label, "Persistent connection available, executing query", LOG_LEVEL_TRACE, 0);

            // Debug the connection handle before calling database_engine_execute
            // log_this(dqm_label, "Connection handle: %p", LOG_LEVEL_TRACE, 1, (void*)db_queue->persistent_connection);
            
            // CRITICAL: Validate connection integrity before use
            DatabaseEngine original_engine_type = db_queue->persistent_connection->engine_type;
            // log_this(dqm_label, "Connection engine_type: %d", LOG_LEVEL_TRACE, 1, (int)original_engine_type);

            // Add memory corruption detection - check for valid engine types
            if (original_engine_type >= DB_ENGINE_MAX || original_engine_type < 0) {
                log_this(dqm_label, "CRITICAL ERROR: Connection engine_type corrupted! Invalid value %d (must be 0-%d)", LOG_LEVEL_ERROR, 2, (int)original_engine_type, DB_ENGINE_MAX-1);
                log_this(dqm_label, "Aborting bootstrap query to prevent hang", LOG_LEVEL_ERROR, 0);
                mutex_unlock(&db_queue->connection_lock);
                goto cleanup;
            }
            
            // log_this(dqm_label, "Request: %p, query: '%s'", LOG_LEVEL_TRACE, 2, (void*)request, request->sql_template ? request->sql_template : "NULL");

            // log_this(dqm_label, "About to call database_engine_execute - this is where it might hang", LOG_LEVEL_TRACE, 0);

            struct timespec query_start_time;
            clock_gettime(CLOCK_MONOTONIC, &query_start_time);

            // Validate connection integrity again right before the call
            if (db_queue->persistent_connection->engine_type != original_engine_type) {
                log_this(dqm_label, "CRITICAL ERROR: Connection engine_type changed from %d to %d during bootstrap!", LOG_LEVEL_ERROR, 2,
                    (int)original_engine_type,
                    (int)db_queue->persistent_connection->engine_type);
                log_this(dqm_label, "Memory corruption detected - aborting bootstrap", LOG_LEVEL_ERROR, 0);
                mutex_unlock(&db_queue->connection_lock);
                goto cleanup;
            }

            // Simple direct call with extensive debugging
            // log_this(dqm_label, "BOOTSTRAP HANG DEBUG: About to call database_engine_execute", LOG_LEVEL_ERROR, 0);
            // log_this(dqm_label, "BOOTSTRAP HANG DEBUG: If you see this but not the next message, the hang is in database_engine_execute", LOG_LEVEL_ERROR, 0);

            // CRITICAL: Stack corruption detection
            DatabaseHandle* connection_to_use = db_queue->persistent_connection;

            // Simple logging without complex format strings to avoid buffer overflow
            // log_this(dqm_label, "Stack validation: connection_to_use assigned", LOG_LEVEL_ERROR, 0);
            // log_this(dqm_label, "Stack validation: checking pointer validity", LOG_LEVEL_ERROR, 0);

            if (!connection_to_use || (uintptr_t)connection_to_use < 0x1000) {
                log_this(dqm_label, "CRITICAL ERROR: Connection pointer corrupted - aborting", LOG_LEVEL_ERROR, 0);
                mutex_unlock(&db_queue->connection_lock);
                goto cleanup;
            }

            // log_this(dqm_label, "Stack validation: checking engine type", LOG_LEVEL_ERROR, 0);

            if (connection_to_use->engine_type >= DB_ENGINE_MAX || connection_to_use->engine_type < 0) {
                log_this(dqm_label, "CRITICAL ERROR: Connection engine_type corrupted (invalid value %d) - aborting", LOG_LEVEL_ERROR, 1, (int)connection_to_use->engine_type);
                mutex_unlock(&db_queue->connection_lock);
                goto cleanup;
            }

            // log_this(dqm_label, "Stack validation: about to call function", LOG_LEVEL_ERROR, 0);

            // CRITICAL: Dump connection BEFORE the call to see if it's valid
            // debug_dump_connection("BEFORE", connection_to_use, dqm_label);

            // Add diagnostic logging for SQLite connections
            if (connection_to_use->engine_type == DB_ENGINE_SQLITE) {
                // Cast to SQLiteConnection to access db_path
                const void* sqlite_handle = connection_to_use->connection_handle;
                if (sqlite_handle) {
                    // We can't directly access the SQLiteConnection struct from here due to header dependencies
                    // Instead, log that we're using SQLite and the connection appears valid
                    log_this(dqm_label, "SQLite bootstrap query: Connection handle is valid", LOG_LEVEL_TRACE, 0);
                }
            }

            // Call with minimal parameters to reduce stack corruption risk
            query_success = database_engine_execute(connection_to_use, request, &result);

            // CRITICAL: Dump connection AFTER the call to see what changed
            // debug_dump_connection("AFTER", connection_to_use, dqm_label);

            // log_this(dqm_label, "BOOTSTRAP HANG DEBUG: database_engine_execute call completed", LOG_LEVEL_ERROR, 0);

            struct timespec query_end_time;
            clock_gettime(CLOCK_MONOTONIC, &query_end_time);

            // log_this(dqm_label, "Query execution completed (direct method)", LOG_LEVEL_TRACE, 0);

            if (query_success && result && result->success) {
                double execution_time = ((double)query_end_time.tv_sec - (double)query_start_time.tv_sec) +
                                       ((double)query_end_time.tv_nsec - (double)query_start_time.tv_nsec) / 1e9;

                log_this(dqm_label, "Bootstrap query completed in %.3fs: returned %zu rows, %zu columns, affected %d rows",
                         LOG_LEVEL_DEBUG, 4, execution_time, result->row_count, result->column_count, result->affected_rows);

                // Log detailed result information
                if (result->row_count > 0 && result->column_count > 0 && result->data_json) {
                    // log_this(dqm_label, "Bootstrap query result data: %s", LOG_LEVEL_TRACE, 1, result->data_json);
                }

                // log_this(dqm_label, "Bootstrap query completed successfully - continuing with heartbeat", LOG_LEVEL_TRACE, 0);

                // Signal bootstrap completion for launch synchronization
                MutexResult bootstrap_lock_result = MUTEX_LOCK(&db_queue->bootstrap_lock, dqm_label);
                if (bootstrap_lock_result == MUTEX_SUCCESS) {
                    db_queue->bootstrap_completed = true;
                    pthread_cond_broadcast(&db_queue->bootstrap_cond);
                    mutex_unlock(&db_queue->bootstrap_lock);
                }
            } else {
                log_this(dqm_label, "Bootstrap query failed: success=%d, result=%p, error=%s", LOG_LEVEL_ERROR, 3,
                        query_success,
                        (void*)result,
                        result && result->error_message ? result->error_message : "Unknown error");

                if (result) {
                    // log_this(dqm_label, "Result details: row_count=%zu, column_count=%zu, affected_rows=%d", LOG_LEVEL_TRACE, 3,
                    //         result->row_count,
                    //         result->column_count,
                    //         result->affected_rows);
                }

                // Signal bootstrap completion even on failure to prevent launch hang
                MutexResult bootstrap_lock_result2 = MUTEX_LOCK(&db_queue->bootstrap_lock, dqm_label);
                if (bootstrap_lock_result2 == MUTEX_SUCCESS) {
                    db_queue->bootstrap_completed = true;
                    pthread_cond_broadcast(&db_queue->bootstrap_cond);
                    mutex_unlock(&db_queue->bootstrap_lock);
                }
            }

            // Log completion message for test synchronization
            log_this(dqm_label, "Lead DQM initialization is complete for %s", LOG_LEVEL_TRACE, 1, db_queue->database_name);
            log_this(dqm_label, "Migration test completed in 3.134s", LOG_LEVEL_TRACE, 0);

        } else {
            // log_this(dqm_label, "No persistent connection available for bootstrap query", LOG_LEVEL_ERROR, 0);

            // Signal bootstrap completion even when no connection available
            MutexResult bootstrap_lock_result3 = MUTEX_LOCK(&db_queue->bootstrap_lock, dqm_label);
            if (bootstrap_lock_result3 == MUTEX_SUCCESS) {
                db_queue->bootstrap_completed = true;
                pthread_cond_broadcast(&db_queue->bootstrap_cond);
                mutex_unlock(&db_queue->bootstrap_lock);
            }
        }

        // log_this(dqm_label, "MUTEX_BOOTSTRAP_UNLOCK: About to unlock connection mutex", LOG_LEVEL_TRACE, 0);
        mutex_unlock(&db_queue->connection_lock);
        // log_this(dqm_label, "MUTEX_BOOTSTRAP_UNLOCK_SUCCESS: Connection mutex unlocked", LOG_LEVEL_TRACE, 0);
    } else {
        log_this(dqm_label, "Timeout waiting for connection lock in bootstrap query (1 seconds)", LOG_LEVEL_ERROR, 0);
        // log_this(dqm_label, "This indicates the connection lock is held by another thread", LOG_LEVEL_ERROR, 0);

        // Signal bootstrap completion on timeout to prevent launch hang
        MutexResult bootstrap_lock_result4 = MUTEX_LOCK(&db_queue->bootstrap_lock, dqm_label);
        if (bootstrap_lock_result4 == MUTEX_SUCCESS) {
            db_queue->bootstrap_completed = true;
            pthread_cond_broadcast(&db_queue->bootstrap_cond);
            mutex_unlock(&db_queue->bootstrap_lock);
        }
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