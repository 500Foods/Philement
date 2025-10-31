/*
 * Database Queue Heartbeat Functions
 *
 * Implements heartbeat monitoring and connection management for the Hydrogen database subsystem.
 * Split from database_queue.c for better maintainability.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/database/database_connstring.h>
#include <src/database/database_bootstrap.h>
#include <src/database/database_pending.h>
#include <src/database/sqlite/types.h>

// Local includes
#include "dbqueue.h"

// External references
extern volatile sig_atomic_t database_stopping;

/*
 * Determine database engine type from connection string
 */
DatabaseEngine database_queue_determine_engine_type(const char* connection_string) {
    if (!connection_string) return DB_ENGINE_SQLITE;

    if (strncmp(connection_string, "postgresql://", 13) == 0) {
        return DB_ENGINE_POSTGRESQL;
    } else if (strncmp(connection_string, "mysql://", 8) == 0) {
        return DB_ENGINE_MYSQL;
    } else if (strstr(connection_string, "DATABASE=") != NULL) {
        // DB2 connection string format contains "DATABASE="
        return DB_ENGINE_DB2;
    } else {
        // If it doesn't match other patterns, assume SQLite
        return DB_ENGINE_SQLITE;
    }
}

/*
 * Mask passwords in connection string for logging
 */
char* database_queue_mask_connection_string(const char* connection_string) {
    if (!connection_string) return NULL;

    char* safe_conn_str = strdup(connection_string);
    if (!safe_conn_str) return NULL;

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

    return safe_conn_str;
}

/*
 * Signal that initial connection attempt is complete for lead queues
 */
void database_queue_signal_initial_connection_complete(DatabaseQueue* db_queue) {
    if (!db_queue || !db_queue->is_lead_queue) return;

    char* dqm_label = database_queue_generate_label(db_queue);
    MutexResult signal_result = MUTEX_LOCK(&db_queue->initial_connection_lock, dqm_label);
    if (signal_result == MUTEX_SUCCESS) {
        db_queue->initial_connection_attempted = true;
        pthread_cond_broadcast(&db_queue->initial_connection_cond);
        mutex_unlock(&db_queue->initial_connection_lock);
    }
    free(dqm_label);
}

/*
 * Handle connection success - store connection and perform health check
 */
bool database_queue_handle_connection_success(DatabaseQueue* db_queue, DatabaseHandle* db_handle, ConnectionConfig* config) {
    char* dqm_designator = database_queue_generate_label(db_queue);

    // Store the persistent connection
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
            database_queue_signal_initial_connection_complete(db_queue);
            free(dqm_designator);
            return false;
        }
    } else {
        // Clean up the handle since we can't store it
        database_engine_cleanup_connection(db_handle);
        db_queue->is_connected = false;
        free(dqm_designator);
        return false;
    }

    // Perform health check on the newly established connection
    char* health_check_label = database_queue_generate_label(db_queue);
    // log_this(health_check_label, "About to perform health check on newly established connection", LOG_LEVEL_TRACE, 0);
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
        database_queue_signal_initial_connection_complete(db_queue);
        free_connection_config(config);
        free(dqm_designator);
        return false;
    }

    free(health_check_label);

    // Execute bootstrap query if configured (only for Lead queues on reconnection)
    // For initial connection, skip bootstrap here - it will be run by the Lead DQM conductor
    // after migration validation sets latest_available_migration correctly
    if (db_queue->is_lead_queue && db_queue->bootstrap_completed) {
        // This is a reconnection - re-run bootstrap query
        database_queue_execute_bootstrap_query(db_queue);
    }
    // For initial connection, bootstrap_completed is false, so skip - the conductor will handle it

    free(dqm_designator);
    return true;
}

/*
 * Perform the actual database connection attempt
 */
bool database_queue_perform_connection_attempt(DatabaseQueue* db_queue, ConnectionConfig* config, DatabaseEngine engine_type) {
    // Attempt real database connection with DQM designator
    DatabaseHandle* db_handle = NULL;
    char* dqm_designator = database_queue_generate_label(db_queue);
    char* dqm_label_conn = database_queue_generate_label(db_queue);

    // Log connection attempt without password for security
    if (config->connection_string) {
        char* safe_conn_str = database_queue_mask_connection_string(config->connection_string);
        if (safe_conn_str) {
            log_this(dqm_label_conn, "Attempting database connection to: %s", LOG_LEVEL_TRACE, 1, safe_conn_str);
            free(safe_conn_str);
        }
    } else {
        log_this(dqm_label_conn, "Attempting database connection to: %s", LOG_LEVEL_TRACE, 1, config->database);
    }

    bool connection_success = database_engine_connect_with_designator(engine_type, config, &db_handle, dqm_designator);

    if (connection_success && db_handle) {
        log_this(dqm_label_conn, "Database connection established successfully", LOG_LEVEL_DEBUG, 0);
        bool success = database_queue_handle_connection_success(db_queue, db_handle, config);
        free(dqm_label_conn);
        free(dqm_designator);
        return success;
    } else {
        // Connection failed
        db_queue->is_connected = false;
        log_this(dqm_label_conn, "Database connection failed - no handle returned", LOG_LEVEL_ERROR, 0);
        free(dqm_label_conn);
        free(dqm_designator);
        return false;
    }
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
        char* safe_conn_str = database_queue_mask_connection_string(db_queue->connection_string);
        if (safe_conn_str) {
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
        database_queue_signal_initial_connection_complete(db_queue);
        return false;
    }

    // Determine database engine type from connection string
    DatabaseEngine engine_type = database_queue_determine_engine_type(db_queue->connection_string);

    // Initialize database engine system if not already done
    if (!database_engine_init()) {
        free_connection_config(config);
        db_queue->is_connected = false;
        db_queue->last_connection_attempt = time(NULL);
        database_queue_signal_initial_connection_complete(db_queue);
        return false;
    }

    // Perform the connection attempt
    bool success = database_queue_perform_connection_attempt(db_queue, config, engine_type);

    db_queue->last_connection_attempt = time(NULL);

    // Clean up config
    free_connection_config(config);

    // Signal that initial connection attempt is complete (for Lead queues)
    database_queue_signal_initial_connection_complete(db_queue);

    return success;
}

/*
 * Perform heartbeat operations
 */
void database_queue_perform_heartbeat(DatabaseQueue* db_queue) {
    if (!db_queue) return;

    // Exit quickly if shutdown is in progress
    if (db_queue->shutdown_requested || database_stopping) {
        return;
    }

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

    // Periodic cleanup of expired pending results
    PendingResultManager* pending_mgr = get_pending_result_manager();
    if (pending_mgr) {
        size_t cleaned = pending_result_cleanup_expired(pending_mgr);
        if (cleaned > 0) {
            char* cleanup_label = database_queue_generate_label(db_queue);
            log_this(cleanup_label, "Cleaned up %zu expired pending results", LOG_LEVEL_DEBUG, 1, cleaned);
            free(cleanup_label);
        }
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
