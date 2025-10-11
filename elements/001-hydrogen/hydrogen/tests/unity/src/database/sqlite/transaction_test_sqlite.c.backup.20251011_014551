/*
 * Unity Test File: SQLite Transaction Management
 * This file contains unit tests for SQLite transaction functions
 */

#include "../../../../../tests/unity/mocks/mock_libsqlite3.h"

#include "../../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../../src/database/database.h"
#include "../../../../../src/database/sqlite/transaction.h"
#include "../../../../../src/database/sqlite/types.h"
#include "../../../../../src/database/sqlite/connection.h"

// Forward declarations for functions being tested
bool sqlite_begin_transaction(DatabaseHandle* connection, DatabaseIsolationLevel level, Transaction** transaction);
bool sqlite_commit_transaction(DatabaseHandle* connection, Transaction* transaction);
bool sqlite_rollback_transaction(DatabaseHandle* connection, Transaction* transaction);

// Function prototypes for test functions
void test_sqlite_begin_transaction_null_connection(void);
void test_sqlite_begin_transaction_null_transaction_ptr(void);
void test_sqlite_begin_transaction_wrong_engine_type(void);
void test_sqlite_begin_transaction_success(void);
void test_sqlite_begin_transaction_sqlite_exec_failure(void);
void test_sqlite_begin_transaction_null_connection_handle(void);
void test_sqlite_commit_transaction_null_connection(void);
void test_sqlite_commit_transaction_null_transaction(void);
void test_sqlite_commit_transaction_wrong_engine_type(void);
void test_sqlite_commit_transaction_success(void);
void test_sqlite_commit_transaction_sqlite_exec_failure(void);
void test_sqlite_commit_transaction_null_connection_handle(void);
void test_sqlite_rollback_transaction_null_connection(void);
void test_sqlite_rollback_transaction_null_transaction(void);
void test_sqlite_rollback_transaction_wrong_engine_type(void);
void test_sqlite_rollback_transaction_success(void);
void test_sqlite_rollback_transaction_sqlite_exec_failure(void);
void test_sqlite_rollback_transaction_null_connection_handle(void);
DatabaseHandle* create_test_database_handle(void);
void destroy_test_database_handle(DatabaseHandle* handle);

void setUp(void) {
    // Reset all mocks to default state
    mock_libsqlite3_reset_all();
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test sqlite_begin_transaction with null parameters
void test_sqlite_begin_transaction_null_connection(void) {
    Transaction* transaction = NULL;
    bool result = sqlite_begin_transaction(NULL, DB_ISOLATION_READ_COMMITTED, &transaction);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(transaction);
}

void test_sqlite_begin_transaction_null_transaction_ptr(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    bool result = sqlite_begin_transaction(&connection, DB_ISOLATION_READ_COMMITTED, NULL);
    TEST_ASSERT_FALSE(result);
}

void test_sqlite_begin_transaction_wrong_engine_type(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MYSQL; // Wrong engine type
    Transaction* transaction = NULL;
    bool result = sqlite_begin_transaction(&connection, DB_ISOLATION_READ_COMMITTED, &transaction);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(transaction);
}

// Test sqlite_commit_transaction with null parameters
void test_sqlite_commit_transaction_null_connection(void) {
    Transaction transaction = {0};
    bool result = sqlite_commit_transaction(NULL, &transaction);
    TEST_ASSERT_FALSE(result);
}

void test_sqlite_commit_transaction_null_transaction(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    bool result = sqlite_commit_transaction(&connection, NULL);
    TEST_ASSERT_FALSE(result);
}

void test_sqlite_commit_transaction_wrong_engine_type(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MYSQL; // Wrong engine type
    Transaction transaction = {0};
    bool result = sqlite_commit_transaction(&connection, &transaction);
    TEST_ASSERT_FALSE(result);
}

// Test sqlite_rollback_transaction with null parameters
void test_sqlite_rollback_transaction_null_connection(void) {
    Transaction transaction = {0};
    bool result = sqlite_rollback_transaction(NULL, &transaction);
    TEST_ASSERT_FALSE(result);
}

void test_sqlite_rollback_transaction_null_transaction(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    bool result = sqlite_rollback_transaction(&connection, NULL);
    TEST_ASSERT_FALSE(result);
}

void test_sqlite_rollback_transaction_wrong_engine_type(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MYSQL; // Wrong engine type
    Transaction transaction = {0};
    bool result = sqlite_rollback_transaction(&connection, &transaction);
    TEST_ASSERT_FALSE(result);
}

// Helper function to create a valid database handle for testing
DatabaseHandle* create_test_database_handle(void) {
    DatabaseHandle* handle = calloc(1, sizeof(DatabaseHandle));
    if (!handle) return NULL;
    SQLiteConnection* sqlite_conn = calloc(1, sizeof(SQLiteConnection));
    if (!sqlite_conn) {
        free(handle);
        return NULL;
    }

    handle->engine_type = DB_ENGINE_SQLITE;
    handle->connection_handle = sqlite_conn;
    sqlite_conn->db = (void*)0x12345678; // Fake database pointer

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
void test_sqlite_begin_transaction_success(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction* transaction = NULL;

    // Mock successful sqlite3_exec
    mock_libsqlite3_set_sqlite3_exec_result(0); // SQLITE_OK

    bool result = sqlite_begin_transaction(connection, DB_ISOLATION_READ_COMMITTED, &transaction);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(transaction);
    TEST_ASSERT_TRUE(transaction->active);
    TEST_ASSERT_EQUAL(DB_ISOLATION_READ_COMMITTED, transaction->isolation_level);
    TEST_ASSERT_EQUAL_STRING("sqlite_tx", transaction->transaction_id);

    // Clean up
    free(transaction->transaction_id);
    free(transaction);
    destroy_test_database_handle(connection);
}

// Test transaction begin with sqlite3_exec failure
void test_sqlite_begin_transaction_sqlite_exec_failure(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction* transaction = NULL;

    // Mock sqlite3_exec failure
    mock_libsqlite3_set_sqlite3_exec_result(1); // SQLITE_ERROR
    mock_libsqlite3_set_sqlite3_errmsg_result("Mock error");

    bool result = sqlite_begin_transaction(connection, DB_ISOLATION_READ_COMMITTED, &transaction);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(transaction);

    destroy_test_database_handle(connection);
}

// Test transaction begin with null connection handle
void test_sqlite_begin_transaction_null_connection_handle(void) {
    DatabaseHandle* connection = create_test_database_handle();
    ((SQLiteConnection*)connection->connection_handle)->db = NULL;
    Transaction* transaction = NULL;

    bool result = sqlite_begin_transaction(connection, DB_ISOLATION_READ_COMMITTED, &transaction);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(transaction);

    destroy_test_database_handle(connection);
}

// Test successful commit
void test_sqlite_commit_transaction_success(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction transaction = {0};
    transaction.active = true;
    transaction.transaction_id = strdup("test_tx");

    // Mock successful sqlite3_exec
    mock_libsqlite3_set_sqlite3_exec_result(0); // SQLITE_OK

    bool result = sqlite_commit_transaction(connection, &transaction);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(transaction.active);
    TEST_ASSERT_NULL(connection->current_transaction);

    free(transaction.transaction_id);
    destroy_test_database_handle(connection);
}

// Test commit with sqlite3_exec failure
void test_sqlite_commit_transaction_sqlite_exec_failure(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction transaction = {0};
    transaction.active = true;

    // Mock sqlite3_exec failure
    mock_libsqlite3_set_sqlite3_exec_result(1); // SQLITE_ERROR
    mock_libsqlite3_set_sqlite3_errmsg_result("Mock commit error");

    bool result = sqlite_commit_transaction(connection, &transaction);

    TEST_ASSERT_FALSE(result);

    destroy_test_database_handle(connection);
}

// Test commit with null connection handle
void test_sqlite_commit_transaction_null_connection_handle(void) {
    DatabaseHandle* connection = create_test_database_handle();
    ((SQLiteConnection*)connection->connection_handle)->db = NULL;
    Transaction transaction = {0};
    transaction.active = true;

    bool result = sqlite_commit_transaction(connection, &transaction);

    TEST_ASSERT_FALSE(result);

    destroy_test_database_handle(connection);
}

// Test successful rollback
void test_sqlite_rollback_transaction_success(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction transaction = {0};
    transaction.active = true;
    transaction.transaction_id = strdup("test_tx");

    // Mock successful sqlite3_exec
    mock_libsqlite3_set_sqlite3_exec_result(0); // SQLITE_OK

    bool result = sqlite_rollback_transaction(connection, &transaction);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(transaction.active);
    TEST_ASSERT_NULL(connection->current_transaction);

    free(transaction.transaction_id);
    destroy_test_database_handle(connection);
}

// Test rollback with sqlite3_exec failure
void test_sqlite_rollback_transaction_sqlite_exec_failure(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction transaction = {0};
    transaction.active = true;

    // Mock sqlite3_exec failure
    mock_libsqlite3_set_sqlite3_exec_result(1); // SQLITE_ERROR
    mock_libsqlite3_set_sqlite3_errmsg_result("Mock rollback error");

    bool result = sqlite_rollback_transaction(connection, &transaction);

    TEST_ASSERT_FALSE(result);

    destroy_test_database_handle(connection);
}

// Test rollback with null connection handle
void test_sqlite_rollback_transaction_null_connection_handle(void) {
    DatabaseHandle* connection = create_test_database_handle();
    ((SQLiteConnection*)connection->connection_handle)->db = NULL;
    Transaction transaction = {0};
    transaction.active = true;

    bool result = sqlite_rollback_transaction(connection, &transaction);

    TEST_ASSERT_FALSE(result);

    destroy_test_database_handle(connection);
}

int main(void) {
    UNITY_BEGIN();

    // Test sqlite_begin_transaction
    RUN_TEST(test_sqlite_begin_transaction_null_connection);
    RUN_TEST(test_sqlite_begin_transaction_null_transaction_ptr);
    RUN_TEST(test_sqlite_begin_transaction_wrong_engine_type);
    RUN_TEST(test_sqlite_begin_transaction_success);
    RUN_TEST(test_sqlite_begin_transaction_sqlite_exec_failure);
    RUN_TEST(test_sqlite_begin_transaction_null_connection_handle);

    // Test sqlite_commit_transaction
    RUN_TEST(test_sqlite_commit_transaction_null_connection);
    RUN_TEST(test_sqlite_commit_transaction_null_transaction);
    RUN_TEST(test_sqlite_commit_transaction_wrong_engine_type);
    RUN_TEST(test_sqlite_commit_transaction_success);
    RUN_TEST(test_sqlite_commit_transaction_sqlite_exec_failure);
    RUN_TEST(test_sqlite_commit_transaction_null_connection_handle);

    // Test sqlite_rollback_transaction
    RUN_TEST(test_sqlite_rollback_transaction_null_connection);
    RUN_TEST(test_sqlite_rollback_transaction_null_transaction);
    RUN_TEST(test_sqlite_rollback_transaction_wrong_engine_type);
    RUN_TEST(test_sqlite_rollback_transaction_success);
    RUN_TEST(test_sqlite_rollback_transaction_sqlite_exec_failure);
    RUN_TEST(test_sqlite_rollback_transaction_null_connection_handle);

    return UNITY_END();
}