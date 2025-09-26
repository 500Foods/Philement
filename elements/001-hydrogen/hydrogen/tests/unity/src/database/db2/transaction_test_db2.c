/*
 * Unity Test File: DB2 Transaction Management
 * This file contains unit tests for DB2 transaction functions
 */

#include "../../../../../tests/unity/mocks/mock_libdb2.h"

#include "../../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../../src/database/database.h"
#include "../../../../../src/database/db2/transaction.h"
#include "../../../../../src/database/db2/types.h"
#include "../../../../../src/database/db2/connection.h"

// Forward declarations for functions being tested
bool db2_begin_transaction(DatabaseHandle* connection, DatabaseIsolationLevel level, Transaction** transaction);
bool db2_commit_transaction(DatabaseHandle* connection, Transaction* transaction);
bool db2_rollback_transaction(DatabaseHandle* connection, Transaction* transaction);

// Function prototypes for test functions
void test_db2_begin_transaction_null_connection(void);
void test_db2_begin_transaction_null_transaction_ptr(void);
void test_db2_begin_transaction_wrong_engine_type(void);
void test_db2_begin_transaction_success(void);
void test_db2_begin_transaction_null_connection_handle(void);
void test_db2_commit_transaction_null_connection(void);
void test_db2_commit_transaction_null_transaction(void);
void test_db2_commit_transaction_wrong_engine_type(void);
void test_db2_commit_transaction_success(void);
void test_db2_commit_transaction_sqilendtran_failure(void);
void test_db2_commit_transaction_null_connection_handle(void);
void test_db2_rollback_transaction_null_connection(void);
void test_db2_rollback_transaction_null_transaction(void);
void test_db2_rollback_transaction_wrong_engine_type(void);
void test_db2_rollback_transaction_success(void);
void test_db2_rollback_transaction_sqilendtran_failure(void);
void test_db2_rollback_transaction_null_connection_handle(void);
DatabaseHandle* create_test_database_handle(void);
void destroy_test_database_handle(DatabaseHandle* handle);

void setUp(void) {
    // Reset all mocks to default state
    mock_libdb2_reset_all();

    // Ensure libdb2 functions are loaded (mocked)
    load_libdb2_functions();
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test db2_begin_transaction with null parameters
void test_db2_begin_transaction_null_connection(void) {
    Transaction* transaction = NULL;
    bool result = db2_begin_transaction(NULL, DB_ISOLATION_READ_COMMITTED, &transaction);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(transaction);
}

void test_db2_begin_transaction_null_transaction_ptr(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    bool result = db2_begin_transaction(&connection, DB_ISOLATION_READ_COMMITTED, NULL);
    TEST_ASSERT_FALSE(result);
}

void test_db2_begin_transaction_wrong_engine_type(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE; // Wrong engine type
    Transaction* transaction = NULL;
    bool result = db2_begin_transaction(&connection, DB_ISOLATION_READ_COMMITTED, &transaction);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(transaction);
}

// Test db2_commit_transaction with null parameters
void test_db2_commit_transaction_null_connection(void) {
    Transaction transaction = {0};
    bool result = db2_commit_transaction(NULL, &transaction);
    TEST_ASSERT_FALSE(result);
}

void test_db2_commit_transaction_null_transaction(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    bool result = db2_commit_transaction(&connection, NULL);
    TEST_ASSERT_FALSE(result);
}

void test_db2_commit_transaction_wrong_engine_type(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE; // Wrong engine type
    Transaction transaction = {0};
    bool result = db2_commit_transaction(&connection, &transaction);
    TEST_ASSERT_FALSE(result);
}

// Test db2_rollback_transaction with null parameters
void test_db2_rollback_transaction_null_connection(void) {
    Transaction transaction = {0};
    bool result = db2_rollback_transaction(NULL, &transaction);
    TEST_ASSERT_FALSE(result);
}

void test_db2_rollback_transaction_null_transaction(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    bool result = db2_rollback_transaction(&connection, NULL);
    TEST_ASSERT_FALSE(result);
}

void test_db2_rollback_transaction_wrong_engine_type(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE; // Wrong engine type
    Transaction transaction = {0};
    bool result = db2_rollback_transaction(&connection, &transaction);
    TEST_ASSERT_FALSE(result);
}

// Helper function to create a valid database handle for testing
DatabaseHandle* create_test_database_handle(void) {
    DatabaseHandle* handle = calloc(1, sizeof(DatabaseHandle));
    if (!handle) return NULL;
    DB2Connection* db2_conn = calloc(1, sizeof(DB2Connection));
    if (!db2_conn) {
        free(handle);
        return NULL;
    }

    handle->engine_type = DB_ENGINE_DB2;
    handle->connection_handle = db2_conn;
    db2_conn->connection = (void*)0x12345678; // Fake connection pointer

    return handle;
}

// Helper function to destroy test database handle
void destroy_test_database_handle(DatabaseHandle* handle) {
    if (handle) {
        free(handle->connection_handle);
        free(handle);
    }
}

// Test successful transaction begin
void test_db2_begin_transaction_success(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction* transaction = NULL;

    bool result = db2_begin_transaction(connection, DB_ISOLATION_READ_COMMITTED, &transaction);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(transaction);
    TEST_ASSERT_TRUE(transaction->active);
    TEST_ASSERT_EQUAL(DB_ISOLATION_READ_COMMITTED, transaction->isolation_level);
    TEST_ASSERT_EQUAL_STRING("db2_tx", transaction->transaction_id);

    // Clean up
    free(transaction->transaction_id);
    free(transaction);
    destroy_test_database_handle(connection);
}

// Test transaction begin with null connection handle
void test_db2_begin_transaction_null_connection_handle(void) {
    DatabaseHandle* connection = create_test_database_handle();
    ((DB2Connection*)connection->connection_handle)->connection = NULL;
    Transaction* transaction = NULL;

    bool result = db2_begin_transaction(connection, DB_ISOLATION_READ_COMMITTED, &transaction);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(transaction);

    destroy_test_database_handle(connection);
}

// Test successful commit
void test_db2_commit_transaction_success(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction transaction = {0};
    transaction.active = true;
    transaction.transaction_id = strdup("test_tx");

    // Mock successful SQLEndTran
    mock_libdb2_set_SQLEndTran_result(SQL_SUCCESS);

    bool result = db2_commit_transaction(connection, &transaction);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(transaction.active);
    TEST_ASSERT_NULL(connection->current_transaction);

    free(transaction.transaction_id);
    destroy_test_database_handle(connection);
}

// Test commit with SQLEndTran failure
void test_db2_commit_transaction_sqilendtran_failure(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction transaction = {0};
    transaction.active = true;

    // Mock SQLEndTran failure
    mock_libdb2_set_SQLEndTran_result(-1); // Failure

    bool result = db2_commit_transaction(connection, &transaction);

    TEST_ASSERT_FALSE(result);

    destroy_test_database_handle(connection);
}

// Test commit with null connection handle
void test_db2_commit_transaction_null_connection_handle(void) {
    DatabaseHandle* connection = create_test_database_handle();
    ((DB2Connection*)connection->connection_handle)->connection = NULL;
    Transaction transaction = {0};
    transaction.active = true;

    bool result = db2_commit_transaction(connection, &transaction);

    TEST_ASSERT_FALSE(result);

    destroy_test_database_handle(connection);
}

// Test successful rollback
void test_db2_rollback_transaction_success(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction transaction = {0};
    transaction.active = true;
    transaction.transaction_id = strdup("test_tx");

    // Mock successful SQLEndTran
    mock_libdb2_set_SQLEndTran_result(SQL_SUCCESS);

    bool result = db2_rollback_transaction(connection, &transaction);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(transaction.active);
    TEST_ASSERT_NULL(connection->current_transaction);

    free(transaction.transaction_id);
    destroy_test_database_handle(connection);
}

// Test rollback with SQLEndTran failure
void test_db2_rollback_transaction_sqilendtran_failure(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction transaction = {0};
    transaction.active = true;

    // Mock SQLEndTran failure
    mock_libdb2_set_SQLEndTran_result(-1); // Failure

    bool result = db2_rollback_transaction(connection, &transaction);

    TEST_ASSERT_FALSE(result);

    destroy_test_database_handle(connection);
}

// Test rollback with null connection handle
void test_db2_rollback_transaction_null_connection_handle(void) {
    DatabaseHandle* connection = create_test_database_handle();
    ((DB2Connection*)connection->connection_handle)->connection = NULL;
    Transaction transaction = {0};
    transaction.active = true;

    bool result = db2_rollback_transaction(connection, &transaction);

    TEST_ASSERT_FALSE(result);

    destroy_test_database_handle(connection);
}

int main(void) {
    UNITY_BEGIN();

    // Test db2_begin_transaction
    RUN_TEST(test_db2_begin_transaction_null_connection);
    RUN_TEST(test_db2_begin_transaction_null_transaction_ptr);
    RUN_TEST(test_db2_begin_transaction_wrong_engine_type);
    RUN_TEST(test_db2_begin_transaction_success);
    RUN_TEST(test_db2_begin_transaction_null_connection_handle);

    // Test db2_commit_transaction
    RUN_TEST(test_db2_commit_transaction_null_connection);
    RUN_TEST(test_db2_commit_transaction_null_transaction);
    RUN_TEST(test_db2_commit_transaction_wrong_engine_type);
    RUN_TEST(test_db2_commit_transaction_success);
    RUN_TEST(test_db2_commit_transaction_sqilendtran_failure);
    RUN_TEST(test_db2_commit_transaction_null_connection_handle);

    // Test db2_rollback_transaction
    RUN_TEST(test_db2_rollback_transaction_null_connection);
    RUN_TEST(test_db2_rollback_transaction_null_transaction);
    RUN_TEST(test_db2_rollback_transaction_wrong_engine_type);
    RUN_TEST(test_db2_rollback_transaction_success);
    RUN_TEST(test_db2_rollback_transaction_sqilendtran_failure);
    RUN_TEST(test_db2_rollback_transaction_null_connection_handle);

    return UNITY_END();
}