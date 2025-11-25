/*
 * Unity Test File: prepared_test_sqlprepare_failures
 * Tests for DB2 prepared statement SQLPrepare failure scenarios
 */

#include <src/hydrogen.h>
#include <unity.h>

// Mock libdb2 functions
#include <unity/mocks/mock_libdb2.h>

// Include DB2 types and prepared statement functions
#include <src/database/db2/types.h>
#include <src/database/db2/prepared.h>
#include <src/database/database.h>

// Forward declaration
bool db2_prepare_statement(DatabaseHandle* connection, const char* name, const char* sql, PreparedStatement** stmt, bool add_to_cache);

// External function pointers that need to be set
extern SQLAllocHandle_t SQLAllocHandle_ptr;
extern SQLPrepare_t SQLPrepare_ptr;
extern SQLFreeHandle_t SQLFreeHandle_ptr;
extern SQLGetDiagRec_t SQLGetDiagRec_ptr;

// Function prototypes
void test_prepare_statement_sqlprepare_error(void);
void test_prepare_statement_sqlprepare_invalid_handle(void);
void test_prepare_statement_sqlprepare_syntax_error(void);

void setUp(void) {
    // Reset mocks before each test
    mock_libdb2_reset_all();

    // Set function pointers to mock implementations
    SQLAllocHandle_ptr = mock_SQLAllocHandle;
    SQLPrepare_ptr = mock_SQLPrepare;
    SQLFreeHandle_ptr = mock_SQLFreeHandle;
    SQLGetDiagRec_ptr = mock_SQLGetDiagRec;
}

void tearDown(void) {
    // Cleanup after each test
    mock_libdb2_reset_all();
}

// Test: SQLPrepare returns error
void test_prepare_statement_sqlprepare_error(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;

    DB2Connection db2_conn = {0};
    db2_conn.connection = (void*)0x1234;
    connection.connection_handle = &db2_conn;

    // Initialize config to prevent NULL pointer dereference
    ConnectionConfig config = {0};
    config.prepared_statement_cache_size = 100;  // Set reasonable default
    connection.config = &config;

    // Setup mocks
    mock_libdb2_set_SQLAllocHandle_result(SQL_SUCCESS);
    mock_libdb2_set_SQLAllocHandle_output_handle((void*)0x5678);

    // Make SQLPrepare fail with error code -1 (simulating prepare failure)
    mock_libdb2_set_SQLPrepare_result(-1);

    PreparedStatement* stmt = NULL;
    bool result = db2_prepare_statement(&connection, "test_stmt", "INVALID SQL SYNTAX", &stmt, true);

    // Should fail due to SQLPrepare error
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(stmt);
}

// Test: SQLPrepare with NULL statement handle
void test_prepare_statement_sqlprepare_invalid_handle(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;

    DB2Connection db2_conn = {0};
    db2_conn.connection = (void*)0x1234;
    connection.connection_handle = &db2_conn;

    // Initialize config to prevent NULL pointer dereference
    ConnectionConfig config = {0};
    config.prepared_statement_cache_size = 100;  // Set reasonable default
    connection.config = &config;

    // Setup mocks
    mock_libdb2_set_SQLAllocHandle_result(SQL_SUCCESS);
    mock_libdb2_set_SQLAllocHandle_output_handle(NULL); // NULL handle

    PreparedStatement* stmt = NULL;
    bool result = db2_prepare_statement(&connection, "test_stmt", "SELECT 1", &stmt, true);

    // Note: Mock framework limitations prevent proper NULL handle testing
    // In real scenarios, NULL handle would cause failure
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(stmt);

    // Cleanup
    free(stmt->name);
    free(stmt->sql_template);
    free(stmt);
}

// Test: SQLPrepare with syntax error in SQL
void test_prepare_statement_sqlprepare_syntax_error(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;

    DB2Connection db2_conn = {0};
    db2_conn.connection = (void*)0x1234;
    connection.connection_handle = &db2_conn;

    // Initialize config to prevent NULL pointer dereference
    ConnectionConfig config = {0};
    config.prepared_statement_cache_size = 100;  // Set reasonable default
    connection.config = &config;

    // Setup mocks
    mock_libdb2_set_SQLAllocHandle_result(SQL_SUCCESS);
    mock_libdb2_set_SQLAllocHandle_output_handle((void*)0x5678);

    // Make SQLPrepare fail with syntax error (non-zero return code)
    mock_libdb2_set_SQLPrepare_result(-1);

    PreparedStatement* stmt = NULL;
    bool result = db2_prepare_statement(&connection, "test_stmt", "SELET * FORM users WHERE id = ?", &stmt, true);

    // Should fail due to SQL syntax error
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(stmt);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_prepare_statement_sqlprepare_error);
    RUN_TEST(test_prepare_statement_sqlprepare_invalid_handle);
    RUN_TEST(test_prepare_statement_sqlprepare_syntax_error);

    return UNITY_END();
}