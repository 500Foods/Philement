/*
 * Unity Test File: prepared_test_lru_cache
 * Tests for DB2 prepared statement LRU cache eviction scenarios
 */

#include <src/hydrogen.h>
#include <unity.h>

// Mock libdb2 (enabled by CMake via -DUSE_MOCK_LIBDB2)
#include <unity/mocks/mock_libdb2.h>

// Include DB2 types and prepared statement functions
#include <src/database/db2/types.h>
#include <src/database/db2/prepared.h>
#include <src/database/database.h>

// Forward declaration
bool db2_prepare_statement(DatabaseHandle* connection, const char* name, const char* sql, PreparedStatement** stmt);

// External function pointers that need to be set
extern SQLAllocHandle_t SQLAllocHandle_ptr;
extern SQLPrepare_t SQLPrepare_ptr;
extern SQLFreeHandle_t SQLFreeHandle_ptr;

// Function prototypes
void test_prepare_statement_lru_eviction_single(void);
void test_prepare_statement_lru_eviction_multiple(void);
void test_prepare_statement_lru_eviction_boundary(void);
void test_prepare_statement_lru_find_least_used(void);
void test_prepare_statement_lru_counter_increment(void);

void setUp(void) {
    // Reset mocks before each test
    mock_libdb2_reset_all();

    // Set function pointers to mock implementations
    SQLAllocHandle_ptr = mock_SQLAllocHandle;
    SQLPrepare_ptr = mock_SQLPrepare;
    SQLFreeHandle_ptr = mock_SQLFreeHandle;
}

void tearDown(void) {
    // Cleanup after each test
    mock_libdb2_reset_all();
}

// Forward declaration for helper function
PreparedStatement* create_test_prepared_statement(const char* name, const char* sql, uint64_t lru_counter);

// Helper function to create a prepared statement
PreparedStatement* create_test_prepared_statement(const char* name, const char* sql, uint64_t lru_counter) {
    PreparedStatement* stmt = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt);

    stmt->name = strdup(name);
    stmt->sql_template = strdup(sql);
    stmt->created_at = time(NULL);
    stmt->usage_count = 0;
    stmt->engine_specific_handle = (void*)(uintptr_t)lru_counter; // Use handle to store LRU counter for testing

    return stmt;
}

// Test: LRU eviction when cache is full (single eviction)
void test_prepare_statement_lru_eviction_single(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    connection.config = calloc(1, sizeof(ConnectionConfig));
    TEST_ASSERT_NOT_NULL(connection.config);
    connection.config->prepared_statement_cache_size = 2; // Small cache for testing

    DB2Connection db2_conn = {0};
    db2_conn.connection = (void*)0x1234;
    connection.connection_handle = &db2_conn;

    // Setup mocks for success
    mock_libdb2_set_SQLAllocHandle_result(SQL_SUCCESS);

    PreparedStatement* stmt1 = NULL;
    PreparedStatement* stmt2 = NULL;
    PreparedStatement* stmt3 = NULL;

    // Create first statement
    mock_libdb2_set_SQLAllocHandle_output_handle((void*)0x1111);
    TEST_ASSERT_TRUE(db2_prepare_statement(&connection, "stmt_1", "SELECT 1", &stmt1));
    TEST_ASSERT_NOT_NULL(stmt1);
    TEST_ASSERT_EQUAL(1, connection.prepared_statement_count);

    // Create second statement
    mock_libdb2_set_SQLAllocHandle_output_handle((void*)0x2222);
    TEST_ASSERT_TRUE(db2_prepare_statement(&connection, "stmt_2", "SELECT 2", &stmt2));
    TEST_ASSERT_NOT_NULL(stmt2);
    TEST_ASSERT_EQUAL(2, connection.prepared_statement_count);

    // Create third statement - should trigger LRU eviction
    mock_libdb2_set_SQLAllocHandle_output_handle((void*)0x3333);
    TEST_ASSERT_TRUE(db2_prepare_statement(&connection, "stmt_3", "SELECT 3", &stmt3));
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

// Test: Multiple LRU evictions
void test_prepare_statement_lru_eviction_multiple(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    connection.config = calloc(1, sizeof(ConnectionConfig));
    TEST_ASSERT_NOT_NULL(connection.config);
    connection.config->prepared_statement_cache_size = 1; // Very small cache

    DB2Connection db2_conn = {0};
    db2_conn.connection = (void*)0x1234;
    connection.connection_handle = &db2_conn;

    // Setup mocks for success
    mock_libdb2_set_SQLAllocHandle_result(SQL_SUCCESS);

    PreparedStatement* stmt1 = NULL;
    PreparedStatement* stmt2 = NULL;
    PreparedStatement* stmt3 = NULL;

    // Create first statement
    mock_libdb2_set_SQLAllocHandle_output_handle((void*)0x1111);
    TEST_ASSERT_TRUE(db2_prepare_statement(&connection, "stmt_1", "SELECT 1", &stmt1));
    TEST_ASSERT_NOT_NULL(stmt1);
    TEST_ASSERT_EQUAL(1, connection.prepared_statement_count);

    // Create second statement - should evict stmt1
    mock_libdb2_set_SQLAllocHandle_output_handle((void*)0x2222);
    TEST_ASSERT_TRUE(db2_prepare_statement(&connection, "stmt_2", "SELECT 2", &stmt2));
    TEST_ASSERT_NOT_NULL(stmt2);
    TEST_ASSERT_EQUAL(1, connection.prepared_statement_count);
    TEST_ASSERT_EQUAL(stmt2, connection.prepared_statements[0]);

    // Create third statement - should evict stmt2
    mock_libdb2_set_SQLAllocHandle_output_handle((void*)0x3333);
    TEST_ASSERT_TRUE(db2_prepare_statement(&connection, "stmt_3", "SELECT 3", &stmt3));
    TEST_ASSERT_NOT_NULL(stmt3);
    TEST_ASSERT_EQUAL(1, connection.prepared_statement_count);
    TEST_ASSERT_EQUAL(stmt3, connection.prepared_statements[0]);

    // Cleanup
    free(stmt3->name);
    free(stmt3->sql_template);
    free(stmt3);
    free(connection.prepared_statements);
    free(connection.prepared_statement_lru_counter);
    free(connection.config);
}

// Test: LRU eviction at boundary conditions
void test_prepare_statement_lru_eviction_boundary(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    connection.config = calloc(1, sizeof(ConnectionConfig));
    TEST_ASSERT_NOT_NULL(connection.config);
    connection.config->prepared_statement_cache_size = 3;

    DB2Connection db2_conn = {0};
    db2_conn.connection = (void*)0x1234;
    connection.connection_handle = &db2_conn;

    // Setup mocks for success
    mock_libdb2_set_SQLAllocHandle_result(SQL_SUCCESS);

    PreparedStatement* stmt1 = NULL;
    PreparedStatement* stmt2 = NULL;
    PreparedStatement* stmt3 = NULL;
    PreparedStatement* stmt4 = NULL;

    // Fill cache exactly to capacity
    mock_libdb2_set_SQLAllocHandle_output_handle((void*)0x1111);
    TEST_ASSERT_TRUE(db2_prepare_statement(&connection, "stmt_1", "SELECT 1", &stmt1));

    mock_libdb2_set_SQLAllocHandle_output_handle((void*)0x2222);
    TEST_ASSERT_TRUE(db2_prepare_statement(&connection, "stmt_2", "SELECT 2", &stmt2));

    mock_libdb2_set_SQLAllocHandle_output_handle((void*)0x3333);
    TEST_ASSERT_TRUE(db2_prepare_statement(&connection, "stmt_3", "SELECT 3", &stmt3));

    TEST_ASSERT_EQUAL(3, connection.prepared_statement_count);

    // Add one more - should trigger eviction
    mock_libdb2_set_SQLAllocHandle_output_handle((void*)0x4444);
    TEST_ASSERT_TRUE(db2_prepare_statement(&connection, "stmt_4", "SELECT 4", &stmt4));
    TEST_ASSERT_NOT_NULL(stmt4);
    TEST_ASSERT_EQUAL(3, connection.prepared_statement_count);

    // Should have evicted stmt1 (first/LRU) and kept stmt2, stmt3, stmt4
    TEST_ASSERT_EQUAL(stmt2, connection.prepared_statements[0]);
    TEST_ASSERT_EQUAL(stmt3, connection.prepared_statements[1]);
    TEST_ASSERT_EQUAL(stmt4, connection.prepared_statements[2]);

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

// Test: Verify LRU counter logic finds least recently used
void test_prepare_statement_lru_find_least_used(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    connection.config = calloc(1, sizeof(ConnectionConfig));
    TEST_ASSERT_NOT_NULL(connection.config);
    connection.config->prepared_statement_cache_size = 3;

    DB2Connection db2_conn = {0};
    db2_conn.connection = (void*)0x1234;
    connection.connection_handle = &db2_conn;

    // Setup mocks for success
    mock_libdb2_set_SQLAllocHandle_result(SQL_SUCCESS);

    PreparedStatement* stmt1 = NULL;
    PreparedStatement* stmt2 = NULL;
    PreparedStatement* stmt3 = NULL;
    PreparedStatement* stmt4 = NULL;

    // Create statements with specific LRU counters to test LRU logic
    mock_libdb2_set_SQLAllocHandle_output_handle((void*)0x1111);
    TEST_ASSERT_TRUE(db2_prepare_statement(&connection, "stmt_1", "SELECT 1", &stmt1));

    mock_libdb2_set_SQLAllocHandle_output_handle((void*)0x2222);
    TEST_ASSERT_TRUE(db2_prepare_statement(&connection, "stmt_2", "SELECT 2", &stmt2));

    mock_libdb2_set_SQLAllocHandle_output_handle((void*)0x3333);
    TEST_ASSERT_TRUE(db2_prepare_statement(&connection, "stmt_3", "SELECT 3", &stmt3));

    // The LRU counter is a global counter that increments with each new statement
    // We can't easily control the exact values, but we can verify the logic works
    // by checking that the eviction happens correctly based on the counter order

    // Add fourth statement - should evict stmt1 (lowest LRU counter)
    mock_libdb2_set_SQLAllocHandle_output_handle((void*)0x4444);
    TEST_ASSERT_TRUE(db2_prepare_statement(&connection, "stmt_4", "SELECT 4", &stmt4));
    TEST_ASSERT_NOT_NULL(stmt4);
    TEST_ASSERT_EQUAL(3, connection.prepared_statement_count);

    // Should have evicted stmt1 (first/oldest) and kept stmt2, stmt3, stmt4
    // The exact order depends on the LRU counter implementation
    TEST_ASSERT_EQUAL(3, connection.prepared_statement_count);

    // Verify that stmt1 was evicted and the others remain
    bool found_stmt2 = false, found_stmt3 = false, found_stmt4 = false;
    for (size_t i = 0; i < connection.prepared_statement_count; i++) {
        if (connection.prepared_statements[i] == stmt2) found_stmt2 = true;
        if (connection.prepared_statements[i] == stmt3) found_stmt3 = true;
        if (connection.prepared_statements[i] == stmt4) found_stmt4 = true;
    }
    TEST_ASSERT_TRUE(found_stmt2);
    TEST_ASSERT_TRUE(found_stmt3);
    TEST_ASSERT_TRUE(found_stmt4);

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

// Test: Verify LRU counter increments properly
void test_prepare_statement_lru_counter_increment(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    connection.config = calloc(1, sizeof(ConnectionConfig));
    TEST_ASSERT_NOT_NULL(connection.config);
    connection.config->prepared_statement_cache_size = 5;

    DB2Connection db2_conn = {0};
    db2_conn.connection = (void*)0x1234;
    connection.connection_handle = &db2_conn;

    // Setup mocks for success
    mock_libdb2_set_SQLAllocHandle_result(SQL_SUCCESS);

    PreparedStatement* stmt1 = NULL;
    PreparedStatement* stmt2 = NULL;

    // Create first statement
    mock_libdb2_set_SQLAllocHandle_output_handle((void*)0x1111);
    TEST_ASSERT_TRUE(db2_prepare_statement(&connection, "stmt_1", "SELECT 1", &stmt1));
    TEST_ASSERT_NOT_NULL(stmt1);
    TEST_ASSERT_EQUAL(1, connection.prepared_statement_count);

    // Verify LRU counter was set for first statement
    TEST_ASSERT_NOT_NULL(connection.prepared_statement_lru_counter);
    TEST_ASSERT_TRUE(connection.prepared_statement_lru_counter[0] > 0);

    // Create second statement
    mock_libdb2_set_SQLAllocHandle_output_handle((void*)0x2222);
    TEST_ASSERT_TRUE(db2_prepare_statement(&connection, "stmt_2", "SELECT 2", &stmt2));
    TEST_ASSERT_NOT_NULL(stmt2);
    TEST_ASSERT_EQUAL(2, connection.prepared_statement_count);

    // Verify LRU counter for second statement is higher than first
    TEST_ASSERT_TRUE(connection.prepared_statement_lru_counter[1] > connection.prepared_statement_lru_counter[0]);

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

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_prepare_statement_lru_eviction_single);
    RUN_TEST(test_prepare_statement_lru_eviction_multiple);
    RUN_TEST(test_prepare_statement_lru_eviction_boundary);
    RUN_TEST(test_prepare_statement_lru_find_least_used);
    RUN_TEST(test_prepare_statement_lru_counter_increment);

    return UNITY_END();
}