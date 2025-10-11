/*
 * Unity Test File: DB2 Execute Prepared Edge Cases
 * Additional edge case tests for db2_execute_prepared function
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Enable system mocks
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

// DB2 mocks (already enabled by CMake)
#include <unity/mocks/mock_libdb2.h>

// Include DB2 types and functions
#include <src/database/db2/types.h>
#include <src/database/db2/query.h>
#include <src/database/db2/connection.h>

// Forward declarations
bool db2_execute_prepared(DatabaseHandle* connection, const PreparedStatement* stmt, QueryRequest* request, QueryResult** result);
bool load_libdb2_functions(const char* designator);

// Function prototypes
void test_db2_execute_prepared_designator_with_value(void);
void test_db2_execute_prepared_designator_null(void);
void test_db2_execute_prepared_success_with_info(void);

void setUp(void) {
    mock_system_reset_all();
    mock_libdb2_reset_all();
    
    // Initialize DB2 function pointers to use mocks
    load_libdb2_functions(NULL);
}

void tearDown(void) {
    mock_system_reset_all();
    mock_libdb2_reset_all();
}

// ============================================================================
// Edge case tests to improve coverage
// ============================================================================

void test_db2_execute_prepared_designator_with_value(void) {
    void* mock_conn = (void*)0x1000;
    DB2Connection db2_conn = {.connection = mock_conn};
    DatabaseHandle connection = {
        .engine_type = DB_ENGINE_DB2,
        .connection_handle = &db2_conn,
        .designator = (char*)"CustomDB"
    };
    PreparedStatement stmt = {.engine_specific_handle = (void*)0x2000};
    QueryRequest request = {.sql_template = (char*)"SELECT 1"};
    QueryResult* result = NULL;
    
    // Configure mock for success
    mock_libdb2_set_SQLExecute_result(0); // SQL_SUCCESS
    mock_libdb2_set_fetch_row_count(0);
    mock_libdb2_set_SQLNumResultCols_result(0, 0);
    
    TEST_ASSERT_TRUE(db2_execute_prepared(&connection, &stmt, &request, &result));
    TEST_ASSERT_NOT_NULL(result);
    
    // Cleanup
    free(result->data_json);
    free(result);
}

void test_db2_execute_prepared_designator_null(void) {
    void* mock_conn = (void*)0x1000;
    DB2Connection db2_conn = {.connection = mock_conn};
    DatabaseHandle connection = {
        .engine_type = DB_ENGINE_DB2,
        .connection_handle = &db2_conn,
        .designator = NULL
    };
    PreparedStatement stmt = {.engine_specific_handle = (void*)0x2000};
    QueryRequest request = {.sql_template = (char*)"SELECT 1"};
    QueryResult* result = NULL;
    
    // Configure mock for success
    mock_libdb2_set_SQLExecute_result(0); // SQL_SUCCESS
    mock_libdb2_set_fetch_row_count(0);
    mock_libdb2_set_SQLNumResultCols_result(0, 0);
    
    TEST_ASSERT_TRUE(db2_execute_prepared(&connection, &stmt, &request, &result));
    TEST_ASSERT_NOT_NULL(result);
    
    // Cleanup
    free(result->data_json);
    free(result);
}

void test_db2_execute_prepared_success_with_info(void) {
    void* mock_conn = (void*)0x1000;
    DB2Connection db2_conn = {.connection = mock_conn};
    DatabaseHandle connection = {
        .engine_type = DB_ENGINE_DB2,
        .connection_handle = &db2_conn
    };
    PreparedStatement stmt = {.engine_specific_handle = (void*)0x2000};
    QueryRequest request = {.sql_template = (char*)"SELECT 1"};
    QueryResult* result = NULL;
    
    // Return SQL_SUCCESS_WITH_INFO (1) instead of SQL_SUCCESS (0)
    mock_libdb2_set_SQLExecute_result(1);
    mock_libdb2_set_fetch_row_count(1);
    mock_libdb2_set_SQLNumResultCols_result(0, 1);
    mock_libdb2_set_SQLDescribeCol_column_name("value");
    mock_libdb2_set_SQLGetData_data("42", 2);
    
    TEST_ASSERT_TRUE(db2_execute_prepared(&connection, &stmt, &request, &result));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    
    // Cleanup
    free(result->data_json);
    free(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_db2_execute_prepared_designator_with_value);
    RUN_TEST(test_db2_execute_prepared_designator_null);
    RUN_TEST(test_db2_execute_prepared_success_with_info);

    return UNITY_END();
}