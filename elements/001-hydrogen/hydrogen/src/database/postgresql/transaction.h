/*
 * PostgreSQL Database Engine - Transaction Management Header
 *
 * Header file for PostgreSQL transaction management functions.
 */

#ifndef DATABASE_ENGINE_POSTGRESQL_TRANSACTION_H
#define DATABASE_ENGINE_POSTGRESQL_TRANSACTION_H

#include <stdbool.h>

#include <src/database/database.h>
#include "connection.h"

// Function prototypes for transaction management
bool postgresql_begin_transaction(DatabaseHandle* connection, DatabaseIsolationLevel level, Transaction** transaction);
bool postgresql_commit_transaction(DatabaseHandle* connection, Transaction* transaction);
bool postgresql_rollback_transaction(DatabaseHandle* connection, Transaction* transaction);

#endif // DATABASE_ENGINE_POSTGRESQL_TRANSACTION_H