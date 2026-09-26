/*
 * Unity Test File: MariaDB Transaction Management
 * This file contains unit tests for MySQL transaction functions
 */

#include <unity/mocks/mock_libmariadb.h>

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/database/database.h>
#include <src/database/mariadb/transaction.h>
#include <src/database/mariadb/types.h>
#include <src/database/mariadb/connection.h>

// Forward declarations for functions being tested
bool mariadb_begin_transaction(DatabaseHandle* connection, DatabaseIsolationLevel level, Transaction** transaction);
bool mariadb_commit_transaction(DatabaseHandle* connection, Transaction* transaction);
bool mariadb_rollback_transaction(DatabaseHandle* connection, Transaction* transaction);

// Function prototypes for test functions
void test_mariadb_begin_transaction_null_connection(void);
void test_mariadb_begin_transaction_null_transaction_ptr(void);
void test_mariadb_begin_transaction_wrong_engine_type(void);
void test_mariadb_begin_transaction_success(void);
void test_mariadb_begin_transaction_mariadb_query_failure(void);
void test_mariadb_begin_transaction_mariadb_autocommit_failure(void);
void test_mariadb_begin_transaction_null_connection_handle(void);
void test_mariadb_commit_transaction_null_connection(void);
void test_mariadb_commit_transaction_null_transaction(void);
void test_mariadb_commit_transaction_wrong_engine_type(void);
void test_mariadb_commit_transaction_success(void);
void test_mariadb_commit_transaction_mariadb_commit_failure(void);
void test_mariadb_commit_transaction_mariadb_query_commit_failure(void);
void test_mariadb_commit_transaction_null_connection_handle(void);
void test_mariadb_rollback_transaction_null_connection(void);
void test_mariadb_rollback_transaction_null_transaction(void);
void test_mariadb_rollback_transaction_wrong_engine_type(void);
void test_mariadb_rollback_transaction_success(void);
void test_mariadb_rollback_transaction_mariadb_rollback_failure(void);
void test_mariadb_rollback_transaction_mariadb_query_rollback_failure(void);
void test_mariadb_rollback_transaction_null_connection_handle(void);
DatabaseHandle* create_test_database_handle(void);
void destroy_test_database_handle(DatabaseHandle* handle);

void setUp(void) {
    // Reset all mocks to default state
    mock_libmariadb_reset_all();
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test mariadb_begin_transaction with null parameters
void test_mariadb_begin_transaction_null_connection(void) {
    Transaction* transaction = NULL;
    bool result = mariadb_begin_transaction(NULL, DB_ISOLATION_READ_COMMITTED, &transaction);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(transaction);
}

void test_mariadb_begin_transaction_null_transaction_ptr(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MARIADB;
    bool result = mariadb_begin_transaction(&connection, DB_ISOLATION_READ_COMMITTED, NULL);
    TEST_ASSERT_FALSE(result);
}

void test_mariadb_begin_transaction_wrong_engine_type(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE; // Wrong engine type
    Transaction* transaction = NULL;
    bool result = mariadb_begin_transaction(&connection, DB_ISOLATION_READ_COMMITTED, &transaction);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(transaction);
}

// Test mariadb_commit_transaction with null parameters
void test_mariadb_commit_transaction_null_connection(void) {
    Transaction transaction = {0};
    bool result = mariadb_commit_transaction(NULL, &transaction);
    TEST_ASSERT_FALSE(result);
}

void test_mariadb_commit_transaction_null_transaction(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MARIADB;
    bool result = mariadb_commit_transaction(&connection, NULL);
    TEST_ASSERT_FALSE(result);
}

void test_mariadb_commit_transaction_wrong_engine_type(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE; // Wrong engine type
    Transaction transaction = {0};
    bool result = mariadb_commit_transaction(&connection, &transaction);
    TEST_ASSERT_FALSE(result);
}

// Test mariadb_rollback_transaction with null parameters
void test_mariadb_rollback_transaction_null_connection(void) {
    Transaction transaction = {0};
    bool result = mariadb_rollback_transaction(NULL, &transaction);
    TEST_ASSERT_FALSE(result);
}

void test_mariadb_rollback_transaction_null_transaction(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MARIADB;
    bool result = mariadb_rollback_transaction(&connection, NULL);
    TEST_ASSERT_FALSE(result);
}

void test_mariadb_rollback_transaction_wrong_engine_type(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE; // Wrong engine type
    Transaction transaction = {0};
    bool result = mariadb_rollback_transaction(&connection, &transaction);
    TEST_ASSERT_FALSE(result);
}

// Helper function to create a valid database handle for testing
DatabaseHandle* create_test_database_handle(void) {
    DatabaseHandle* handle = calloc(1, sizeof(DatabaseHandle));
    if (!handle) return NULL;
    MariadbConnection* mariadb_conn = calloc(1, sizeof(MariadbConnection));
    if (!mariadb_conn) {
        free(handle);
        return NULL;
    }

    handle->engine_type = DB_ENGINE_MARIADB;
    handle->connection_handle = mariadb_conn;
    mariadb_conn->connection = (void*)0x12345678; // Fake connection pointer

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
void test_mariadb_begin_transaction_success(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction* transaction = NULL;

    // Mock successful mariadb_query and mariadb_autocommit
    mock_libmariadb_set_mysql_query_result(0); // Success
    mock_libmariadb_set_mysql_autocommit_result(0); // Success

    bool result = mariadb_begin_transaction(connection, DB_ISOLATION_READ_COMMITTED, &transaction);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(transaction);
    TEST_ASSERT_TRUE(transaction->active);
    TEST_ASSERT_EQUAL(DB_ISOLATION_READ_COMMITTED, transaction->isolation_level);
    TEST_ASSERT_NOT_NULL(transaction->transaction_id);
    TEST_ASSERT_EQUAL(36, strlen(transaction->transaction_id));

    // Clean up
    free(transaction->transaction_id);
    free(transaction);
    destroy_test_database_handle(connection);
}

// Test transaction begin with mariadb_query failure
void test_mariadb_begin_transaction_mariadb_query_failure(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction* transaction = NULL;

    // Mock mariadb_query failure
    mock_libmariadb_set_mysql_query_result(-1); // Failure

    bool result = mariadb_begin_transaction(connection, DB_ISOLATION_READ_COMMITTED, &transaction);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(transaction);

    destroy_test_database_handle(connection);
}

// Test transaction begin with mariadb_autocommit failure
void test_mariadb_begin_transaction_mariadb_autocommit_failure(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction* transaction = NULL;

    // Mock mariadb_query success but mariadb_autocommit failure
    mock_libmariadb_set_mysql_query_result(0); // Success
    mock_libmariadb_set_mysql_autocommit_result(-1); // Failure

    bool result = mariadb_begin_transaction(connection, DB_ISOLATION_READ_COMMITTED, &transaction);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(transaction);

    destroy_test_database_handle(connection);
}

// Test transaction begin with null connection handle
void test_mariadb_begin_transaction_null_connection_handle(void) {
    DatabaseHandle* connection = create_test_database_handle();
    ((MariadbConnection*)connection->connection_handle)->connection = NULL;
    Transaction* transaction = NULL;

    bool result = mariadb_begin_transaction(connection, DB_ISOLATION_READ_COMMITTED, &transaction);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(transaction);

    destroy_test_database_handle(connection);
}

// Test successful commit
void test_mariadb_commit_transaction_success(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction transaction = {0};
    transaction.active = true;
    transaction.transaction_id = strdup("test_tx");

    // Mock successful mariadb_commit and mariadb_autocommit
    mock_libmariadb_set_mysql_commit_result(0); // Success
    mock_libmariadb_set_mysql_autocommit_result(0); // Success

    bool result = mariadb_commit_transaction(connection, &transaction);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(transaction.active);
    TEST_ASSERT_NULL(connection->current_transaction);

    free(transaction.transaction_id);
    destroy_test_database_handle(connection);
}

// Test commit with mariadb_commit failure
void test_mariadb_commit_transaction_mariadb_commit_failure(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction transaction = {0};
    transaction.active = true;

    // Mock mariadb_commit failure
    mock_libmariadb_set_mysql_commit_result(-1); // Failure

    bool result = mariadb_commit_transaction(connection, &transaction);

    TEST_ASSERT_FALSE(result);

    destroy_test_database_handle(connection);
}

// Test commit with mariadb_query commit failure (fallback path)
void test_mariadb_commit_transaction_mariadb_query_commit_failure(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction transaction = {0};
    transaction.active = true;

    // Mock mariadb_commit failure and mariadb_query failure
    mock_libmariadb_set_mysql_commit_result(-1); // Failure
    mock_libmariadb_set_mysql_query_result(-1); // Failure

    bool result = mariadb_commit_transaction(connection, &transaction);

    TEST_ASSERT_FALSE(result);

    destroy_test_database_handle(connection);
}

// Test commit with null connection handle
void test_mariadb_commit_transaction_null_connection_handle(void) {
    DatabaseHandle* connection = create_test_database_handle();
    ((MariadbConnection*)connection->connection_handle)->connection = NULL;
    Transaction transaction = {0};
    transaction.active = true;

    bool result = mariadb_commit_transaction(connection, &transaction);

    TEST_ASSERT_FALSE(result);

    destroy_test_database_handle(connection);
}

// Test successful rollback
void test_mariadb_rollback_transaction_success(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction transaction = {0};
    transaction.active = true;
    transaction.transaction_id = strdup("test_tx");

    // Mock successful mariadb_rollback and mariadb_autocommit
    mock_libmariadb_set_mysql_rollback_result(0); // Success
    mock_libmariadb_set_mysql_autocommit_result(0); // Success

    bool result = mariadb_rollback_transaction(connection, &transaction);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(transaction.active);
    TEST_ASSERT_NULL(connection->current_transaction);

    free(transaction.transaction_id);
    destroy_test_database_handle(connection);
}

// Test rollback with mariadb_rollback failure
void test_mariadb_rollback_transaction_mariadb_rollback_failure(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction transaction = {0};
    transaction.active = true;

    // Mock mariadb_rollback failure
    mock_libmariadb_set_mysql_rollback_result(-1); // Failure

    bool result = mariadb_rollback_transaction(connection, &transaction);

    TEST_ASSERT_FALSE(result);

    destroy_test_database_handle(connection);
}

// Test rollback with mariadb_query rollback failure (fallback path)
void test_mariadb_rollback_transaction_mariadb_query_rollback_failure(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction transaction = {0};
    transaction.active = true;

    // Mock mariadb_rollback failure and mariadb_query failure
    mock_libmariadb_set_mysql_rollback_result(-1); // Failure
    mock_libmariadb_set_mysql_query_result(-1); // Failure

    bool result = mariadb_rollback_transaction(connection, &transaction);

    TEST_ASSERT_FALSE(result);

    destroy_test_database_handle(connection);
}

// Test rollback with null connection handle
void test_mariadb_rollback_transaction_null_connection_handle(void) {
    DatabaseHandle* connection = create_test_database_handle();
    ((MariadbConnection*)connection->connection_handle)->connection = NULL;
    Transaction transaction = {0};
    transaction.active = true;

    bool result = mariadb_rollback_transaction(connection, &transaction);

    TEST_ASSERT_FALSE(result);

    destroy_test_database_handle(connection);
}

int main(void) {
    UNITY_BEGIN();

    // Test mariadb_begin_transaction
    RUN_TEST(test_mariadb_begin_transaction_null_connection);
    RUN_TEST(test_mariadb_begin_transaction_null_transaction_ptr);
    RUN_TEST(test_mariadb_begin_transaction_wrong_engine_type);
    RUN_TEST(test_mariadb_begin_transaction_success);
    RUN_TEST(test_mariadb_begin_transaction_mariadb_query_failure);
    RUN_TEST(test_mariadb_begin_transaction_mariadb_autocommit_failure);
    RUN_TEST(test_mariadb_begin_transaction_null_connection_handle);

    // Test mariadb_commit_transaction
    RUN_TEST(test_mariadb_commit_transaction_null_connection);
    RUN_TEST(test_mariadb_commit_transaction_null_transaction);
    RUN_TEST(test_mariadb_commit_transaction_wrong_engine_type);
    RUN_TEST(test_mariadb_commit_transaction_success);
    RUN_TEST(test_mariadb_commit_transaction_mariadb_commit_failure);
    RUN_TEST(test_mariadb_commit_transaction_mariadb_query_commit_failure);
    RUN_TEST(test_mariadb_commit_transaction_null_connection_handle);

    // Test mariadb_rollback_transaction
    RUN_TEST(test_mariadb_rollback_transaction_null_connection);
    RUN_TEST(test_mariadb_rollback_transaction_null_transaction);
    RUN_TEST(test_mariadb_rollback_transaction_wrong_engine_type);
    RUN_TEST(test_mariadb_rollback_transaction_success);
    RUN_TEST(test_mariadb_rollback_transaction_mariadb_rollback_failure);
    RUN_TEST(test_mariadb_rollback_transaction_mariadb_query_rollback_failure);
    RUN_TEST(test_mariadb_rollback_transaction_null_connection_handle);

    return UNITY_END();
}