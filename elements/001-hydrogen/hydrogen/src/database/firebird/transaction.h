/*
 * Firebird Database Engine - Transaction Management Header
 *
 * Header file for Firebird transaction management functions.
 */

#ifndef DATABASE_ENGINE_FIREBIRD_TRANSACTION_H
#define DATABASE_ENGINE_FIREBIRD_TRANSACTION_H

#include <src/database/database.h>

// Transaction management
bool firebird_begin_transaction(DatabaseHandle* connection, DatabaseIsolationLevel level, Transaction** transaction);
bool firebird_commit_transaction(DatabaseHandle* connection, Transaction* transaction);
bool firebird_rollback_transaction(DatabaseHandle* connection, Transaction* transaction);

// TPB builder (non-static for testability)
const char* firebird_build_tpb(DatabaseIsolationLevel level);

#endif // DATABASE_ENGINE_FIREBIRD_TRANSACTION_H
