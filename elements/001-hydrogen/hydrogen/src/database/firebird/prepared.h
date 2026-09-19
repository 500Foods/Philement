/*
 * Firebird Database Engine - Prepared Statement Header
 *
 * Header file for Firebird prepared statement management functions.
 */

#ifndef DATABASE_ENGINE_FIREBIRD_PREPARED_H
#define DATABASE_ENGINE_FIREBIRD_PREPARED_H

#include <src/database/database.h>
#include "types.h"

/*
 * Prepared statement management.
 * Phase 5: prepare allocates a PreparedStatement struct (no isc_dsql call);
 *          unprepare frees it. Phase 6 will invoke isc_dsql_prepare.
 */
bool firebird_prepare_statement(DatabaseHandle* connection,
                                 const char* name,
                                 const char* sql,
                                 PreparedStatement** stmt,
                                 bool add_to_cache);

bool firebird_unprepare_statement(DatabaseHandle* connection,
                                  PreparedStatement* stmt);

/*
 * Free a PreparedStatement and its associated resources.
 * Public so query.c and transaction.c can call it.
 */
void firebird_free_prepared_statement(PreparedStatement* stmt);

#endif // DATABASE_ENGINE_FIREBIRD_PREPARED_H
