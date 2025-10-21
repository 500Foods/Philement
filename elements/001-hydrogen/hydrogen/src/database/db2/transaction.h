/*
 * DB2 Database Engine - Transaction Management Header
 *
 * Header file for DB2 transaction management functions.
 */

#ifndef DATABASE_ENGINE_DB2_TRANSACTION_H
#define DATABASE_ENGINE_DB2_TRANSACTION_H

#include <src/database/database.h>

// Transaction management
bool db2_begin_transaction(DatabaseHandle* connection, DatabaseIsolationLevel level, Transaction** transaction);
bool db2_commit_transaction(DatabaseHandle* connection, Transaction* transaction);
bool db2_rollback_transaction(DatabaseHandle* connection, Transaction* transaction);

#endif // DATABASE_ENGINE_DB2_TRANSACTION_H