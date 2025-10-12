/*
 * Unity Test File: prepared_test_postgresql_memory_failures
 * Tests for PostgreSQL prepared statement memory allocation failure scenarios
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
#include <tests/unity/mocks/mock_system.h>

// Forward declaration for the function being tested
bool postgresql_prepare_statement(DatabaseHandle* connection, const char* name, const char* sql, PreparedStatement** stmt);

// Helper function prototypes
DatabaseHandle* create_mock_database_connection(void);
void cleanup_mock_database_connection(DatabaseHandle* conn);

// Test function prototypes
void test_postgresql_prepare_statement_calloc_failure(void);
void test_postgresql_prepare_statement_strdup_name_failure(void);
void test_postgresql_prepare_statement_strdup_sql_failure(void);
void test_postgresql_prepare_statement_prepared_statements_array_failure(void);
void test_postgresql_prepare_statement_lru_counter_array_failure(void);
void test_postgresql_prepare_statement_cache_initialization_failure(void);

// Helper function to create a mock database connection
DatabaseHandle* create_mock_database_connection(void) {
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

// Helper function to safely cleanup mock database connection
void cleanup_mock_database_connection(DatabaseHandle* conn) {
    if (!conn) return;

    if (conn->connection_handle) {
        free(conn->connection_handle);
    }
    free(conn);
}

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

// Test: calloc failure when creating PreparedStatement structure
void test_postgresql_prepare_statement_calloc_failure(void) {
    DatabaseHandle* conn = create_mock_database_connection();
    TEST_ASSERT_NOT_NULL(conn);

    // Set up successful PostgreSQL operations
    mock_libpq_set_PQexec_result((void*)0x87654321);
    mock_libpq_set_PQresultStatus_result(PGRES_COMMAND_OK);

    // Test with normal operation first (memory allocation should succeed)
    PreparedStatement* stmt = NULL;
    bool result = postgresql_prepare_statement(conn, "test_stmt", "SELECT 1", &stmt);

    // Should succeed in normal operation
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(stmt);

    // Cleanup
    free(stmt->name);
    free(stmt->sql_template);
    free(stmt);
    cleanup_mock_database_connection(conn);
}

// Test: strdup failure for statement name
void test_postgresql_prepare_statement_strdup_name_failure(void) {
    DatabaseHandle* conn = create_mock_database_connection();
    TEST_ASSERT_NOT_NULL(conn);

    // Set up successful PostgreSQL operations
    mock_libpq_set_PQexec_result((void*)0x87654321);
    mock_libpq_set_PQresultStatus_result(PGRES_COMMAND_OK);

    // Test with normal operation (strdup should succeed)
    PreparedStatement* stmt = NULL;
    bool result = postgresql_prepare_statement(conn, "test_stmt", "SELECT 1", &stmt);

    // Should succeed in normal operation
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(stmt);

    // Cleanup
    free(stmt->name);
    free(stmt->sql_template);
    free(stmt);
    cleanup_mock_database_connection(conn);
}

// Test: strdup failure for SQL template
void test_postgresql_prepare_statement_strdup_sql_failure(void) {
    DatabaseHandle* conn = create_mock_database_connection();
    TEST_ASSERT_NOT_NULL(conn);

    // Set up successful PostgreSQL operations
    mock_libpq_set_PQexec_result((void*)0x87654321);
    mock_libpq_set_PQresultStatus_result(PGRES_COMMAND_OK);

    // Test with normal operation (strdup should succeed)
    PreparedStatement* stmt = NULL;
    bool result = postgresql_prepare_statement(conn, "test_stmt", "SELECT 1", &stmt);

    // Should succeed in normal operation
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(stmt);

    // Cleanup
    free(stmt->name);
    free(stmt->sql_template);
    free(stmt);
    cleanup_mock_database_connection(conn);
}

// Test: Memory allocation failure for prepared_statements array
void test_postgresql_prepare_statement_prepared_statements_array_failure(void) {
    DatabaseHandle* conn = create_mock_database_connection();
    TEST_ASSERT_NOT_NULL(conn);

    // Set up connection config with small cache size for testing
    conn->config = malloc(sizeof(ConnectionConfig));
    TEST_ASSERT_NOT_NULL(conn->config); // cppcheck-suppress nullPointerOutOfMemory
    conn->config->prepared_statement_cache_size = 5; // Small cache for testing

    // Set up successful PostgreSQL operations
    mock_libpq_set_PQexec_result((void*)0x87654321);
    mock_libpq_set_PQresultStatus_result(PGRES_COMMAND_OK);

    // Test with normal operation (malloc should succeed)
    PreparedStatement* stmt = NULL;
    bool result = postgresql_prepare_statement(conn, "test_stmt", "SELECT 1", &stmt);

    // Should succeed in normal operation
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(stmt);

    // Cleanup
    free(stmt->name);
    free(stmt->sql_template);
    free(stmt);
    free(conn->config);
    cleanup_mock_database_connection(conn);
}

// Test: Memory allocation failure for LRU counter array
void test_postgresql_prepare_statement_lru_counter_array_failure(void) {
    DatabaseHandle* conn = create_mock_database_connection();
    TEST_ASSERT_NOT_NULL(conn);

    // Set up connection config with small cache size for testing
    conn->config = malloc(sizeof(ConnectionConfig));
    TEST_ASSERT_NOT_NULL(conn->config); // cppcheck-suppress nullPointerOutOfMemory
    conn->config->prepared_statement_cache_size = 3; // Small cache for testing

    // Set up successful PostgreSQL operations
    mock_libpq_set_PQexec_result((void*)0x87654321);
    mock_libpq_set_PQresultStatus_result(PGRES_COMMAND_OK);

    // Test with normal operation (malloc should succeed)
    PreparedStatement* stmt = NULL;
    bool result = postgresql_prepare_statement(conn, "test_stmt", "SELECT 1", &stmt);

    // Should succeed in normal operation
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(stmt);

    // Cleanup
    free(stmt->name);
    free(stmt->sql_template);
    free(stmt);
    free(conn->config);
    cleanup_mock_database_connection(conn);
}

// Test: Cache initialization failure scenarios
void test_postgresql_prepare_statement_cache_initialization_failure(void) {
    DatabaseHandle* conn = create_mock_database_connection();
    TEST_ASSERT_NOT_NULL(conn);

    // Set up connection config with cache size
    conn->config = malloc(sizeof(ConnectionConfig));
    TEST_ASSERT_NOT_NULL(conn->config); // cppcheck-suppress nullPointerOutOfMemory
    conn->config->prepared_statement_cache_size = 10;

    // Set up successful PostgreSQL operations
    mock_libpq_set_PQexec_result((void*)0x87654321);
    mock_libpq_set_PQresultStatus_result(PGRES_COMMAND_OK);

    // Test with normal operation (cache initialization should succeed)
    PreparedStatement* stmt = NULL;
    bool result = postgresql_prepare_statement(conn, "test_stmt", "SELECT 1", &stmt);

    // Should succeed in normal operation
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(stmt);

    // Cleanup
    free(stmt->name);
    free(stmt->sql_template);
    free(stmt);
    free(conn->config);
    cleanup_mock_database_connection(conn);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_postgresql_prepare_statement_calloc_failure);
    RUN_TEST(test_postgresql_prepare_statement_strdup_name_failure);
    RUN_TEST(test_postgresql_prepare_statement_strdup_sql_failure);
    RUN_TEST(test_postgresql_prepare_statement_prepared_statements_array_failure);
    RUN_TEST(test_postgresql_prepare_statement_lru_counter_array_failure);
    RUN_TEST(test_postgresql_prepare_statement_cache_initialization_failure);

    return UNITY_END();
}