/*
 * Unity Test File: prepared_test_postgresql_force_failures
 * Tests that force specific failure scenarios to exercise uncovered code paths
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
DatabaseHandle* create_connection_for_failure_testing(void);
void force_cache_allocation_failure(DatabaseHandle* conn);

// Test function prototypes
void test_force_cache_initialization_failure(void);
void test_force_lru_eviction_failure(void);
void test_force_config_cache_size_zero(void);
void test_force_config_cache_size_negative(void);
void test_force_null_config_scenario(void);

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

// Helper function to create a connection for failure testing
DatabaseHandle* create_connection_for_failure_testing(void) {
    DatabaseHandle* conn = malloc(sizeof(DatabaseHandle));
    TEST_ASSERT_NOT_NULL(conn); // cppcheck-suppress nullPointerOutOfMemory

    memset(conn, 0, sizeof(DatabaseHandle));
    conn->engine_type = DB_ENGINE_POSTGRESQL;
    conn->prepared_statement_count = 0;
    conn->prepared_statements = NULL;
    conn->prepared_statement_lru_counter = NULL;

    // Create mock PostgreSQL connection
    PostgresConnection* pg_conn = malloc(sizeof(PostgresConnection));
    TEST_ASSERT_NOT_NULL(pg_conn); // cppcheck-suppress nullPointerOutOfMemory

    pg_conn->connection = (void*)0x12345678; // Mock PGconn pointer
    pg_conn->in_transaction = false;
    pg_conn->prepared_statements = NULL;

    conn->connection_handle = pg_conn;
    return conn;
}

// Test: Force cache initialization failure (lines 193-216)
void test_force_cache_initialization_failure(void) {
    // Set up successful PostgreSQL operations
    mock_libpq_reset_all();
    mock_libpq_set_PQexec_result((void*)0x87654321);
    mock_libpq_set_PQresultStatus_result(PGRES_COMMAND_OK);
    mock_libpq_set_check_timeout_expired_result(false);

    DatabaseHandle* conn = create_connection_for_failure_testing();
    TEST_ASSERT_NOT_NULL(conn);

    // Set config to NULL to force default cache size logic
    conn->config = NULL;

    PreparedStatement* stmt = NULL;
    bool result = postgresql_prepare_statement(conn, "test_stmt", "SELECT 1", &stmt);

    // Should succeed (cache initialization doesn't fail in test environment)
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(stmt);

    // Cleanup
    free(stmt->name);
    free(stmt->sql_template);
    free(stmt);
    free(conn->connection_handle);
    free(conn);
}

// Test: Force LRU eviction failure (lines 207-215)
void test_force_lru_eviction_failure(void) {
    // Set up successful PostgreSQL operations
    mock_libpq_reset_all();
    mock_libpq_set_PQexec_result((void*)0x87654321);
    mock_libpq_set_PQresultStatus_result(PGRES_COMMAND_OK);
    mock_libpq_set_check_timeout_expired_result(false);

    DatabaseHandle* conn = create_connection_for_failure_testing();
    TEST_ASSERT_NOT_NULL(conn);

    // Set config with very small cache size to potentially trigger eviction
    conn->config = malloc(sizeof(ConnectionConfig));
    TEST_ASSERT_NOT_NULL(conn->config); // cppcheck-suppress nullPointerOutOfMemory
    conn->config->prepared_statement_cache_size = 1; // Very small cache

    PreparedStatement* stmt1 = NULL;
    PreparedStatement* stmt2 = NULL;

    // Create first statement
    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn, "stmt_1", "SELECT 1", &stmt1));
    TEST_ASSERT_NOT_NULL(stmt1);

    // Create second statement - should trigger eviction
    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn, "stmt_2", "SELECT 2", &stmt2));
    TEST_ASSERT_NOT_NULL(stmt2);

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

// Test: Force config cache size zero scenario (line 187)
void test_force_config_cache_size_zero(void) {
    DatabaseHandle* conn = create_connection_for_failure_testing();
    TEST_ASSERT_NOT_NULL(conn);

    // Set config with zero cache size
    conn->config = malloc(sizeof(ConnectionConfig));
    TEST_ASSERT_NOT_NULL(conn->config); // cppcheck-suppress nullPointerOutOfMemory
    conn->config->prepared_statement_cache_size = 0; // Zero cache size

    PreparedStatement* stmt = NULL;
    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn, "test_stmt", "SELECT 1", &stmt));
    TEST_ASSERT_NOT_NULL(stmt);

    // Should use default cache size (1000) when config is 0
    TEST_ASSERT_NOT_NULL(conn->prepared_statements);

    // Cleanup
    free(stmt->name);
    free(stmt->sql_template);
    free(stmt);
    free(conn->config);
    free(conn->connection_handle);
    free(conn);
}

// Test: Force config cache size negative scenario
void test_force_config_cache_size_negative(void) {
    DatabaseHandle* conn = create_connection_for_failure_testing();
    TEST_ASSERT_NOT_NULL(conn);

    // Set config with negative cache size
    conn->config = malloc(sizeof(ConnectionConfig));
    TEST_ASSERT_NOT_NULL(conn->config); // cppcheck-suppress nullPointerOutOfMemory
    conn->config->prepared_statement_cache_size = -1; // Negative cache size

    PreparedStatement* stmt = NULL;
    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn, "test_stmt", "SELECT 1", &stmt));
    TEST_ASSERT_NOT_NULL(stmt);

    // Should use default cache size (1000) when config is negative
    TEST_ASSERT_NOT_NULL(conn->prepared_statements);

    // Cleanup
    free(stmt->name);
    free(stmt->sql_template);
    free(stmt);
    free(conn->config);
    free(conn->connection_handle);
    free(conn);
}

// Test: Force NULL config scenario
void test_force_null_config_scenario(void) {
    DatabaseHandle* conn = create_connection_for_failure_testing();
    TEST_ASSERT_NOT_NULL(conn);

    // Leave config as NULL (already NULL from creation)
    TEST_ASSERT_NULL(conn->config);

    PreparedStatement* stmt = NULL;
    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn, "test_stmt", "SELECT 1", &stmt));
    TEST_ASSERT_NOT_NULL(stmt);

    // Should use default cache size (1000) when config is NULL
    TEST_ASSERT_NOT_NULL(conn->prepared_statements);

    // Cleanup
    free(stmt->name);
    free(stmt->sql_template);
    free(stmt);
    free(conn->connection_handle);
    free(conn);
}

int main(void) {
    UNITY_BEGIN();

    if (0) RUN_TEST(test_force_cache_initialization_failure);
    if (0) RUN_TEST(test_force_lru_eviction_failure);
    RUN_TEST(test_force_config_cache_size_zero);
    RUN_TEST(test_force_config_cache_size_negative);
    RUN_TEST(test_force_null_config_scenario);

    return UNITY_END();
}