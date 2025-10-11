/*
 * Mock DB2 Transaction functions implementation
 */

#include <unity.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "mock_db2_transaction.h"
#include <src/database/database.h>

// Static mock state
static bool mock_begin_result = true;
static bool mock_commit_result = true;
static bool mock_rollback_result = true;
static Transaction* mock_transaction = NULL;

bool mock_db2_begin_transaction(DatabaseHandle* connection, DatabaseIsolationLevel level, Transaction** transaction) {
    (void)connection;
    
    // Only create transaction if begin will succeed
    if (mock_begin_result) {
        // Always create a fresh transaction for each begin
        // Free old one if it exists
        if (mock_transaction) {
            if (mock_transaction->transaction_id) free(mock_transaction->transaction_id);
            free(mock_transaction);
        }
        
        mock_transaction = calloc(1, sizeof(Transaction));
        if (mock_transaction) {
            mock_transaction->transaction_id = strdup("mock_tx_id");
            mock_transaction->isolation_level = level;
            mock_transaction->started_at = time(NULL);
            mock_transaction->active = true;
        }
        
        if (transaction) {
            *transaction = mock_transaction;
        }
    } else {
        // Begin failed, return NULL transaction
        if (transaction) {
            *transaction = NULL;
        }
    }

    return mock_begin_result;
}

bool mock_db2_commit_transaction(DatabaseHandle* connection, Transaction* transaction) {
    (void)connection;
    if (transaction && transaction == mock_transaction) {
        transaction->active = false;
        // Don't free transaction_id for mock transaction, it's static
    }
    return mock_commit_result;
}

bool mock_db2_rollback_transaction(DatabaseHandle* connection, Transaction* transaction) {
    (void)connection;
    if (transaction && transaction == mock_transaction) {
        transaction->active = false;
        // Don't free transaction_id for mock transaction, it's static
    }
    return mock_rollback_result;
}

void mock_db2_transaction_reset_all(void) {
    mock_begin_result = true;
    mock_commit_result = true;
    mock_rollback_result = true;
    if (mock_transaction) {
        if (mock_transaction->transaction_id) free(mock_transaction->transaction_id);
        free(mock_transaction);
        mock_transaction = NULL;
    }
}

void mock_db2_transaction_set_begin_result(bool result) {
    mock_begin_result = result;
}

void mock_db2_transaction_set_commit_result(bool result) {
    mock_commit_result = result;
}

void mock_db2_transaction_set_rollback_result(bool result) {
    mock_rollback_result = result;
}

void mock_db2_transaction_set_transaction(Transaction* tx) {
    mock_transaction = tx;
}