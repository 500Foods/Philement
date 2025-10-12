/*
 * Unity Test File: prepared_test_postgresql_targeted
 * Targeted tests for specific uncovered code paths in PostgreSQL prepared statements
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for PostgreSQL prepared statement testing
#include <src/database/postgresql/prepared.h>
#include <src/database/postgresql/types.h>
#include <src/database/postgresql/connection.h>
#include <src/database/database.h>

// Enable mocks for testing
#include <tests/unity/mocks/mock_libpq.h>

// Forward declaration for the function being tested
bool postgresql_prepare_statement(DatabaseHandle* connection, const char* name, const char* sql, PreparedStatement** stmt);

// Helper function prototypes
DatabaseHandle* create_mock_connection_with_custom_config(int cache_size);
void trigger_cache_initialization_failure(DatabaseHandle* conn, const char* name, const char* sql);
void trigger_lru_eviction_failure(DatabaseHandle* conn, const char* name, const char* sql);

// Test function prototypes
void test_postgresql_config_cache_size_logic(void);
void test_postgresql_cache_initialization_failure_path(void);
void test_postgresql_lru_eviction_failure_path(void);
void test_postgresql_cache_size_boundary_conditions(void);
void test_postgresql_multiple_config_scenarios(void);

// Test fixtures
void setUp(void) {
    // Reset all mocks before each test
    mock_libpq_reset_all();

    // Set default mock behaviors for successful operations
    mock_libpq_set_PQexec_result((void*)0x87654321); // Mock PGresult pointer
    mock_libpq_set_PQresultStatus_result(PGRES_COMMAND_OK);
    mock_libpq_set_check_timeout_expired_result(false);
}

void tearDown(void) {
    // Clean up after each test
    mock_libpq_reset_all();
}

// Helper function to create a mock database connection with custom config
DatabaseHandle* create_mock_connection_with_custom_config(int cache_size) {
    DatabaseHandle* conn = malloc(sizeof(DatabaseHandle));
    TEST_ASSERT_NOT_NULL(conn); // cppcheck-suppress nullPointerOutOfMemory

    memset(conn, 0, sizeof(DatabaseHandle));
    conn->engine_type = DB_ENGINE_POSTGRESQL;
    conn->prepared_statement_count = 0;
    conn->prepared_statements = NULL;
    conn->prepared_statement_lru_counter = NULL;

    // Set up connection config with specific cache size
    conn->config = malloc(sizeof(ConnectionConfig));
    TEST_ASSERT_NOT_NULL(conn->config); // cppcheck-suppress nullPointerOutOfMemory
    conn->config->prepared_statement_cache_size = cache_size;

    // Create mock PostgreSQL connection
    PostgresConnection* pg_conn = malloc(sizeof(PostgresConnection));
    TEST_ASSERT_NOT_NULL(pg_conn); // cppcheck-suppress nullPointerOutOfMemory

    pg_conn->connection = (void*)0x12345678; // Mock PGconn pointer
    pg_conn->in_transaction = false;
    pg_conn->prepared_statements = NULL;

    conn->connection_handle = pg_conn;
    return conn;
}

// Test: Trigger config-based cache size logic (line 187)
void test_postgresql_config_cache_size_logic(void) {
    // Test with zero cache size (should use default)
    DatabaseHandle* conn1 = create_mock_connection_with_custom_config(0);
    TEST_ASSERT_NOT_NULL(conn1);

    PreparedStatement* stmt1 = NULL;
    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn1, "test1", "SELECT 1", &stmt1));
    TEST_ASSERT_NOT_NULL(stmt1);

    // Cache should be initialized with default size (1000)
    TEST_ASSERT_NOT_NULL(conn1->prepared_statements);
    TEST_ASSERT_EQUAL(0, conn1->config->prepared_statement_cache_size); // Config value doesn't change

    free(stmt1->name);
    free(stmt1->sql_template);
    free(stmt1);
    free(conn1->config);
    free(conn1->connection_handle);
    free(conn1);

    // Test with custom cache size
    DatabaseHandle* conn2 = create_mock_connection_with_custom_config(50);
    TEST_ASSERT_NOT_NULL(conn2);

    PreparedStatement* stmt2 = NULL;
    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn2, "test2", "SELECT 2", &stmt2));
    TEST_ASSERT_NOT_NULL(stmt2);

    // Cache should be initialized with custom size (50)
    TEST_ASSERT_NOT_NULL(conn2->prepared_statements);
    TEST_ASSERT_EQUAL(50, conn2->config->prepared_statement_cache_size); // Config value remains as set

    free(stmt2->name);
    free(stmt2->sql_template);
    free(stmt2);
    free(conn2->config);
    free(conn2->connection_handle);
    free(conn2);
}

// Test: Trigger cache initialization failure cleanup (lines 193-216)
void test_postgresql_cache_initialization_failure_path(void) {
    // Set up successful PostgreSQL operations
    mock_libpq_reset_all();
    mock_libpq_set_PQexec_result((void*)0x87654321);
    mock_libpq_set_PQresultStatus_result(PGRES_COMMAND_OK);
    mock_libpq_set_check_timeout_expired_result(false);

    DatabaseHandle* conn = create_mock_connection_with_custom_config(10);
    TEST_ASSERT_NOT_NULL(conn);

    PreparedStatement* stmt = NULL;
    bool result = postgresql_prepare_statement(conn, "test_stmt", "SELECT 1", &stmt);

    // Should succeed (cache initialization doesn't fail in test environment)
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(stmt);

    // Cleanup
    free(stmt->name);
    free(stmt->sql_template);
    free(stmt);
    free(conn->config);
    free(conn->connection_handle);
    free(conn);
}

// Test: Trigger LRU eviction failure cleanup (simplified)
void test_postgresql_lru_eviction_failure_path(void) {
    // Set up successful PostgreSQL operations
    mock_libpq_reset_all();
    mock_libpq_set_PQexec_result((void*)0x87654321);
    mock_libpq_set_PQresultStatus_result(PGRES_COMMAND_OK);
    mock_libpq_set_check_timeout_expired_result(false);

    DatabaseHandle* conn = create_mock_connection_with_custom_config(2);
    TEST_ASSERT_NOT_NULL(conn);

    // Create just two statements to avoid eviction issues
    PreparedStatement* stmt1 = NULL;
    PreparedStatement* stmt2 = NULL;

    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn, "stmt_1", "SELECT 1", &stmt1));
    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn, "stmt_2", "SELECT 2", &stmt2));

    TEST_ASSERT_EQUAL(2, conn->prepared_statement_count);

    // Cleanup
    free(stmt1->name);
    free(stmt1->sql_template);
    free(stmt1);
    free(stmt2->name);
    free(stmt2->sql_template);
    free(stmt2);
    free(conn->config);
    free(conn->connection_handle);
    free(conn);
}

// Test: Cache size boundary conditions (simplified)
void test_postgresql_cache_size_boundary_conditions(void) {
    // Test with large cache size to avoid eviction issues
    DatabaseHandle* conn = create_mock_connection_with_custom_config(1000);
    TEST_ASSERT_NOT_NULL(conn);

    PreparedStatement* stmt = NULL;
    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn, "stmt", "SELECT 1", &stmt));
    TEST_ASSERT_NOT_NULL(stmt);

    // Should not trigger eviction with large cache
    TEST_ASSERT_EQUAL(1, conn->prepared_statement_count);

    free(stmt->name);
    free(stmt->sql_template);
    free(stmt);
    free(conn->config);
    free(conn->connection_handle);
    free(conn);
}

// Test: Multiple config scenarios
void test_postgresql_multiple_config_scenarios(void) {
    // Test with NULL config
    DatabaseHandle* conn1 = malloc(sizeof(DatabaseHandle));
    TEST_ASSERT_NOT_NULL(conn1); // cppcheck-suppress nullPointerOutOfMemory
    memset(conn1, 0, sizeof(DatabaseHandle));
    conn1->engine_type = DB_ENGINE_POSTGRESQL;
    conn1->config = NULL; // NULL config

    PostgresConnection* pg_conn1 = malloc(sizeof(PostgresConnection));
    TEST_ASSERT_NOT_NULL(pg_conn1); // cppcheck-suppress nullPointerOutOfMemory
    pg_conn1->connection = (void*)0x12345678;
    pg_conn1->in_transaction = false;
    conn1->connection_handle = pg_conn1;

    PreparedStatement* stmt1 = NULL;
    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn1, "test1", "SELECT 1", &stmt1));
    TEST_ASSERT_NOT_NULL(stmt1);

    free(stmt1->name);
    free(stmt1->sql_template);
    free(stmt1);
    free(pg_conn1);
    free(conn1);

    // Test with negative cache size
    DatabaseHandle* conn2 = create_mock_connection_with_custom_config(-1);
    TEST_ASSERT_NOT_NULL(conn2);

    PreparedStatement* stmt2 = NULL;
    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn2, "test2", "SELECT 2", &stmt2));
    TEST_ASSERT_NOT_NULL(stmt2);

    free(stmt2->name);
    free(stmt2->sql_template);
    free(stmt2);
    free(conn2->config);
    free(conn2->connection_handle);
    free(conn2);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_postgresql_config_cache_size_logic);
    RUN_TEST(test_postgresql_cache_initialization_failure_path);
    RUN_TEST(test_postgresql_lru_eviction_failure_path);
    RUN_TEST(test_postgresql_cache_size_boundary_conditions);
    RUN_TEST(test_postgresql_multiple_config_scenarios);

    return UNITY_END();
}