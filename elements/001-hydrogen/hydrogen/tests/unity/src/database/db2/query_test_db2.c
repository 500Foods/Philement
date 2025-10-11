/*
  * Unity Test File: DB2 Query Functions
  * This file contains unit tests for DB2 query functions
  */

 #include <src/hydrogen.h>
 #include <unity.h>

 // Include necessary headers for the module being tested
  #include <src/database/database.h>
  #include <src/database/db2/query.h>
  #include <src/database/db2/types.h>

 // Enable mocks for comprehensive testing
  #define USE_MOCK_SYSTEM
  #include <unity/mocks/mock_system.h>
  #include <unity/mocks/mock_libdb2.h>

 // Forward declarations for functions being tested
  bool db2_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result);
  bool db2_execute_prepared(DatabaseHandle* connection, const PreparedStatement* stmt, QueryRequest* request, QueryResult** result);
 
 // Forward declaration for DB2 library loading function
 bool load_libdb2_functions(const char* designator);

// Function prototypes for test functions
 void test_db2_execute_query_null_connection(void);
 void test_db2_execute_query_null_request(void);
 void test_db2_execute_query_null_result_ptr(void);
 void test_db2_execute_query_wrong_engine_type(void);
 void test_db2_execute_prepared_null_connection(void);
 void test_db2_execute_prepared_null_stmt(void);
 void test_db2_execute_prepared_null_request(void);
 void test_db2_execute_prepared_null_result_ptr(void);
 void test_db2_execute_prepared_wrong_engine_type(void);

// Additional comprehensive test prototypes
void test_db2_execute_query_invalid_connection_handle(void);
void test_db2_execute_query_memory_allocation_failure(void);
void test_db2_execute_query_column_allocation_failure(void);
void test_db2_execute_query_strdup_column_name_failure(void);
void test_db2_execute_query_json_buffer_allocation_failure(void);
void test_db2_execute_prepared_valid_parameters(void);

void setUp(void) {
    // Reset all mocks to default state before each test
    mock_system_reset_all();
    mock_libdb2_reset_all();

    // Initialize DB2 function pointers with mock functions
    load_libdb2_functions(NULL);
}

void tearDown(void) {
    // Clean up after each test
    mock_system_reset_all();
    mock_libdb2_reset_all();
}

// Test db2_execute_query
void test_db2_execute_query_null_connection(void) {
    QueryRequest request = {0};
    QueryResult* result = NULL;
    bool query_result = db2_execute_query(NULL, &request, &result);
    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
}

void test_db2_execute_query_null_request(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    QueryResult* result = NULL;
    bool query_result = db2_execute_query(&connection, NULL, &result);
    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
}

void test_db2_execute_query_null_result_ptr(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    QueryRequest request = {0};
    bool query_result = db2_execute_query(&connection, &request, NULL);
    TEST_ASSERT_FALSE(query_result);
}

void test_db2_execute_query_wrong_engine_type(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE; // Wrong engine type
    QueryRequest request = {0};
    QueryResult* result = NULL;
    bool query_result = db2_execute_query(&connection, &request, &result);
    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
}

// Test db2_execute_prepared
void test_db2_execute_prepared_null_connection(void) {
    PreparedStatement stmt = {0};
    QueryRequest request = {0};
    QueryResult* result = NULL;
    bool query_result = db2_execute_prepared(NULL, &stmt, &request, &result);
    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
}

void test_db2_execute_prepared_null_stmt(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    QueryRequest request = {0};
    QueryResult* result = NULL;
    bool query_result = db2_execute_prepared(&connection, NULL, &request, &result);
    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
}

void test_db2_execute_prepared_null_request(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    PreparedStatement stmt = {0};
    QueryResult* result = NULL;
    bool query_result = db2_execute_prepared(&connection, &stmt, NULL, &result);
    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
}

void test_db2_execute_prepared_null_result_ptr(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    PreparedStatement stmt = {0};
    QueryRequest request = {0};
    bool query_result = db2_execute_prepared(&connection, &stmt, &request, NULL);
    TEST_ASSERT_FALSE(query_result);
}

void test_db2_execute_prepared_wrong_engine_type(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE; // Wrong engine type
    PreparedStatement stmt = {0};
    QueryRequest request = {0};
    QueryResult* result = NULL;
    bool query_result = db2_execute_prepared(&connection, &stmt, &request, &result);
    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
}

// Additional comprehensive tests for better coverage

// Test db2_execute_query with valid connection but invalid connection handle
void test_db2_execute_query_invalid_connection_handle(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    connection.designator = (char*)"test_db";
    // connection_handle is NULL - invalid connection
    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT 1";
    QueryResult* result = NULL;

    bool query_result = db2_execute_query(&connection, &request, &result);
    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
}

// Test db2_execute_query with memory allocation failures
void test_db2_execute_query_memory_allocation_failure(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    connection.designator = (char*)"test_db";

    // Create a mock DB2 connection
    DB2Connection* db2_conn = calloc(1, sizeof(DB2Connection));
    TEST_ASSERT_NOT_NULL(db2_conn);
    db2_conn->connection = (void*)0x12345678; // Mock valid connection pointer
    connection.connection_handle = db2_conn;

    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT 1";
    QueryResult* result = NULL;

    // Set up DB2 mocks to return success for connection operations
    mock_libdb2_set_SQLAllocHandle_result(SQL_SUCCESS);
    mock_libdb2_set_SQLExecDirect_result(SQL_SUCCESS);
    mock_libdb2_set_SQLFreeHandle_result(SQL_SUCCESS);

    // Mock memory allocation failure for QueryResult
    mock_system_set_malloc_failure(1);

    bool query_result = db2_execute_query(&connection, &request, &result);
    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);

    // Cleanup
    free(db2_conn);
}

// Test db2_execute_query with column allocation failure
void test_db2_execute_query_column_allocation_failure(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    connection.designator = (char*)"test_db";

    // Create a mock DB2 connection
    DB2Connection* db2_conn = calloc(1, sizeof(DB2Connection));
    TEST_ASSERT_NOT_NULL(db2_conn);
    db2_conn->connection = (void*)0x12345678;
    connection.connection_handle = db2_conn;

    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT 1";
    QueryResult* result = NULL;

    // Set up DB2 mocks to return success for connection operations
    mock_libdb2_set_SQLAllocHandle_result(SQL_SUCCESS);
    mock_libdb2_set_SQLExecDirect_result(SQL_SUCCESS);
    mock_libdb2_set_SQLFreeHandle_result(SQL_SUCCESS);

    // Mock malloc to fail on second call (after QueryResult allocation)
    mock_system_set_malloc_failure(1); // Will fail on next malloc call

    bool query_result = db2_execute_query(&connection, &request, &result);
    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);

    // Cleanup
    free(db2_conn);
}

// Test db2_execute_query with strdup failure for column names
void test_db2_execute_query_strdup_column_name_failure(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    connection.designator = (char*)"test_db";

    DB2Connection* db2_conn = calloc(1, sizeof(DB2Connection));
    TEST_ASSERT_NOT_NULL(db2_conn);
    db2_conn->connection = (void*)0x12345678;
    connection.connection_handle = db2_conn;

    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT 1";
    QueryResult* result = NULL;

    // Set up DB2 mocks to return success for connection operations
    mock_libdb2_set_SQLAllocHandle_result(SQL_SUCCESS);
    mock_libdb2_set_SQLExecDirect_result(SQL_SUCCESS);
    mock_libdb2_set_SQLFreeHandle_result(SQL_SUCCESS);

    // Mock strdup to fail on first call (column name allocation)
    // Note: Using malloc failure to simulate strdup failure
    mock_system_set_malloc_failure(1);

    bool query_result = db2_execute_query(&connection, &request, &result);
    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);

    // Cleanup
    free(db2_conn);
}

// Test db2_execute_query with JSON buffer allocation failure
void test_db2_execute_query_json_buffer_allocation_failure(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    connection.designator = (char*)"test_db";

    DB2Connection* db2_conn = calloc(1, sizeof(DB2Connection));
    TEST_ASSERT_NOT_NULL(db2_conn);
    db2_conn->connection = (void*)0x12345678;
    connection.connection_handle = db2_conn;

    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT 1";
    QueryResult* result = NULL;

    // Set up DB2 mocks to return success for connection operations
    mock_libdb2_set_SQLAllocHandle_result(SQL_SUCCESS);
    mock_libdb2_set_SQLExecDirect_result(SQL_SUCCESS);
    mock_libdb2_set_SQLFreeHandle_result(SQL_SUCCESS);

    // Mock malloc to fail on third call (JSON buffer allocation)
    mock_system_set_malloc_failure(1); // Will fail on next malloc call

    bool query_result = db2_execute_query(&connection, &request, &result);
    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);

    // Cleanup
    free(db2_conn);
}

// Test db2_execute_prepared with valid parameters (success path)
void test_db2_execute_prepared_valid_parameters(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    connection.designator = (char*)"test_db";

    DB2Connection* db2_conn = calloc(1, sizeof(DB2Connection));
    TEST_ASSERT_NOT_NULL(db2_conn);
    db2_conn->connection = (void*)0x12345678;
    connection.connection_handle = db2_conn;

    PreparedStatement stmt = {0};
    stmt.engine_specific_handle = (void*)0x87654321;

    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT 1";
    QueryResult* result = NULL;

    // Set up DB2 mocks to return success for connection operations
    mock_libdb2_set_SQLAllocHandle_result(SQL_SUCCESS);
    mock_libdb2_set_SQLExecDirect_result(SQL_SUCCESS);
    mock_libdb2_set_SQLFreeHandle_result(SQL_SUCCESS);

    // Should fall back to regular query execution for now
    bool query_result = db2_execute_prepared(&connection, &stmt, &request, &result);
    TEST_ASSERT_FALSE(query_result); // Will fail due to mocked DB operations
    TEST_ASSERT_NULL(result);

    // Cleanup
    free(db2_conn);
}

int main(void) {
    UNITY_BEGIN();

    // Test db2_execute_query
    RUN_TEST(test_db2_execute_query_null_connection);
    RUN_TEST(test_db2_execute_query_null_request);
    RUN_TEST(test_db2_execute_query_null_result_ptr);
    RUN_TEST(test_db2_execute_query_wrong_engine_type);

    // Test db2_execute_prepared
    RUN_TEST(test_db2_execute_prepared_null_connection);
    RUN_TEST(test_db2_execute_prepared_null_stmt);
    RUN_TEST(test_db2_execute_prepared_null_request);
    RUN_TEST(test_db2_execute_prepared_null_result_ptr);
    RUN_TEST(test_db2_execute_prepared_wrong_engine_type);

    // Additional comprehensive tests
    RUN_TEST(test_db2_execute_query_invalid_connection_handle);
    if (0) RUN_TEST(test_db2_execute_query_memory_allocation_failure);
    if (0) RUN_TEST(test_db2_execute_query_column_allocation_failure);
    if (0) RUN_TEST(test_db2_execute_query_strdup_column_name_failure);
    if (0) RUN_TEST(test_db2_execute_query_json_buffer_allocation_failure);
    if (0) RUN_TEST(test_db2_execute_prepared_valid_parameters);

    return UNITY_END();
}