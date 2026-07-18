/*
 * Unity Test File: database_engine_transaction_test_null_paths
 * This file contains unit tests for the database_engine transaction wrappers
 * (defined in src/database/database_engine_transaction.c), focusing on the
 * NULL-parameter and unregistered-engine error paths.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>

// Mock connection config for testing
static ConnectionConfig mock_config = {
    .host = NULL,
    .port = 5432,
    .database = NULL,
    .username = NULL,
    .password = NULL,
    .connection_string = NULL,
    .timeout_seconds = 30,
    .ssl_enabled = false,
    .ssl_cert_path = NULL,
    .ssl_key_path = NULL,
    .ssl_ca_path = NULL
};

// Mock database handle for testing
static DatabaseHandle mock_connection = {
    .engine_type = DB_ENGINE_AI,
    .connection_handle = NULL,
    .config = &mock_config,
    .designator = NULL,
    .status = DB_CONNECTION_DISCONNECTED,
    .connected_since = 0,
    .current_transaction = NULL,
    .prepared_statements = NULL,
    .prepared_statement_count = 0,
    .in_use = false,
    .last_health_check = 0,
    .consecutive_failures = 0
};

// Mock transaction for testing
static Transaction mock_transaction = {
    .transaction_id = NULL,
    .isolation_level = DB_ISOLATION_READ_COMMITTED,
    .started_at = 1234567890,
    .active = true,
    .engine_specific_handle = NULL
};

// Test function prototypes
void test_database_engine_begin_transaction_null_connection(void);
void test_database_engine_begin_transaction_null_transaction(void);
void test_database_engine_begin_transaction_no_engine(void);
void test_database_engine_commit_transaction_null_connection(void);
void test_database_engine_commit_transaction_null_transaction(void);
void test_database_engine_commit_transaction_no_engine(void);
void test_database_engine_rollback_transaction_null_connection(void);
void test_database_engine_rollback_transaction_null_transaction(void);
void test_database_engine_rollback_transaction_no_engine(void);

void setUp(void) {
    database_engine_init();

    // Initialize string fields for mock structures
    mock_config.host = strdup("localhost");
    mock_config.database = strdup("testdb");
    mock_config.username = strdup("testuser");
    mock_config.password = strdup("testpass");
    mock_connection.designator = strdup("TEST-CONN");
    mock_transaction.transaction_id = strdup("test-tx-123");

    // Initialize mutex for mock connection
    pthread_mutex_init(&mock_connection.connection_lock, NULL);
}

void tearDown(void) {
    pthread_mutex_destroy(&mock_connection.connection_lock);

    if (mock_config.host) {
        free(mock_config.host);
        mock_config.host = NULL;
    }
    if (mock_config.database) {
        free(mock_config.database);
        mock_config.database = NULL;
    }
    if (mock_config.username) {
        free(mock_config.username);
        mock_config.username = NULL;
    }
    if (mock_config.password) {
        free(mock_config.password);
        mock_config.password = NULL;
    }
    if (mock_connection.designator) {
        free(mock_connection.designator);
        mock_connection.designator = NULL;
    }
    if (mock_transaction.transaction_id) {
        free(mock_transaction.transaction_id);
        mock_transaction.transaction_id = NULL;
    }
}

// Test transaction functions
void test_database_engine_begin_transaction_null_connection(void) {
    // Test begin transaction with NULL connection
    Transaction* transaction = NULL;
    bool result = database_engine_begin_transaction(NULL, DB_ISOLATION_READ_COMMITTED, &transaction);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(transaction);
}

void test_database_engine_begin_transaction_null_transaction(void) {
    // Test begin transaction with NULL transaction pointer
    bool result = database_engine_begin_transaction(&mock_connection, DB_ISOLATION_READ_COMMITTED, NULL);
    TEST_ASSERT_FALSE(result);
}

void test_database_engine_begin_transaction_no_engine(void) {
    // Test begin transaction when no engine is registered
    DatabaseHandle test_conn = mock_connection;
    test_conn.engine_type = DB_ENGINE_AI; // AI engine not registered
    Transaction* transaction = NULL;
    bool result = database_engine_begin_transaction(&test_conn, DB_ISOLATION_READ_COMMITTED, &transaction);
    TEST_ASSERT_FALSE(result);
}

void test_database_engine_commit_transaction_null_connection(void) {
    // Test commit transaction with NULL connection
    Transaction test_tx = mock_transaction;
    bool result = database_engine_commit_transaction(NULL, &test_tx);
    TEST_ASSERT_FALSE(result);
}

void test_database_engine_commit_transaction_null_transaction(void) {
    // Test commit transaction with NULL transaction
    bool result = database_engine_commit_transaction(&mock_connection, NULL);
    TEST_ASSERT_FALSE(result);
}

void test_database_engine_commit_transaction_no_engine(void) {
    // Test commit transaction when no engine is registered
    DatabaseHandle test_conn = mock_connection;
    test_conn.engine_type = DB_ENGINE_AI; // AI engine not registered
    Transaction test_tx = mock_transaction;
    bool result = database_engine_commit_transaction(&test_conn, &test_tx);
    TEST_ASSERT_FALSE(result);
}

void test_database_engine_rollback_transaction_null_connection(void) {
    // Test rollback transaction with NULL connection
    Transaction test_tx = mock_transaction;
    bool result = database_engine_rollback_transaction(NULL, &test_tx);
    TEST_ASSERT_FALSE(result);
}

void test_database_engine_rollback_transaction_null_transaction(void) {
    // Test rollback transaction with NULL transaction
    bool result = database_engine_rollback_transaction(&mock_connection, NULL);
    TEST_ASSERT_FALSE(result);
}

void test_database_engine_rollback_transaction_no_engine(void) {
    // Test rollback transaction when no engine is registered
    DatabaseHandle test_conn = mock_connection;
    test_conn.engine_type = DB_ENGINE_AI; // AI engine not registered
    Transaction test_tx = mock_transaction;
    bool result = database_engine_rollback_transaction(&test_conn, &test_tx);
    TEST_ASSERT_FALSE(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_engine_begin_transaction_null_connection);
    RUN_TEST(test_database_engine_begin_transaction_null_transaction);
    RUN_TEST(test_database_engine_begin_transaction_no_engine);

    RUN_TEST(test_database_engine_commit_transaction_null_connection);
    RUN_TEST(test_database_engine_commit_transaction_null_transaction);
    RUN_TEST(test_database_engine_commit_transaction_no_engine);

    RUN_TEST(test_database_engine_rollback_transaction_null_connection);
    RUN_TEST(test_database_engine_rollback_transaction_null_transaction);
    RUN_TEST(test_database_engine_rollback_transaction_no_engine);

    return UNITY_END();
}
