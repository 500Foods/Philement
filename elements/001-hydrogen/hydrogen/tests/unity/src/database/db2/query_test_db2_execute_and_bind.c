/*
 * Unity Test File: DB2 Query Functions - Execute and Bind Tests
 * Tests for db2_execute_query, db2_execute_prepared, and db2_bind_single_parameter
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Enable system mocks for testing memory allocation failures
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

// DB2 mocks (already enabled by CMake)
#include <unity/mocks/mock_libdb2.h>

// Include the module being tested
#include <src/database/database.h>
#include <src/database/database_params.h>
#include <src/database/db2/query.h>
#include <src/database/db2/types.h>
#include <src/database/db2/query_helpers.h>

// Forward declarations for functions being tested
bool db2_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result);
bool db2_execute_prepared(DatabaseHandle* connection, const PreparedStatement* stmt, QueryRequest* request, QueryResult** result);
bool db2_bind_single_parameter(void* stmt_handle, unsigned short param_index, TypedParameter* param,
                               void** bound_values, long* str_len_indicators, const char* designator);

// Forward declaration for DB2 library loading function
bool load_libdb2_functions(const char* designator);

// Test function prototypes
void test_db2_execute_query_convert_named_to_positional_failure(void);
void test_db2_execute_query_allocate_binding_arrays_failure(void);
void test_db2_execute_query_bind_parameter_failure(void);
void test_db2_execute_query_sqlprepare_failure(void);
void test_db2_execute_query_sqlexecute_failure(void);
void test_db2_execute_prepared_sqlexecute_failure(void);
void test_db2_bind_single_parameter_null_stmt_handle(void);
void test_db2_bind_single_parameter_null_param(void);
void test_db2_bind_single_parameter_null_bound_values(void);
void test_db2_bind_single_parameter_null_str_len_indicators(void);
void test_db2_bind_single_parameter_null_designator(void);
void test_db2_bind_single_parameter_no_sqlbindparameter(void);
void test_db2_bind_single_parameter_boolean_type(void);
void test_db2_bind_single_parameter_boolean_malloc_failure(void);
void test_db2_bind_single_parameter_float_type(void);
void test_db2_bind_single_parameter_float_malloc_failure(void);
void test_db2_bind_single_parameter_text_type(void);
void test_db2_bind_single_parameter_text_malloc_failure(void);
void test_db2_bind_single_parameter_date_type(void);
void test_db2_bind_single_parameter_date_malloc_failure(void);
void test_db2_bind_single_parameter_date_invalid_format(void);
void test_db2_bind_single_parameter_time_type(void);
void test_db2_bind_single_parameter_time_malloc_failure(void);
void test_db2_bind_single_parameter_time_invalid_format(void);
void test_db2_bind_single_parameter_datetime_type(void);
void test_db2_bind_single_parameter_datetime_malloc_failure(void);
void test_db2_bind_single_parameter_datetime_invalid_format(void);
void test_db2_bind_single_parameter_timestamp_type(void);
void test_db2_bind_single_parameter_timestamp_malloc_failure(void);
void test_db2_bind_single_parameter_timestamp_invalid_format(void);
void test_db2_bind_single_parameter_bind_failure(void);

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
// Tests for db2_execute_query error paths
// ============================================================================

void test_db2_execute_query_convert_named_to_positional_failure(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    connection.designator = (char*)"test_db";

    DB2Connection* db2_conn = calloc(1, sizeof(DB2Connection));
    TEST_ASSERT_NOT_NULL(db2_conn);
    db2_conn->connection = (void*)0x12345678;
    connection.connection_handle = db2_conn;

    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT * FROM test WHERE id = :param";
    request.parameters_json = (char*)"{\"INTEGER\": {\"param\": 123}}";
    QueryResult* result = NULL;

    // Set up DB2 mocks
    mock_libdb2_set_SQLAllocHandle_result(SQL_SUCCESS);

    // Mock convert_named_to_positional to fail (simulate memory allocation failure)
    mock_system_set_malloc_failure(1);

    bool query_result = db2_execute_query(&connection, &request, &result);
    TEST_ASSERT_FALSE(query_result); // Should return false (lines 595-598)
    TEST_ASSERT_NULL(result);

    // Cleanup
    free(db2_conn);
}

void test_db2_execute_query_allocate_binding_arrays_failure(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    connection.designator = (char*)"test_db";

    DB2Connection* db2_conn = calloc(1, sizeof(DB2Connection));
    TEST_ASSERT_NOT_NULL(db2_conn);
    db2_conn->connection = (void*)0x12345678;
    connection.connection_handle = db2_conn;

    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT * FROM test WHERE id = ?";
    request.parameters_json = (char*)"{\"INTEGER\": {\"param\": 123}}";
    QueryResult* result = NULL;

    // Set up DB2 mocks
    mock_libdb2_set_SQLAllocHandle_result(SQL_SUCCESS);
    mock_libdb2_set_SQLPrepare_result(SQL_SUCCESS);

    // Mock calloc to fail for binding arrays
    mock_system_set_malloc_failure(1);

    bool query_result = db2_execute_query(&connection, &request, &result);
    TEST_ASSERT_FALSE(query_result); // Should return false (lines 619-626)
    TEST_ASSERT_NULL(result);

    // Cleanup
    free(db2_conn);
}

void test_db2_execute_query_bind_parameter_failure(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    connection.designator = (char*)"test_db";

    DB2Connection* db2_conn = calloc(1, sizeof(DB2Connection));
    TEST_ASSERT_NOT_NULL(db2_conn);
    db2_conn->connection = (void*)0x12345678;
    connection.connection_handle = db2_conn;

    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT * FROM test WHERE id = :param";
    request.parameters_json = (char*)"{\"INTEGER\": {\"param\": 123}}";
    QueryResult* result = NULL;

    // Set up DB2 mocks
    mock_libdb2_set_SQLAllocHandle_result(SQL_SUCCESS);
    mock_libdb2_set_SQLPrepare_result(SQL_SUCCESS);
    mock_libdb2_set_SQLBindParameter_result(-1); // Bind failure

    // Mock SQLGetData to return data
    mock_libdb2_set_SQLGetData_result(0);
    mock_libdb2_set_SQLGetData_data("test", 4);

    bool query_result = db2_execute_query(&connection, &request, &result);
    TEST_ASSERT_FALSE(query_result); // Should return false (lines 633-640)
    TEST_ASSERT_NULL(result);

    // Cleanup
    free(db2_conn);
}

void test_db2_execute_query_sqlprepare_failure(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    connection.designator = (char*)"test_db";

    DB2Connection* db2_conn = calloc(1, sizeof(DB2Connection));
    TEST_ASSERT_NOT_NULL(db2_conn);
    db2_conn->connection = (void*)0x12345678;
    connection.connection_handle = db2_conn;

    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT * FROM test WHERE id = :param";
    request.parameters_json = (char*)"{\"INTEGER\": {\"param\": 123}}";
    QueryResult* result = NULL;

    // Set up DB2 mocks
    mock_libdb2_set_SQLAllocHandle_result(SQL_SUCCESS);
    mock_libdb2_set_SQLPrepare_result(-1); // Prepare failure

    // Mock SQLGetData to return data
    mock_libdb2_set_SQLGetData_result(0);
    mock_libdb2_set_SQLGetData_data("test", 4);

    bool query_result = db2_execute_query(&connection, &request, &result);
    TEST_ASSERT_FALSE(query_result); // Should return false (lines 607-613)
    TEST_ASSERT_NULL(result);

    // Cleanup
    free(db2_conn);
}

void test_db2_execute_query_sqlexecute_failure(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    connection.designator = (char*)"test_db";

    DB2Connection* db2_conn = calloc(1, sizeof(DB2Connection));
    TEST_ASSERT_NOT_NULL(db2_conn);
    db2_conn->connection = (void*)0x12345678;
    connection.connection_handle = db2_conn;

    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT * FROM test WHERE id = :param";
    request.parameters_json = (char*)"{\"INTEGER\": {\"param\": 123}}";
    QueryResult* result = NULL;

    // Set up DB2 mocks
    mock_libdb2_set_SQLAllocHandle_result(SQL_SUCCESS);
    mock_libdb2_set_SQLPrepare_result(SQL_SUCCESS);
    mock_libdb2_set_SQLBindParameter_result(SQL_SUCCESS);
    mock_libdb2_set_SQLExecute_result(-1); // Execute failure

    // Mock SQLGetData to return data
    mock_libdb2_set_SQLGetData_result(0);
    mock_libdb2_set_SQLGetData_data("test", 4);

    bool query_result = db2_execute_query(&connection, &request, &result);
    TEST_ASSERT_FALSE(query_result); // Should return false (lines 664-691)
    TEST_ASSERT_NOT_NULL(result); // Should create error result
    TEST_ASSERT_FALSE(result->success); // Should be error result
    TEST_ASSERT_NOT_NULL(result->error_message); // Should have error message

    // Cleanup
    free(result->error_message);
    free(result->data_json);
    free(result);
    free(db2_conn);
}

// ============================================================================
// Tests for db2_execute_prepared error paths
// ============================================================================

void test_db2_execute_prepared_sqlexecute_failure(void) {
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

    // Mock SQLExecute to fail
    mock_libdb2_set_SQLExecute_result(-1);

    bool query_result = db2_execute_prepared(&connection, &stmt, &request, &result);
    TEST_ASSERT_FALSE(query_result); // Should return false (line 730)
    TEST_ASSERT_NULL(result);

    // Cleanup
    free(db2_conn);
}

// ============================================================================
// Tests for db2_bind_single_parameter
// ============================================================================

void test_db2_bind_single_parameter_null_stmt_handle(void) {
    TypedParameter param = {0};
    param.type = PARAM_TYPE_INTEGER;
    param.name = (char*)"test";
    param.value.int_value = 42;

    void** bound_values = calloc(1, sizeof(void*));
    TEST_ASSERT_NOT_NULL(bound_values);
    long* str_len_indicators = calloc(1, sizeof(long));
    TEST_ASSERT_NOT_NULL(str_len_indicators);

    bool result = db2_bind_single_parameter(NULL, 1, &param, bound_values, str_len_indicators, "test");
    TEST_ASSERT_FALSE(result); // Should return false (line 318)

    free(bound_values);
    free(str_len_indicators);
}

void test_db2_bind_single_parameter_null_param(void) {
    void* stmt = (void*)0x1234;
    void** bound_values = calloc(1, sizeof(void*));
    TEST_ASSERT_NOT_NULL(bound_values);
    long* str_len_indicators = calloc(1, sizeof(long));
    TEST_ASSERT_NOT_NULL(str_len_indicators);

    bool result = db2_bind_single_parameter(stmt, 1, NULL, bound_values, str_len_indicators, "test");
    TEST_ASSERT_FALSE(result); // Should return false (line 318)

    free(bound_values);
    free(str_len_indicators);
}

void test_db2_bind_single_parameter_null_bound_values(void) {
    void* stmt = (void*)0x1234;
    TypedParameter param = {0};
    param.type = PARAM_TYPE_INTEGER;
    param.name = (char*)"test";
    param.value.int_value = 42;

    long* str_len_indicators = calloc(1, sizeof(long));
    TEST_ASSERT_NOT_NULL(str_len_indicators);

    bool result = db2_bind_single_parameter(stmt, 1, &param, NULL, str_len_indicators, "test");
    TEST_ASSERT_FALSE(result); // Should return false (line 318)

    free(str_len_indicators);
}

void test_db2_bind_single_parameter_null_str_len_indicators(void) {
    void* stmt = (void*)0x1234;
    TypedParameter param = {0};
    param.type = PARAM_TYPE_INTEGER;
    param.name = (char*)"test";
    param.value.int_value = 42;

    void** bound_values = calloc(1, sizeof(void*));
    TEST_ASSERT_NOT_NULL(bound_values);

    bool result = db2_bind_single_parameter(stmt, 1, &param, bound_values, NULL, "test");
    TEST_ASSERT_FALSE(result); // Should return false (line 318)

    free(bound_values);
}

void test_db2_bind_single_parameter_null_designator(void) {
    void* stmt = (void*)0x1234;
    TypedParameter param = {0};
    param.type = PARAM_TYPE_INTEGER;
    param.name = (char*)"test";
    param.value.int_value = 42;

    void** bound_values = calloc(1, sizeof(void*));
    TEST_ASSERT_NOT_NULL(bound_values);
    long* str_len_indicators = calloc(1, sizeof(long));
    TEST_ASSERT_NOT_NULL(str_len_indicators);

    bool result = db2_bind_single_parameter(stmt, 1, &param, bound_values, str_len_indicators, NULL);
    TEST_ASSERT_FALSE(result); // Should return false (line 318)

    free(bound_values);
    free(str_len_indicators);
}

void test_db2_bind_single_parameter_no_sqlbindparameter(void) {
    void* stmt = (void*)0x1234;
    TypedParameter param = {0};
    param.type = PARAM_TYPE_INTEGER;
    param.name = (char*)"test";
    param.value.int_value = 42;

    void** bound_values = calloc(1, sizeof(void*));
    TEST_ASSERT_NOT_NULL(bound_values);
    long* str_len_indicators = calloc(1, sizeof(long));
    TEST_ASSERT_NOT_NULL(str_len_indicators);

    // Mock SQLBindParameter as unavailable
    SQLBindParameter_ptr = NULL;

    bool result = db2_bind_single_parameter(stmt, 1, &param, bound_values, str_len_indicators, "test");
    TEST_ASSERT_FALSE(result); // Should return false (line 318)

    free(bound_values);
    free(str_len_indicators);
}

void test_db2_bind_single_parameter_boolean_type(void) {
    void* stmt = (void*)0x1234;
    TypedParameter param = {0};
    param.type = PARAM_TYPE_BOOLEAN;
    param.name = (char*)"test";
    param.value.bool_value = true;

    void** bound_values = calloc(1, sizeof(void*));
    TEST_ASSERT_NOT_NULL(bound_values);
    long* str_len_indicators = calloc(1, sizeof(long));
    TEST_ASSERT_NOT_NULL(str_len_indicators);

    // Mock successful binding
    mock_libdb2_set_SQLBindParameter_result(SQL_SUCCESS);

    bool result = db2_bind_single_parameter(stmt, 1, &param, bound_values, str_len_indicators, "test");
    TEST_ASSERT_TRUE(result); // Should succeed

    // Verify bound value
    TEST_ASSERT_NOT_NULL(bound_values[0]);
    TEST_ASSERT_EQUAL(1, *(short*)bound_values[0]);
    TEST_ASSERT_EQUAL(0, str_len_indicators[0]);

    free(bound_values[0]);
    free(bound_values);
    free(str_len_indicators);
}

void test_db2_bind_single_parameter_boolean_malloc_failure(void) {
    void* stmt = (void*)0x1234;
    TypedParameter param = {0};
    param.type = PARAM_TYPE_BOOLEAN;
    param.name = (char*)"test";
    param.value.bool_value = true;

    void** bound_values = calloc(1, sizeof(void*));
    TEST_ASSERT_NOT_NULL(bound_values);
    long* str_len_indicators = calloc(1, sizeof(long));
    TEST_ASSERT_NOT_NULL(str_len_indicators);

    // Mock malloc failure
    mock_system_set_malloc_failure(1);

    bool result = db2_bind_single_parameter(stmt, 1, &param, bound_values, str_len_indicators, "test");
    TEST_ASSERT_FALSE(result); // Should return false (line 356)

    free(bound_values);
    free(str_len_indicators);
}

void test_db2_bind_single_parameter_float_type(void) {
    void* stmt = (void*)0x1234;
    TypedParameter param = {0};
    param.type = PARAM_TYPE_FLOAT;
    param.name = (char*)"test";
    param.value.float_value = 3.14;

    void** bound_values = calloc(1, sizeof(void*));
    TEST_ASSERT_NOT_NULL(bound_values);
    long* str_len_indicators = calloc(1, sizeof(long));
    TEST_ASSERT_NOT_NULL(str_len_indicators);

    // Mock successful binding
    mock_libdb2_set_SQLBindParameter_result(SQL_SUCCESS);

    bool result = db2_bind_single_parameter(stmt, 1, &param, bound_values, str_len_indicators, "test");
    TEST_ASSERT_TRUE(result); // Should succeed

    // Verify bound value
    TEST_ASSERT_NOT_NULL(bound_values[0]);
    TEST_ASSERT_EQUAL_DOUBLE(3.14, *(double*)bound_values[0]);
    TEST_ASSERT_EQUAL(0, str_len_indicators[0]);

    free(bound_values[0]);
    free(bound_values);
    free(str_len_indicators);
}

void test_db2_bind_single_parameter_float_malloc_failure(void) {
    void* stmt = (void*)0x1234;
    TypedParameter param = {0};
    param.type = PARAM_TYPE_FLOAT;
    param.name = (char*)"test";
    param.value.float_value = 3.14;

    void** bound_values = calloc(1, sizeof(void*));
    TEST_ASSERT_NOT_NULL(bound_values);
    long* str_len_indicators = calloc(1, sizeof(long));
    TEST_ASSERT_NOT_NULL(str_len_indicators);

    // Mock malloc failure
    mock_system_set_malloc_failure(1);

    bool result = db2_bind_single_parameter(stmt, 1, &param, bound_values, str_len_indicators, "test");
    TEST_ASSERT_FALSE(result); // Should return false (line 369)

    free(bound_values);
    free(str_len_indicators);
}

void test_db2_bind_single_parameter_text_type(void) {
    void* stmt = (void*)0x1234;
    TypedParameter param = {0};
    param.type = PARAM_TYPE_TEXT;
    param.name = (char*)"test";
    param.value.text_value = (char*)"hello world";

    void** bound_values = calloc(1, sizeof(void*));
    TEST_ASSERT_NOT_NULL(bound_values);
    long* str_len_indicators = calloc(1, sizeof(long));
    TEST_ASSERT_NOT_NULL(str_len_indicators);

    // Mock successful binding
    mock_libdb2_set_SQLBindParameter_result(SQL_SUCCESS);

    bool result = db2_bind_single_parameter(stmt, 1, &param, bound_values, str_len_indicators, "test");
    TEST_ASSERT_TRUE(result); // Should succeed

    // Verify bound value
    TEST_ASSERT_NOT_NULL(bound_values[0]);
    TEST_ASSERT_EQUAL_STRING("hello world", (char*)bound_values[0]);
    TEST_ASSERT_EQUAL(11, str_len_indicators[0]);

    free(bound_values[0]);
    free(bound_values);
    free(str_len_indicators);
}

void test_db2_bind_single_parameter_text_malloc_failure(void) {
    void* stmt = (void*)0x1234;
    TypedParameter param = {0};
    param.type = PARAM_TYPE_TEXT;
    param.name = (char*)"test";
    param.value.text_value = (char*)"hello world";

    void** bound_values = calloc(1, sizeof(void*));
    TEST_ASSERT_NOT_NULL(bound_values);
    long* str_len_indicators = calloc(1, sizeof(long));
    TEST_ASSERT_NOT_NULL(str_len_indicators);

    // Mock malloc failure
    mock_system_set_malloc_failure(1);

    bool result = db2_bind_single_parameter(stmt, 1, &param, bound_values, str_len_indicators, "test");
    TEST_ASSERT_FALSE(result); // Should return false (line 382)

    free(bound_values);
    free(str_len_indicators);
}

void test_db2_bind_single_parameter_date_type(void) {
    void* stmt = (void*)0x1234;
    TypedParameter param = {0};
    param.type = PARAM_TYPE_DATE;
    param.name = (char*)"test";
    param.value.date_value = (char*)"2023-12-25";

    void** bound_values = calloc(1, sizeof(void*));
    TEST_ASSERT_NOT_NULL(bound_values);
    long* str_len_indicators = calloc(1, sizeof(long));
    TEST_ASSERT_NOT_NULL(str_len_indicators);

    // Mock successful binding
    mock_libdb2_set_SQLBindParameter_result(SQL_SUCCESS);

    bool result = db2_bind_single_parameter(stmt, 1, &param, bound_values, str_len_indicators, "test");
    TEST_ASSERT_TRUE(result); // Should succeed

    // Verify bound value
    TEST_ASSERT_NOT_NULL(bound_values[0]);
    SQL_DATE_STRUCT* date = (SQL_DATE_STRUCT*)bound_values[0];
    TEST_ASSERT_EQUAL(2023, date->year);
    TEST_ASSERT_EQUAL(12, date->month);
    TEST_ASSERT_EQUAL(25, date->day);
    TEST_ASSERT_EQUAL(0, str_len_indicators[0]);

    free(bound_values[0]);
    free(bound_values);
    free(str_len_indicators);
}

void test_db2_bind_single_parameter_date_malloc_failure(void) {
    void* stmt = (void*)0x1234;
    TypedParameter param = {0};
    param.type = PARAM_TYPE_DATE;
    param.name = (char*)"test";
    param.value.date_value = (char*)"2023-12-25";

    void** bound_values = calloc(1, sizeof(void*));
    TEST_ASSERT_NOT_NULL(bound_values);
    long* str_len_indicators = calloc(1, sizeof(long));
    TEST_ASSERT_NOT_NULL(str_len_indicators);

    // Mock malloc failure
    mock_system_set_malloc_failure(1);

    bool result = db2_bind_single_parameter(stmt, 1, &param, bound_values, str_len_indicators, "test");
    TEST_ASSERT_FALSE(result); // Should return false (line 396)

    free(bound_values);
    free(str_len_indicators);
}

void test_db2_bind_single_parameter_date_invalid_format(void) {
    void* stmt = (void*)0x1234;
    TypedParameter param = {0};
    param.type = PARAM_TYPE_DATE;
    param.name = (char*)"test";
    param.value.date_value = (char*)"invalid-date";

    void** bound_values = calloc(1, sizeof(void*));
    TEST_ASSERT_NOT_NULL(bound_values);
    long* str_len_indicators = calloc(1, sizeof(long));
    TEST_ASSERT_NOT_NULL(str_len_indicators);

    bool result = db2_bind_single_parameter(stmt, 1, &param, bound_values, str_len_indicators, "test");
    TEST_ASSERT_FALSE(result); // Should return false (line 402)

    free(bound_values);
    free(str_len_indicators);
}

void test_db2_bind_single_parameter_time_type(void) {
    void* stmt = (void*)0x1234;
    TypedParameter param = {0};
    param.type = PARAM_TYPE_TIME;
    param.name = (char*)"test";
    param.value.time_value = (char*)"14:30:45";

    void** bound_values = calloc(1, sizeof(void*));
    TEST_ASSERT_NOT_NULL(bound_values);
    long* str_len_indicators = calloc(1, sizeof(long));
    TEST_ASSERT_NOT_NULL(str_len_indicators);

    // Mock successful binding
    mock_libdb2_set_SQLBindParameter_result(SQL_SUCCESS);

    bool result = db2_bind_single_parameter(stmt, 1, &param, bound_values, str_len_indicators, "test");
    TEST_ASSERT_TRUE(result); // Should succeed

    // Verify bound value
    TEST_ASSERT_NOT_NULL(bound_values[0]);
    SQL_TIME_STRUCT* time = (SQL_TIME_STRUCT*)bound_values[0];
    TEST_ASSERT_EQUAL(14, time->hour);
    TEST_ASSERT_EQUAL(30, time->minute);
    TEST_ASSERT_EQUAL(45, time->second);
    TEST_ASSERT_EQUAL(0, str_len_indicators[0]);

    free(bound_values[0]);
    free(bound_values);
    free(str_len_indicators);
}

void test_db2_bind_single_parameter_time_malloc_failure(void) {
    void* stmt = (void*)0x1234;
    TypedParameter param = {0};
    param.type = PARAM_TYPE_TIME;
    param.name = (char*)"test";
    param.value.time_value = (char*)"14:30:45";

    void** bound_values = calloc(1, sizeof(void*));
    TEST_ASSERT_NOT_NULL(bound_values);
    long* str_len_indicators = calloc(1, sizeof(long));
    TEST_ASSERT_NOT_NULL(str_len_indicators);

    // Mock malloc failure
    mock_system_set_malloc_failure(1);

    bool result = db2_bind_single_parameter(stmt, 1, &param, bound_values, str_len_indicators, "test");
    TEST_ASSERT_FALSE(result); // Should return false (line 425)

    free(bound_values);
    free(str_len_indicators);
}

void test_db2_bind_single_parameter_time_invalid_format(void) {
    void* stmt = (void*)0x1234;
    TypedParameter param = {0};
    param.type = PARAM_TYPE_TIME;
    param.name = (char*)"test";
    param.value.time_value = (char*)"invalid-time";

    void** bound_values = calloc(1, sizeof(void*));
    TEST_ASSERT_NOT_NULL(bound_values);
    long* str_len_indicators = calloc(1, sizeof(long));
    TEST_ASSERT_NOT_NULL(str_len_indicators);

    bool result = db2_bind_single_parameter(stmt, 1, &param, bound_values, str_len_indicators, "test");
    TEST_ASSERT_FALSE(result); // Should return false (line 431)

    free(bound_values);
    free(str_len_indicators);
}

void test_db2_bind_single_parameter_datetime_type(void) {
    void* stmt = (void*)0x1234;
    TypedParameter param = {0};
    param.type = PARAM_TYPE_DATETIME;
    param.name = (char*)"test";
    param.value.datetime_value = (char*)"2023-12-25 14:30:45";

    void** bound_values = calloc(1, sizeof(void*));
    TEST_ASSERT_NOT_NULL(bound_values);
    long* str_len_indicators = calloc(1, sizeof(long));
    TEST_ASSERT_NOT_NULL(str_len_indicators);

    // Mock successful binding
    mock_libdb2_set_SQLBindParameter_result(SQL_SUCCESS);

    bool result = db2_bind_single_parameter(stmt, 1, &param, bound_values, str_len_indicators, "test");
    TEST_ASSERT_TRUE(result); // Should succeed

    // Verify bound value (now bound as string)
    TEST_ASSERT_NOT_NULL(bound_values[0]);
    TEST_ASSERT_EQUAL_STRING("2023-12-25 14:30:45", (char*)bound_values[0]);
    TEST_ASSERT_EQUAL(19, str_len_indicators[0]); // Length of datetime string

    free(bound_values[0]);
    free(bound_values);
    free(str_len_indicators);
}

void test_db2_bind_single_parameter_datetime_malloc_failure(void) {
    void* stmt = (void*)0x1234;
    TypedParameter param = {0};
    param.type = PARAM_TYPE_DATETIME;
    param.name = (char*)"test";
    param.value.datetime_value = (char*)"2023-12-25 14:30:45";

    void** bound_values = calloc(1, sizeof(void*));
    TEST_ASSERT_NOT_NULL(bound_values);
    long* str_len_indicators = calloc(1, sizeof(long));
    TEST_ASSERT_NOT_NULL(str_len_indicators);

    // Mock malloc failure
    mock_system_set_malloc_failure(1);

    bool result = db2_bind_single_parameter(stmt, 1, &param, bound_values, str_len_indicators, "test");
    TEST_ASSERT_FALSE(result); // Should return false (line 454)

    free(bound_values);
    free(str_len_indicators);
}

void test_db2_bind_single_parameter_datetime_invalid_format(void) {
    void* stmt = (void*)0x1234;
    TypedParameter param = {0};
    param.type = PARAM_TYPE_DATETIME;
    param.name = (char*)"test";
    param.value.datetime_value = (char*)"invalid-datetime";

    void** bound_values = calloc(1, sizeof(void*));
    TEST_ASSERT_NOT_NULL(bound_values);
    long* str_len_indicators = calloc(1, sizeof(long));
    TEST_ASSERT_NOT_NULL(str_len_indicators);

    bool result = db2_bind_single_parameter(stmt, 1, &param, bound_values, str_len_indicators, "test");
    TEST_ASSERT_FALSE(result); // Should return false (line 460)

    free(bound_values);
    free(str_len_indicators);
}

void test_db2_bind_single_parameter_timestamp_type(void) {
    void* stmt = (void*)0x1234;
    TypedParameter param = {0};
    param.type = PARAM_TYPE_TIMESTAMP;
    param.name = (char*)"test";
    param.value.timestamp_value = (char*)"2023-12-25 14:30:45.123";

    void** bound_values = calloc(1, sizeof(void*));
    TEST_ASSERT_NOT_NULL(bound_values);
    long* str_len_indicators = calloc(1, sizeof(long));
    TEST_ASSERT_NOT_NULL(str_len_indicators);

    // Mock successful binding
    mock_libdb2_set_SQLBindParameter_result(SQL_SUCCESS);

    bool result = db2_bind_single_parameter(stmt, 1, &param, bound_values, str_len_indicators, "test");
    TEST_ASSERT_TRUE(result); // Should succeed

    // Verify bound value (now bound as string)
    TEST_ASSERT_NOT_NULL(bound_values[0]);
    TEST_ASSERT_EQUAL_STRING("2023-12-25 14:30:45.123", (char*)bound_values[0]);
    TEST_ASSERT_EQUAL(23, str_len_indicators[0]); // Length of timestamp string

    free(bound_values[0]);
    free(bound_values);
    free(str_len_indicators);
}

void test_db2_bind_single_parameter_timestamp_malloc_failure(void) {
    void* stmt = (void*)0x1234;
    TypedParameter param = {0};
    param.type = PARAM_TYPE_TIMESTAMP;
    param.name = (char*)"test";
    param.value.timestamp_value = (char*)"2023-12-25 14:30:45.123";

    void** bound_values = calloc(1, sizeof(void*));
    TEST_ASSERT_NOT_NULL(bound_values);
    long* str_len_indicators = calloc(1, sizeof(long));
    TEST_ASSERT_NOT_NULL(str_len_indicators);

    // Mock malloc failure
    mock_system_set_malloc_failure(1);

    bool result = db2_bind_single_parameter(stmt, 1, &param, bound_values, str_len_indicators, "test");
    TEST_ASSERT_FALSE(result); // Should return false (line 487)

    free(bound_values);
    free(str_len_indicators);
}

void test_db2_bind_single_parameter_timestamp_invalid_format(void) {
    void* stmt = (void*)0x1234;
    TypedParameter param = {0};
    param.type = PARAM_TYPE_TIMESTAMP;
    param.name = (char*)"test";
    param.value.timestamp_value = (char*)"invalid-timestamp";

    void** bound_values = calloc(1, sizeof(void*));
    TEST_ASSERT_NOT_NULL(bound_values);
    long* str_len_indicators = calloc(1, sizeof(long));
    TEST_ASSERT_NOT_NULL(str_len_indicators);

    bool result = db2_bind_single_parameter(stmt, 1, &param, bound_values, str_len_indicators, "test");
    TEST_ASSERT_FALSE(result); // Should return false (line 494)

    free(bound_values);
    free(str_len_indicators);
}

void test_db2_bind_single_parameter_bind_failure(void) {
    void* stmt = (void*)0x1234;
    TypedParameter param = {0};
    param.type = PARAM_TYPE_INTEGER;
    param.name = (char*)"test";
    param.value.int_value = 42;

    void** bound_values = calloc(1, sizeof(void*));
    TEST_ASSERT_NOT_NULL(bound_values);
    long* str_len_indicators = calloc(1, sizeof(long));
    TEST_ASSERT_NOT_NULL(str_len_indicators);

    // Mock SQLBindParameter to fail
    mock_libdb2_set_SQLBindParameter_result(-1);

    bool result = db2_bind_single_parameter(stmt, 1, &param, bound_values, str_len_indicators, "test");
    TEST_ASSERT_FALSE(result); // Should return false (line 522)

    free(bound_values);
    free(str_len_indicators);
}

int main(void) {
    UNITY_BEGIN();

    // db2_execute_query error path tests
    RUN_TEST(test_db2_execute_query_convert_named_to_positional_failure);
    RUN_TEST(test_db2_execute_query_allocate_binding_arrays_failure);
    RUN_TEST(test_db2_execute_query_bind_parameter_failure);
    RUN_TEST(test_db2_execute_query_sqlprepare_failure);
    RUN_TEST(test_db2_execute_query_sqlexecute_failure);

    // db2_execute_prepared error path tests
    RUN_TEST(test_db2_execute_prepared_sqlexecute_failure);

    // db2_bind_single_parameter tests
    RUN_TEST(test_db2_bind_single_parameter_null_stmt_handle);
    RUN_TEST(test_db2_bind_single_parameter_null_param);
    RUN_TEST(test_db2_bind_single_parameter_null_bound_values);
    RUN_TEST(test_db2_bind_single_parameter_null_str_len_indicators);
    RUN_TEST(test_db2_bind_single_parameter_null_designator);
    RUN_TEST(test_db2_bind_single_parameter_no_sqlbindparameter);
    RUN_TEST(test_db2_bind_single_parameter_boolean_type);
    RUN_TEST(test_db2_bind_single_parameter_boolean_malloc_failure);
    RUN_TEST(test_db2_bind_single_parameter_float_type);
    RUN_TEST(test_db2_bind_single_parameter_float_malloc_failure);
    RUN_TEST(test_db2_bind_single_parameter_text_type);
    RUN_TEST(test_db2_bind_single_parameter_text_malloc_failure);
    RUN_TEST(test_db2_bind_single_parameter_date_type);
    RUN_TEST(test_db2_bind_single_parameter_date_malloc_failure);
    RUN_TEST(test_db2_bind_single_parameter_date_invalid_format);
    RUN_TEST(test_db2_bind_single_parameter_time_type);
    RUN_TEST(test_db2_bind_single_parameter_time_malloc_failure);
    RUN_TEST(test_db2_bind_single_parameter_time_invalid_format);
    RUN_TEST(test_db2_bind_single_parameter_datetime_type);
    RUN_TEST(test_db2_bind_single_parameter_datetime_malloc_failure);
    RUN_TEST(test_db2_bind_single_parameter_datetime_invalid_format);
    RUN_TEST(test_db2_bind_single_parameter_timestamp_type);
    RUN_TEST(test_db2_bind_single_parameter_timestamp_malloc_failure);
    RUN_TEST(test_db2_bind_single_parameter_timestamp_invalid_format);
    RUN_TEST(test_db2_bind_single_parameter_bind_failure);

    return UNITY_END();
}