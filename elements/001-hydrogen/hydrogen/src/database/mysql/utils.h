/*
 * MySQL Database Engine - Utility Functions Header
 *
 * Header file for MySQL utility functions.
 */

#ifndef DATABASE_ENGINE_MYSQL_UTILS_H
#define DATABASE_ENGINE_MYSQL_UTILS_H

#include "../database.h"

// Utility functions
char* mysql_get_connection_string(const ConnectionConfig* config);
bool mysql_validate_connection_string(const char* connection_string);
char* mysql_escape_string(const DatabaseHandle* connection, const char* input);
int mysql_json_escape_string(const char* input, char* output, size_t output_size);

#endif // DATABASE_ENGINE_MYSQL_UTILS_H