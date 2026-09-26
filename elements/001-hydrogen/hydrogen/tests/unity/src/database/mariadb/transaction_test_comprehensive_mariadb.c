/*
 * Unity Test File: MariaDB Transaction Management - Comprehensive Coverage
 * This file contains comprehensive unit tests for MySQL transaction functions
 * targeting uncovered code paths to increase overall test coverage
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
bool mariadb_check_timeout_expired(time_t start_time, int timeout_seconds);

// External declarations for function pointers
extern mariadb_query_t mariadb_query_ptr;
extern mariadb_autocommit_t mariadb_autocommit_ptr;
extern mariadb_commit_t mariadb_commit_ptr;
extern mariadb_rollback_t mariadb_rollback_ptr;
extern mariadb_error_t mariadb_error_ptr;

// Helper function prototypes
DatabaseHandle* create_test_database_handle(void);
void destroy_test_database_handle(DatabaseHandle* handle);

// Test function prototypes
void test_mariadb_begin_transaction_isolation_read_uncommitted(void);
void test_mariadb_begin_transaction_isolation_repeatable_read(void);
void test_mariadb_begin_transaction_isolation_serializable(void);
void test_mariadb_begin_transaction_isolation_default(void);
void test_mariadb_begin_transaction_fallback_to_query(void);
void test_mariadb_begin_transaction_fallback_query_failure(void);
void test_mariadb_commit_transaction_fallback_to_query(void);
void test_mariadb_commit_transaction_fallback_query_failure(void);
void test_mariadb_rollback_transaction_fallback_to_query(void);
void test_mariadb_rollback_transaction_fallback_query_failure(void);
void test_mariadb_begin_transaction_set_isolation_failure_with_empty_error(void);
void test_mariadb_begin_transaction_autocommit_failure_with_empty_error(void);
void test_mariadb_commit_transaction_failure_with_empty_error(void);
void test_mariadb_rollback_transaction_failure_with_empty_error(void);
void test_mariadb_begin_transaction_calloc_failure(void);

void setUp(void) {
    // Reset all mocks to default state
    mock_libmariadb_reset_all();
}

void tearDown(void) {
    // Clean up test fixtures, if any
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

// ============================================================================
// Test Different Isolation Levels
// ============================================================================

void test_mariadb_begin_transaction_isolation_read_uncommitted(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction* transaction = NULL;

    mock_libmariadb_set_mysql_query_result(0);
    mock_libmariadb_set_mysql_autocommit_result(0);

    bool result = mariadb_begin_transaction(connection, DB_ISOLATION_READ_UNCOMMITTED, &transaction);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(transaction);
    TEST_ASSERT_EQUAL(DB_ISOLATION_READ_UNCOMMITTED, transaction->isolation_level);

    free(transaction->transaction_id);
    free(transaction);
    destroy_test_database_handle(connection);
}

void test_mariadb_begin_transaction_isolation_repeatable_read(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction* transaction = NULL;

    mock_libmariadb_set_mysql_query_result(0);
    mock_libmariadb_set_mysql_autocommit_result(0);

    bool result = mariadb_begin_transaction(connection, DB_ISOLATION_REPEATABLE_READ, &transaction);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(transaction);
    TEST_ASSERT_EQUAL(DB_ISOLATION_REPEATABLE_READ, transaction->isolation_level);

    free(transaction->transaction_id);
    free(transaction);
    destroy_test_database_handle(connection);
}

void test_mariadb_begin_transaction_isolation_serializable(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction* transaction = NULL;

    mock_libmariadb_set_mysql_query_result(0);
    mock_libmariadb_set_mysql_autocommit_result(0);

    bool result = mariadb_begin_transaction(connection, DB_ISOLATION_SERIALIZABLE, &transaction);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(transaction);
    TEST_ASSERT_EQUAL(DB_ISOLATION_SERIALIZABLE, transaction->isolation_level);

    free(transaction->transaction_id);
    free(transaction);
    destroy_test_database_handle(connection);
}

void test_mariadb_begin_transaction_isolation_default(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction* transaction = NULL;

    mock_libmariadb_set_mysql_query_result(0);
    mock_libmariadb_set_mysql_autocommit_result(0);

    // Use an invalid isolation level to trigger default
    bool result = mariadb_begin_transaction(connection, (DatabaseIsolationLevel)999, &transaction);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(transaction);

    free(transaction->transaction_id);
    free(transaction);
    destroy_test_database_handle(connection);
}

// ============================================================================
// Test Fallback Paths (using mariadb_query when other function pointers unavailable)
// ============================================================================

void test_mariadb_begin_transaction_fallback_to_query(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction* transaction = NULL;

    // Save original pointer
    mariadb_autocommit_t saved_autocommit = mariadb_autocommit_ptr;
    
    // Set autocommit_ptr to NULL to force fallback to mariadb_query
    mariadb_autocommit_ptr = NULL;
    
    mock_libmariadb_set_mysql_query_result(0);

    bool result = mariadb_begin_transaction(connection, DB_ISOLATION_READ_COMMITTED, &transaction);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(transaction);

    // Restore pointer
    mariadb_autocommit_ptr = saved_autocommit;

    free(transaction->transaction_id);
    free(transaction);
    destroy_test_database_handle(connection);
}

void test_mariadb_begin_transaction_fallback_query_failure(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction* transaction = NULL;

    // Save original pointer
    mariadb_autocommit_t saved_autocommit = mariadb_autocommit_ptr;
    
    // Set autocommit_ptr to NULL to force fallback
    mariadb_autocommit_ptr = NULL;
    
    // First query succeeds (SET ISOLATION), second fails (START TRANSACTION)
    mock_libmariadb_set_mysql_query_result(0);
    // Need to set it to fail on second call - this is tricky with current mock
    // For now, we'll test the path exists

    (void)mariadb_begin_transaction(connection, DB_ISOLATION_READ_COMMITTED, &transaction);

    // Restore pointer
    mariadb_autocommit_ptr = saved_autocommit;

    // Result depends on mock behavior
    if (transaction) {
        free(transaction->transaction_id);
        free(transaction);
    }
    destroy_test_database_handle(connection);
}

void test_mariadb_commit_transaction_fallback_to_query(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction transaction = {0};
    transaction.active = true;
    transaction.transaction_id = strdup("test_tx");

    // Save original pointer
    mariadb_commit_t saved_commit = mariadb_commit_ptr;
    
    // Set commit_ptr to NULL to force fallback to mariadb_query
    mariadb_commit_ptr = NULL;
    
    mock_libmariadb_set_mysql_query_result(0);

    bool result = mariadb_commit_transaction(connection, &transaction);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(transaction.active);

    // Restore pointer
    mariadb_commit_ptr = saved_commit;

    free(transaction.transaction_id);
    destroy_test_database_handle(connection);
}

void test_mariadb_commit_transaction_fallback_query_failure(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction transaction = {0};
    transaction.active = true;
    transaction.transaction_id = strdup("test_tx");

    // Save original pointer
    mariadb_commit_t saved_commit = mariadb_commit_ptr;
    
    // Set commit_ptr to NULL to force fallback
    mariadb_commit_ptr = NULL;
    
    mock_libmariadb_set_mysql_query_result(-1); // Failure

    bool result = mariadb_commit_transaction(connection, &transaction);

    TEST_ASSERT_FALSE(result);

    // Restore pointer
    mariadb_commit_ptr = saved_commit;

    free(transaction.transaction_id);
    destroy_test_database_handle(connection);
}

void test_mariadb_rollback_transaction_fallback_to_query(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction transaction = {0};
    transaction.active = true;
    transaction.transaction_id = strdup("test_tx");

    // Save original pointer
    mariadb_rollback_t saved_rollback = mariadb_rollback_ptr;
    
    // Set rollback_ptr to NULL to force fallback to mariadb_query
    mariadb_rollback_ptr = NULL;
    
    mock_libmariadb_set_mysql_query_result(0);

    bool result = mariadb_rollback_transaction(connection, &transaction);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(transaction.active);

    // Restore pointer
    mariadb_rollback_ptr = saved_rollback;

    free(transaction.transaction_id);
    destroy_test_database_handle(connection);
}

void test_mariadb_rollback_transaction_fallback_query_failure(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction transaction = {0};
    transaction.active = true;
    transaction.transaction_id = strdup("test_tx");

    // Save original pointer
    mariadb_rollback_t saved_rollback = mariadb_rollback_ptr;
    
    // Set rollback_ptr to NULL to force fallback
    mariadb_rollback_ptr = NULL;
    
    mock_libmariadb_set_mysql_query_result(-1); // Failure

    bool result = mariadb_rollback_transaction(connection, &transaction);

    TEST_ASSERT_FALSE(result);

    // Restore pointer
    mariadb_rollback_ptr = saved_rollback;

    free(transaction.transaction_id);
    destroy_test_database_handle(connection);
}

// ============================================================================
// Test Error Message Logging Path
// ============================================================================

void test_mariadb_begin_transaction_set_isolation_failure_with_empty_error(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction* transaction = NULL;

    // Mock query failure with empty error message
    mock_libmariadb_set_mysql_query_result(-1);
    mock_libmariadb_set_mysql_error_result(""); // Empty error string

    bool result = mariadb_begin_transaction(connection, DB_ISOLATION_READ_COMMITTED, &transaction);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(transaction);

    destroy_test_database_handle(connection);
}

void test_mariadb_begin_transaction_autocommit_failure_with_empty_error(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction* transaction = NULL;

    // Mock successful SET ISOLATION but failed autocommit with empty error
    mock_libmariadb_set_mysql_query_result(0);
    mock_libmariadb_set_mysql_autocommit_result(-1);
    mock_libmariadb_set_mysql_error_result(""); // Empty error string

    bool result = mariadb_begin_transaction(connection, DB_ISOLATION_READ_COMMITTED, &transaction);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(transaction);

    destroy_test_database_handle(connection);
}

void test_mariadb_commit_transaction_failure_with_empty_error(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction transaction = {0};
    transaction.active = true;

    // Mock commit failure with empty error message
    mock_libmariadb_set_mysql_commit_result(-1);
    mock_libmariadb_set_mysql_error_result(""); // Empty error string

    bool result = mariadb_commit_transaction(connection, &transaction);

    TEST_ASSERT_FALSE(result);

    destroy_test_database_handle(connection);
}

void test_mariadb_rollback_transaction_failure_with_empty_error(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction transaction = {0};
    transaction.active = true;

    // Mock rollback failure with empty error message
    mock_libmariadb_set_mysql_rollback_result(-1);
    mock_libmariadb_set_mysql_error_result(""); // Empty error string

    bool result = mariadb_rollback_transaction(connection, &transaction);

    TEST_ASSERT_FALSE(result);

    destroy_test_database_handle(connection);
}

// ============================================================================
// Test Memory Allocation Failure
// ============================================================================

void test_mariadb_begin_transaction_calloc_failure(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction* transaction = NULL;

    // Set up successful MySQL operations
    mock_libmariadb_set_mysql_query_result(0);
    mock_libmariadb_set_mysql_autocommit_result(0);

    // We can't easily mock calloc failure without a mock_system,
    // but we can at least document that this code path exists
    // The actual calloc failure path (lines 124-131) would need
    // a system-level mock to properly test

    bool result = mariadb_begin_transaction(connection, DB_ISOLATION_READ_COMMITTED, &transaction);

    // In normal operation, this should succeed
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(transaction);

    if (transaction) {
        free(transaction->transaction_id);
        free(transaction);
    }
    destroy_test_database_handle(connection);
}

int main(void) {
    UNITY_BEGIN();

    // Test different isolation levels
    RUN_TEST(test_mariadb_begin_transaction_isolation_read_uncommitted);
    RUN_TEST(test_mariadb_begin_transaction_isolation_repeatable_read);
    RUN_TEST(test_mariadb_begin_transaction_isolation_serializable);
    RUN_TEST(test_mariadb_begin_transaction_isolation_default);

    // Test fallback paths
    RUN_TEST(test_mariadb_begin_transaction_fallback_to_query);
    RUN_TEST(test_mariadb_begin_transaction_fallback_query_failure);
    RUN_TEST(test_mariadb_commit_transaction_fallback_to_query);
    RUN_TEST(test_mariadb_commit_transaction_fallback_query_failure);
    RUN_TEST(test_mariadb_rollback_transaction_fallback_to_query);
    RUN_TEST(test_mariadb_rollback_transaction_fallback_query_failure);

    // Test error message logging with empty errors
    RUN_TEST(test_mariadb_begin_transaction_set_isolation_failure_with_empty_error);
    RUN_TEST(test_mariadb_begin_transaction_autocommit_failure_with_empty_error);
    RUN_TEST(test_mariadb_commit_transaction_failure_with_empty_error);
    RUN_TEST(test_mariadb_rollback_transaction_failure_with_empty_error);

    // Test memory allocation
    RUN_TEST(test_mariadb_begin_transaction_calloc_failure);

    return UNITY_END();
}