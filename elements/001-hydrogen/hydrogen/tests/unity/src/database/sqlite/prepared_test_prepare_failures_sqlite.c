/*
 * Unity Test File: prepared_test_prepare_failures
 * Tests for SQLite prepared statement creation - failure scenarios
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
bool sqlite_prepare_statement(DatabaseHandle* connection, const char* name, const char* sql, PreparedStatement** stmt);

// External function pointers
extern sqlite3_prepare_v2_t sqlite3_prepare_v2_ptr;
extern sqlite3_finalize_t sqlite3_finalize_ptr;
extern sqlite3_errmsg_t sqlite3_errmsg_ptr;

// Function prototypes
void test_prepare_statement_null_sqlite_connection(void);
void test_prepare_statement_null_db_field(void);
void test_prepare_statement_no_function_pointers(void);
void test_prepare_statement_prepare_v2_failure(void);
void test_prepare_statement_prepare_v2_failure_with_errmsg(void);
void test_prepare_statement_prepare_v2_failure_no_errmsg_ptr(void);

void setUp(void) {
    mock_libsqlite3_reset_all();
    sqlite3_prepare_v2_ptr = mock_sqlite3_prepare_v2;
    sqlite3_finalize_ptr = mock_sqlite3_finalize;
    sqlite3_errmsg_ptr = mock_sqlite3_errmsg;
}

void tearDown(void) {
    mock_libsqlite3_reset_all();
}

// Test: NULL sqlite connection handle
void test_prepare_statement_null_sqlite_connection(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.connection_handle = NULL;
    
    PreparedStatement* stmt = NULL;
    bool result = sqlite_prepare_statement(&connection, "test", "SELECT 1", &stmt);
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(stmt);
}

// Test: NULL db field in SQLite connection
void test_prepare_statement_null_db_field(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    
    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = NULL;
    connection.connection_handle = &sqlite_conn;
    
    PreparedStatement* stmt = NULL;
    bool result = sqlite_prepare_statement(&connection, "test", "SELECT 1", &stmt);
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(stmt);
}

// Test: Function pointers not available
void test_prepare_statement_no_function_pointers(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    
    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x1234;
    connection.connection_handle = &sqlite_conn;
    
    // Clear function pointers
    sqlite3_prepare_v2_ptr = NULL;
    sqlite3_finalize_ptr = NULL;
    
    PreparedStatement* stmt = NULL;
    bool result = sqlite_prepare_statement(&connection, "test", "SELECT 1", &stmt);
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(stmt);
}

// Test: sqlite3_prepare_v2 failure
void test_prepare_statement_prepare_v2_failure(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    
    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x1234;
    connection.connection_handle = &sqlite_conn;
    
    // Make sqlite3_prepare_v2 fail
    mock_libsqlite3_set_sqlite3_prepare_v2_result(1); // SQLITE_ERROR
    mock_libsqlite3_set_sqlite3_errmsg_result("SQL syntax error");
    
    PreparedStatement* stmt = NULL;
    bool result = sqlite_prepare_statement(&connection, "test", "INVALID SQL", &stmt);
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(stmt);
}

// Test: sqlite3_prepare_v2 failure with error message
void test_prepare_statement_prepare_v2_failure_with_errmsg(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    
    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x1234;
    connection.connection_handle = &sqlite_conn;
    
    // Make sqlite3_prepare_v2 fail and provide error message
    mock_libsqlite3_set_sqlite3_prepare_v2_result(1); // SQLITE_ERROR
    mock_libsqlite3_set_sqlite3_errmsg_result("near \"INVALID\": syntax error");
    
    PreparedStatement* stmt = NULL;
    bool result = sqlite_prepare_statement(&connection, "test", "INVALID SQL", &stmt);
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(stmt);
}

// Test: sqlite3_prepare_v2 failure without errmsg_ptr
void test_prepare_statement_prepare_v2_failure_no_errmsg_ptr(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    
    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x1234;
    connection.connection_handle = &sqlite_conn;
    
    // Make sqlite3_prepare_v2 fail but clear errmsg function pointer
    mock_libsqlite3_set_sqlite3_prepare_v2_result(1); // SQLITE_ERROR
    sqlite3_errmsg_ptr = NULL;
    
    PreparedStatement* stmt = NULL;
    bool result = sqlite_prepare_statement(&connection, "test", "INVALID SQL", &stmt);
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(stmt);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_prepare_statement_null_sqlite_connection);
    RUN_TEST(test_prepare_statement_null_db_field);
    RUN_TEST(test_prepare_statement_no_function_pointers);
    RUN_TEST(test_prepare_statement_prepare_v2_failure);
    RUN_TEST(test_prepare_statement_prepare_v2_failure_with_errmsg);
    RUN_TEST(test_prepare_statement_prepare_v2_failure_no_errmsg_ptr);
    
    return UNITY_END();
}