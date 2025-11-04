// Database Connection String Management Header
// Infrastructure for connection pooling and management

#ifndef DATABASE_CONNSTRING_H
#define DATABASE_CONNSTRING_H

#include <src/hydrogen.h>
#include <src/database/database_types.h>
#include <src/database/database.h>

// Forward declarations to avoid circular dependencies
typedef struct DatabaseHandle DatabaseHandle;

// Connection configuration structure (forward declared in database.h)

// Connection pool entry
typedef struct ConnectionPoolEntry {
    DatabaseHandle* connection;
    volatile bool in_use;
    volatile time_t last_used;
    volatile time_t created_at;
    char* connection_string_hash;  // For validation
} ConnectionPoolEntry;

// Connection pool for a database
typedef struct ConnectionPool {
    char* database_name;
    DatabaseEngine engine_type;
    ConnectionPoolEntry** connections;
    size_t pool_size;
    size_t max_pool_size;
    volatile size_t active_connections;
    pthread_mutex_t pool_lock;
    volatile bool initialized;
} ConnectionPool;

// Global connection pool manager
typedef struct ConnectionPoolManager {
    ConnectionPool** pools;
    size_t pool_count;
    size_t max_pools;
    pthread_mutex_t manager_lock;
    volatile bool initialized;
} ConnectionPoolManager;

// Connection string parsing functions
ConnectionConfig* parse_connection_string(const char* connection_string);
void free_connection_config(ConnectionConfig* config);

// Forward declaration of ConnectionConfig structure (defined in database.h)

// Connection pool system functions
bool connection_pool_system_init(size_t max_pools);
ConnectionPoolManager* connection_pool_get_global_manager(void);

// Function prototypes
ConnectionPoolManager* connection_pool_manager_create(size_t max_pools);
void connection_pool_manager_destroy(ConnectionPoolManager* manager);
bool connection_pool_manager_add_pool(ConnectionPoolManager* manager, ConnectionPool* pool);
ConnectionPool* connection_pool_manager_get_pool(ConnectionPoolManager* manager, const char* database_name);

ConnectionPool* connection_pool_create(const char* database_name, DatabaseEngine engine_type, size_t max_pool_size);
void connection_pool_destroy(ConnectionPool* pool);
DatabaseHandle* connection_pool_acquire_connection(ConnectionPool* pool, const char* connection_string);
bool connection_pool_release_connection(ConnectionPool* pool, const DatabaseHandle* connection);
void connection_pool_cleanup_idle(ConnectionPool* pool, time_t max_idle_seconds);

#endif // DATABASE_CONNSTRING_H