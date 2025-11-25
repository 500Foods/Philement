/*
 * PostgreSQL Database Engine - Prepared Statement Management Header
 *
 * Header file for PostgreSQL prepared statement management functions.
 */

#ifndef DATABASE_ENGINE_POSTGRESQL_PREPARED_H
#define DATABASE_ENGINE_POSTGRESQL_PREPARED_H

#include <stdbool.h>

#include <src/database/database.h>
#include "connection.h"

// Function prototypes for prepared statement management
bool postgresql_prepare_statement(DatabaseHandle* connection, const char* name, const char* sql, PreparedStatement** stmt, bool add_to_cache);
bool postgresql_unprepare_statement(DatabaseHandle* connection, PreparedStatement* stmt);

#endif // DATABASE_ENGINE_POSTGRESQL_PREPARED_H