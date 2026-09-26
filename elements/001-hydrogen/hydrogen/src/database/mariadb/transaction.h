/*
 * MariaDB Database Engine - Transaction Management Header
 *
 * Header file for MariaDB transaction management functions.
 */

#ifndef DATABASE_ENGINE_MARIADB_TRANSACTION_H
#define DATABASE_ENGINE_MARIADB_TRANSACTION_H

#include <src/database/database.h>

// Transaction management
bool mariadb_begin_transaction(DatabaseHandle* connection, DatabaseIsolationLevel level, Transaction** transaction);
bool mariadb_commit_transaction(DatabaseHandle* connection, Transaction* transaction);
bool mariadb_rollback_transaction(DatabaseHandle* connection, Transaction* transaction);

#endif // DATABASE_ENGINE_MARIADB_TRANSACTION_H
