/*
 * DB2 Database Engine - Utility Functions Header
 *
 * Header file for DB2 utility functions.
 */

#ifndef DATABASE_ENGINE_DB2_UTILS_H
#define DATABASE_ENGINE_DB2_UTILS_H

#include <src/database/database.h>

// Utility functions
char* db2_get_connection_string(const ConnectionConfig* config);
bool db2_validate_connection_string(const char* connection_string);
void db2_parse_connection_string(const char* conn_str, char* server, char* username, char* password, char* database);
char* db2_escape_string(const DatabaseHandle* connection, const char* input);

#endif // DATABASE_ENGINE_DB2_UTILS_H