/*
 * Unity Test File: prepared_test_lru_eviction
 * Tests for SQLite prepared statement LRU cache eviction
 */

#include <src/hydrogen.h>
#include <unity.h>

// Mock libsqlite3 functions
#include <unity/mocks/mock_libsqlite3.h>

// Include necessary headers
#include <src/database/database.h>
#include <src/database/sqlite/types.h>
#include <src/database/sqlite/prepared.h>

// Forward declarations
bool sqlite_prepare_statement(DatabaseHandle* connection, const char* name, const char* sql, PreparedStatement** stmt, bool add_to_cache);

// External function pointers
extern sqlite3_prepare_v2_t sqlite3_prepare_v2_ptr;
extern sqlite3_finalize_t sqlite3_finalize_ptr;
extern sqlite3_errmsg_t sqlite3_errmsg_ptr;

// Function prototypes
void test_prepare_statement_lru_eviction_single(void);
void test_prepare_statement_lru_eviction_multiple(void);
void test_prepare_statement_lru_eviction_boundary(void);
void test_prepare_statement_lru_counter_increment(void);

void setUp(void) {
    mock_libsqlite3_reset_all();
    sqlite3_prepare_v2_ptr = mock_sqlite3_prepare_v2;
    sqlite3_finalize_ptr = mock_sqlite3_finalize;
    sqlite3_errmsg_ptr = mock_sqlite3_errmsg;
}

void tearDown(void) {
    mock_libsqlite3_reset_all();
}

// Test: LRU eviction when cache is full (single eviction)
void test_prepare_statement_lru_eviction_single(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.config = calloc(1, sizeof(ConnectionConfig));
    TEST_ASSERT_NOT_NULL(connection.config);
    connection.config->prepared_statement_cache_size = 2; // Small cache for testing
    
    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x1234;
    connection.connection_handle = &sqlite_conn;
    
    mock_libsqlite3_set_sqlite3_prepare_v2_result(SQLITE_OK);
    
    PreparedStatement* stmt1 = NULL;
    PreparedStatement* stmt2 = NULL;
    PreparedStatement* stmt3 = NULL;
    
    // Create first statement
    mock_libsqlite3_set_sqlite3_prepare_v2_output_handle((void*)0x1111);
    TEST_ASSERT_TRUE(sqlite_prepare_statement(&connection, "stmt_1", "SELECT 1", &stmt1, true));
    TEST_ASSERT_NOT_NULL(stmt1);
    TEST_ASSERT_EQUAL(1, connection.prepared_statement_count);
    
    // Create second statement
    mock_libsqlite3_set_sqlite3_prepare_v2_output_handle((void*)0x2222);
    TEST_ASSERT_TRUE(sqlite_prepare_statement(&connection, "stmt_2", "SELECT 2", &stmt2, true));
    TEST_ASSERT_NOT_NULL(stmt2);
    TEST_ASSERT_EQUAL(2, connection.prepared_statement_count);
    
    // Create third statement - should trigger LRU eviction
    mock_libsqlite3_set_sqlite3_prepare_v2_output_handle((void*)0x3333);
    TEST_ASSERT_TRUE(sqlite_prepare_statement(&connection, "stmt_3", "SELECT 3", &stmt3, true));
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

// Test: Multiple consecutive LRU evictions
void test_prepare_statement_lru_eviction_multiple(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.config = calloc(1, sizeof(ConnectionConfig));
    TEST_ASSERT_NOT_NULL(connection.config);
    connection.config->prepared_statement_cache_size = 1; // Very small cache
    
    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x1234;
    connection.connection_handle = &sqlite_conn;
    
    mock_libsqlite3_set_sqlite3_prepare_v2_result(SQLITE_OK);
    
    PreparedStatement* stmt1 = NULL;
    PreparedStatement* stmt2 = NULL;
    PreparedStatement* stmt3 = NULL;
    
    // Create first statement
    mock_libsqlite3_set_sqlite3_prepare_v2_output_handle((void*)0x1111);
    TEST_ASSERT_TRUE(sqlite_prepare_statement(&connection, "stmt_1", "SELECT 1", &stmt1, true));
    TEST_ASSERT_NOT_NULL(stmt1);
    TEST_ASSERT_EQUAL(1, connection.prepared_statement_count);
    
    // Create second statement - should evict stmt1
    mock_libsqlite3_set_sqlite3_prepare_v2_output_handle((void*)0x2222);
    TEST_ASSERT_TRUE(sqlite_prepare_statement(&connection, "stmt_2", "SELECT 2", &stmt2, true));
    TEST_ASSERT_NOT_NULL(stmt2);
    TEST_ASSERT_EQUAL(1, connection.prepared_statement_count);
    TEST_ASSERT_EQUAL(stmt2, connection.prepared_statements[0]);
    
    // Create third statement - should evict stmt2
    mock_libsqlite3_set_sqlite3_prepare_v2_output_handle((void*)0x3333);
    TEST_ASSERT_TRUE(sqlite_prepare_statement(&connection, "stmt_3", "SELECT 3", &stmt3, true));
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
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.config = calloc(1, sizeof(ConnectionConfig));
    TEST_ASSERT_NOT_NULL(connection.config);
    connection.config->prepared_statement_cache_size = 3;
    
    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x1234;
    connection.connection_handle = &sqlite_conn;
    
    mock_libsqlite3_set_sqlite3_prepare_v2_result(SQLITE_OK);
    
    PreparedStatement* stmt1 = NULL;
    PreparedStatement* stmt2 = NULL;
    PreparedStatement* stmt3 = NULL;
    PreparedStatement* stmt4 = NULL;
    
    // Fill cache exactly to capacity
    mock_libsqlite3_set_sqlite3_prepare_v2_output_handle((void*)0x1111);
    TEST_ASSERT_TRUE(sqlite_prepare_statement(&connection, "stmt_1", "SELECT 1", &stmt1, true));
    
    mock_libsqlite3_set_sqlite3_prepare_v2_output_handle((void*)0x2222);
    TEST_ASSERT_TRUE(sqlite_prepare_statement(&connection, "stmt_2", "SELECT 2", &stmt2, true));
    
    mock_libsqlite3_set_sqlite3_prepare_v2_output_handle((void*)0x3333);
    TEST_ASSERT_TRUE(sqlite_prepare_statement(&connection, "stmt_3", "SELECT 3", &stmt3, true));
    
    TEST_ASSERT_EQUAL(3, connection.prepared_statement_count);
    
    // Add one more - should trigger eviction
    mock_libsqlite3_set_sqlite3_prepare_v2_output_handle((void*)0x4444);
    TEST_ASSERT_TRUE(sqlite_prepare_statement(&connection, "stmt_4", "SELECT 4", &stmt4, true));
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

// Test: Verify LRU counter increments properly
void test_prepare_statement_lru_counter_increment(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.config = calloc(1, sizeof(ConnectionConfig));
    TEST_ASSERT_NOT_NULL(connection.config);
    connection.config->prepared_statement_cache_size = 5;
    
    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x1234;
    connection.connection_handle = &sqlite_conn;
    
    mock_libsqlite3_set_sqlite3_prepare_v2_result(SQLITE_OK);
    
    PreparedStatement* stmt1 = NULL;
    PreparedStatement* stmt2 = NULL;
    
    // Create first statement
    mock_libsqlite3_set_sqlite3_prepare_v2_output_handle((void*)0x1111);
    TEST_ASSERT_TRUE(sqlite_prepare_statement(&connection, "stmt_1", "SELECT 1", &stmt1, true));
    TEST_ASSERT_NOT_NULL(stmt1);
    TEST_ASSERT_EQUAL(1, connection.prepared_statement_count);
    
    // Verify LRU counter was set for first statement
    TEST_ASSERT_NOT_NULL(connection.prepared_statement_lru_counter);
    TEST_ASSERT_TRUE(connection.prepared_statement_lru_counter[0] > 0);
    
    // Create second statement
    mock_libsqlite3_set_sqlite3_prepare_v2_output_handle((void*)0x2222);
    TEST_ASSERT_TRUE(sqlite_prepare_statement(&connection, "stmt_2", "SELECT 2", &stmt2, true));
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
    RUN_TEST(test_prepare_statement_lru_counter_increment);
    
    return UNITY_END();
}