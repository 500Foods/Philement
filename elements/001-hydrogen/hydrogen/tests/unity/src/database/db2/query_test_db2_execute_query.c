/*
 * Unity Test File: DB2 Query Execution Tests
 * Comprehensive tests for db2_execute_query function
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
bool db2_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result);
bool load_libdb2_functions(const char* designator);

// Function prototypes
void test_db2_execute_query_null_connection(void);
void test_db2_execute_query_null_request(void);
void test_db2_execute_query_null_result(void);
void test_db2_execute_query_wrong_engine_type(void);
void test_db2_execute_query_invalid_connection_handle(void);
void test_db2_execute_query_alloc_handle_failure(void);
void test_db2_execute_query_exec_failure(void);
void test_db2_execute_query_exec_failure_with_diag(void);
void test_db2_execute_query_result_alloc_failure(void);
void test_db2_execute_query_success_no_rows(void);
void test_db2_execute_query_success_with_rows(void);
void test_db2_execute_query_success_null_data(void);
void test_db2_execute_query_json_buffer_alloc_failure(void);

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
// Parameter validation tests
// ============================================================================

void test_db2_execute_query_null_connection(void) {
    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT 1";
    QueryResult* result = NULL;
    
    TEST_ASSERT_FALSE(db2_execute_query(NULL, &request, &result));
}

void test_db2_execute_query_null_request(void) {
    DatabaseHandle connection = {.engine_type = DB_ENGINE_DB2};
    QueryResult* result = NULL;
    
    TEST_ASSERT_FALSE(db2_execute_query(&connection, NULL, &result));
}

void test_db2_execute_query_null_result(void) {
    DatabaseHandle connection = {.engine_type = DB_ENGINE_DB2};
    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT 1";
    
    TEST_ASSERT_FALSE(db2_execute_query(&connection, &request, NULL));
}

void test_db2_execute_query_wrong_engine_type(void) {
    DatabaseHandle connection = {.engine_type = DB_ENGINE_POSTGRESQL};
    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT 1";
    QueryResult* result = NULL;
    
    TEST_ASSERT_FALSE(db2_execute_query(&connection, &request, &result));
}

void test_db2_execute_query_invalid_connection_handle(void) {
    DB2Connection db2_conn = {.connection = NULL};
    DatabaseHandle connection = {
        .engine_type = DB_ENGINE_DB2,
        .connection_handle = &db2_conn
    };
    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT 1";
    QueryResult* result = NULL;
    
    TEST_ASSERT_FALSE(db2_execute_query(&connection, &request, &result));
}

// ============================================================================
// Error path tests
// ============================================================================

void test_db2_execute_query_alloc_handle_failure(void) {
    void* mock_conn = (void*)0x1000;
    DB2Connection db2_conn = {.connection = mock_conn};
    DatabaseHandle connection = {
        .engine_type = DB_ENGINE_DB2,
        .connection_handle = &db2_conn
    };
    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT 1";
    QueryResult* result = NULL;
    
    // Make SQLAllocHandle fail
    mock_libdb2_set_SQLAllocHandle_result(-1);
    
    TEST_ASSERT_FALSE(db2_execute_query(&connection, &request, &result));
}

void test_db2_execute_query_exec_failure(void) {
    void* mock_conn = (void*)0x1000;
    DB2Connection db2_conn = {.connection = mock_conn};
    DatabaseHandle connection = {
        .engine_type = DB_ENGINE_DB2,
        .connection_handle = &db2_conn
    };
    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT 1";
    QueryResult* result = NULL;
    
    // Make SQLExecDirect fail
    mock_libdb2_set_SQLExecDirect_result(-1);
    // Make SQLGetDiagRec unavailable (returns error)
    mock_libdb2_set_SQLGetDiagRec_result(-1);
    
    TEST_ASSERT_FALSE(db2_execute_query(&connection, &request, &result));
}

void test_db2_execute_query_exec_failure_with_diag(void) {
    void* mock_conn = (void*)0x1000;
    DB2Connection db2_conn = {.connection = mock_conn};
    DatabaseHandle connection = {
        .engine_type = DB_ENGINE_DB2,
        .connection_handle = &db2_conn
    };
    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT * FROM invalid_table";
    QueryResult* result = NULL;
    
    // Make SQLExecDirect fail
    mock_libdb2_set_SQLExecDirect_result(-1);
    // Set up diagnostic information
    mock_libdb2_set_SQLGetDiagRec_result(0); // Success
    mock_libdb2_set_SQLGetDiagRec_error("42S02", -204, "Table not found\nInvalid object name");
    
    TEST_ASSERT_FALSE(db2_execute_query(&connection, &request, &result));
}

void test_db2_execute_query_result_alloc_failure(void) {
    void* mock_conn = (void*)0x1000;
    DB2Connection db2_conn = {.connection = mock_conn};
    DatabaseHandle connection = {
        .engine_type = DB_ENGINE_DB2,
        .connection_handle = &db2_conn
    };
    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT 1";
    QueryResult* result = NULL;
    
    // Make calloc fail for QueryResult
    mock_system_set_malloc_failure(1);
    
    TEST_ASSERT_FALSE(db2_execute_query(&connection, &request, &result));
}

void test_db2_execute_query_json_buffer_alloc_failure(void) {
    void* mock_conn = (void*)0x1000;
    DB2Connection db2_conn = {.connection = mock_conn};
    DatabaseHandle connection = {
        .engine_type = DB_ENGINE_DB2,
        .connection_handle = &db2_conn
    };
    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT 1";
    QueryResult* result = NULL;
    
    // Set up successful query with 1 column
    mock_libdb2_set_SQLNumResultCols_result(0, 1);
    mock_libdb2_set_SQLDescribeCol_column_name("id");
    
    // Make the second calloc fail (first is QueryResult, second is column_names, third is json_buffer)
    mock_system_set_malloc_failure(3);
    
    TEST_ASSERT_FALSE(db2_execute_query(&connection, &request, &result));
}

// ============================================================================
// Success path tests
// ============================================================================

void test_db2_execute_query_success_no_rows(void) {
    void* mock_conn = (void*)0x1000;
    DB2Connection db2_conn = {.connection = mock_conn};
    DatabaseHandle connection = {
        .engine_type = DB_ENGINE_DB2,
        .connection_handle = &db2_conn
    };
    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT 1";
    QueryResult* result = NULL;
    
    // Configure mock to return 0 rows
    mock_libdb2_set_fetch_row_count(0);
    mock_libdb2_set_SQLNumResultCols_result(0, 1);
    mock_libdb2_set_SQLDescribeCol_column_name("value");
    
    TEST_ASSERT_TRUE(db2_execute_query(&connection, &request, &result));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL(0, result->row_count);
    TEST_ASSERT_EQUAL_STRING("[]", result->data_json);
    
    // Cleanup
    free(result->data_json);
    free(result);
}

void test_db2_execute_query_success_with_rows(void) {
    void* mock_conn = (void*)0x1000;
    DB2Connection db2_conn = {.connection = mock_conn};
    DatabaseHandle connection = {
        .engine_type = DB_ENGINE_DB2,
        .connection_handle = &db2_conn
    };
    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT id, name FROM users";
    QueryResult* result = NULL;
    
    // Configure mock to return 2 rows with 2 columns
    mock_libdb2_set_fetch_row_count(2);
    mock_libdb2_set_SQLNumResultCols_result(0, 2);
    mock_libdb2_set_SQLDescribeCol_column_name("id");
    mock_libdb2_set_SQLGetData_data("123", 3);
    
    TEST_ASSERT_TRUE(db2_execute_query(&connection, &request, &result));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL(2, result->row_count);
    TEST_ASSERT_NOT_NULL(result->data_json);
    
    // Cleanup
    free(result->data_json);
    free(result);
}

void test_db2_execute_query_success_null_data(void) {
    void* mock_conn = (void*)0x1000;
    DB2Connection db2_conn = {.connection = mock_conn};
    DatabaseHandle connection = {
        .engine_type = DB_ENGINE_DB2,
        .connection_handle = &db2_conn
    };
    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT optional_field FROM table1";
    QueryResult* result = NULL;
    
    // Configure mock to return 1 row with NULL data
    mock_libdb2_set_fetch_row_count(1);
    mock_libdb2_set_SQLNumResultCols_result(0, 1);
    mock_libdb2_set_SQLDescribeCol_column_name("optional_field");
    mock_libdb2_set_SQLGetData_data("", -1); // SQL_NULL_DATA
    
    TEST_ASSERT_TRUE(db2_execute_query(&connection, &request, &result));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL(1, result->row_count);
    
    // Should contain null value in JSON
    TEST_ASSERT_NOT_NULL(strstr(result->data_json, "null"));
    
    // Cleanup
    free(result->data_json);
    free(result);
}

int main(void) {
    UNITY_BEGIN();

    // Parameter validation
    RUN_TEST(test_db2_execute_query_null_connection);
    RUN_TEST(test_db2_execute_query_null_request);
    RUN_TEST(test_db2_execute_query_null_result);
    RUN_TEST(test_db2_execute_query_wrong_engine_type);
    RUN_TEST(test_db2_execute_query_invalid_connection_handle);
    
    // Error paths
    RUN_TEST(test_db2_execute_query_alloc_handle_failure);
    RUN_TEST(test_db2_execute_query_exec_failure);
    RUN_TEST(test_db2_execute_query_exec_failure_with_diag);
    RUN_TEST(test_db2_execute_query_result_alloc_failure);
    RUN_TEST(test_db2_execute_query_json_buffer_alloc_failure);
    
    // Success paths
    RUN_TEST(test_db2_execute_query_success_no_rows);
    RUN_TEST(test_db2_execute_query_success_with_rows);
    RUN_TEST(test_db2_execute_query_success_null_data);

    return UNITY_END();
}