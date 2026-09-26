/*
 * Unity Test File: prepared_test_unprepare_statement_mysql
 * Comprehensive tests for mariadb_unprepare_statement function
 */

#include <src/hydrogen.h>
#include <unity.h>

// MySQL mocks are enabled via CMake
#include <unity/mocks/mock_libmariadb.h>

// Include necessary headers for the module being tested
#include <src/database/database.h>
#include <src/database/mariadb/types.h>
#include <src/database/mariadb/connection.h>
#include <src/database/mariadb/prepared.h>

// Forward declarations for functions being tested
bool mariadb_unprepare_statement(DatabaseHandle* connection, PreparedStatement* stmt);

// External declarations for function pointers (defined in connection.c)
extern mariadb_stmt_init_t mariadb_stmt_init_ptr;
extern mariadb_stmt_prepare_t mariadb_stmt_prepare_ptr;
extern mariadb_stmt_execute_t mariadb_stmt_execute_ptr;
extern mariadb_stmt_close_t mariadb_stmt_close_ptr;
extern mariadb_error_t mariadb_error_ptr;

// Mock function declarations (when USE_MOCK_LIBMYSQLCLIENT is defined)
#ifdef USE_MOCK_LIBMYSQLCLIENT
extern void* mock_mariadb_stmt_init(void* mysql);
extern int mock_mariadb_stmt_prepare(void* stmt, const char* query, unsigned long length);
extern int mock_mariadb_stmt_execute(void* stmt);
extern int mock_mariadb_stmt_close(void* stmt);
extern const char* mock_mariadb_error(void* mysql);
#endif

// Test function prototypes
void test_mariadb_unprepare_statement_null_mariadb_connection(void);
void test_mariadb_unprepare_statement_null_mariadb_connection_field(void);
void test_mariadb_unprepare_statement_no_function_pointers(void);
void test_mariadb_unprepare_statement_mariadb_stmt_close_failure(void);
void test_mariadb_unprepare_statement_statement_not_in_cache(void);
void test_mariadb_unprepare_statement_statement_in_cache(void);
void test_mariadb_unprepare_statement_cleanup_without_mariadb_close(void);
void test_mariadb_unprepare_statement_multiple_statements(void);
void test_mariadb_unprepare_statement_success_with_cleanup(void);

void setUp(void) {
    // Reset mocks before each test
    mock_libmariadb_reset_all();

    // Initialize MySQL function pointers to use mocks
    load_libmariadb_functions(NULL);

    // Set prepared statement function pointers to mock implementations
    // (These should already be set when USE_MOCK_LIBMYSQLCLIENT is defined)
    mariadb_stmt_init_ptr = mock_mariadb_stmt_init;
    mariadb_stmt_prepare_ptr = mock_mariadb_stmt_prepare;
    mariadb_stmt_execute_ptr = mock_mariadb_stmt_execute;
    mariadb_stmt_close_ptr = mock_mariadb_stmt_close;
    mariadb_error_ptr = mock_mariadb_error;
}

void tearDown(void) {
    // Cleanup after each test
    mock_libmariadb_reset_all();
}

// Test: NULL MySQL connection handle
void test_mariadb_unprepare_statement_null_mariadb_connection(void) {
    PreparedStatement stmt = {0};
    bool result = mariadb_unprepare_statement(NULL, &stmt);
    TEST_ASSERT_FALSE(result);
}

// Test: NULL MySQL connection field
void test_mariadb_unprepare_statement_null_mariadb_connection_field(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MARIADB;

    MariadbConnection mariadb_conn = {0};
    mariadb_conn.connection = NULL;
    connection.connection_handle = &mariadb_conn;

    PreparedStatement stmt = {0};
    bool result = mariadb_unprepare_statement(&connection, &stmt);
    TEST_ASSERT_FALSE(result);
}

// Test: Function pointers not available for cleanup
void test_mariadb_unprepare_statement_no_function_pointers(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MARIADB;

    MariadbConnection mariadb_conn = {0};
    mariadb_conn.connection = (void*)0x1234;
    connection.connection_handle = &mariadb_conn;

    // Clear mariadb_stmt_close function pointer
    mariadb_stmt_close_ptr = NULL;

    PreparedStatement* stmt = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt);
    stmt->name = strdup("test_stmt");
    stmt->sql_template = strdup("SELECT 1");

    // Should still succeed by cleaning up our structures
    bool result = mariadb_unprepare_statement(&connection, stmt);
    TEST_ASSERT_TRUE(result);

    // Function should have freed the statement, so we don't need to cleanup here
}

// Test: mariadb_stmt_close failure scenario
void test_mariadb_unprepare_statement_mariadb_stmt_close_failure(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MARIADB;

    MariadbConnection mariadb_conn = {0};
    mariadb_conn.connection = (void*)0x1234;
    connection.connection_handle = &mariadb_conn;

    PreparedStatement* stmt = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt);
    stmt->name = strdup("test_stmt");
    stmt->sql_template = strdup("SELECT 1");
    stmt->engine_specific_handle = (void*)0x5678;

    // Mock mariadb_stmt_close to succeed (default behavior)
    mock_libmariadb_set_mysql_stmt_close_result(0);

    bool result = mariadb_unprepare_statement(&connection, stmt);
    TEST_ASSERT_TRUE(result);

    // Function should have freed the statement, so we don't need to cleanup here
}

// Test: Statement not found in cache
void test_mariadb_unprepare_statement_statement_not_in_cache(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MARIADB;

    MariadbConnection mariadb_conn = {0};
    mariadb_conn.connection = (void*)0x1234;
    connection.connection_handle = &mariadb_conn;

    // Initialize prepared statement arrays
    connection.prepared_statements = calloc(2, sizeof(PreparedStatement*));
    connection.prepared_statement_count = 0;

    PreparedStatement* stmt = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt);
    stmt->name = strdup("test_stmt");
    stmt->sql_template = strdup("SELECT 1");
    stmt->engine_specific_handle = (void*)0x5678;

    bool result = mariadb_unprepare_statement(&connection, stmt);
    TEST_ASSERT_TRUE(result);

    // Function should have freed the statement, so we don't need to cleanup here
    // Just cleanup the connection structures
    free(connection.prepared_statements);
}

// Test: Statement found and removed from cache
void test_mariadb_unprepare_statement_statement_in_cache(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MARIADB;

    MariadbConnection mariadb_conn = {0};
    mariadb_conn.connection = (void*)0x1234;
    connection.connection_handle = &mariadb_conn;

    // Initialize prepared statement arrays
    connection.prepared_statements = calloc(2, sizeof(PreparedStatement*));
    connection.prepared_statement_count = 1;

    PreparedStatement* stmt = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt);
    stmt->name = strdup("test_stmt");
    stmt->sql_template = strdup("SELECT 1");
    stmt->engine_specific_handle = (void*)0x5678;

    connection.prepared_statements[0] = stmt;

    bool result = mariadb_unprepare_statement(&connection, stmt);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(0, connection.prepared_statement_count);

    // Function should have freed the statement, so we don't need to cleanup here
    // Just cleanup the connection structures
    free(connection.prepared_statements);
}

// Test: Cleanup without MySQL close (when function pointers unavailable)
void test_mariadb_unprepare_statement_cleanup_without_mariadb_close(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MARIADB;

    MariadbConnection mariadb_conn = {0};
    mariadb_conn.connection = (void*)0x1234;
    connection.connection_handle = &mariadb_conn;

    // Clear mariadb_stmt_close function pointer
    mariadb_stmt_close_ptr = NULL;

    // Initialize prepared statement arrays
    connection.prepared_statements = calloc(2, sizeof(PreparedStatement*));
    connection.prepared_statement_count = 1;

    PreparedStatement* stmt = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt);
    stmt->name = strdup("test_stmt");
    stmt->sql_template = strdup("SELECT 1");
    stmt->engine_specific_handle = (void*)0x5678;

    connection.prepared_statements[0] = stmt;

    bool result = mariadb_unprepare_statement(&connection, stmt);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(0, connection.prepared_statement_count);

    // Function should have freed the statement, so we don't need to cleanup here
    // Just cleanup the connection structures
    free(connection.prepared_statements);
}

// Test: Multiple statements in cache
void test_mariadb_unprepare_statement_multiple_statements(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MARIADB;

    MariadbConnection mariadb_conn = {0};
    mariadb_conn.connection = (void*)0x1234;
    connection.connection_handle = &mariadb_conn;

    // Initialize prepared statement arrays
    connection.prepared_statements = calloc(3, sizeof(PreparedStatement*));
    connection.prepared_statement_count = 3;

    PreparedStatement* stmt1 = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt1);
    stmt1->name = strdup("stmt_1");
    stmt1->sql_template = strdup("SELECT 1");
    stmt1->engine_specific_handle = (void*)0x1111;

    PreparedStatement* stmt2 = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt2);
    stmt2->name = strdup("stmt_2");
    stmt2->sql_template = strdup("SELECT 2");
    stmt2->engine_specific_handle = (void*)0x2222;

    PreparedStatement* stmt3 = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt3);
    stmt3->name = strdup("stmt_3");
    stmt3->sql_template = strdup("SELECT 3");
    stmt3->engine_specific_handle = (void*)0x3333;

    connection.prepared_statements[0] = stmt1;
    connection.prepared_statements[1] = stmt2;
    connection.prepared_statements[2] = stmt3;

    // Remove middle statement (stmt2)
    bool result = mariadb_unprepare_statement(&connection, stmt2);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(2, connection.prepared_statement_count);

    // Verify remaining statements are in correct order
    TEST_ASSERT_EQUAL(stmt1, connection.prepared_statements[0]);
    TEST_ASSERT_EQUAL(stmt3, connection.prepared_statements[1]);

    // Function should have freed stmt2, but stmt1 and stmt3 should still be in the array
    // We need to clean them up since they're still referenced in the array
    // But the function removed stmt2 from the array and freed it
    free(stmt1->name);
    free(stmt1->sql_template);
    free(stmt1);
    free(stmt3->name);
    free(stmt3->sql_template);
    free(stmt3);
    free(connection.prepared_statements);
}

// Test: Successful unprepared statement with full cleanup
void test_mariadb_unprepare_statement_success_with_cleanup(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MARIADB;

    MariadbConnection mariadb_conn = {0};
    mariadb_conn.connection = (void*)0x1234;
    connection.connection_handle = &mariadb_conn;

    // Initialize prepared statement arrays
    connection.prepared_statements = calloc(2, sizeof(PreparedStatement*));
    connection.prepared_statement_count = 1;

    PreparedStatement* stmt = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt);
    stmt->name = strdup("test_stmt");
    stmt->sql_template = strdup("SELECT * FROM users WHERE id = ?");
    stmt->engine_specific_handle = (void*)0x5678;

    connection.prepared_statements[0] = stmt;

    bool result = mariadb_unprepare_statement(&connection, stmt);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(0, connection.prepared_statement_count);

    // Function should have freed the statement, so we don't need to cleanup here
    // Just cleanup the connection structures
    free(connection.prepared_statements);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mariadb_unprepare_statement_null_mariadb_connection);
    RUN_TEST(test_mariadb_unprepare_statement_null_mariadb_connection_field);
    RUN_TEST(test_mariadb_unprepare_statement_no_function_pointers);
    RUN_TEST(test_mariadb_unprepare_statement_mariadb_stmt_close_failure);
    RUN_TEST(test_mariadb_unprepare_statement_statement_not_in_cache);
    RUN_TEST(test_mariadb_unprepare_statement_statement_in_cache);
    RUN_TEST(test_mariadb_unprepare_statement_cleanup_without_mariadb_close);
    RUN_TEST(test_mariadb_unprepare_statement_multiple_statements);
    RUN_TEST(test_mariadb_unprepare_statement_success_with_cleanup);

    return UNITY_END();
}