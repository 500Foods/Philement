/*
 * Unity Test File: prepared_test_prepare_statement_success
 * Tests for SQLite prepared statement creation - success paths
 */

#include <src/hydrogen.h>
#include <unity.h>

// Mock libsqlite3 functions
#include <unity/mocks/mock_libsqlite3.h>

// Include necessary headers
#include <src/database/database.h>
#include <src/database/sqlite/types.h>
#include <src/database/sqlite/prepared.h>
#include <src/database/sqlite/connection.h>

// Forward declarations
bool sqlite_prepare_statement(DatabaseHandle* connection, const char* name, const char* sql, PreparedStatement** stmt, bool add_to_cache);
bool sqlite_check_timeout_expired(time_t start_time, int timeout_seconds);
PreparedStatementCache* sqlite_create_prepared_statement_cache(void);
void sqlite_destroy_prepared_statement_cache(PreparedStatementCache* cache);

// External function pointers
extern sqlite3_prepare_v2_t sqlite3_prepare_v2_ptr;
extern sqlite3_finalize_t sqlite3_finalize_ptr;
extern sqlite3_errmsg_t sqlite3_errmsg_ptr;

// Function prototypes
void test_prepare_statement_success_basic(void);
void test_prepare_statement_success_multiple(void);
void test_prepare_statement_success_custom_cache_size(void);
void test_prepare_statement_success_default_cache_size(void);

void setUp(void) {
    // Reset mocks before each test
    mock_libsqlite3_reset_all();
    
    // Set function pointers to mock implementations
    sqlite3_prepare_v2_ptr = mock_sqlite3_prepare_v2;
    sqlite3_finalize_ptr = mock_sqlite3_finalize;
    sqlite3_errmsg_ptr = mock_sqlite3_errmsg;
}

void tearDown(void) {
    // Cleanup after each test
    mock_libsqlite3_reset_all();
}

// Test: Successful prepared statement creation with valid parameters
void test_prepare_statement_success_basic(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.config = calloc(1, sizeof(ConnectionConfig));
    TEST_ASSERT_NOT_NULL(connection.config);
    connection.config->prepared_statement_cache_size = 100;
    
    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x1234;
    connection.connection_handle = &sqlite_conn;
    
    // Setup mocks for success
    mock_libsqlite3_set_sqlite3_prepare_v2_result(SQLITE_OK);
    mock_libsqlite3_set_sqlite3_prepare_v2_output_handle((void*)0x5678);
    
    PreparedStatement* stmt = NULL;
    bool result = sqlite_prepare_statement(&connection, "test_stmt", "SELECT * FROM users WHERE id = ?", &stmt, true);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(stmt);
    TEST_ASSERT_NOT_NULL(stmt->name);
    TEST_ASSERT_EQUAL_STRING("test_stmt", stmt->name);
    TEST_ASSERT_NOT_NULL(stmt->sql_template);
    TEST_ASSERT_EQUAL_STRING("SELECT * FROM users WHERE id = ?", stmt->sql_template);
    TEST_ASSERT_EQUAL(0, stmt->usage_count);
    TEST_ASSERT_EQUAL((void*)0x5678, stmt->engine_specific_handle);
    TEST_ASSERT_EQUAL(1, connection.prepared_statement_count);
    TEST_ASSERT_NOT_NULL(connection.prepared_statements);
    TEST_ASSERT_EQUAL(stmt, connection.prepared_statements[0]);
    TEST_ASSERT_NOT_NULL(connection.prepared_statement_lru_counter);
    TEST_ASSERT_TRUE(connection.prepared_statement_lru_counter[0] > 0);
    
    // Cleanup
    free(stmt->name);
    free(stmt->sql_template);
    free(stmt);
    free(connection.prepared_statements);
    free(connection.prepared_statement_lru_counter);
    free(connection.config);
}

// Test: Multiple prepared statements
void test_prepare_statement_success_multiple(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    
    // Initialize config to prevent NULL pointer dereference
    ConnectionConfig config = {0};
    config.prepared_statement_cache_size = 100;  // Set reasonable default
    connection.config = &config;
    
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
    
    // Create third statement
    mock_libsqlite3_set_sqlite3_prepare_v2_output_handle((void*)0x3333);
    TEST_ASSERT_TRUE(sqlite_prepare_statement(&connection, "stmt_3", "SELECT 3", &stmt3, true));
    TEST_ASSERT_NOT_NULL(stmt3);
    TEST_ASSERT_EQUAL(3, connection.prepared_statement_count);
    
    // Verify all statements are in array
    TEST_ASSERT_EQUAL(stmt1, connection.prepared_statements[0]);
    TEST_ASSERT_EQUAL(stmt2, connection.prepared_statements[1]);
    TEST_ASSERT_EQUAL(stmt3, connection.prepared_statements[2]);
    
    // Verify LRU counters are incrementing
    TEST_ASSERT_TRUE(connection.prepared_statement_lru_counter[1] > connection.prepared_statement_lru_counter[0]);
    TEST_ASSERT_TRUE(connection.prepared_statement_lru_counter[2] > connection.prepared_statement_lru_counter[1]);
    
    // Cleanup
    for (size_t i = 0; i < connection.prepared_statement_count; i++) {
        PreparedStatement* s = connection.prepared_statements[i];
        free(s->name);
        free(s->sql_template);
        free(s);
    }
    free(connection.prepared_statements);
    free(connection.prepared_statement_lru_counter);
}

// Test: Custom cache size configuration
void test_prepare_statement_success_custom_cache_size(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.config = calloc(1, sizeof(ConnectionConfig));
    TEST_ASSERT_NOT_NULL(connection.config);
    connection.config->prepared_statement_cache_size = 50;
    
    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x1234;
    connection.connection_handle = &sqlite_conn;
    
    mock_libsqlite3_set_sqlite3_prepare_v2_result(SQLITE_OK);
    mock_libsqlite3_set_sqlite3_prepare_v2_output_handle((void*)0x5678);
    
    PreparedStatement* stmt = NULL;
    bool result = sqlite_prepare_statement(&connection, "test_stmt", "SELECT 1", &stmt, true);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(connection.prepared_statements);
    
    // Cleanup
    free(stmt->name);
    free(stmt->sql_template);
    free(stmt);
    free(connection.prepared_statements);
    free(connection.prepared_statement_lru_counter);
    free(connection.config);
}

// Test: Default cache size when config not set
void test_prepare_statement_success_default_cache_size(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    // No config set - should use default 1000
    
    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x1234;
    connection.connection_handle = &sqlite_conn;
    
    mock_libsqlite3_set_sqlite3_prepare_v2_result(SQLITE_OK);
    mock_libsqlite3_set_sqlite3_prepare_v2_output_handle((void*)0x5678);
    
    PreparedStatement* stmt = NULL;
    bool result = sqlite_prepare_statement(&connection, "test_stmt", "SELECT 1", &stmt, true);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(stmt);
    
    // Cleanup
    free(stmt->name);
    free(stmt->sql_template);
    free(stmt);
    free(connection.prepared_statements);
    free(connection.prepared_statement_lru_counter);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_prepare_statement_success_basic);
    RUN_TEST(test_prepare_statement_success_multiple);
    RUN_TEST(test_prepare_statement_success_custom_cache_size);
    RUN_TEST(test_prepare_statement_success_default_cache_size);
    
    return UNITY_END();
}