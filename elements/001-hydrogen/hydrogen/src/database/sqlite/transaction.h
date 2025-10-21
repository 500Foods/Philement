/*
 * SQLite Database Engine - Transaction Management Header
 *
 * Header file for SQLite transaction management functions.
 */

#ifndef SQLITE_TRANSACTION_H
#define SQLITE_TRANSACTION_H

#include <src/database/database.h>

// Transaction management
bool sqlite_begin_transaction(DatabaseHandle* connection, DatabaseIsolationLevel level, Transaction** transaction);
bool sqlite_commit_transaction(DatabaseHandle* connection, Transaction* transaction);
bool sqlite_rollback_transaction(DatabaseHandle* connection, Transaction* transaction);

#endif // SQLITE_TRANSACTION_H