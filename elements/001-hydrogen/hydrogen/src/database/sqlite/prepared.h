/*
 * SQLite Database Engine - Prepared Statement Management Header
 *
 * Header file for SQLite prepared statement management functions.
 */

#ifndef SQLITE_PREPARED_H
#define SQLITE_PREPARED_H

#include "../database.h"

// Prepared statement management
bool sqlite_prepare_statement(DatabaseHandle* connection, const char* name, const char* sql, PreparedStatement** stmt);
bool sqlite_unprepare_statement(DatabaseHandle* connection, PreparedStatement* stmt);

#endif // SQLITE_PREPARED_H