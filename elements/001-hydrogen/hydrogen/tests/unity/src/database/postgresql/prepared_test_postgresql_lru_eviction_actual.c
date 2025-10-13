/*
 * Unity Test File: prepared_test_postgresql_lru_eviction_actual
 * Tests for PostgreSQL prepared statement LRU cache eviction
 * This test actually triggers the LRU eviction code path
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for PostgreSQL prepared statement testing
#include <src/database/postgresql/prepared.h>
#include <src/database/postgresql/types.h>
#include <src/database/postgresql/connection.h>
#include <src/database/database.h>

// Include mocks for testing (USE_MOCK_LIBPQ is defined by CMake)
#include <tests/unity/mocks/mock_libpq.h>

// Forward declarations
bool postgresql_prepare_statement(DatabaseHandle* connection, const char* name, const char* sql, PreparedStatement** stmt);
bool postgresql_unprepare_statement(DatabaseHandle* connection, PreparedStatement* stmt);
bool postgresql_evict_lru_prepared_statement(DatabaseHandle* connection, PostgresConnection* pg_conn, const char* new_stmt_name);

// Mock function pointers
extern PQprepare_t PQprepare_ptr;
extern PQexec_t PQexec_ptr;
extern PQresultStatus_t PQresultStatus_ptr;
extern PQclear_t PQclear_ptr;
extern PQerrorMessage_t PQerrorMessage_ptr;

// Helper function prototypes
DatabaseHandle* create_test_connection(size_t cache_size);
void cleanup_test_connection(DatabaseHandle* conn);

// Test function prototypes
void test_lru_eviction_single_statement(void);
void test_lru_eviction_cache_size_one(void);
void test_lru_eviction_larger_cache(void);
void test_lru_counter_ordering(void);
void test_lru_eviction_array_shifting(void);
void test_unprepare_statement_basic(void);
void test_unprepare_statement_null_connection(void);
void test_unprepare_statement_null_statement(void);
void test_unprepare_statement_wrong_engine(void);
void test_unprepare_statement_null_pg_conn(void);
void test_unprepare_statement_deallocate_failure(void);

// Test fixtures
void setUp(void) {
    // Reset all mocks before each test
    mock_libpq_reset_all();

    // Set function pointers to mock implementations
    PQprepare_ptr = mock_PQprepare;
    PQexec_ptr = mock_PQexec;
    PQresultStatus_ptr = mock_PQresultStatus;
    PQclear_ptr = mock_PQclear;
    PQerrorMessage_ptr = mock_PQerrorMessage;

    // Set default successful mock behaviors
    mock_libpq_set_PQexec_result((void*)0x87654321);
    mock_libpq_set_PQresultStatus_result(PGRES_COMMAND_OK);
    mock_libpq_set_check_timeout_expired_result(false);
}

void tearDown(void) {
    mock_libpq_reset_all();
}

// Helper function to create a mock database connection with specific cache size
DatabaseHandle* create_test_connection(size_t cache_size) {
    DatabaseHandle* conn = calloc(1, sizeof(DatabaseHandle));
    TEST_ASSERT_NOT_NULL(conn);

    conn->engine_type = DB_ENGINE_POSTGRESQL;
    conn->prepared_statement_count = 0;
    conn->prepared_statements = NULL;
    conn->prepared_statement_lru_counter = NULL;

    // Set up connection config with specific cache size
    conn->config = calloc(1, sizeof(ConnectionConfig));
    TEST_ASSERT_NOT_NULL(conn->config);
    conn->config->prepared_statement_cache_size = (int)cache_size;

    // Create mock PostgreSQL connection
    PostgresConnection* pg_conn = calloc(1, sizeof(PostgresConnection));
    TEST_ASSERT_NOT_NULL(pg_conn);
    pg_conn->connection = (void*)0x12345678;
    pg_conn->in_transaction = false;

    conn->connection_handle = pg_conn;
    return conn;
}

// Helper function to cleanup connection
void cleanup_test_connection(DatabaseHandle* conn) {
    if (!conn) return;

    // Free all prepared statements
    if (conn->prepared_statements) {
        for (size_t i = 0; i < conn->prepared_statement_count; i++) {
            if (conn->prepared_statements[i]) {
                free(conn->prepared_statements[i]->name);
                free(conn->prepared_statements[i]->sql_template);
                free(conn->prepared_statements[i]);
            }
        }
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

// Test: LRU eviction when cache is full - single eviction
void test_lru_eviction_single_statement(void) {
    DatabaseHandle* conn = create_test_connection(2);

    PreparedStatement* stmt1 = NULL;
    PreparedStatement* stmt2 = NULL;
    PreparedStatement* stmt3 = NULL;

    // Create first statement
    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn, "stmt_1", "SELECT 1", &stmt1));
    TEST_ASSERT_NOT_NULL(stmt1);
    TEST_ASSERT_EQUAL(1, conn->prepared_statement_count);

    // Create second statement - fills cache
    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn, "stmt_2", "SELECT 2", &stmt2));
    TEST_ASSERT_NOT_NULL(stmt2);
    TEST_ASSERT_EQUAL(2, conn->prepared_statement_count);

    // Create third statement - should trigger LRU eviction of stmt_1
    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn, "stmt_3", "SELECT 3", &stmt3));
    TEST_ASSERT_NOT_NULL(stmt3);
    TEST_ASSERT_EQUAL(2, conn->prepared_statement_count);

    // Verify stmt_1 was evicted and stmt_2, stmt_3 remain
    bool found_stmt1 = false;
    bool found_stmt2 = false;
    bool found_stmt3 = false;
    
    for (size_t i = 0; i < conn->prepared_statement_count; i++) {
        if (conn->prepared_statements[i] == stmt1) found_stmt1 = true;
        if (conn->prepared_statements[i] == stmt2) found_stmt2 = true;
        if (conn->prepared_statements[i] == stmt3) found_stmt3 = true;
    }

    TEST_ASSERT_FALSE(found_stmt1); // stmt1 should be evicted
    TEST_ASSERT_TRUE(found_stmt2);  // stmt2 should remain
    TEST_ASSERT_TRUE(found_stmt3);  // stmt3 should be present

    cleanup_test_connection(conn);
}

// Test: Multiple LRU evictions with cache size of 1
void test_lru_eviction_cache_size_one(void) {
    DatabaseHandle* conn = create_test_connection(1);

    PreparedStatement* stmt1 = NULL;
    PreparedStatement* stmt2 = NULL;
    PreparedStatement* stmt3 = NULL;

    // First statement
    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn, "stmt_1", "SELECT 1", &stmt1));
    TEST_ASSERT_EQUAL(1, conn->prepared_statement_count);

    // Second statement - evicts first
    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn, "stmt_2", "SELECT 2", &stmt2));
    TEST_ASSERT_EQUAL(1, conn->prepared_statement_count);
    TEST_ASSERT_EQUAL(stmt2, conn->prepared_statements[0]);

    // Third statement - evicts second
    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn, "stmt_3", "SELECT 3", &stmt3));
    TEST_ASSERT_EQUAL(1, conn->prepared_statement_count);
    TEST_ASSERT_EQUAL(stmt3, conn->prepared_statements[0]);

    cleanup_test_connection(conn);
}

// Test: LRU eviction with larger cache
void test_lru_eviction_larger_cache(void) {
    DatabaseHandle* conn = create_test_connection(5);

    PreparedStatement* stmt1 = NULL;
    PreparedStatement* stmt2 = NULL;
    PreparedStatement* stmt3 = NULL;
    PreparedStatement* stmt4 = NULL;
    PreparedStatement* stmt5 = NULL;

    // Fill cache to capacity (5 statements)
    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn, "stmt_1", "SELECT 1", &stmt1));
    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn, "stmt_2", "SELECT 2", &stmt2));
    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn, "stmt_3", "SELECT 3", &stmt3));
    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn, "stmt_4", "SELECT 4", &stmt4));
    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn, "stmt_5", "SELECT 5", &stmt5));
    TEST_ASSERT_EQUAL(5, conn->prepared_statement_count);

    // Store LRU counters before eviction
    uint64_t lru_counter_stmt3 = conn->prepared_statement_lru_counter[2];
    uint64_t lru_counter_stmt4 = conn->prepared_statement_lru_counter[3];
    uint64_t lru_counter_stmt5 = conn->prepared_statement_lru_counter[4];

    // Add 2 more statements - should trigger 2 evictions
    PreparedStatement* stmt6 = NULL;
    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn, "stmt_6", "SELECT 6", &stmt6));
    TEST_ASSERT_EQUAL(5, conn->prepared_statement_count);

    PreparedStatement* stmt7 = NULL;
    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn, "stmt_7", "SELECT 7", &stmt7));
    TEST_ASSERT_EQUAL(5, conn->prepared_statement_count);

    // Verify first two LRU counters were evicted (stmt1 and stmt2 had lowest counters)
    // After eviction, stmt3's counter should now be at index 0
    TEST_ASSERT_EQUAL(lru_counter_stmt3, conn->prepared_statement_lru_counter[0]);
    TEST_ASSERT_EQUAL(lru_counter_stmt4, conn->prepared_statement_lru_counter[1]);
    TEST_ASSERT_EQUAL(lru_counter_stmt5, conn->prepared_statement_lru_counter[2]);

    // Verify the remaining statements are stmt3, stmt4, stmt5, stmt6, stmt7
    TEST_ASSERT_EQUAL(stmt3, conn->prepared_statements[0]);
    TEST_ASSERT_EQUAL(stmt4, conn->prepared_statements[1]);
    TEST_ASSERT_EQUAL(stmt5, conn->prepared_statements[2]);
    TEST_ASSERT_EQUAL(stmt6, conn->prepared_statements[3]);
    TEST_ASSERT_EQUAL(stmt7, conn->prepared_statements[4]);

    cleanup_test_connection(conn);
}

// Test: LRU counter ordering
void test_lru_counter_ordering(void) {
    DatabaseHandle* conn = create_test_connection(3);

    PreparedStatement* stmt1 = NULL;
    PreparedStatement* stmt2 = NULL;
    PreparedStatement* stmt3 = NULL;

    // Create three statements
    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn, "stmt_1", "SELECT 1", &stmt1));
    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn, "stmt_2", "SELECT 2", &stmt2));
    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn, "stmt_3", "SELECT 3", &stmt3));

    // Verify LRU counters are in ascending order
    TEST_ASSERT_TRUE(conn->prepared_statement_lru_counter[0] < conn->prepared_statement_lru_counter[1]);
    TEST_ASSERT_TRUE(conn->prepared_statement_lru_counter[1] < conn->prepared_statement_lru_counter[2]);

    cleanup_test_connection(conn);
}

// Test: Verify LRU eviction shifts array correctly
void test_lru_eviction_array_shifting(void) {
    DatabaseHandle* conn = create_test_connection(3);

    PreparedStatement* stmt1 = NULL;
    PreparedStatement* stmt2 = NULL;
    PreparedStatement* stmt3 = NULL;
    PreparedStatement* stmt4 = NULL;

    // Fill cache
    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn, "stmt_1", "SELECT 1", &stmt1));
    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn, "stmt_2", "SELECT 2", &stmt2));
    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn, "stmt_3", "SELECT 3", &stmt3));
    TEST_ASSERT_EQUAL(3, conn->prepared_statement_count);

    // Add fourth statement - should evict stmt_1 and shift array
    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn, "stmt_4", "SELECT 4", &stmt4));
    TEST_ASSERT_EQUAL(3, conn->prepared_statement_count);

    // Verify stmt_2 is now at index 0 (shifted from index 1)
    TEST_ASSERT_EQUAL(stmt2, conn->prepared_statements[0]);
    TEST_ASSERT_EQUAL(stmt3, conn->prepared_statements[1]);
    TEST_ASSERT_EQUAL(stmt4, conn->prepared_statements[2]);

    cleanup_test_connection(conn);
}

// Test: Test postgresql_unprepare_statement function
void test_unprepare_statement_basic(void) {
    DatabaseHandle* conn = create_test_connection(10);

    PreparedStatement* stmt1 = NULL;
    PreparedStatement* stmt2 = NULL;
    PreparedStatement* stmt3 = NULL;

    // Create three statements
    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn, "stmt_1", "SELECT 1", &stmt1));
    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn, "stmt_2", "SELECT 2", &stmt2));
    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn, "stmt_3", "SELECT 3", &stmt3));
    TEST_ASSERT_EQUAL(3, conn->prepared_statement_count);

    // Unprepare middle statement
    TEST_ASSERT_TRUE(postgresql_unprepare_statement(conn, stmt2));
    TEST_ASSERT_EQUAL(2, conn->prepared_statement_count);

    // Verify stmt2 is gone, stmt1 and stmt3 remain
    bool found_stmt1 = false;
    bool found_stmt3 = false;
    for (size_t i = 0; i < conn->prepared_statement_count; i++) {
        if (conn->prepared_statements[i] == stmt1) found_stmt1 = true;
        if (conn->prepared_statements[i] == stmt3) found_stmt3 = true;
    }
    TEST_ASSERT_TRUE(found_stmt1);
    TEST_ASSERT_TRUE(found_stmt3);

    cleanup_test_connection(conn);
}

// Test: Unprepare statement with NULL connection
void test_unprepare_statement_null_connection(void) {
    PreparedStatement stmt = {0};
    stmt.name = strdup("test");
    stmt.sql_template = strdup("SELECT 1");

    TEST_ASSERT_FALSE(postgresql_unprepare_statement(NULL, &stmt));

    free(stmt.name);
    free(stmt.sql_template);
}

// Test: Unprepare statement with NULL statement
void test_unprepare_statement_null_statement(void) {
    DatabaseHandle* conn = create_test_connection(10);
    TEST_ASSERT_FALSE(postgresql_unprepare_statement(conn, NULL));
    cleanup_test_connection(conn);
}

// Test: Unprepare statement with wrong engine type
void test_unprepare_statement_wrong_engine(void) {
    DatabaseHandle* conn = create_test_connection(10);
    conn->engine_type = DB_ENGINE_MYSQL; // Wrong engine

    PreparedStatement stmt = {0};
    stmt.name = strdup("test");
    stmt.sql_template = strdup("SELECT 1");

    TEST_ASSERT_FALSE(postgresql_unprepare_statement(conn, &stmt));

    free(stmt.name);
    free(stmt.sql_template);
    cleanup_test_connection(conn);
}

// Test: Unprepare statement with NULL pg_conn
void test_unprepare_statement_null_pg_conn(void) {
    DatabaseHandle* conn = create_test_connection(10);
    free(conn->connection_handle);
    conn->connection_handle = NULL;

    PreparedStatement stmt = {0};
    stmt.name = strdup("test");
    stmt.sql_template = strdup("SELECT 1");

    TEST_ASSERT_FALSE(postgresql_unprepare_statement(conn, &stmt));

    free(stmt.name);
    free(stmt.sql_template);
    cleanup_test_connection(conn);
}

// Test: Unprepare with DEALLOCATE failure
void test_unprepare_statement_deallocate_failure(void) {
    DatabaseHandle* conn = create_test_connection(10);

    PreparedStatement* stmt = NULL;
    TEST_ASSERT_TRUE(postgresql_prepare_statement(conn, "stmt_1", "SELECT 1", &stmt));

    // Set up mock to fail the DEALLOCATE
    mock_libpq_set_PQresultStatus_result(PGRES_FATAL_ERROR);

    TEST_ASSERT_FALSE(postgresql_unprepare_statement(conn, stmt));

    // When unprepare fails, the statement remains in the cache
    // cleanup_test_connection will free it
    cleanup_test_connection(conn);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_lru_eviction_single_statement);
    RUN_TEST(test_lru_eviction_cache_size_one);
    RUN_TEST(test_lru_eviction_larger_cache);
    RUN_TEST(test_lru_counter_ordering);
    RUN_TEST(test_lru_eviction_array_shifting);
    RUN_TEST(test_unprepare_statement_basic);
    RUN_TEST(test_unprepare_statement_null_connection);
    RUN_TEST(test_unprepare_statement_null_statement);
    RUN_TEST(test_unprepare_statement_wrong_engine);
    RUN_TEST(test_unprepare_statement_null_pg_conn);
    RUN_TEST(test_unprepare_statement_deallocate_failure);

    return UNITY_END();
}