/*
 * Database Connection String Parsing Header
 *
 * Provides functions for parsing and managing database connection strings
 * across different database engines (PostgreSQL, MySQL, SQLite, DB2).
 */

#ifndef DATABASE_CONNSTRING_H
#define DATABASE_CONNSTRING_H

#include "../hydrogen.h"

// Forward declaration from database.h
typedef struct ConnectionConfig ConnectionConfig;

/*
 * Parse connection string into ConnectionConfig
 * Supports formats like: postgresql://user:pass@host:port/database
 * Returns NULL on failure
 */
ConnectionConfig* parse_connection_string(const char* conn_string);

/*
 * Free ConnectionConfig and all its allocated strings
 */
void free_connection_config(ConnectionConfig* config);

#endif // DATABASE_CONNSTRING_H