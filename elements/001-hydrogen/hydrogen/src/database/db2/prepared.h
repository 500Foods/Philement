/*
 * DB2 Database Engine - Prepared Statement Management Header
 *
 * Header file for DB2 prepared statement management functions.
 */

#ifndef DATABASE_ENGINE_DB2_PREPARED_H
#define DATABASE_ENGINE_DB2_PREPARED_H

#include <src/database/database.h>
#include "types.h"

// Prepared statement management
bool db2_prepare_statement(DatabaseHandle* connection, const char* name, const char* sql, PreparedStatement** stmt);
bool db2_unprepare_statement(DatabaseHandle* connection, PreparedStatement* stmt);

// Utility functions for prepared statement cache
bool db2_add_prepared_statement(PreparedStatementCache* cache, const char* name);
bool db2_remove_prepared_statement(PreparedStatementCache* cache, const char* name);

// Utility functions for prepared statement cache
bool db2_add_prepared_statement(PreparedStatementCache* cache, const char* name);
bool db2_remove_prepared_statement(PreparedStatementCache* cache, const char* name);

#endif // DATABASE_ENGINE_DB2_PREPARED_H