/*
 * Mock DB2 Transaction functions for unit testing
 *
 * This file provides mock implementations of DB2 transaction wrapper functions
 * to enable testing of DB2 transaction operations.
 */

#ifndef MOCK_DB2_TRANSACTION_H
#define MOCK_DB2_TRANSACTION_H

#include <src/database/database.h>

// Enable mock override
#ifdef USE_MOCK_DB2_TRANSACTION
#define db2_begin_transaction mock_db2_begin_transaction
#define db2_commit_transaction mock_db2_commit_transaction
#define db2_rollback_transaction mock_db2_rollback_transaction
#endif

// Mock function prototypes - must match signatures in src/database/db2/transaction.h
bool mock_db2_begin_transaction(DatabaseHandle* connection, DatabaseIsolationLevel level, Transaction** transaction);
bool mock_db2_commit_transaction(DatabaseHandle* connection, Transaction* transaction);
bool mock_db2_rollback_transaction(DatabaseHandle* connection, Transaction* transaction);

// Mock control functions
void mock_db2_transaction_reset_all(void);
void mock_db2_transaction_set_begin_result(bool result);
void mock_db2_transaction_set_commit_result(bool result);
void mock_db2_transaction_set_rollback_result(bool result);
void mock_db2_transaction_set_transaction(Transaction* tx);

#endif // MOCK_DB2_TRANSACTION_H