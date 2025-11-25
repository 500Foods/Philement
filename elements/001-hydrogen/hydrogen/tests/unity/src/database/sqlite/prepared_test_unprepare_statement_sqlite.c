/*
 * Unity Test File: prepared_test_unprepare_statement
 * Tests for SQLite prepared statement cleanup
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
bool sqlite_unprepare_statement(DatabaseHandle* connection, PreparedStatement* stmt);

// External function pointers
extern sqlite3_prepare_v2_t sqlite3_prepare_v2_ptr;
extern sqlite3_finalize_t sqlite3_finalize_ptr;
extern sqlite3_errmsg_t sqlite3_errmsg_ptr;

// Function prototypes
void test_unprepare_statement_success(void);
void test_unprepare_statement_no_finalize_ptr(void);
void test_unprepare_statement_multiple(void);
void test_unprepare_statement_null_handle(void);
void test_unprepare_statement_null_db_connection(void);

void setUp(void) {
    mock_libsqlite3_reset_all();
    sqlite3_prepare_v2_ptr = mock_sqlite3_prepare_v2;
    sqlite3_finalize_ptr = mock_sqlite3_finalize;
    sqlite3_errmsg_ptr = mock_sqlite3_errmsg;
}

void tearDown(void) {
    mock_libsqlite3_reset_all();
}

// Test: Successful unprepare with valid statement
void test_unprepare_statement_success(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    
    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x1234;
    connection.connection_handle = &sqlite_conn;
    
    // First create a prepared statement
    mock_libsqlite3_set_sqlite3_prepare_v2_result(SQLITE_OK);
    mock_libsqlite3_set_sqlite3_prepare_v2_output_handle((void*)0x5678);
    
    PreparedStatement* stmt = NULL;
    TEST_ASSERT_TRUE(sqlite_prepare_statement(&connection, "test_stmt", "SELECT 1", &stmt, true));
    TEST_ASSERT_NOT_NULL(stmt);
    TEST_ASSERT_EQUAL(1, connection.prepared_statement_count);
    
    // Now unprepare it
    bool result = sqlite_unprepare_statement(&connection, stmt);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(0, connection.prepared_statement_count);
    
    // Cleanup
    free(connection.prepared_statements);
    free(connection.prepared_statement_lru_counter);
}

// Test: Unprepare with NULL finalize pointer
void test_unprepare_statement_no_finalize_ptr(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    
    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x1234;
    connection.connection_handle = &sqlite_conn;
    
    // First create a prepared statement
    mock_libsqlite3_set_sqlite3_prepare_v2_result(SQLITE_OK);
    mock_libsqlite3_set_sqlite3_prepare_v2_output_handle((void*)0x5678);
    
    PreparedStatement* stmt = NULL;
    TEST_ASSERT_TRUE(sqlite_prepare_statement(&connection, "test_stmt", "SELECT 1", &stmt, true));
    TEST_ASSERT_NOT_NULL(stmt);
    
    // Clear finalize pointer before unpreparing
    sqlite3_finalize_ptr = NULL;
    
    // Should still clean up our structures
    bool result = sqlite_unprepare_statement(&connection, stmt);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(0, connection.prepared_statement_count);
    
    // Cleanup
    free(connection.prepared_statements);
    free(connection.prepared_statement_lru_counter);
}

// Test: Unprepare multiple statements
void test_unprepare_statement_multiple(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    
    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x1234;
    connection.connection_handle = &sqlite_conn;
    
    mock_libsqlite3_set_sqlite3_prepare_v2_result(SQLITE_OK);
    
    // Create three statements
    PreparedStatement* stmt1 = NULL;
    PreparedStatement* stmt2 = NULL;
    PreparedStatement* stmt3 = NULL;
    
    mock_libsqlite3_set_sqlite3_prepare_v2_output_handle((void*)0x1111);
    TEST_ASSERT_TRUE(sqlite_prepare_statement(&connection, "stmt_1", "SELECT 1", &stmt1, true));
    
    mock_libsqlite3_set_sqlite3_prepare_v2_output_handle((void*)0x2222);
    TEST_ASSERT_TRUE(sqlite_prepare_statement(&connection, "stmt_2", "SELECT 2", &stmt2, true));
    
    mock_libsqlite3_set_sqlite3_prepare_v2_output_handle((void*)0x3333);
    TEST_ASSERT_TRUE(sqlite_prepare_statement(&connection, "stmt_3", "SELECT 3", &stmt3, true));
    
    TEST_ASSERT_EQUAL(3, connection.prepared_statement_count);
    
    // Unprepare middle statement
    TEST_ASSERT_TRUE(sqlite_unprepare_statement(&connection, stmt2));
    TEST_ASSERT_EQUAL(2, connection.prepared_statement_count);
    
    // Verify remaining statements
    TEST_ASSERT_EQUAL(stmt1, connection.prepared_statements[0]);
    TEST_ASSERT_EQUAL(stmt3, connection.prepared_statements[1]);
    
    // Unprepare first statement
    TEST_ASSERT_TRUE(sqlite_unprepare_statement(&connection, stmt1));
    TEST_ASSERT_EQUAL(1, connection.prepared_statement_count);
    TEST_ASSERT_EQUAL(stmt3, connection.prepared_statements[0]);
    
    // Unprepare last statement
    TEST_ASSERT_TRUE(sqlite_unprepare_statement(&connection, stmt3));
    TEST_ASSERT_EQUAL(0, connection.prepared_statement_count);
    
    // Cleanup
    free(connection.prepared_statements);
    free(connection.prepared_statement_lru_counter);
}

// Test: Unprepare statement with NULL engine_specific_handle
void test_unprepare_statement_null_handle(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    
    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x1234;
    connection.connection_handle = &sqlite_conn;
    
    // Create a prepared statement
    mock_libsqlite3_set_sqlite3_prepare_v2_result(SQLITE_OK);
    mock_libsqlite3_set_sqlite3_prepare_v2_output_handle((void*)0x5678);
    
    PreparedStatement* stmt = NULL;
    TEST_ASSERT_TRUE(sqlite_prepare_statement(&connection, "test_stmt", "SELECT 1", &stmt, true));
    TEST_ASSERT_NOT_NULL(stmt);
    
    // Clear the engine_specific_handle to NULL
    stmt->engine_specific_handle = NULL;
    
    // Should still clean up successfully
    bool result = sqlite_unprepare_statement(&connection, stmt);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(0, connection.prepared_statement_count);
    
    // Cleanup
    free(connection.prepared_statements);
    free(connection.prepared_statement_lru_counter);
}

// Test: Unprepare statement with NULL db connection
void test_unprepare_statement_null_db_connection(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    
    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = NULL;
    connection.connection_handle = &sqlite_conn;
    
    PreparedStatement stmt = {0};
    stmt.name = strdup("test");
    stmt.sql_template = strdup("SELECT 1");
    
    bool result = sqlite_unprepare_statement(&connection, &stmt);
    TEST_ASSERT_FALSE(result);
    
    // Cleanup
    free(stmt.name);
    free(stmt.sql_template);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_unprepare_statement_success);
    RUN_TEST(test_unprepare_statement_no_finalize_ptr);
    RUN_TEST(test_unprepare_statement_multiple);
    RUN_TEST(test_unprepare_statement_null_handle);
    RUN_TEST(test_unprepare_statement_null_db_connection);
    
    return UNITY_END();
}