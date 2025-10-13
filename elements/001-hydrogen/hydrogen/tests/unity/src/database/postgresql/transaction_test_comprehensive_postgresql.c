/*
 * Unity Test File: PostgreSQL Transaction Management - Comprehensive Coverage
 * This file contains comprehensive unit tests for PostgreSQL transaction functions
 * targeting uncovered code paths to increase overall test coverage
 */

#include <unity/mocks/mock_libpq.h>

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/database/database.h>
#include <src/database/postgresql/transaction.h>
#include <src/database/postgresql/types.h>
#include <src/database/postgresql/connection.h>

// Forward declarations for functions being tested
bool postgresql_begin_transaction(DatabaseHandle* connection, DatabaseIsolationLevel level, Transaction** transaction);
bool postgresql_commit_transaction(DatabaseHandle* connection, Transaction* transaction);
bool postgresql_rollback_transaction(DatabaseHandle* connection, Transaction* transaction);

// Helper function prototypes
DatabaseHandle* create_test_database_handle(void);
void destroy_test_database_handle(DatabaseHandle* handle);

// Test function prototypes
void test_postgresql_begin_transaction_isolation_read_uncommitted(void);
void test_postgresql_begin_transaction_isolation_repeatable_read(void);
void test_postgresql_begin_transaction_isolation_serializable(void);
void test_postgresql_begin_transaction_isolation_default(void);
void test_postgresql_commit_transaction_pqexec_failure(void);
void test_postgresql_commit_transaction_bad_status(void);
void test_postgresql_rollback_transaction_null_connection_handle(void);
void test_postgresql_rollback_transaction_null_pg_connection(void);
void test_postgresql_begin_transaction_null_pg_conn(void);
void test_postgresql_begin_transaction_null_connection_in_pg_conn(void);

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

// ============================================================================
// Test Different Isolation Levels
// ============================================================================

void test_postgresql_begin_transaction_isolation_read_uncommitted(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction* transaction = NULL;

    // Mock successful PQexec and PQresultStatus
    void* fake_result = (void*)0xdeadbeef;
    mock_libpq_set_PQexec_result(fake_result);
    mock_libpq_set_PQresultStatus_result(PGRES_COMMAND_OK);

    bool result = postgresql_begin_transaction(connection, DB_ISOLATION_READ_UNCOMMITTED, &transaction);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(transaction);
    TEST_ASSERT_EQUAL(DB_ISOLATION_READ_UNCOMMITTED, transaction->isolation_level);

    free(transaction->transaction_id);
    free(transaction);
    destroy_test_database_handle(connection);
}

void test_postgresql_begin_transaction_isolation_repeatable_read(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction* transaction = NULL;

    // Mock successful PQexec and PQresultStatus
    void* fake_result = (void*)0xdeadbeef;
    mock_libpq_set_PQexec_result(fake_result);
    mock_libpq_set_PQresultStatus_result(PGRES_COMMAND_OK);

    bool result = postgresql_begin_transaction(connection, DB_ISOLATION_REPEATABLE_READ, &transaction);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(transaction);
    TEST_ASSERT_EQUAL(DB_ISOLATION_REPEATABLE_READ, transaction->isolation_level);

    free(transaction->transaction_id);
    free(transaction);
    destroy_test_database_handle(connection);
}

void test_postgresql_begin_transaction_isolation_serializable(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction* transaction = NULL;

    // Mock successful PQexec and PQresultStatus
    void* fake_result = (void*)0xdeadbeef;
    mock_libpq_set_PQexec_result(fake_result);
    mock_libpq_set_PQresultStatus_result(PGRES_COMMAND_OK);

    bool result = postgresql_begin_transaction(connection, DB_ISOLATION_SERIALIZABLE, &transaction);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(transaction);
    TEST_ASSERT_EQUAL(DB_ISOLATION_SERIALIZABLE, transaction->isolation_level);

    free(transaction->transaction_id);
    free(transaction);
    destroy_test_database_handle(connection);
}

void test_postgresql_begin_transaction_isolation_default(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction* transaction = NULL;

    // Mock successful PQexec and PQresultStatus
    void* fake_result = (void*)0xdeadbeef;
    mock_libpq_set_PQexec_result(fake_result);
    mock_libpq_set_PQresultStatus_result(PGRES_COMMAND_OK);

    // Use an invalid isolation level to trigger default case
    bool result = postgresql_begin_transaction(connection, (DatabaseIsolationLevel)999, &transaction);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(transaction);

    free(transaction->transaction_id);
    free(transaction);
    destroy_test_database_handle(connection);
}

// ============================================================================
// Test Commit Transaction Error Paths
// ============================================================================

void test_postgresql_commit_transaction_pqexec_failure(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction transaction = {0};
    transaction.active = true;
    transaction.transaction_id = strdup("test_tx");

    ((PostgresConnection*)connection->connection_handle)->in_transaction = true;

    // Mock PQexec returning NULL (failure)
    mock_libpq_set_PQexec_result(NULL);

    bool result = postgresql_commit_transaction(connection, &transaction);

    TEST_ASSERT_FALSE(result);

    free(transaction.transaction_id);
    destroy_test_database_handle(connection);
}

void test_postgresql_commit_transaction_bad_status(void) {
    DatabaseHandle* connection = create_test_database_handle();
    Transaction transaction = {0};
    transaction.active = true;
    transaction.transaction_id = strdup("test_tx");

    ((PostgresConnection*)connection->connection_handle)->in_transaction = true;

    // Mock PQexec success but bad status
    void* fake_result = (void*)0xdeadbeef;
    mock_libpq_set_PQexec_result(fake_result);
    mock_libpq_set_PQresultStatus_result(PGRES_FATAL_ERROR);

    bool result = postgresql_commit_transaction(connection, &transaction);

    TEST_ASSERT_FALSE(result);

    free(transaction.transaction_id);
    destroy_test_database_handle(connection);
}

// ============================================================================
// Test Rollback Transaction Error Paths
// ============================================================================

void test_postgresql_rollback_transaction_null_connection_handle(void) {
    DatabaseHandle* connection = create_test_database_handle();
    free(connection->connection_handle);
    connection->connection_handle = NULL;
    Transaction transaction = {0};
    transaction.active = true;

    bool result = postgresql_rollback_transaction(connection, &transaction);

    TEST_ASSERT_FALSE(result);

    free(connection);
}

void test_postgresql_rollback_transaction_null_pg_connection(void) {
    DatabaseHandle* connection = create_test_database_handle();
    ((PostgresConnection*)connection->connection_handle)->connection = NULL;
    Transaction transaction = {0};
    transaction.active = true;

    bool result = postgresql_rollback_transaction(connection, &transaction);

    TEST_ASSERT_FALSE(result);

    destroy_test_database_handle(connection);
}

// ============================================================================
// Test Begin Transaction Error Paths
// ============================================================================

void test_postgresql_begin_transaction_null_pg_conn(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_POSTGRESQL;
    connection.connection_handle = NULL;
    Transaction* transaction = NULL;

    bool result = postgresql_begin_transaction(&connection, DB_ISOLATION_READ_COMMITTED, &transaction);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(transaction);
}

void test_postgresql_begin_transaction_null_connection_in_pg_conn(void) {
    DatabaseHandle* connection = create_test_database_handle();
    ((PostgresConnection*)connection->connection_handle)->connection = NULL;
    Transaction* transaction = NULL;

    bool result = postgresql_begin_transaction(connection, DB_ISOLATION_READ_COMMITTED, &transaction);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(transaction);

    destroy_test_database_handle(connection);
}

int main(void) {
    UNITY_BEGIN();

    // Test different isolation levels
    RUN_TEST(test_postgresql_begin_transaction_isolation_read_uncommitted);
    RUN_TEST(test_postgresql_begin_transaction_isolation_repeatable_read);
    RUN_TEST(test_postgresql_begin_transaction_isolation_serializable);
    RUN_TEST(test_postgresql_begin_transaction_isolation_default);

    // Test commit transaction error paths
    RUN_TEST(test_postgresql_commit_transaction_pqexec_failure);
    RUN_TEST(test_postgresql_commit_transaction_bad_status);

    // Test rollback transaction error paths
    RUN_TEST(test_postgresql_rollback_transaction_null_connection_handle);
    RUN_TEST(test_postgresql_rollback_transaction_null_pg_connection);

    // Test begin transaction error paths
    RUN_TEST(test_postgresql_begin_transaction_null_pg_conn);
    RUN_TEST(test_postgresql_begin_transaction_null_connection_in_pg_conn);

    return UNITY_END();
}