/*
 * MariaDB Database Engine - Utility Functions Header
 *
 * Header file for MariaDB utility functions.
 */

#ifndef DATABASE_ENGINE_MARIADB_UTILS_H
#define DATABASE_ENGINE_MARIADB_UTILS_H

#include <src/database/database.h>

// Utility functions
char* mariadb_get_connection_string(const ConnectionConfig* config);
bool mariadb_validate_connection_string(const char* connection_string);
char* mariadb_h_escape_string(const DatabaseHandle* connection, const char* input);
int mariadb_json_escape_string(const char* input, char* output, size_t output_size);

#endif // DATABASE_ENGINE_MARIADB_UTILS_H
