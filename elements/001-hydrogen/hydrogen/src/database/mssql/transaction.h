/*
 * MSSQL Database Engine - Transaction Management Header
 *
 * Header file for MSSQL transaction management functions.
 */

#ifndef DATABASE_ENGINE_MSSQL_TRANSACTION_H
#define DATABASE_ENGINE_MSSQL_TRANSACTION_H

#include <src/database/database.h>

// Transaction management
bool mssql_begin_transaction(DatabaseHandle* connection, DatabaseIsolationLevel level, Transaction** transaction);
bool mssql_commit_transaction(DatabaseHandle* connection, Transaction* transaction);
bool mssql_rollback_transaction(DatabaseHandle* connection, Transaction* transaction);

#endif // DATABASE_ENGINE_MSSQL_TRANSACTION_H
