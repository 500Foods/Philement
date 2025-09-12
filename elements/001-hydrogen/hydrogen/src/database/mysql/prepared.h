/*
 * MySQL Database Engine - Prepared Statement Management Header
 *
 * Header file for MySQL prepared statement management functions.
 */

#ifndef DATABASE_ENGINE_MYSQL_PREPARED_H
#define DATABASE_ENGINE_MYSQL_PREPARED_H

#include "../database.h"
#include "types.h"

// Prepared statement management
bool mysql_prepare_statement(DatabaseHandle* connection, const char* name, const char* sql, PreparedStatement** stmt);
bool mysql_unprepare_statement(DatabaseHandle* connection, PreparedStatement* stmt);

// Utility functions for prepared statement cache
bool mysql_add_prepared_statement(PreparedStatementCache* cache, const char* name);
bool mysql_remove_prepared_statement(PreparedStatementCache* cache, const char* name);

#endif // DATABASE_ENGINE_MYSQL_PREPARED_H