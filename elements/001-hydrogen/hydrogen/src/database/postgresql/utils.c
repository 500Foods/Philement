/*
 * PostgreSQL Database Engine - Utility Functions Implementation
 *
 * Implements PostgreSQL utility functions.
 */

#include <src/hydrogen.h>
#include "../database.h"
#include "types.h"
#include "connection.h"
#include "utils.h"

// External declarations for libpq function pointers (defined in connection.c)
extern PQescapeStringConn_t PQescapeStringConn_ptr;

// Utility Functions
char* postgresql_get_connection_string(const ConnectionConfig* config) {
    if (!config) return NULL;

    char* conn_str = calloc(1, 1024);
    if (!conn_str) return NULL;

    if (config->connection_string) {
        strcpy(conn_str, config->connection_string);
    } else {
        snprintf(conn_str, 1024,
                 "postgresql://%s:%s@%s:%d/%s",
                 config->username ? config->username : "",
                 config->password ? config->password : "",
                 config->host ? config->host : "localhost",
                 config->port ? config->port : 5432,
                 config->database ? config->database : "postgres");
    }

    return conn_str;
}

bool postgresql_validate_connection_string(const char* connection_string) {
    if (!connection_string) return false;

    // Basic validation - check for postgresql:// prefix
    return strncmp(connection_string, "postgresql://", 13) == 0;
}

char* postgresql_escape_string(const DatabaseHandle* connection, const char* input) {
    if (!connection || !input || connection->engine_type != DB_ENGINE_POSTGRESQL) {
        return NULL;
    }

    PostgresConnection* pg_conn = (PostgresConnection*)connection->connection_handle;
    if (!pg_conn || !pg_conn->connection) {
        return NULL;
    }

    // PostgreSQL escaping
    size_t escaped_len = strlen(input) * 2 + 1; // Worst case
    char* escaped = calloc(1, escaped_len);
    if (!escaped) return NULL;

    PQescapeStringConn_ptr(pg_conn->connection, escaped, input, strlen(input), NULL);
    return escaped;
}