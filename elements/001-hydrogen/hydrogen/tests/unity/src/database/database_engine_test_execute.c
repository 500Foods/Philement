/*
 * Unity Test File: database_engine_test_execute
 * This file contains unit tests for database_engine_execute function
 * to increase test coverage for query execution
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>

// Forward declarations for mock functions
bool mock_connect(ConnectionConfig* config, DatabaseHandle** connection, const char* designator);
bool mock_disconnect(DatabaseHandle* connection);
bool mock_health_check(DatabaseHandle* connection);
bool mock_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result);
bool mock_execute_prepared(DatabaseHandle* connection, const PreparedStatement* stmt, QueryRequest* request, QueryResult** result);
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
            (*connection)->engine_type = DB_ENGINE_AI;  // Use AI engine for execute tests
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
    if (result) {
        *result = malloc(sizeof(QueryResult));
        if (*result) {
            memset(*result, 0, sizeof(QueryResult));
            (*result)->success = true;
            (*result)->row_count = 1;
            (*result)->execution_time_ms = 10;
        }
    }
    return true;
}

bool mock_execute_prepared(DatabaseHandle* connection, const PreparedStatement* stmt, QueryRequest* request, QueryResult** result) {
    (void)connection;
    (void)stmt;
    (void)request;
    if (result) {
        *result = malloc(sizeof(QueryResult));
        if (*result) {
            memset(*result, 0, sizeof(QueryResult));
            (*result)->success = true;
            (*result)->row_count = 1;
            (*result)->execution_time_ms = 5;
        }
    }
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
    .execute_prepared = mock_execute_prepared,
    .begin_transaction = NULL,
    .commit_transaction = NULL,
    .rollback_transaction = NULL,
    .prepare_statement = NULL,
    .unprepare_statement = NULL,
    .get_connection_string = mock_get_connection_string,
    .validate_connection_string = mock_validate_connection_string,
    .escape_string = NULL
};

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

// Test function prototypes
void test_database_engine_execute_basic(void);
void test_database_engine_execute_null_connection(void);
void test_database_engine_execute_null_request(void);
void test_database_engine_execute_null_result(void);
void test_database_engine_execute_invalid_connection_pointer(void);
void test_database_engine_execute_invalid_engine_type(void);
void test_database_engine_execute_with_prepared_statement(void);
void test_database_engine_execute_corrupted_connection_0x1(void);

// Mock query request for testing
static QueryRequest mock_request = {
    .query_id = NULL,
    .sql_template = NULL,
    .parameters_json = NULL,
    .timeout_seconds = 5,
    .isolation_level = DB_ISOLATION_READ_COMMITTED,
    .use_prepared_statement = false,
    .prepared_statement_name = NULL
};

// Mock prepared statement for testing
static PreparedStatement mock_stmt = {
    .name = NULL,
    .sql_template = NULL,
    .engine_specific_handle = NULL,
    .created_at = 0,
    .usage_count = 0
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
    mock_request.sql_template = strdup("SELECT * FROM test_table");
    mock_stmt.name = strdup("test_stmt");
    mock_stmt.sql_template = strdup("SELECT * FROM test_table WHERE id = ?");
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
    if (mock_request.sql_template) {
        free(mock_request.sql_template);
        mock_request.sql_template = NULL;
    }
    if (mock_stmt.name) {
        free(mock_stmt.name);
        mock_stmt.name = NULL;
    }
    if (mock_stmt.sql_template) {
        free(mock_stmt.sql_template);
        mock_stmt.sql_template = NULL;
    }
}

// Test database_engine_execute function
void test_database_engine_execute_basic(void) {
    // Create a connection for testing
    DatabaseHandle* connection = NULL;
    bool connect_result = database_engine_connect_with_designator(DB_ENGINE_AI, &mock_config, &connection, "test-conn");
    TEST_ASSERT_TRUE(connect_result);
    TEST_ASSERT_NOT_NULL(connection);

    // Test basic execute functionality
    QueryResult* result = NULL;
    bool exec_result = database_engine_execute(connection, &mock_request, &result);
    TEST_ASSERT_TRUE(exec_result);  // Should succeed with mock engine
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success); // cppcheck-suppress nullPointerRedundantCheck

    // Clean up
    if (result) {
        database_engine_cleanup_result(result);
    }
    if (connection) {
        database_engine_cleanup_connection(connection);
    }
}

void test_database_engine_execute_null_connection(void) {
    // Test execute with NULL connection
    QueryResult* result = NULL;
    bool exec_result = database_engine_execute(NULL, &mock_request, &result);
    TEST_ASSERT_FALSE(exec_result);
}

void test_database_engine_execute_null_request(void) {
    // Create a connection for testing
    DatabaseHandle* connection = NULL;
    bool connect_result = database_engine_connect_with_designator(DB_ENGINE_AI, &mock_config, &connection, "test-conn");
    TEST_ASSERT_TRUE(connect_result);
    TEST_ASSERT_NOT_NULL(connection);

    // Test execute with NULL request
    QueryResult* result = NULL;
    bool exec_result = database_engine_execute(connection, NULL, &result);
    TEST_ASSERT_FALSE(exec_result);

    // Clean up
    if (connection) {
        database_engine_cleanup_connection(connection);
    }
}

void test_database_engine_execute_null_result(void) {
    // Create a connection for testing
    DatabaseHandle* connection = NULL;
    bool connect_result = database_engine_connect_with_designator(DB_ENGINE_AI, &mock_config, &connection, "test-conn");
    TEST_ASSERT_TRUE(connect_result);
    TEST_ASSERT_NOT_NULL(connection);

    // Test execute with NULL result pointer
    bool exec_result = database_engine_execute(connection, &mock_request, NULL);
    TEST_ASSERT_FALSE(exec_result);

    // Clean up
    if (connection) {
        database_engine_cleanup_connection(connection);
    }
}

void test_database_engine_execute_invalid_connection_pointer(void) {
    // Test execute with invalid connection pointer (very low address)
    DatabaseHandle* invalid_conn = (DatabaseHandle*)0x1;
    QueryResult* result = NULL;
    bool exec_result = database_engine_execute(invalid_conn, &mock_request, &result);
    TEST_ASSERT_FALSE(exec_result);
}

void test_database_engine_execute_invalid_engine_type(void) {
    // Create a connection and modify its engine type to invalid
    DatabaseHandle* connection = NULL;
    bool connect_result = database_engine_connect_with_designator(DB_ENGINE_AI, &mock_config, &connection, "test-conn");
    TEST_ASSERT_TRUE(connect_result);
    TEST_ASSERT_NOT_NULL(connection);

    // Modify engine type to invalid
    connection->engine_type = DB_ENGINE_MAX; // cppcheck-suppress nullPointerRedundantCheck

    // Test execute with invalid engine type
    QueryResult* result = NULL;
    bool exec_result = database_engine_execute(connection, &mock_request, &result);
    TEST_ASSERT_FALSE(exec_result);

    // Clean up
    if (connection) {
        database_engine_cleanup_connection(connection);
    }
}

void test_database_engine_execute_with_prepared_statement(void) {
    // Create a connection for testing
    DatabaseHandle* connection = NULL;
    bool connect_result = database_engine_connect_with_designator(DB_ENGINE_AI, &mock_config, &connection, "test-conn");
    TEST_ASSERT_TRUE(connect_result);
    TEST_ASSERT_NOT_NULL(connection);

    // Set up prepared statement in connection
    connection->prepared_statement_count = 1; // cppcheck-suppress nullPointerRedundantCheck
    connection->prepared_statements = malloc(sizeof(PreparedStatement*)); // cppcheck-suppress nullPointerRedundantCheck
    TEST_ASSERT_NOT_NULL(connection->prepared_statements); // cppcheck-suppress nullPointerRedundantCheck
    connection->prepared_statements[0] = malloc(sizeof(PreparedStatement)); // cppcheck-suppress nullPointerRedundantCheck
    TEST_ASSERT_NOT_NULL(connection->prepared_statements[0]); // cppcheck-suppress nullPointerRedundantCheck
    memcpy(connection->prepared_statements[0], &mock_stmt, sizeof(PreparedStatement)); // cppcheck-suppress nullPointerRedundantCheck
    connection->prepared_statements[0]->name = strdup("test_stmt"); // cppcheck-suppress nullPointerRedundantCheck
    connection->prepared_statements[0]->sql_template = strdup("SELECT * FROM test_table WHERE id = ?"); // cppcheck-suppress nullPointerRedundantCheck

    // Create request that uses prepared statement
    QueryRequest prep_request = mock_request;
    prep_request.use_prepared_statement = true;
    prep_request.prepared_statement_name = strdup("test_stmt");

    // Test execute with prepared statement
    QueryResult* result = NULL;
    bool exec_result = database_engine_execute(connection, &prep_request, &result);
    TEST_ASSERT_TRUE(exec_result);
    TEST_ASSERT_NOT_NULL(result);

    // Clean up
    free(prep_request.prepared_statement_name);
    if (result) {
        database_engine_cleanup_result(result);
    }
    if (connection) {
        database_engine_cleanup_connection(connection);
    }
}

void test_database_engine_execute_corrupted_connection_0x1(void) {
    // Test execute with corrupted connection pointer 0x1
    DatabaseHandle* corrupted_conn = (DatabaseHandle*)0x1;
    QueryResult* result = NULL;
    bool exec_result = database_engine_execute(corrupted_conn, &mock_request, &result);
    TEST_ASSERT_FALSE(exec_result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_engine_execute_basic);
    RUN_TEST(test_database_engine_execute_null_connection);
    RUN_TEST(test_database_engine_execute_null_request);
    RUN_TEST(test_database_engine_execute_null_result);
    RUN_TEST(test_database_engine_execute_invalid_connection_pointer);
    RUN_TEST(test_database_engine_execute_invalid_engine_type);
    RUN_TEST(test_database_engine_execute_with_prepared_statement);
    RUN_TEST(test_database_engine_execute_corrupted_connection_0x1);

    return UNITY_END();
}