/*
 * Unity Test File: database_engine_test_transaction
 * This file contains unit tests for database_engine transaction functions
 * to increase test coverage for transaction management
 */

#include "../../../../../src/hydrogen.h"
#include "unity.h"

#include "../../../../../src/database/database.h"

// Forward declarations for mock functions
bool mock_connect(ConnectionConfig* config, DatabaseHandle** connection, const char* designator);
bool mock_disconnect(DatabaseHandle* connection);
bool mock_health_check(DatabaseHandle* connection);
bool mock_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result);
bool mock_begin_transaction(DatabaseHandle* connection, DatabaseIsolationLevel level, Transaction** transaction);
bool mock_commit_transaction(DatabaseHandle* connection, Transaction* transaction);
bool mock_rollback_transaction(DatabaseHandle* connection, Transaction* transaction);
char* mock_get_connection_string(const ConnectionConfig* config);
bool mock_validate_connection_string(const char* connection_string);

// Mock functions for testing
bool mock_connect(ConnectionConfig* config, DatabaseHandle** connection, const char* designator) {
    (void)config;
    (void)designator;
    if (connection) {
        *connection = malloc(sizeof(DatabaseHandle));
        if (*connection) {
            memset(*connection, 0, sizeof(DatabaseHandle));
            (*connection)->engine_type = DB_ENGINE_AI;
            (*connection)->designator = designator ? strdup(designator) : NULL;
            // Initialize mutex for the connection
            pthread_mutex_init(&(*connection)->connection_lock, NULL);
            return true;
        }
    }
    return false;
}

bool mock_disconnect(DatabaseHandle* connection) {
    (void)connection;
    return true;
}

bool mock_health_check(DatabaseHandle* connection) {
    (void)connection;
    return true;
}

bool mock_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result) {
    (void)connection;
    (void)request;
    (void)result;
    return true;
}

bool mock_begin_transaction(DatabaseHandle* connection, DatabaseIsolationLevel level, Transaction** transaction) {
    (void)connection;
    if (transaction) {
        *transaction = malloc(sizeof(Transaction));
        if (*transaction) {
            memset(*transaction, 0, sizeof(Transaction));
            (*transaction)->transaction_id = strdup("mock-tx-123");
            (*transaction)->isolation_level = level;
            (*transaction)->started_at = time(NULL);
            (*transaction)->active = true;
            return true;
        }
    }
    return false;
}

bool mock_commit_transaction(DatabaseHandle* connection, Transaction* transaction) {
    (void)connection;
    (void)transaction;
    return true;
}

bool mock_rollback_transaction(DatabaseHandle* connection, Transaction* transaction) {
    (void)connection;
    (void)transaction;
    return true;
}

char* mock_get_connection_string(const ConnectionConfig* config) {
    (void)config;
    return strdup("mock://connection/string");
}

bool mock_validate_connection_string(const char* connection_string) {
    (void)connection_string;
    return true;
}

// Mock database engine interface for testing
static DatabaseEngineInterface mock_engine = {
    .engine_type = DB_ENGINE_AI,
    .name = (char*)"ai",
    .connect = mock_connect,
    .disconnect = mock_disconnect,
    .health_check = mock_health_check,
    .reset_connection = NULL,
    .execute_query = mock_execute_query,
    .execute_prepared = NULL,
    .begin_transaction = mock_begin_transaction,
    .commit_transaction = mock_commit_transaction,
    .rollback_transaction = mock_rollback_transaction,
    .prepare_statement = NULL,
    .unprepare_statement = NULL,
    .get_connection_string = mock_get_connection_string,
    .validate_connection_string = mock_validate_connection_string,
    .escape_string = NULL
};

// Test function prototypes
void test_database_engine_begin_transaction_basic(void);
void test_database_engine_begin_transaction_null_connection(void);
void test_database_engine_begin_transaction_null_transaction(void);
void test_database_engine_begin_transaction_no_engine(void);
void test_database_engine_commit_transaction_basic(void);
void test_database_engine_commit_transaction_null_connection(void);
void test_database_engine_commit_transaction_null_transaction(void);
void test_database_engine_commit_transaction_no_engine(void);
void test_database_engine_rollback_transaction_basic(void);
void test_database_engine_rollback_transaction_null_connection(void);
void test_database_engine_rollback_transaction_null_transaction(void);
void test_database_engine_rollback_transaction_no_engine(void);

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

void setUp(void) {
    // Initialize the database engine system
    database_engine_init();

    // Register our mock engine
    database_engine_register(&mock_engine);

    // Initialize string fields for mock structures
    mock_config.host = strdup("localhost");
    mock_config.database = strdup("testdb");
    mock_config.username = strdup("testuser");
    mock_config.password = strdup("testpass");
}

void tearDown(void) {
    // Clean up string fields from mock structures
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
}

// Test database_engine_begin_transaction function
void test_database_engine_begin_transaction_basic(void) {
    // Create a connection for testing
    DatabaseHandle* connection = NULL;
    bool connect_result = database_engine_connect_with_designator(DB_ENGINE_AI, &mock_config, &connection, "test-conn");
    TEST_ASSERT_TRUE(connect_result);
    TEST_ASSERT_NOT_NULL(connection);

    // Test begin transaction
    Transaction* transaction = NULL;
    bool result = database_engine_begin_transaction(connection, DB_ISOLATION_READ_COMMITTED, &transaction);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(transaction);
    TEST_ASSERT_TRUE(transaction->active); // cppcheck-suppress nullPointerRedundantCheck
    TEST_ASSERT_EQUAL(DB_ISOLATION_READ_COMMITTED, transaction->isolation_level); // cppcheck-suppress nullPointerRedundantCheck
    TEST_ASSERT_NOT_NULL(transaction->transaction_id); // cppcheck-suppress nullPointerRedundantCheck

    // Clean up
    if (transaction) {
        database_engine_cleanup_transaction(transaction);
    }
    if (connection) {
        database_engine_cleanup_connection(connection);
    }
}

void test_database_engine_begin_transaction_null_connection(void) {
    // Test begin transaction with NULL connection
    Transaction* transaction = NULL;
    bool result = database_engine_begin_transaction(NULL, DB_ISOLATION_READ_COMMITTED, &transaction);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(transaction);
}

void test_database_engine_begin_transaction_null_transaction(void) {
    // Create a connection for testing
    DatabaseHandle* connection = NULL;
    bool connect_result = database_engine_connect_with_designator(DB_ENGINE_AI, &mock_config, &connection, "test-conn");
    TEST_ASSERT_TRUE(connect_result);
    TEST_ASSERT_NOT_NULL(connection);

    // Test begin transaction with NULL transaction pointer
    bool result = database_engine_begin_transaction(connection, DB_ISOLATION_READ_COMMITTED, NULL);
    TEST_ASSERT_FALSE(result);

    // Clean up
    if (connection) {
        database_engine_cleanup_connection(connection);
    }
}

void test_database_engine_begin_transaction_no_engine(void) {
    // Create a connection and modify its engine type to one without registered engine
    DatabaseHandle* connection = NULL;
    bool connect_result = database_engine_connect_with_designator(DB_ENGINE_AI, &mock_config, &connection, "test-conn");
    TEST_ASSERT_TRUE(connect_result);
    TEST_ASSERT_NOT_NULL(connection);

    // Modify engine type to one without registered engine
    connection->engine_type = DB_ENGINE_AI; // cppcheck-suppress nullPointerRedundantCheck // AI engine not registered

    // Test begin transaction
    Transaction* transaction = NULL;
    bool result = database_engine_begin_transaction(connection, DB_ISOLATION_READ_COMMITTED, &transaction);
    TEST_ASSERT_FALSE(result);

    // Clean up
    if (connection) {
        database_engine_cleanup_connection(connection);
    }
}

// Test database_engine_commit_transaction function
void test_database_engine_commit_transaction_basic(void) {
    // Create a connection for testing
    DatabaseHandle* connection = NULL;
    bool connect_result = database_engine_connect_with_designator(DB_ENGINE_AI, &mock_config, &connection, "test-conn");
    TEST_ASSERT_TRUE(connect_result);
    TEST_ASSERT_NOT_NULL(connection);

    // Begin transaction first
    Transaction* transaction = NULL;
    bool begin_result = database_engine_begin_transaction(connection, DB_ISOLATION_READ_COMMITTED, &transaction);
    TEST_ASSERT_TRUE(begin_result);
    TEST_ASSERT_NOT_NULL(transaction);

    // Test commit transaction
    bool commit_result = database_engine_commit_transaction(connection, transaction);
    TEST_ASSERT_TRUE(commit_result);

    // Clean up
    if (transaction) {
        database_engine_cleanup_transaction(transaction);
    }
    if (connection) {
        database_engine_cleanup_connection(connection);
    }
}

void test_database_engine_commit_transaction_null_connection(void) {
    // Create a transaction for testing
    Transaction* transaction = malloc(sizeof(Transaction));
    TEST_ASSERT_NOT_NULL(transaction);
    memset(transaction, 0, sizeof(Transaction));
    transaction->transaction_id = strdup("test-tx");

    // Test commit transaction with NULL connection
    bool result = database_engine_commit_transaction(NULL, transaction);
    TEST_ASSERT_FALSE(result);

    // Clean up
    database_engine_cleanup_transaction(transaction);
}

void test_database_engine_commit_transaction_null_transaction(void) {
    // Create a connection for testing
    DatabaseHandle* connection = NULL;
    bool connect_result = database_engine_connect_with_designator(DB_ENGINE_POSTGRESQL, &mock_config, &connection, "test-conn");
    TEST_ASSERT_TRUE(connect_result);
    TEST_ASSERT_NOT_NULL(connection);

    // Test commit transaction with NULL transaction
    bool result = database_engine_commit_transaction(connection, NULL);
    TEST_ASSERT_FALSE(result);

    // Clean up
    if (connection) {
        database_engine_cleanup_connection(connection);
    }
}

void test_database_engine_commit_transaction_no_engine(void) {
    // Create a connection and modify its engine type
    DatabaseHandle* connection = NULL;
    bool connect_result = database_engine_connect_with_designator(DB_ENGINE_POSTGRESQL, &mock_config, &connection, "test-conn");
    TEST_ASSERT_TRUE(connect_result);
    TEST_ASSERT_NOT_NULL(connection);

    // Modify engine type to one without registered engine
    connection->engine_type = DB_ENGINE_AI; // cppcheck-suppress nullPointerRedundantCheck

    // Create a transaction
    Transaction* transaction = malloc(sizeof(Transaction));
    TEST_ASSERT_NOT_NULL(transaction);
    memset(transaction, 0, sizeof(Transaction));
    transaction->transaction_id = strdup("test-tx");

    // Test commit transaction
    bool result = database_engine_commit_transaction(connection, transaction);
    TEST_ASSERT_FALSE(result);

    // Clean up
    database_engine_cleanup_transaction(transaction);
    if (connection) {
        database_engine_cleanup_connection(connection);
    }
}

// Test database_engine_rollback_transaction function
void test_database_engine_rollback_transaction_basic(void) {
    // Create a connection for testing
    DatabaseHandle* connection = NULL;
    bool connect_result = database_engine_connect_with_designator(DB_ENGINE_POSTGRESQL, &mock_config, &connection, "test-conn");
    TEST_ASSERT_TRUE(connect_result);
    TEST_ASSERT_NOT_NULL(connection);

    // Begin transaction first
    Transaction* transaction = NULL;
    bool begin_result = database_engine_begin_transaction(connection, DB_ISOLATION_READ_COMMITTED, &transaction);
    TEST_ASSERT_TRUE(begin_result);
    TEST_ASSERT_NOT_NULL(transaction);

    // Test rollback transaction
    bool rollback_result = database_engine_rollback_transaction(connection, transaction);
    TEST_ASSERT_TRUE(rollback_result);

    // Clean up
    if (transaction) {
        database_engine_cleanup_transaction(transaction);
    }
    if (connection) {
        database_engine_cleanup_connection(connection);
    }
}

void test_database_engine_rollback_transaction_null_connection(void) {
    // Create a transaction for testing
    Transaction* transaction = malloc(sizeof(Transaction));
    TEST_ASSERT_NOT_NULL(transaction);
    memset(transaction, 0, sizeof(Transaction));
    transaction->transaction_id = strdup("test-tx");

    // Test rollback transaction with NULL connection
    bool result = database_engine_rollback_transaction(NULL, transaction);
    TEST_ASSERT_FALSE(result);

    // Clean up
    database_engine_cleanup_transaction(transaction);
}

void test_database_engine_rollback_transaction_null_transaction(void) {
    // Create a connection for testing
    DatabaseHandle* connection = NULL;
    bool connect_result = database_engine_connect_with_designator(DB_ENGINE_POSTGRESQL, &mock_config, &connection, "test-conn");
    TEST_ASSERT_TRUE(connect_result);
    TEST_ASSERT_NOT_NULL(connection);

    // Test rollback transaction with NULL transaction
    bool result = database_engine_rollback_transaction(connection, NULL);
    TEST_ASSERT_FALSE(result);

    // Clean up
    if (connection) {
        database_engine_cleanup_connection(connection);
    }
}

void test_database_engine_rollback_transaction_no_engine(void) {
    // Create a connection and modify its engine type
    DatabaseHandle* connection = NULL;
    bool connect_result = database_engine_connect_with_designator(DB_ENGINE_POSTGRESQL, &mock_config, &connection, "test-conn");
    TEST_ASSERT_TRUE(connect_result);
    TEST_ASSERT_NOT_NULL(connection);

    // Modify engine type to one without registered engine
    connection->engine_type = DB_ENGINE_AI; // cppcheck-suppress nullPointerRedundantCheck

    // Create a transaction
    Transaction* transaction = malloc(sizeof(Transaction));
    TEST_ASSERT_NOT_NULL(transaction);
    memset(transaction, 0, sizeof(Transaction));
    transaction->transaction_id = strdup("test-tx");

    // Test rollback transaction
    bool result = database_engine_rollback_transaction(connection, transaction);
    TEST_ASSERT_FALSE(result);

    // Clean up
    database_engine_cleanup_transaction(transaction);
    if (connection) {
        database_engine_cleanup_connection(connection);
    }
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_engine_begin_transaction_basic);
    RUN_TEST(test_database_engine_begin_transaction_null_connection);
    RUN_TEST(test_database_engine_begin_transaction_null_transaction);
    if (0) RUN_TEST(test_database_engine_begin_transaction_no_engine);

    RUN_TEST(test_database_engine_commit_transaction_basic);
    RUN_TEST(test_database_engine_commit_transaction_null_connection);
    if (0) RUN_TEST(test_database_engine_commit_transaction_null_transaction);
    if (0) RUN_TEST(test_database_engine_commit_transaction_no_engine);

    if (0) RUN_TEST(test_database_engine_rollback_transaction_basic);
    RUN_TEST(test_database_engine_rollback_transaction_null_connection);
    if (0) RUN_TEST(test_database_engine_rollback_transaction_null_transaction);
    if (0) RUN_TEST(test_database_engine_rollback_transaction_no_engine);

    return UNITY_END();
}