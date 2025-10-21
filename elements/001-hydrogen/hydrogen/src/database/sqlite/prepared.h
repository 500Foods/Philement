/*
 * SQLite Database Engine - Prepared Statement Management Header
 *
 * Header file for SQLite prepared statement management functions.
 */

#ifndef SQLITE_PREPARED_H
#define SQLITE_PREPARED_H

#include <src/database/database.h>

// Prepared statement management
bool sqlite_prepare_statement(DatabaseHandle* connection, const char* name, const char* sql, PreparedStatement** stmt);
bool sqlite_unprepare_statement(DatabaseHandle* connection, PreparedStatement* stmt);

// Utility functions for prepared statement cache
bool sqlite_add_prepared_statement(PreparedStatementCache* cache, const char* name);
bool sqlite_remove_prepared_statement(PreparedStatementCache* cache, const char* name);

#endif // SQLITE_PREPARED_H