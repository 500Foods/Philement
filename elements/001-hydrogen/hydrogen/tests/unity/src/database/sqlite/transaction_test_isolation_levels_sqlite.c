/*
 * Unity Test File: SQLite Transaction Isolation Levels
 * This file contains unit tests for SQLite transaction isolation level handling
 */

#include <unity/mocks/mock_libsqlite3.h>

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/database/database.h>
#include <src/database/sqlite/transaction.h>
#include <src/database/sqlite/types.h>
#include <src/database/sqlite/connection.h>

// Forward declarations for functions being tested
bool sqlite_begin_transaction(DatabaseHandle* connection, DatabaseIsolationLevel level, Transaction** transaction);
bool sqlite_commit_transaction(DatabaseHandle* connection, Transaction* transaction);
bool sqlite_rollback_transaction(DatabaseHandle* connection, Transaction* transaction);

// Function prototypes for test functions
void test_sqlite_begin_transaction_read_uncommitted(void);
void test_sqlite_begin_transaction_repeatable_read(void);
void test_sqlite_begin_transaction_serializable(void);
void test_sqlite_begin_transaction_invalid_level(void);
void test_sqlite_begin_transaction_all_isolation_levels(void);
DatabaseHandle* create_test_database_handle(void);
void destroy_test_database_handle(DatabaseHandle* handle);

void setUp(void) {
    // Reset all mocks to default state
    mock_libsqlite3_reset_all();
}

void tearDown(void) {
    // Clean up test fixtures, if any
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

// Test DB_ISOLATION_READ_UNCOMMITTED isolation level
void test_sqlite_begin_transaction_read_uncommitted(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction* transaction = NULL;

    // Mock successful sqlite3_exec
    mock_libsqlite3_set_sqlite3_exec_result(0); // SQLITE_OK

    bool result = sqlite_begin_transaction(connection, DB_ISOLATION_READ_UNCOMMITTED, &transaction);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(transaction);
    TEST_ASSERT_TRUE(transaction->active);
    TEST_ASSERT_EQUAL(DB_ISOLATION_READ_UNCOMMITTED, transaction->isolation_level);
    TEST_ASSERT_EQUAL_STRING("sqlite_tx", transaction->transaction_id);

    // Clean up
    free(transaction->transaction_id);
    free(transaction);
    destroy_test_database_handle(connection);
}

// Test DB_ISOLATION_REPEATABLE_READ isolation level
void test_sqlite_begin_transaction_repeatable_read(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction* transaction = NULL;

    // Mock successful sqlite3_exec
    mock_libsqlite3_set_sqlite3_exec_result(0); // SQLITE_OK

    bool result = sqlite_begin_transaction(connection, DB_ISOLATION_REPEATABLE_READ, &transaction);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(transaction);
    TEST_ASSERT_TRUE(transaction->active);
    TEST_ASSERT_EQUAL(DB_ISOLATION_REPEATABLE_READ, transaction->isolation_level);
    TEST_ASSERT_EQUAL_STRING("sqlite_tx", transaction->transaction_id);

    // Clean up
    free(transaction->transaction_id);
    free(transaction);
    destroy_test_database_handle(connection);
}

// Test DB_ISOLATION_SERIALIZABLE isolation level
void test_sqlite_begin_transaction_serializable(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction* transaction = NULL;

    // Mock successful sqlite3_exec
    mock_libsqlite3_set_sqlite3_exec_result(0); // SQLITE_OK

    bool result = sqlite_begin_transaction(connection, DB_ISOLATION_SERIALIZABLE, &transaction);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(transaction);
    TEST_ASSERT_TRUE(transaction->active);
    TEST_ASSERT_EQUAL(DB_ISOLATION_SERIALIZABLE, transaction->isolation_level);
    TEST_ASSERT_EQUAL_STRING("sqlite_tx", transaction->transaction_id);

    // Clean up
    free(transaction->transaction_id);
    free(transaction);
    destroy_test_database_handle(connection);
}

// Test with an invalid isolation level (to trigger default case)
void test_sqlite_begin_transaction_invalid_level(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction* transaction = NULL;

    // Mock successful sqlite3_exec
    mock_libsqlite3_set_sqlite3_exec_result(0); // SQLITE_OK

    // Use an out-of-range value to trigger default case
    bool result = sqlite_begin_transaction(connection, (DatabaseIsolationLevel)999, &transaction);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(transaction);
    TEST_ASSERT_TRUE(transaction->active);

    // Clean up
    free(transaction->transaction_id);
    free(transaction);
    destroy_test_database_handle(connection);
}

// Test all isolation levels in sequence
void test_sqlite_begin_transaction_all_isolation_levels(void) {
    DatabaseHandle* connection = create_test_database_handle();
    
    // Mock successful sqlite3_exec for all operations
    mock_libsqlite3_set_sqlite3_exec_result(0); // SQLITE_OK

    // Test each isolation level
    DatabaseIsolationLevel levels[] = {
        DB_ISOLATION_READ_UNCOMMITTED,
        DB_ISOLATION_READ_COMMITTED,
        DB_ISOLATION_REPEATABLE_READ,
        DB_ISOLATION_SERIALIZABLE
    };

    for (int i = 0; i < 4; i++) {
        Transaction* transaction = NULL;
        bool result = sqlite_begin_transaction(connection, levels[i], &transaction);

        TEST_ASSERT_TRUE(result);
        TEST_ASSERT_NOT_NULL(transaction);
        TEST_ASSERT_EQUAL(levels[i], transaction->isolation_level);

        // Clean up
        free(transaction->transaction_id);
        free(transaction);
        
        // Reset mock for next iteration
        mock_libsqlite3_reset_all();
        mock_libsqlite3_set_sqlite3_exec_result(0);
    }

    destroy_test_database_handle(connection);
}

int main(void) {
    UNITY_BEGIN();

    // Test different isolation levels
    RUN_TEST(test_sqlite_begin_transaction_read_uncommitted);
    RUN_TEST(test_sqlite_begin_transaction_repeatable_read);
    RUN_TEST(test_sqlite_begin_transaction_serializable);
    RUN_TEST(test_sqlite_begin_transaction_invalid_level);
    RUN_TEST(test_sqlite_begin_transaction_all_isolation_levels);

    return UNITY_END();
}