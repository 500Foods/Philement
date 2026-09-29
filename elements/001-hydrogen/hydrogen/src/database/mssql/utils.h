/*
 * MSSQL Database Engine - Utility Functions Header
 *
 * Header file for MSSQL utility functions.
 */

#ifndef DATABASE_ENGINE_MSSQL_UTILS_H
#define DATABASE_ENGINE_MSSQL_UTILS_H

#include <src/database/database.h>

// Utility functions
char* mssql_get_connection_string(const ConnectionConfig* config);
bool mssql_validate_connection_string(const char* connection_string);
void mssql_parse_connection_string(const char* conn_str, char* server, char* username, char* password, char* database);
char* mssql_escape_string(const DatabaseHandle* connection, const char* input);

#endif // DATABASE_ENGINE_MSSQL_UTILS_H
