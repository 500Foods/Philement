/*
 * Firebase engine - transactions (Phase 3: not implemented).
 */

#ifndef DATABASE_ENGINE_FIREBASE_TRANSACTION_H
#define DATABASE_ENGINE_FIREBASE_TRANSACTION_H

#include <src/database/database.h>

bool firebase_begin_transaction(DatabaseHandle* connection, DatabaseIsolationLevel level,
                                Transaction** transaction);
bool firebase_commit_transaction(DatabaseHandle* connection, Transaction* transaction);
bool firebase_rollback_transaction(DatabaseHandle* connection, Transaction* transaction);

#endif /* DATABASE_ENGINE_FIREBASE_TRANSACTION_H */
