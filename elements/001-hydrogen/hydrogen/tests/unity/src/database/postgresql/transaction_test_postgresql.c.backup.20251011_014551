/*
 * Unity Test File: PostgreSQL Transaction Management
 * This file contains unit tests for PostgreSQL transaction functions
 */

#include "../../../../../tests/unity/mocks/mock_libpq.h"

#include "../../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../../src/database/database.h"
#include "../../../../../src/database/postgresql/transaction.h"
#include "../../../../../src/database/postgresql/types.h"
#include "../../../../../src/database/postgresql/connection.h"

// Forward declarations for functions being tested
bool postgresql_begin_transaction(DatabaseHandle* connection, DatabaseIsolationLevel level, Transaction** transaction);
bool postgresql_commit_transaction(DatabaseHandle* connection, Transaction* transaction);
bool postgresql_rollback_transaction(DatabaseHandle* connection, Transaction* transaction);

// Function prototypes for test functions
void test_postgresql_begin_transaction_null_connection(void);
void test_postgresql_begin_transaction_null_transaction_ptr(void);
void test_postgresql_begin_transaction_wrong_engine_type(void);
void test_postgresql_begin_transaction_success(void);
void test_postgresql_begin_transaction_pqexec_failure(void);
void test_postgresql_begin_transaction_bad_status(void);
void test_postgresql_begin_transaction_already_in_transaction(void);
void test_postgresql_commit_transaction_null_connection(void);
void test_postgresql_commit_transaction_null_transaction(void);
void test_postgresql_commit_transaction_wrong_engine_type(void);
void test_postgresql_commit_transaction_success(void);
void test_postgresql_commit_transaction_not_in_transaction(void);
void test_postgresql_rollback_transaction_null_connection(void);
void test_postgresql_rollback_transaction_null_transaction(void);
void test_postgresql_rollback_transaction_wrong_engine_type(void);
void test_postgresql_rollback_transaction_success(void);
void test_postgresql_rollback_transaction_bad_status(void);
DatabaseHandle* create_test_database_handle(void);
void destroy_test_database_handle(DatabaseHandle* handle);

// Function prototypes for test functions
void test_postgresql_begin_transaction_null_connection(void);
void test_postgresql_begin_transaction_null_transaction_ptr(void);
void test_postgresql_begin_transaction_wrong_engine_type(void);
void test_postgresql_commit_transaction_null_connection(void);
void test_postgresql_commit_transaction_null_transaction(void);
void test_postgresql_commit_transaction_wrong_engine_type(void);
void test_postgresql_rollback_transaction_null_connection(void);
void test_postgresql_rollback_transaction_null_transaction(void);
void test_postgresql_rollback_transaction_wrong_engine_type(void);

void setUp(void) {
    // Reset all mocks to default state
    mock_libpq_reset_all();
    // For transaction tests, mock check_timeout_expired to never timeout
    mock_libpq_set_check_timeout_expired_use_mock(true);
    mock_libpq_set_check_timeout_expired_result(false);
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test postgresql_begin_transaction with null parameters
void test_postgresql_begin_transaction_null_connection(void) {
    Transaction* transaction = NULL;
    bool result = postgresql_begin_transaction(NULL, DB_ISOLATION_READ_COMMITTED, &transaction);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(transaction);
}

void test_postgresql_begin_transaction_null_transaction_ptr(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_POSTGRESQL;
    bool result = postgresql_begin_transaction(&connection, DB_ISOLATION_READ_COMMITTED, NULL);
    TEST_ASSERT_FALSE(result);
}

void test_postgresql_begin_transaction_wrong_engine_type(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE; // Wrong engine type
    Transaction* transaction = NULL;
    bool result = postgresql_begin_transaction(&connection, DB_ISOLATION_READ_COMMITTED, &transaction);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(transaction);
}

// Test postgresql_commit_transaction with null parameters
void test_postgresql_commit_transaction_null_connection(void) {
    Transaction transaction = {0};
    bool result = postgresql_commit_transaction(NULL, &transaction);
    TEST_ASSERT_FALSE(result);
}

void test_postgresql_commit_transaction_null_transaction(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_POSTGRESQL;
    bool result = postgresql_commit_transaction(&connection, NULL);
    TEST_ASSERT_FALSE(result);
}

void test_postgresql_commit_transaction_wrong_engine_type(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE; // Wrong engine type
    Transaction transaction = {0};
    bool result = postgresql_commit_transaction(&connection, &transaction);
    TEST_ASSERT_FALSE(result);
}

// Test postgresql_rollback_transaction with null parameters
void test_postgresql_rollback_transaction_null_connection(void) {
    Transaction transaction = {0};
    bool result = postgresql_rollback_transaction(NULL, &transaction);
    TEST_ASSERT_FALSE(result);
}

void test_postgresql_rollback_transaction_null_transaction(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_POSTGRESQL;
    bool result = postgresql_rollback_transaction(&connection, NULL);
    TEST_ASSERT_FALSE(result);
}

void test_postgresql_rollback_transaction_wrong_engine_type(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE; // Wrong engine type
    Transaction transaction = {0};
    bool result = postgresql_rollback_transaction(&connection, &transaction);
    TEST_ASSERT_FALSE(result);
}

// Helper function to create a valid database handle for testing
DatabaseHandle* create_test_database_handle(void) {
    DatabaseHandle* handle = calloc(1, sizeof(DatabaseHandle));
    if (!handle) return NULL;
    PostgresConnection* pg_conn = calloc(1, sizeof(PostgresConnection));
    if (!pg_conn) {
        free(handle);
        return NULL;
    }

    handle->engine_type = DB_ENGINE_POSTGRESQL;
    handle->connection_handle = pg_conn;
    pg_conn->connection = (void*)0x12345678; // Fake connection pointer
    pg_conn->in_transaction = false;

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
void test_postgresql_begin_transaction_success(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction* transaction = NULL;

    // Mock successful PQexec and PQresultStatus
    void* fake_result = (void*)0xdeadbeef;
    mock_libpq_set_PQexec_result(fake_result);
    mock_libpq_set_PQresultStatus_result(PGRES_COMMAND_OK);

    bool result = postgresql_begin_transaction(connection, DB_ISOLATION_READ_COMMITTED, &transaction);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(transaction);
    TEST_ASSERT_TRUE(transaction->active);
    TEST_ASSERT_EQUAL(DB_ISOLATION_READ_COMMITTED, transaction->isolation_level);
    TEST_ASSERT_EQUAL_STRING("postgresql_tx", transaction->transaction_id);

    // Clean up
    free(transaction->transaction_id);
    free(transaction);
    // destroy_test_database_handle(connection);
}

// Test transaction begin with PQexec failure
void test_postgresql_begin_transaction_pqexec_failure(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction* transaction = NULL;

    // Mock PQexec returning NULL (failure)
    mock_libpq_set_PQexec_result(NULL);

    bool result = postgresql_begin_transaction(connection, DB_ISOLATION_READ_COMMITTED, &transaction);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(transaction);
}

// Test transaction begin with bad result status
void test_postgresql_begin_transaction_bad_status(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction* transaction = NULL;

    // Mock PQexec success but bad status
    void* fake_result = (void*)0xdeadbeef;
    mock_libpq_set_PQexec_result(fake_result);
    mock_libpq_set_PQresultStatus_result(PGRES_FATAL_ERROR);

    bool result = postgresql_begin_transaction(connection, DB_ISOLATION_READ_COMMITTED, &transaction);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(transaction);
}

// Test transaction begin when already in transaction
void test_postgresql_begin_transaction_already_in_transaction(void) {
    DatabaseHandle* connection = create_test_database_handle();
    ((PostgresConnection*)connection->connection_handle)->in_transaction = true;
    Transaction* transaction = NULL;

    bool result = postgresql_begin_transaction(connection, DB_ISOLATION_READ_COMMITTED, &transaction);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(transaction);
}

// Test successful commit
void test_postgresql_commit_transaction_success(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction transaction = {0};
    transaction.active = true;
    transaction.transaction_id = strdup("test_tx");

    ((PostgresConnection*)connection->connection_handle)->in_transaction = true;

    // Mock successful commit
    void* fake_result = (void*)0xdeadbeef;
    mock_libpq_set_PQexec_result(fake_result);
    mock_libpq_set_PQresultStatus_result(PGRES_COMMAND_OK);

    bool result = postgresql_commit_transaction(connection, &transaction);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(transaction.active);
    TEST_ASSERT_NULL(connection->current_transaction);

    free(transaction.transaction_id);
    // destroy_test_database_handle(connection);
    destroy_test_database_handle(connection);
}

// Test commit when not in transaction
void test_postgresql_commit_transaction_not_in_transaction(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction transaction = {0};
    transaction.active = true;

    ((PostgresConnection*)connection->connection_handle)->in_transaction = false;

    bool result = postgresql_commit_transaction(connection, &transaction);

    TEST_ASSERT_FALSE(result);
}

// Test successful rollback
void test_postgresql_rollback_transaction_success(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction transaction = {0};
    transaction.active = true;
    transaction.transaction_id = strdup("test_tx");

    ((PostgresConnection*)connection->connection_handle)->in_transaction = true;

    // Mock successful rollback
    void* fake_result = (void*)0xdeadbeef;
    mock_libpq_set_PQexec_result(fake_result);
    mock_libpq_set_PQresultStatus_result(PGRES_COMMAND_OK);

    bool result = postgresql_rollback_transaction(connection, &transaction);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(transaction.active);
    TEST_ASSERT_NULL(connection->current_transaction);

    free(transaction.transaction_id);
}

// Test rollback with bad status
void test_postgresql_rollback_transaction_bad_status(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction transaction = {0};
    transaction.active = true;

    ((PostgresConnection*)connection->connection_handle)->in_transaction = true;

    // Mock bad status
    void* fake_result = (void*)0xdeadbeef;
    mock_libpq_set_PQexec_result(fake_result);
    mock_libpq_set_PQresultStatus_result(PGRES_FATAL_ERROR);

    bool result = postgresql_rollback_transaction(connection, &transaction);

    TEST_ASSERT_FALSE(result);
}

int main(void) {
    UNITY_BEGIN();

    // Test postgresql_begin_transaction
    RUN_TEST(test_postgresql_begin_transaction_null_connection);
    RUN_TEST(test_postgresql_begin_transaction_null_transaction_ptr);
    RUN_TEST(test_postgresql_begin_transaction_wrong_engine_type);
    RUN_TEST(test_postgresql_begin_transaction_success);
    RUN_TEST(test_postgresql_begin_transaction_pqexec_failure);
    RUN_TEST(test_postgresql_begin_transaction_bad_status);
    RUN_TEST(test_postgresql_begin_transaction_already_in_transaction);

    // Test postgresql_commit_transaction
    RUN_TEST(test_postgresql_commit_transaction_null_connection);
    RUN_TEST(test_postgresql_commit_transaction_null_transaction);
    RUN_TEST(test_postgresql_commit_transaction_wrong_engine_type);
    RUN_TEST(test_postgresql_commit_transaction_success);
    RUN_TEST(test_postgresql_commit_transaction_not_in_transaction);

    // Test postgresql_rollback_transaction
    RUN_TEST(test_postgresql_rollback_transaction_null_connection);
    RUN_TEST(test_postgresql_rollback_transaction_null_transaction);
    RUN_TEST(test_postgresql_rollback_transaction_wrong_engine_type);
    RUN_TEST(test_postgresql_rollback_transaction_success);
    RUN_TEST(test_postgresql_rollback_transaction_bad_status);

    return UNITY_END();
}