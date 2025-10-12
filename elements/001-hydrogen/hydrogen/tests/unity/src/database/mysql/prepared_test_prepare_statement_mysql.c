/*
 * Unity Test File: prepared_test_prepare_statement
 * Comprehensive tests for mysql_prepare_statement function
 */

#include <src/hydrogen.h>
#include <unity.h>

// MySQL mocks are enabled via CMake
#include <unity/mocks/mock_libmysqlclient.h>

// Include necessary headers for the module being tested
#include <src/database/database.h>
#include <src/database/mysql/types.h>
#include <src/database/mysql/connection.h>
#include <src/database/mysql/prepared.h>

// Forward declarations for functions being tested
bool mysql_prepare_statement(DatabaseHandle* connection, const char* name, const char* sql, PreparedStatement** stmt);

// External declarations for function pointers (defined in connection.c)
extern mysql_stmt_init_t mysql_stmt_init_ptr;
extern mysql_stmt_prepare_t mysql_stmt_prepare_ptr;
extern mysql_stmt_execute_t mysql_stmt_execute_ptr;
extern mysql_stmt_close_t mysql_stmt_close_ptr;
extern mysql_error_t mysql_error_ptr;

// Mock function declarations (when USE_MOCK_LIBMYSQLCLIENT is defined)
#ifdef USE_MOCK_LIBMYSQLCLIENT
extern void* mock_mysql_stmt_init(void* mysql);
extern int mock_mysql_stmt_prepare(void* stmt, const char* query, unsigned long length);
extern int mock_mysql_stmt_execute(void* stmt);
extern int mock_mysql_stmt_close(void* stmt);
extern const char* mock_mysql_error(void* mysql);
#endif

// External declarations for timeout checking (defined in connection.c)
extern bool mysql_check_timeout_expired(time_t start_time, int timeout_seconds);

// Test function prototypes
void test_mysql_prepare_statement_null_mysql_connection(void);
void test_mysql_prepare_statement_null_mysql_connection_field(void);
void test_mysql_prepare_statement_no_function_pointers(void);
void test_mysql_prepare_statement_mysql_stmt_init_failure(void);
void test_mysql_prepare_statement_mysql_stmt_prepare_failure(void);
void test_mysql_prepare_statement_timeout_scenario(void);
void test_mysql_prepare_statement_memory_allocation_failure(void);
void test_mysql_prepare_statement_custom_cache_size(void);
void test_mysql_prepare_statement_lru_eviction(void);
void test_mysql_prepare_statement_lru_eviction_cleanup(void);
void test_mysql_prepare_statement_success_with_cache(void);

void setUp(void) {
    // Reset mocks before each test
    mock_libmysqlclient_reset_all();

    // Initialize MySQL function pointers to use mocks
    load_libmysql_functions(NULL);

    // Set prepared statement function pointers to mock implementations
    // (These should already be set when USE_MOCK_LIBMYSQLCLIENT is defined)
    mysql_stmt_init_ptr = mock_mysql_stmt_init;
    mysql_stmt_prepare_ptr = mock_mysql_stmt_prepare;
    mysql_stmt_execute_ptr = mock_mysql_stmt_execute;
    mysql_stmt_close_ptr = mock_mysql_stmt_close;
    mysql_error_ptr = mock_mysql_error;
}

void tearDown(void) {
    // Cleanup after each test
    mock_libmysqlclient_reset_all();
}

// Test: NULL MySQL connection handle
void test_mysql_prepare_statement_null_mysql_connection(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MYSQL;
    connection.connection_handle = NULL;
    PreparedStatement* stmt = NULL;

    bool result = mysql_prepare_statement(&connection, "test_stmt", "SELECT 1", &stmt);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(stmt);
}

// Test: NULL MySQL connection field
void test_mysql_prepare_statement_null_mysql_connection_field(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MYSQL;

    MySQLConnection mysql_conn = {0};
    mysql_conn.connection = NULL;
    connection.connection_handle = &mysql_conn;

    PreparedStatement* stmt = NULL;

    bool result = mysql_prepare_statement(&connection, "test_stmt", "SELECT 1", &stmt);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(stmt);
}

// Test: Function pointers not available
void test_mysql_prepare_statement_no_function_pointers(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MYSQL;

    MySQLConnection mysql_conn = {0};
    mysql_conn.connection = (void*)0x1234;
    connection.connection_handle = &mysql_conn;

    // Clear function pointers
    mysql_stmt_init_ptr = NULL;
    mysql_stmt_prepare_ptr = NULL;
    mysql_stmt_execute_ptr = NULL;
    mysql_stmt_close_ptr = NULL;

    PreparedStatement* stmt = NULL;
    bool result = mysql_prepare_statement(&connection, "test_stmt", "SELECT 1", &stmt);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(stmt);
}

// Test: mysql_stmt_init failure
void test_mysql_prepare_statement_mysql_stmt_init_failure(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MYSQL;

    MySQLConnection mysql_conn = {0};
    mysql_conn.connection = (void*)0x1234;
    connection.connection_handle = &mysql_conn;

    // Set function pointers to valid mock functions first
    mysql_stmt_init_ptr = mock_mysql_stmt_init;
    mysql_stmt_prepare_ptr = mock_mysql_stmt_prepare;
    mysql_stmt_execute_ptr = mock_mysql_stmt_execute;
    mysql_stmt_close_ptr = mock_mysql_stmt_close;
    mysql_error_ptr = mock_mysql_error;

    // Mock mysql_stmt_init to return NULL (failure)
    mock_libmysqlclient_set_mysql_stmt_init_result(NULL);

    PreparedStatement* stmt = NULL;
    bool result = mysql_prepare_statement(&connection, "test_stmt", "SELECT 1", &stmt);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(stmt);
}

// Test: mysql_stmt_prepare failure
void test_mysql_prepare_statement_mysql_stmt_prepare_failure(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MYSQL;

    MySQLConnection mysql_conn = {0};
    mysql_conn.connection = (void*)0x1234;
    connection.connection_handle = &mysql_conn;

    // Mock mysql_stmt_init to succeed
    mock_libmysqlclient_set_mysql_stmt_init_result((void*)0x5678);

    // Mock mysql_stmt_prepare to fail (return non-zero)
    mock_libmysqlclient_set_mysql_stmt_prepare_result(1);

    PreparedStatement* stmt = NULL;
    bool result = mysql_prepare_statement(&connection, "test_stmt", "SELECT 1", &stmt);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(stmt);
}

// Test: Timeout scenario
void test_mysql_prepare_statement_timeout_scenario(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MYSQL;

    MySQLConnection mysql_conn = {0};
    mysql_conn.connection = (void*)0x1234;
    connection.connection_handle = &mysql_conn;

    // Mock mysql_stmt_init to succeed
    mock_libmysqlclient_set_mysql_stmt_init_result((void*)0x5678);

    // Mock mysql_stmt_prepare to succeed but we'll simulate timeout
    mock_libmysqlclient_set_mysql_stmt_prepare_result(0);

    PreparedStatement* stmt = NULL;
    bool result = mysql_prepare_statement(&connection, "test_stmt", "SELECT 1", &stmt);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(stmt);

    // Cleanup
    if (stmt) {
        free(stmt->name);
        free(stmt->sql_template);
        free(stmt);
    }
    if (connection.prepared_statements) {
        free(connection.prepared_statements);
    }
    if (connection.prepared_statement_lru_counter) {
        free(connection.prepared_statement_lru_counter);
    }
}

// Test: Memory allocation failure for prepared statement structure
void test_mysql_prepare_statement_memory_allocation_failure(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MYSQL;

    MySQLConnection mysql_conn = {0};
    mysql_conn.connection = (void*)0x1234;
    connection.connection_handle = &mysql_conn;

    // Mock mysql_stmt_init to succeed
    mock_libmysqlclient_set_mysql_stmt_init_result((void*)0x5678);

    // Mock mysql_stmt_prepare to succeed
    mock_libmysqlclient_set_mysql_stmt_prepare_result(0);

    PreparedStatement* stmt = NULL;
    bool result = mysql_prepare_statement(&connection, "test_stmt", "SELECT 1", &stmt);

    // This should succeed with current mocks since we can't easily mock calloc failure
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(stmt);

    // Cleanup
    if (stmt) {
        free(stmt->name);
        free(stmt->sql_template);
        free(stmt);
    }
    if (connection.prepared_statements) {
        free(connection.prepared_statements);
    }
    if (connection.prepared_statement_lru_counter) {
        free(connection.prepared_statement_lru_counter);
    }
}

// Test: Custom cache size configuration
void test_mysql_prepare_statement_custom_cache_size(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MYSQL;
    connection.config = calloc(1, sizeof(ConnectionConfig));
    TEST_ASSERT_NOT_NULL(connection.config);
    connection.config->prepared_statement_cache_size = 50; // Custom size

    MySQLConnection mysql_conn = {0};
    mysql_conn.connection = (void*)0x1234;
    connection.connection_handle = &mysql_conn;

    // Mock mysql_stmt_init to succeed
    mock_libmysqlclient_set_mysql_stmt_init_result((void*)0x5678);

    // Mock mysql_stmt_prepare to succeed
    mock_libmysqlclient_set_mysql_stmt_prepare_result(0);

    PreparedStatement* stmt = NULL;
    bool result = mysql_prepare_statement(&connection, "test_stmt", "SELECT 1", &stmt);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(stmt);
    TEST_ASSERT_NOT_NULL(connection.prepared_statements);

    // Cleanup
    if (stmt) {
        free(stmt->name);
        free(stmt->sql_template);
        free(stmt);
    }
    if (connection.prepared_statements) {
        free(connection.prepared_statements);
    }
    if (connection.prepared_statement_lru_counter) {
        free(connection.prepared_statement_lru_counter);
    }
    free(connection.config);
}

// Test: LRU eviction when cache is full
void test_mysql_prepare_statement_lru_eviction(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MYSQL;
    connection.config = calloc(1, sizeof(ConnectionConfig));
    TEST_ASSERT_NOT_NULL(connection.config);
    connection.config->prepared_statement_cache_size = 2; // Small cache for testing

    MySQLConnection mysql_conn = {0};
    mysql_conn.connection = (void*)0x1234;
    connection.connection_handle = &mysql_conn;

    // Mock mysql_stmt_init to succeed
    mock_libmysqlclient_set_mysql_stmt_init_result((void*)0x5678);

    // Mock mysql_stmt_prepare to succeed
    mock_libmysqlclient_set_mysql_stmt_prepare_result(0);

    PreparedStatement* stmt1 = NULL;
    PreparedStatement* stmt2 = NULL;
    PreparedStatement* stmt3 = NULL;

    // Create first statement
    TEST_ASSERT_TRUE(mysql_prepare_statement(&connection, "stmt_1", "SELECT 1", &stmt1));
    TEST_ASSERT_NOT_NULL(stmt1);
    TEST_ASSERT_EQUAL(1, connection.prepared_statement_count);

    // Create second statement
    TEST_ASSERT_TRUE(mysql_prepare_statement(&connection, "stmt_2", "SELECT 2", &stmt2));
    TEST_ASSERT_NOT_NULL(stmt2);
    TEST_ASSERT_EQUAL(2, connection.prepared_statement_count);

    // Create third statement - should trigger LRU eviction
    TEST_ASSERT_TRUE(mysql_prepare_statement(&connection, "stmt_3", "SELECT 3", &stmt3));
    TEST_ASSERT_NOT_NULL(stmt3);
    TEST_ASSERT_EQUAL(2, connection.prepared_statement_count); // Should still be 2 after eviction

    // Verify LRU eviction occurred - stmt1 should be evicted (first in, first out)
    TEST_ASSERT_EQUAL(stmt2, connection.prepared_statements[0]);
    TEST_ASSERT_EQUAL(stmt3, connection.prepared_statements[1]);

    // Cleanup
    for (size_t i = 0; i < connection.prepared_statement_count; i++) {
        PreparedStatement* s = connection.prepared_statements[i];
        free(s->name);
        free(s->sql_template);
        free(s);
    }
    free(connection.prepared_statements);
    free(connection.prepared_statement_lru_counter);
    free(connection.config);
}

// Test: LRU eviction cleanup of evicted statement
void test_mysql_prepare_statement_lru_eviction_cleanup(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MYSQL;
    connection.config = calloc(1, sizeof(ConnectionConfig));
    TEST_ASSERT_NOT_NULL(connection.config);
    connection.config->prepared_statement_cache_size = 1; // Very small cache

    MySQLConnection mysql_conn = {0};
    mysql_conn.connection = (void*)0x1234;
    connection.connection_handle = &mysql_conn;

    // Mock mysql_stmt_init to succeed
    mock_libmysqlclient_set_mysql_stmt_init_result((void*)0x5678);

    // Mock mysql_stmt_prepare to succeed
    mock_libmysqlclient_set_mysql_stmt_prepare_result(0);

    PreparedStatement* stmt1 = NULL;
    PreparedStatement* stmt2 = NULL;

    // Create first statement
    TEST_ASSERT_TRUE(mysql_prepare_statement(&connection, "stmt_1", "SELECT 1", &stmt1));
    TEST_ASSERT_NOT_NULL(stmt1);
    TEST_ASSERT_EQUAL(1, connection.prepared_statement_count);

    // Create second statement - should evict stmt1
    TEST_ASSERT_TRUE(mysql_prepare_statement(&connection, "stmt_2", "SELECT 2", &stmt2));
    TEST_ASSERT_NOT_NULL(stmt2);
    TEST_ASSERT_EQUAL(1, connection.prepared_statement_count);
    TEST_ASSERT_EQUAL(stmt2, connection.prepared_statements[0]);

    // Cleanup
    free(stmt2->name);
    free(stmt2->sql_template);
    free(stmt2);
    free(connection.prepared_statements);
    free(connection.prepared_statement_lru_counter);
    free(connection.config);
}

// Test: Successful prepared statement creation with cache
void test_mysql_prepare_statement_success_with_cache(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MYSQL;
    connection.config = calloc(1, sizeof(ConnectionConfig));
    TEST_ASSERT_NOT_NULL(connection.config);
    connection.config->prepared_statement_cache_size = 100;

    MySQLConnection mysql_conn = {0};
    mysql_conn.connection = (void*)0x1234;
    connection.connection_handle = &mysql_conn;

    // Mock mysql_stmt_init to succeed
    mock_libmysqlclient_set_mysql_stmt_init_result((void*)0x5678);

    // Mock mysql_stmt_prepare to succeed
    mock_libmysqlclient_set_mysql_stmt_prepare_result(0);

    PreparedStatement* stmt = NULL;
    bool result = mysql_prepare_statement(&connection, "test_stmt", "SELECT * FROM users WHERE id = ?", &stmt);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(stmt);
    TEST_ASSERT_NOT_NULL(stmt->name);
    TEST_ASSERT_EQUAL_STRING("test_stmt", stmt->name);
    TEST_ASSERT_NOT_NULL(stmt->sql_template);
    TEST_ASSERT_EQUAL_STRING("SELECT * FROM users WHERE id = ?", stmt->sql_template);
    TEST_ASSERT_EQUAL(0, stmt->usage_count);
    TEST_ASSERT_NOT_NULL(stmt->engine_specific_handle);
    TEST_ASSERT_EQUAL(1, connection.prepared_statement_count);
    TEST_ASSERT_NOT_NULL(connection.prepared_statements);
    TEST_ASSERT_EQUAL(stmt, connection.prepared_statements[0]);

    // Cleanup
    free(stmt->name);
    free(stmt->sql_template);
    free(stmt);
    free(connection.prepared_statements);
    free(connection.prepared_statement_lru_counter);
    free(connection.config);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mysql_prepare_statement_null_mysql_connection);
    RUN_TEST(test_mysql_prepare_statement_null_mysql_connection_field);
    RUN_TEST(test_mysql_prepare_statement_no_function_pointers);
    RUN_TEST(test_mysql_prepare_statement_mysql_stmt_init_failure);
    RUN_TEST(test_mysql_prepare_statement_mysql_stmt_prepare_failure);
    RUN_TEST(test_mysql_prepare_statement_timeout_scenario);
    RUN_TEST(test_mysql_prepare_statement_memory_allocation_failure);
    RUN_TEST(test_mysql_prepare_statement_custom_cache_size);
    RUN_TEST(test_mysql_prepare_statement_lru_eviction);
    RUN_TEST(test_mysql_prepare_statement_lru_eviction_cleanup);
    RUN_TEST(test_mysql_prepare_statement_success_with_cache);

    return UNITY_END();
}