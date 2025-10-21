/*
 * SQLite Database Engine - Utility Functions Header
 *
 * Header file for SQLite utility functions.
 */

#ifndef SQLITE_UTILS_H
#define SQLITE_UTILS_H

#include <src/database/database.h>

// Utility functions
char* sqlite_get_connection_string(const ConnectionConfig* config);
bool sqlite_validate_connection_string(const char* connection_string);
char* sqlite_escape_string(const DatabaseHandle* connection, const char* input);

#endif // SQLITE_UTILS_H