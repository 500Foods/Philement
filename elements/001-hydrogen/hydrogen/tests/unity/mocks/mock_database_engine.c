/*
 * Mock Database Engine functions implementation
 */

#include <unity.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "mock_database_engine.h"
#include <src/database/database.h>

// Need to declare the mock functions that the interface will point to
extern bool mock_database_engine_begin_transaction(DatabaseHandle* connection, DatabaseIsolationLevel level, Transaction** transaction);
extern bool mock_database_engine_commit_transaction(DatabaseHandle* connection, Transaction* transaction);
extern bool mock_database_engine_rollback_transaction(DatabaseHandle* connection, Transaction* transaction);
extern bool mock_database_engine_execute(DatabaseHandle* connection, QueryRequest* request, QueryResult** result);

// Static mock state
static bool mock_begin_result = true;
static bool mock_commit_result = true;
static bool mock_rollback_result = true;
static bool mock_execute_success = true;
static QueryResult* mock_query_result = NULL;
static int mock_affected_rows = 0;
static Transaction* mock_tx = NULL;

bool mock_database_engine_begin_transaction(DatabaseHandle* connection, DatabaseIsolationLevel level, Transaction** transaction) {
    (void)connection;
    
    // Only create transaction if begin will succeed
    if (mock_begin_result) {
        if (!mock_tx) {
            mock_tx = calloc(1, sizeof(Transaction));
            if (mock_tx) {
                mock_tx->transaction_id = strdup("mock_engine_tx");
                mock_tx->isolation_level = level;
                mock_tx->started_at = time(NULL);
                mock_tx->active = true;
            }
        }
        
        if (transaction) {
            *transaction = mock_tx;
        }
    } else {
        // Begin failed, return NULL transaction
        if (transaction) {
            *transaction = NULL;
        }
    }

    return mock_begin_result;
}

bool mock_database_engine_commit_transaction(DatabaseHandle* connection, Transaction* transaction) {
    (void)connection;
    if (transaction) {
        transaction->active = false;
        if (transaction->transaction_id) {
            free(transaction->transaction_id);
            transaction->transaction_id = NULL;
        }
    }
    return mock_commit_result;
}

bool mock_database_engine_rollback_transaction(DatabaseHandle* connection, Transaction* transaction) {
    (void)connection;
    if (transaction) {
        transaction->active = false;
        if (transaction->transaction_id) {
            free(transaction->transaction_id);
            transaction->transaction_id = NULL;
        }
    }
    return mock_rollback_result;
}

bool mock_database_engine_execute(DatabaseHandle* connection, QueryRequest* request, QueryResult** result) {
    (void)connection;
    (void)request;
    
    // Create a fresh result for each call to avoid lifecycle issues
    QueryResult* fresh_result = calloc(1, sizeof(QueryResult));
    if (fresh_result) {
        fresh_result->success = mock_execute_success;
        fresh_result->data_json = strdup("{}");
        fresh_result->row_count = 0;
        fresh_result->column_count = 0;
        fresh_result->affected_rows = mock_affected_rows;
        fresh_result->error_message = NULL;
        fresh_result->execution_time_ms = 0;
    }

    if (result) {
        *result = fresh_result;
    } else if (fresh_result) {
        // cppcheck-suppress memleak - Caller must provide result pointer to receive ownership
        // If no result pointer provided, we must clean up to avoid leak
        if (fresh_result->data_json) free(fresh_result->data_json);
        free(fresh_result);
    }

    return mock_execute_success;
}

void mock_database_engine_cleanup_result(QueryResult* result) {
    // Actually free the result - we create fresh ones each time
    if (result) {
        if (result->data_json) {
            free(result->data_json);
            result->data_json = NULL;
        }
        if (result->error_message) {
            free(result->error_message);
            result->error_message = NULL;
        }
        free(result);
    }
}

void mock_database_engine_cleanup_transaction(Transaction* transaction) {
    // Don't free anything - the mock owns these resources and will clean them up in reset
    // Just mark as inactive
    if (transaction) {
        transaction->active = false;
    }
}

void mock_database_engine_reset_all(void) {
    mock_begin_result = true;
    mock_commit_result = true;
    mock_rollback_result = true;
    mock_execute_success = true;
    mock_affected_rows = 0;
    
    // Note: mock_query_result is no longer static - results are freed by cleanup_result
    mock_query_result = NULL;
    
    // Clean up mock transaction
    if (mock_tx) {
        if (mock_tx->transaction_id) {
            free(mock_tx->transaction_id);
            mock_tx->transaction_id = NULL;
        }
        free(mock_tx);
        mock_tx = NULL;
    }
}

void mock_database_engine_set_begin_result(bool result) {
    mock_begin_result = result;
}

void mock_database_engine_set_commit_result(bool result) {
    mock_commit_result = result;
}

void mock_database_engine_set_rollback_result(bool result) {
    mock_rollback_result = result;
}

void mock_database_engine_set_execute_result(bool success) {
    mock_execute_success = success;
}

void mock_database_engine_set_execute_query_result(QueryResult* result) {
    mock_query_result = result;
}

void mock_database_engine_set_affected_rows(int rows) {
    mock_affected_rows = rows;
}

DatabaseEngineInterface* mock_database_engine_get(DatabaseEngine engine_type) {
    (void)engine_type;
    // Return a mock engine interface that has the functions we need
    static DatabaseEngineInterface mock_engine = {
        .engine_type = DB_ENGINE_POSTGRESQL,  // Default
        .name = (char*)"mock_engine",
        .begin_transaction = mock_database_engine_begin_transaction,
        .commit_transaction = mock_database_engine_commit_transaction,
        .rollback_transaction = mock_database_engine_rollback_transaction,
        .execute_query = mock_database_engine_execute
    };
    return &mock_engine;
}