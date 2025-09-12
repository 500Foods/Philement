/*
 * PostgreSQL Database Engine - Utility Functions Header
 *
 * Header file for PostgreSQL utility functions.
 */

#ifndef DATABASE_ENGINE_POSTGRESQL_UTILS_H
#define DATABASE_ENGINE_POSTGRESQL_UTILS_H

#include <stdbool.h>

#include "../database.h"

// Function prototypes for utility functions
char* postgresql_get_connection_string(ConnectionConfig* config);
bool postgresql_validate_connection_string(const char* connection_string);
char* postgresql_escape_string(DatabaseHandle* connection, const char* input);

#endif // DATABASE_ENGINE_POSTGRESQL_UTILS_H