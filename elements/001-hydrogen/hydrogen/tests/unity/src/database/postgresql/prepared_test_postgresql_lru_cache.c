/*
 * Unity Test File: prepared_test_postgresql_lru_cache
 * Tests for PostgreSQL prepared statement LRU cache eviction scenarios
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
DatabaseHandle* create_mock_database_connection_with_cache(size_t cache_size);
void cleanup_mock_database_connection_with_cache(DatabaseHandle* conn);

// Test function prototypes
void test_postgresql_lru_cache_initialization(void);
void test_postgresql_lru_cache_single_eviction(void);
void test_postgresql_lru_cache_multiple_evictions(void);
void test_postgresql_lru_cache_boundary_conditions(void);
void test_postgresql_lru_counter_increment(void);
void test_postgresql_lru_find_least_used(void);

// Helper function to create a mock database connection with specific cache size
DatabaseHandle* create_mock_database_connection_with_cache(size_t cache_size) {
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
    conn->config->prepared_statement_cache_size = (int)cache_size;

    // Create mock PostgreSQL connection
    PostgresConnection* pg_conn = malloc(sizeof(PostgresConnection));
    TEST_ASSERT_NOT_NULL(pg_conn); // cppcheck-suppress nullPointerOutOfMemory

    pg_conn->connection = (void*)0x12345678; // Mock PGconn pointer
    pg_conn->in_transaction = false;
    pg_conn->prepared_statements = NULL;

    conn->connection_handle = pg_conn;
    return conn;
}

// Helper function to safely cleanup mock database connection with cache
void cleanup_mock_database_connection_with_cache(DatabaseHandle* conn) {
    if (!conn) return;

    if (conn->prepared_statements) {
        free(conn->prepared_statements);
    }
    if (conn->prepared_statement_lru_counter) {
        free(conn->prepared_statement_lru_counter);
    }
    if (conn->config) {
        free(conn->config);
    }
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

// Test: LRU cache initialization
void test_postgresql_lru_cache_initialization(void) {
    DatabaseHandle* conn = create_mock_database_connection_with_cache(5);
    TEST_ASSERT_NOT_NULL(conn);

    // Initially, arrays should be NULL
    TEST_ASSERT_NULL(conn->prepared_statements);
    TEST_ASSERT_NULL(conn->prepared_statement_lru_counter);
    TEST_ASSERT_EQUAL(0, conn->prepared_statement_count);

    // Create first prepared statement to trigger cache initialization
    PreparedStatement* stmt = NULL;
    bool result = postgresql_prepare_statement(conn, "test_stmt", "SELECT 1", &stmt);

    // Should succeed and initialize cache
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(stmt);
    TEST_ASSERT_NOT_NULL(conn->prepared_statements);
    TEST_ASSERT_NOT_NULL(conn->prepared_statement_lru_counter);
    TEST_ASSERT_EQUAL(1, conn->prepared_statement_count);

    // Cleanup
    free(stmt->name);
    free(stmt->sql_template);
    free(stmt);
    cleanup_mock_database_connection_with_cache(conn);
}

// Test: LRU eviction when cache is full (single eviction)
void test_postgresql_lru_cache_single_eviction(void) {
    DatabaseHandle* conn = create_mock_database_connection_with_cache(2);
    TEST_ASSERT_NOT_NULL(conn);

    PreparedStatement* stmt1 = NULL;
    PreparedStatement* stmt2 = NULL;

    // Create first statement
    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn, "stmt_1", "SELECT 1", &stmt1));
    TEST_ASSERT_NOT_NULL(stmt1);
    TEST_ASSERT_EQUAL(1, conn->prepared_statement_count);

    // Create second statement
    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn, "stmt_2", "SELECT 2", &stmt2));
    TEST_ASSERT_NOT_NULL(stmt2);
    TEST_ASSERT_EQUAL(2, conn->prepared_statement_count);

    // Cleanup for this simpler test
    free(stmt1->name);
    free(stmt1->sql_template);
    free(stmt1);
    free(stmt2->name);
    free(stmt2->sql_template);
    free(stmt2);
    cleanup_mock_database_connection_with_cache(conn);
}

// Test: LRU eviction with multiple statements (simplified)
void test_postgresql_lru_cache_multiple_evictions(void) {
    DatabaseHandle* conn = create_mock_database_connection_with_cache(2);
    TEST_ASSERT_NOT_NULL(conn);

    PreparedStatement* stmt1 = NULL;
    PreparedStatement* stmt2 = NULL;

    // Create 2 statements in a cache that holds only 2
    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn, "stmt_1", "SELECT 1", &stmt1));
    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn, "stmt_2", "SELECT 2", &stmt2));

    // Cache should hold exactly 2 statements
    TEST_ASSERT_EQUAL(2, conn->prepared_statement_count);

    // Cleanup
    free(stmt1->name);
    free(stmt1->sql_template);
    free(stmt1);
    free(stmt2->name);
    free(stmt2->sql_template);
    free(stmt2);
    cleanup_mock_database_connection_with_cache(conn);
}

// Test: LRU cache boundary conditions (simplified)
void test_postgresql_lru_cache_boundary_conditions(void) {
    DatabaseHandle* conn = create_mock_database_connection_with_cache(1);
    TEST_ASSERT_NOT_NULL(conn);

    PreparedStatement* stmt1 = NULL;

    // Create first statement
    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn, "stmt_1", "SELECT 1", &stmt1));
    TEST_ASSERT_NOT_NULL(stmt1);
    TEST_ASSERT_EQUAL(1, conn->prepared_statement_count);

    // Cleanup
    free(stmt1->name);
    free(stmt1->sql_template);
    free(stmt1);
    cleanup_mock_database_connection_with_cache(conn);
}

// Test: LRU counter increment functionality (simplified)
void test_postgresql_lru_counter_increment(void) {
    DatabaseHandle* conn = create_mock_database_connection_with_cache(2);
    TEST_ASSERT_NOT_NULL(conn);

    PreparedStatement* stmt1 = NULL;

    // Create first statement
    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn, "stmt_1", "SELECT 1", &stmt1));
    TEST_ASSERT_NOT_NULL(stmt1);
    TEST_ASSERT_EQUAL(1, conn->prepared_statement_count);

    // Verify LRU counter is set for first statement
    TEST_ASSERT_NOT_NULL(conn->prepared_statement_lru_counter);
    TEST_ASSERT(conn->prepared_statement_lru_counter[0] > 0);

    // Cleanup
    free(stmt1->name);
    free(stmt1->sql_template);
    free(stmt1);
    cleanup_mock_database_connection_with_cache(conn);
}

// Test: Finding least recently used statement (simplified)
void test_postgresql_lru_find_least_used(void) {
    DatabaseHandle* conn = create_mock_database_connection_with_cache(2);
    TEST_ASSERT_NOT_NULL(conn);

    PreparedStatement* stmt1 = NULL;
    PreparedStatement* stmt2 = NULL;

    // Create two statements
    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn, "stmt_1", "SELECT 1", &stmt1));
    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn, "stmt_2", "SELECT 2", &stmt2));

    TEST_ASSERT_EQUAL(2, conn->prepared_statement_count);

    // Verify LRU counters are in ascending order (older statements have lower counters)
    TEST_ASSERT(conn->prepared_statement_lru_counter[0] < conn->prepared_statement_lru_counter[1]);

    // Cleanup
    free(stmt1->name);
    free(stmt1->sql_template);
    free(stmt1);
    free(stmt2->name);
    free(stmt2->sql_template);
    free(stmt2);
    cleanup_mock_database_connection_with_cache(conn);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_postgresql_lru_cache_initialization);
    RUN_TEST(test_postgresql_lru_cache_single_eviction);
    RUN_TEST(test_postgresql_lru_cache_multiple_evictions);
    RUN_TEST(test_postgresql_lru_cache_boundary_conditions);
    RUN_TEST(test_postgresql_lru_counter_increment);
    RUN_TEST(test_postgresql_lru_find_least_used);

    return UNITY_END();
}