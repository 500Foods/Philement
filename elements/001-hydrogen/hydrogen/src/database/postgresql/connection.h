/*
 * PostgreSQL Database Engine - Connection Management Header
 *
 * Header file for PostgreSQL connection management functions.
 */

#ifndef DATABASE_ENGINE_POSTGRESQL_CONNECTION_H
#define DATABASE_ENGINE_POSTGRESQL_CONNECTION_H

#include "../database.h"
#include "types.h"

// PostgreSQL-specific connection structure
typedef struct PostgresConnection {
    void* connection;  // PGconn* loaded dynamically
    bool in_transaction;
    struct PreparedStatementCache* prepared_statements;
} PostgresConnection;

// Prepared statement cache structure
typedef struct PreparedStatementCache {
    char** names;
    size_t count;
    size_t capacity;
    pthread_mutex_t lock;
} PreparedStatementCache;

// Function prototypes for connection management
bool postgresql_connect(ConnectionConfig* config, DatabaseHandle** connection, const char* designator);
bool postgresql_disconnect(DatabaseHandle* connection);
bool postgresql_health_check(DatabaseHandle* connection);
bool postgresql_reset_connection(DatabaseHandle* connection);

// Library loading function
bool load_libpq_functions(const char* designator);

// Utility functions for prepared statement cache
PreparedStatementCache* create_prepared_statement_cache(void);
void destroy_prepared_statement_cache(PreparedStatementCache* cache);
bool add_prepared_statement(PreparedStatementCache* cache, const char* name);
bool remove_prepared_statement(PreparedStatementCache* cache, const char* name);

// Timeout utility
bool check_timeout_expired(time_t start_time, int timeout_seconds);

#endif // DATABASE_ENGINE_POSTGRESQL_CONNECTION_H