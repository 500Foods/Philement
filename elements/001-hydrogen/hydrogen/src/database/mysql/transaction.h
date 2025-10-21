/*
 * MySQL Database Engine - Transaction Management Header
 *
 * Header file for MySQL transaction management functions.
 */

#ifndef DATABASE_ENGINE_MYSQL_TRANSACTION_H
#define DATABASE_ENGINE_MYSQL_TRANSACTION_H

#include <src/database/database.h>

// Transaction management
bool mysql_begin_transaction(DatabaseHandle* connection, DatabaseIsolationLevel level, Transaction** transaction);
bool mysql_commit_transaction(DatabaseHandle* connection, Transaction* transaction);
bool mysql_rollback_transaction(DatabaseHandle* connection, Transaction* transaction);

#endif // DATABASE_ENGINE_MYSQL_TRANSACTION_H