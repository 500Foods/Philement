// Database Connection String Management Implementation
// Connection pooling infrastructure for database subsystem

#include <src/hydrogen.h>
#include <src/database/database_connstring.h>
#include <src/database/database_engine.h>
#include <src/database/database.h>
#include <src/mutex/mutex.h>

// Global connection pool manager instance
static ConnectionPoolManager* global_connection_pool_manager = NULL;
static pthread_mutex_t global_pool_manager_lock = PTHREAD_MUTEX_INITIALIZER;

/*
 * Create a connection pool manager
 */
ConnectionPoolManager* connection_pool_manager_create(size_t max_pools) {
    ConnectionPoolManager* manager = calloc(1, sizeof(ConnectionPoolManager));
    if (!manager) {
        return NULL;
    }

    manager->pools = calloc(max_pools, sizeof(ConnectionPool*));
    if (!manager->pools) {
        free(manager);
        return NULL;
    }

    manager->max_pools = max_pools;
    manager->pool_count = 0;

    if (pthread_mutex_init(&manager->manager_lock, NULL) != 0) {
        free(manager->pools);
        free(manager);
        return NULL;
    }

    manager->initialized = true;
    return manager;
}

/*
 * Destroy a connection pool manager
 */
void connection_pool_manager_destroy(ConnectionPoolManager* manager) {
    if (!manager) return;

    MutexResult lock_result = MUTEX_LOCK(&manager->manager_lock, SR_DATABASE);
    if (lock_result == MUTEX_SUCCESS) {
        for (size_t i = 0; i < manager->pool_count; i++) {
            if (manager->pools[i]) {
                connection_pool_destroy(manager->pools[i]);
            }
        }
        free(manager->pools);
        mutex_unlock(&manager->manager_lock);
    }

    pthread_mutex_destroy(&manager->manager_lock);
    free(manager);
}

/*
 * Add a connection pool to the manager
 */
bool connection_pool_manager_add_pool(ConnectionPoolManager* manager, ConnectionPool* pool) {
    if (!manager || !pool) return false;

    MutexResult lock_result = MUTEX_LOCK(&manager->manager_lock, SR_DATABASE);
    if (lock_result != MUTEX_SUCCESS) {
        return false;
    }

    if (manager->pool_count >= manager->max_pools) {
        mutex_unlock(&manager->manager_lock);
        return false;
    }

    manager->pools[manager->pool_count] = pool;
    manager->pool_count++;

    mutex_unlock(&manager->manager_lock);
    return true;
}

/*
 * Get a connection pool by database name
 */
ConnectionPool* connection_pool_manager_get_pool(ConnectionPoolManager* manager, const char* database_name) {
    if (!manager || !database_name) return NULL;

    MutexResult lock_result = MUTEX_LOCK(&manager->manager_lock, SR_DATABASE);
    if (lock_result != MUTEX_SUCCESS) {
        return NULL;
    }

    ConnectionPool* found_pool = NULL;
    for (size_t i = 0; i < manager->pool_count; i++) {
        if (manager->pools[i] && strcmp(manager->pools[i]->database_name, database_name) == 0) {
            found_pool = manager->pools[i];
            break;
        }
    }

    mutex_unlock(&manager->manager_lock);
    return found_pool;
}

/*
 * Create a connection pool for a database
 */
ConnectionPool* connection_pool_create(const char* database_name, DatabaseEngine engine_type, size_t max_pool_size) {
    ConnectionPool* pool = calloc(1, sizeof(ConnectionPool));
    if (!pool) return NULL;

    pool->database_name = strdup(database_name);
    if (!pool->database_name) {
        free(pool);
        return NULL;
    }

    pool->engine_type = engine_type;
    pool->max_pool_size = max_pool_size;
    pool->pool_size = 0;
    pool->active_connections = 0;

    pool->connections = calloc(max_pool_size, sizeof(ConnectionPoolEntry*));
    if (!pool->connections) {
        free(pool->database_name);
        free(pool);
        return NULL;
    }

    if (pthread_mutex_init(&pool->pool_lock, NULL) != 0) {
        free(pool->connections);
        free(pool->database_name);
        free(pool);
        return NULL;
    }

    pool->initialized = true;
    return pool;
}

/*
 * Destroy a connection pool
 */
void connection_pool_destroy(ConnectionPool* pool) {
    if (!pool) return;

    MutexResult lock_result = MUTEX_LOCK(&pool->pool_lock, SR_DATABASE);
    if (lock_result == MUTEX_SUCCESS) {
        for (size_t i = 0; i < pool->pool_size; i++) {
            if (pool->connections[i]) {
                if (pool->connections[i]->connection) {
                    database_engine_cleanup_connection(pool->connections[i]->connection);
                }
                free(pool->connections[i]->connection_string_hash);
                free(pool->connections[i]);
            }
        }
        free(pool->connections);
        mutex_unlock(&pool->pool_lock);
    }

    pthread_mutex_destroy(&pool->pool_lock);
    free(pool->database_name);
    free(pool);
}

/*
 * Acquire a connection from the pool
 */
DatabaseHandle* connection_pool_acquire_connection(ConnectionPool* pool, const char* connection_string) {
    if (!pool || !connection_string) return NULL;

    // Generate hash of connection string for validation
    char conn_hash[64];
    get_stmt_hash("CONN", connection_string, strlen(connection_string), conn_hash);

    MutexResult lock_result = MUTEX_LOCK(&pool->pool_lock, SR_DATABASE);
    if (lock_result != MUTEX_SUCCESS) {
        return NULL;
    }

    // First, try to find an available connection
    for (size_t i = 0; i < pool->pool_size; i++) {
        ConnectionPoolEntry* entry = pool->connections[i];
        if (entry && !entry->in_use && strcmp(entry->connection_string_hash, conn_hash) == 0) {
            entry->in_use = true;
            entry->last_used = time(NULL);
            pool->active_connections++;
            mutex_unlock(&pool->pool_lock);
            return entry->connection;
        }
    }

    // No available connection found, try to create a new one if under limit
    if (pool->pool_size < pool->max_pool_size) {
        ConnectionPoolEntry* new_entry = calloc(1, sizeof(ConnectionPoolEntry));
        if (new_entry) {
            new_entry->connection_string_hash = strdup(conn_hash);
            if (new_entry->connection_string_hash) {
                // Create new connection
                ConnectionConfig* temp_config = parse_connection_string(connection_string);
                DatabaseHandle* new_conn = NULL;
                if (temp_config) {
                    database_engine_connect_with_designator(pool->engine_type, temp_config, &new_conn, SR_DATABASE);
                    free_connection_config(temp_config);
                }
                if (new_conn) {
                    new_entry->connection = new_conn;
                    new_entry->in_use = true;
                    new_entry->created_at = time(NULL);
                    new_entry->last_used = time(NULL);

                    pool->connections[pool->pool_size] = new_entry;
                    pool->pool_size++;
                    pool->active_connections++;

                    mutex_unlock(&pool->pool_lock);
                    return new_conn;
                }
                free(new_entry->connection_string_hash);
            }
            free(new_entry);
        }
    }

    mutex_unlock(&pool->pool_lock);
    return NULL; // Pool exhausted or creation failed
}

/*
 * Release a connection back to the pool
 */
bool connection_pool_release_connection(ConnectionPool* pool, const DatabaseHandle* connection) {
    if (!pool || !connection) return false;

    MutexResult lock_result = MUTEX_LOCK(&pool->pool_lock, SR_DATABASE);
    if (lock_result != MUTEX_SUCCESS) {
        return false;
    }

    // Find the connection in the pool
    for (size_t i = 0; i < pool->pool_size; i++) {
        ConnectionPoolEntry* entry = pool->connections[i];
        if (entry && entry->connection == connection && entry->in_use) {
            entry->in_use = false;
            entry->last_used = time(NULL);
            pool->active_connections--;
            mutex_unlock(&pool->pool_lock);
            return true;
        }
    }

    mutex_unlock(&pool->pool_lock);
    return false; // Connection not found in pool
}

/*
 * Clean up idle connections in the pool
 */
void connection_pool_cleanup_idle(ConnectionPool* pool, time_t max_idle_seconds) {
    if (!pool) return;

    time_t now = time(NULL);

    MutexResult lock_result = MUTEX_LOCK(&pool->pool_lock, SR_DATABASE);
    if (lock_result != MUTEX_SUCCESS) {
        return;
    }

    // Iterate backwards to safely remove entries
    for (size_t i = pool->pool_size; i > 0; i--) {
        size_t index = i - 1;
        ConnectionPoolEntry* entry = pool->connections[index];
        if (entry && !entry->in_use && (now - entry->last_used) > max_idle_seconds) {
            // Close and remove idle connection
            if (entry->connection) {
                database_engine_cleanup_connection(entry->connection);
            }
            free(entry->connection_string_hash);
            free(entry);

            // Compact the array
            for (size_t j = index; j < pool->pool_size - 1; j++) {
                pool->connections[j] = pool->connections[j + 1];
            }
            pool->connections[pool->pool_size - 1] = NULL;
            pool->pool_size--;
        }
    }

    mutex_unlock(&pool->pool_lock);
}

/*
 * Initialize global connection pool manager
 */
bool connection_pool_system_init(size_t max_pools) {
    MutexResult lock_result = MUTEX_LOCK(&global_pool_manager_lock, SR_DATABASE);
    if (lock_result != MUTEX_SUCCESS) {
        return false;
    }

    if (global_connection_pool_manager) {
        mutex_unlock(&global_pool_manager_lock);
        return true; // Already initialized
    }

    global_connection_pool_manager = connection_pool_manager_create(max_pools);
    if (!global_connection_pool_manager) {
        mutex_unlock(&global_pool_manager_lock);
        return false;
    }

    mutex_unlock(&global_pool_manager_lock);
    return true;
}

/*
 * Get global connection pool manager
 */
ConnectionPoolManager* connection_pool_get_global_manager(void) {
    MutexResult lock_result = MUTEX_LOCK(&global_pool_manager_lock, SR_DATABASE);
    if (lock_result != MUTEX_SUCCESS) {
        return NULL;
    }

    ConnectionPoolManager* manager = global_connection_pool_manager;
    mutex_unlock(&global_pool_manager_lock);
    return manager;
}

/*
 * Parse a connection string into ConnectionConfig
 * Supports PostgreSQL, MySQL, SQLite, and DB2 connection string formats
 */
ConnectionConfig* parse_connection_string(const char* connection_string) {
    if (!connection_string) return NULL;

    ConnectionConfig* config = calloc(1, sizeof(ConnectionConfig));
    if (!config) return NULL;

    // Default values
    config->port = 0;  // Engine-specific defaults will be set

    // Parse based on connection string format
    if (strstr(connection_string, "postgresql://") == connection_string) {
        // PostgreSQL format: postgresql://user:password@host:port/database?sslmode=require
        config->port = 5432;  // PostgreSQL default

        // Basic parsing - extract host, port, database
        char* temp = strdup(connection_string + 13);  // Skip "postgresql://"
        if (!temp) {
            free(config);
            return NULL;
        }

        char* at_pos = strchr(temp, '@');
        if (at_pos) {
            *at_pos = '\0';
            // Parse user:password (we'll skip detailed parsing for now)
            config->username = strdup(temp);

            char* host_start = at_pos + 1;
            char* slash_pos = strchr(host_start, '/');
            if (slash_pos) {
                *slash_pos = '\0';
                config->database = strdup(slash_pos + 1);

                // Parse host:port
                char* colon_pos = strchr(host_start, ':');
                if (colon_pos) {
                    *colon_pos = '\0';
                    config->host = strdup(host_start);
                    config->port = atoi(colon_pos + 1);
                } else {
                    config->host = strdup(host_start);
                }
            }
        }
        free(temp);

    } else if (strstr(connection_string, "mysql://") == connection_string) {
        // MySQL format: mysql://user:password@host:port/database
        config->port = 3306;  // MySQL default

        // Similar parsing logic as PostgreSQL
        char* temp = strdup(connection_string + 8);  // Skip "mysql://"
        if (!temp) {
            free(config);
            return NULL;
        }

        char* at_pos = strchr(temp, '@');
        if (at_pos) {
            *at_pos = '\0';
            config->username = strdup(temp);

            char* host_start = at_pos + 1;
            char* slash_pos = strchr(host_start, '/');
            if (slash_pos) {
                *slash_pos = '\0';
                config->database = strdup(slash_pos + 1);

                char* colon_pos = strchr(host_start, ':');
                if (colon_pos) {
                    *colon_pos = '\0';
                    config->host = strdup(host_start);
                    config->port = atoi(colon_pos + 1);
                } else {
                    config->host = strdup(host_start);
                }
            }
        }
        free(temp);

    } else if (strstr(connection_string, ".db") || strcmp(connection_string, ":memory:") == 0) {
        // SQLite format: /path/to/database.db or :memory:
        config->database = strdup(connection_string);

    } else if (strstr(connection_string, "DRIVER={IBM DB2 ODBC DRIVER}") == connection_string) {
        // DB2 ODBC format: DRIVER={IBM DB2 ODBC DRIVER};DATABASE=database;HOSTNAME=host;PORT=port;PROTOCOL=TCPIP;UID=username;PWD=password;
        // Store the entire connection string as-is for DB2
        config->connection_string = strdup(connection_string);
        // Parse key components for convenience
        char* db_pos = strstr(connection_string, "DATABASE=");
        if (db_pos) {
            db_pos += 9; // Skip "DATABASE="
            const char* end_pos = strchr(db_pos, ';');
            if (end_pos) {
                size_t len = (size_t)(end_pos - db_pos);
                config->database = calloc(1, len + 1);
                if (config->database) {
                    strncpy(config->database, db_pos, len);
                }
            } else {
                config->database = strdup(db_pos);
            }
        }
        const char* host_pos = strstr(connection_string, "HOSTNAME=");
        if (host_pos) {
            host_pos += 9; // Skip "HOSTNAME="
            const char* end_pos = strchr(host_pos, ';');
            if (end_pos) {
                size_t len = (size_t)(end_pos - host_pos);
                config->host = calloc(1, len + 1);
                if (config->host) {
                    strncpy(config->host, host_pos, len);
                }
            } else {
                config->host = strdup(host_pos);
            }
        }
        char* port_pos = strstr(connection_string, "PORT=");
        if (port_pos) {
            port_pos += 5; // Skip "PORT="
            char* end_pos = strchr(port_pos, ';');
            if (end_pos) {
                *end_pos = '\0';
                config->port = atoi(port_pos);
                *end_pos = ';'; // Restore
            } else {
                config->port = atoi(port_pos);
            }
        }
        const char* uid_pos = strstr(connection_string, "UID=");
        if (uid_pos) {
            uid_pos += 4; // Skip "UID="
            const char* end_pos = strchr(uid_pos, ';');
            if (end_pos) {
                size_t len = (size_t)(end_pos - uid_pos);
                config->username = calloc(1, len + 1);
                if (config->username) {
                    strncpy(config->username, uid_pos, len);
                }
            } else {
                config->username = strdup(uid_pos);
            }
        }
        const char* pwd_pos = strstr(connection_string, "PWD=");
        if (pwd_pos) {
            pwd_pos += 4; // Skip "PWD="
            const char* end_pos = strchr(pwd_pos, ';');
            if (end_pos) {
                size_t len = (size_t)(end_pos - pwd_pos);
                config->password = calloc(1, len + 1);
                if (config->password) {
                    strncpy(config->password, pwd_pos, len);
                }
            } else {
                config->password = strdup(pwd_pos);
            }
        }
    } else {
        // Assume other format - store as-is
        config->database = strdup(connection_string);
    }

    return config;
}

/*
 * Build connection string from engine and config
 */
char* database_build_connection_string(const char* engine, const DatabaseConnection* conn_config) {
    if (!engine || !conn_config) return NULL;

    // Get engine interface to build connection string
    DatabaseEngineInterface* engine_interface = database_get_engine_interface(engine);
    if (!engine_interface) {
        return NULL;
    }

    // Create ConnectionConfig from DatabaseConnection
    ConnectionConfig config = {
        .host = conn_config->host ? strdup(conn_config->host) : NULL,
        .port = conn_config->port ? atoi(conn_config->port) : 0,
        .database = conn_config->database ? strdup(conn_config->database) : NULL,
        .username = conn_config->user ? strdup(conn_config->user) : NULL,
        .password = conn_config->pass ? strdup(conn_config->pass) : NULL,
        .connection_string = NULL,  // Will be built by engine
        .timeout_seconds = 30,  // Default timeout
        .ssl_enabled = false,   // Default SSL disabled
        .ssl_cert_path = NULL,
        .ssl_key_path = NULL,
        .ssl_ca_path = NULL,
        .prepared_statement_cache_size = conn_config->prepared_statement_cache_size
    };

    // Use engine to build connection string
    char* conn_str = engine_interface->get_connection_string(&config);

    // Clean up temporary config
    free(config.host);
    free(config.database);
    free(config.username);
    free(config.password);
    free(config.ssl_cert_path);
    free(config.ssl_key_path);
    free(config.ssl_ca_path);

    return conn_str;
}

/*
 * Free a ConnectionConfig structure
 */
void free_connection_config(ConnectionConfig* config) {
    if (!config) return;

    free(config->host);
    free(config->database);
    free(config->username);
    free(config->password);
    free(config->connection_string);
    free(config->ssl_cert_path);
    free(config->ssl_key_path);
    free(config->ssl_ca_path);
    free(config);
}