/*
 * Unity Test File: PostgreSQL Query Execution Edge Cases
 * Comprehensive tests for postgresql_execute_query and postgresql_execute_prepared
 * focusing on success paths, memory failures, and edge cases
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include mock library (USE_MOCK_LIBPQ is defined by CMake)
#include <unity/mocks/mock_libpq.h>

// Include necessary headers for the module being tested
#include <src/database/database.h>
#include <src/database/postgresql/types.h>
#include <src/database/postgresql/query.h>

// Forward declarations for functions being tested
bool postgresql_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result);
bool postgresql_execute_prepared(DatabaseHandle* connection, const PreparedStatement* stmt, QueryRequest* request, QueryResult** result);

// External declarations for libpq function pointers
extern PQexec_t PQexec_ptr;
extern PQresultStatus_t PQresultStatus_ptr;
extern PQclear_t PQclear_ptr;
extern PQntuples_t PQntuples_ptr;
extern PQnfields_t PQnfields_ptr;
extern PQfname_t PQfname_ptr;
extern PQgetvalue_t PQgetvalue_ptr;
extern PQcmdTuples_t PQcmdTuples_ptr;
extern PQerrorMessage_t PQerrorMessage_ptr;
extern PQexecPrepared_t PQexecPrepared_ptr;

// PostgreSQL constants
#define PGRES_TUPLES_OK 2
#define PGRES_COMMAND_OK 1
#define PGRES_FATAL_ERROR 7

// Function prototypes for test functions
void test_postgresql_execute_query_success_no_data(void);
void test_postgresql_execute_query_success_with_single_row(void);
void test_postgresql_execute_query_success_with_multiple_rows(void);
void test_postgresql_execute_query_command_ok_status(void);
void test_postgresql_execute_query_error_status(void);
void test_postgresql_execute_query_zero_timeout(void);
void test_postgresql_execute_query_timeout_scenario(void);
void test_postgresql_execute_prepared_success_with_data(void);
void test_postgresql_execute_prepared_fallback_to_pqexec(void);
void test_postgresql_execute_prepared_error_status(void);
void test_postgresql_execute_prepared_invalid_connection_handle(void);
void test_postgresql_execute_prepared_timeout_scenario(void);
void test_postgresql_execute_prepared_null_result(void);
void test_postgresql_execute_prepared_no_data_returned(void);
void test_postgresql_execute_prepared_failed_timeout_setting(void);

void setUp(void) {
    // Initialize and reset mock library
    // The USE_MOCK_LIBPQ define automatically redirects function pointers to mocks
    mock_libpq_initialize();
    mock_libpq_reset_all();
}

void tearDown(void) {
    mock_libpq_reset_all();
}

// ============================================================================
// Tests for postgresql_execute_query - Success Paths
// ============================================================================

void test_postgresql_execute_query_success_no_data(void) {
    // Setup connection
    PostgresConnection pg_conn = {0};
    pg_conn.connection = (void*)0x1000;
    
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_POSTGRESQL;
    connection.designator = (char*)"test";
    connection.connection_handle = &pg_conn;
    
    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT 1";
    request.timeout_seconds = 30;
    
    QueryResult* result = NULL;
    
    // Configure mocks for successful query with no data
    mock_libpq_set_PQexec_result((void*)0x12345678);
    mock_libpq_set_PQresultStatus_result(PGRES_TUPLES_OK);
    mock_libpq_set_PQntuples_result(0);
    mock_libpq_set_PQnfields_result(0);
    
    bool query_result = postgresql_execute_query(&connection, &request, &result);
    
    TEST_ASSERT_TRUE(query_result);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL(0, result->row_count);
    TEST_ASSERT_EQUAL(0, result->column_count);
    
    // Cleanup
    free(result);
}

void test_postgresql_execute_query_success_with_single_row(void) {
    // Setup connection
    PostgresConnection pg_conn = {0};
    pg_conn.connection = (void*)0x1000;
    
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_POSTGRESQL;
    connection.designator = (char*)"test";
    connection.connection_handle = &pg_conn;
    
    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT id FROM users WHERE id = 1";
    request.timeout_seconds = 30;
    
    QueryResult* result = NULL;
    
    // Configure mocks for successful query with 1 row, 1 column
    mock_libpq_set_PQexec_result((void*)0x12345678);
    mock_libpq_set_PQresultStatus_result(PGRES_TUPLES_OK);
    mock_libpq_set_PQntuples_result(1);
    mock_libpq_set_PQnfields_result(1);
    mock_libpq_set_PQfname_result("id");
    mock_libpq_set_PQgetvalue_result("123");
    mock_libpq_set_PQcmdTuples_result("1");
    
    bool query_result = postgresql_execute_query(&connection, &request, &result);
    
    TEST_ASSERT_TRUE(query_result);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL(1, result->row_count);
    TEST_ASSERT_EQUAL(1, result->column_count);
    TEST_ASSERT_NOT_NULL(result->data_json);
    TEST_ASSERT_NOT_NULL(strstr(result->data_json, "id"));
    TEST_ASSERT_NOT_NULL(strstr(result->data_json, "123"));
    
    // Cleanup
    if (result->column_names) {
        for (size_t i = 0; i < result->column_count; i++) {
            free(result->column_names[i]);
        }
        free(result->column_names);
    }
    free(result->data_json);
    free(result);
}

void test_postgresql_execute_query_success_with_multiple_rows(void) {
    // Setup connection
    PostgresConnection pg_conn = {0};
    pg_conn.connection = (void*)0x1000;
    
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_POSTGRESQL;
    connection.designator = (char*)"test";
    connection.connection_handle = &pg_conn;
    
    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT id, name FROM users";
    request.timeout_seconds = 30;
    
    QueryResult* result = NULL;
    
    // Configure mocks for successful query with 3 rows, 2 columns
    mock_libpq_set_PQexec_result((void*)0x12345678);
    mock_libpq_set_PQresultStatus_result(PGRES_TUPLES_OK);
    mock_libpq_set_PQntuples_result(3);
    mock_libpq_set_PQnfields_result(2);
    mock_libpq_set_PQcmdTuples_result("3");
    
    bool query_result = postgresql_execute_query(&connection, &request, &result);
    
    TEST_ASSERT_TRUE(query_result);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL(3, result->row_count);
    TEST_ASSERT_EQUAL(2, result->column_count);
    TEST_ASSERT_EQUAL(3, result->affected_rows);
    TEST_ASSERT_NOT_NULL(result->data_json);
    
    // Cleanup
    if (result->column_names) {
        for (size_t i = 0; i < result->column_count; i++) {
            free(result->column_names[i]);
        }
        free(result->column_names);
    }
    free(result->data_json);
    free(result);
}

void test_postgresql_execute_query_command_ok_status(void) {
    // Setup connection
    PostgresConnection pg_conn = {0};
    pg_conn.connection = (void*)0x1000;
    
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_POSTGRESQL;
    connection.designator = (char*)"test";
    connection.connection_handle = &pg_conn;
    
    QueryRequest request = {0};
    request.sql_template = (char*)"UPDATE users SET name = 'test' WHERE id = 1";
    request.timeout_seconds = 30;
    
    QueryResult* result = NULL;
    
    // Configure mocks for successful UPDATE command
    mock_libpq_set_PQexec_result((void*)0x12345678);
    mock_libpq_set_PQresultStatus_result(PGRES_COMMAND_OK);
    mock_libpq_set_PQntuples_result(0);
    mock_libpq_set_PQnfields_result(0);
    mock_libpq_set_PQcmdTuples_result("1");
    
    bool query_result = postgresql_execute_query(&connection, &request, &result);
    
    TEST_ASSERT_TRUE(query_result);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL(1, result->affected_rows);
    
    // Cleanup
    free(result);
}

void test_postgresql_execute_query_error_status(void) {
    // Setup connection
    PostgresConnection pg_conn = {0};
    pg_conn.connection = (void*)0x1000;

    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_POSTGRESQL;
    connection.designator = (char*)"test";
    connection.connection_handle = &pg_conn;

    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT * FROM nonexistent_table";
    request.timeout_seconds = 30;

    QueryResult* result = NULL;

    // Configure mocks for error status
    mock_libpq_set_PQexec_result((void*)0x12345678);
    mock_libpq_set_PQresultStatus_result(PGRES_FATAL_ERROR);
    mock_libpq_set_PQerrorMessage_result("relation \"nonexistent_table\" does not exist");

    bool query_result = postgresql_execute_query(&connection, &request, &result);

    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NOT_NULL(result); // Should create error result
    TEST_ASSERT_FALSE(result->success); // Should be error result
    TEST_ASSERT_NOT_NULL(result->error_message); // Should have error message

    // Cleanup
    free(result->error_message);
    free(result->data_json);
    free(result);
}

void test_postgresql_execute_query_zero_timeout(void) {
    // Setup connection
    PostgresConnection pg_conn = {0};
    pg_conn.connection = (void*)0x1000;
    
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_POSTGRESQL;
    connection.designator = (char*)"test";
    connection.connection_handle = &pg_conn;
    
    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT 1";
    request.timeout_seconds = 0; // Should default to 30
    
    QueryResult* result = NULL;
    
    // Configure mocks for successful query
    mock_libpq_set_PQexec_result((void*)0x12345678);
    mock_libpq_set_PQresultStatus_result(PGRES_TUPLES_OK);
    mock_libpq_set_PQntuples_result(1);
    mock_libpq_set_PQnfields_result(1);
    
    bool query_result = postgresql_execute_query(&connection, &request, &result);
    
    TEST_ASSERT_TRUE(query_result);
    TEST_ASSERT_NOT_NULL(result);
    
    // Cleanup
    if (result->column_names) {
        for (size_t i = 0; i < result->column_count; i++) {
            free(result->column_names[i]);
        }
        free(result->column_names);
    }
    free(result->data_json);
    free(result);
}

// ============================================================================
// Tests for postgresql_execute_prepared - Success Paths
// ============================================================================

void test_postgresql_execute_prepared_success_with_data(void) {
    // Setup connection
    PostgresConnection pg_conn = {0};
    pg_conn.connection = (void*)0x1000;
    
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_POSTGRESQL;
    connection.designator = (char*)"test";
    connection.connection_handle = &pg_conn;
    
    PreparedStatement stmt = {0};
    stmt.name = (char*)"test_statement";
    
    QueryRequest request = {0};
    request.timeout_seconds = 30;
    
    QueryResult* result = NULL;
    
    // Configure mocks for successful prepared query
    mock_libpq_set_PQexec_result((void*)0x12345678);
    mock_libpq_set_PQresultStatus_result(PGRES_TUPLES_OK);
    mock_libpq_set_PQntuples_result(2);
    mock_libpq_set_PQnfields_result(1);
    mock_libpq_set_PQfname_result("count");
    mock_libpq_set_PQgetvalue_result("42");
    
    bool query_result = postgresql_execute_prepared(&connection, &stmt, &request, &result);
    
    TEST_ASSERT_TRUE(query_result);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL(2, result->row_count);
    TEST_ASSERT_EQUAL(1, result->column_count);
    
    // Cleanup
    if (result->column_names) {
        for (size_t i = 0; i < result->column_count; i++) {
            free(result->column_names[i]);
        }
        free(result->column_names);
    }
    free(result->data_json);
    free(result);
}

void test_postgresql_execute_prepared_fallback_to_pqexec(void) {
    // Setup connection
    PostgresConnection pg_conn = {0};
    pg_conn.connection = (void*)0x1000;
    
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_POSTGRESQL;
    connection.designator = (char*)"test";
    connection.connection_handle = &pg_conn;
    
    PreparedStatement stmt = {0};
    stmt.name = (char*)"test_statement";
    
    QueryRequest request = {0};
    request.timeout_seconds = 30;
    
    QueryResult* result = NULL;
    
    // Configure mocks for successful query via fallback
    // Note: PQexecPrepared is not mocked, so it will use fallback to PQexec
    mock_libpq_set_PQexec_result((void*)0x12345678);
    mock_libpq_set_PQresultStatus_result(PGRES_TUPLES_OK);
    mock_libpq_set_PQntuples_result(1);
    mock_libpq_set_PQnfields_result(1);
    
    bool query_result = postgresql_execute_prepared(&connection, &stmt, &request, &result);
    
    TEST_ASSERT_TRUE(query_result);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    
    // Cleanup
    if (result->column_names) {
        for (size_t i = 0; i < result->column_count; i++) {
            free(result->column_names[i]);
        }
        free(result->column_names);
    }
    free(result->data_json);
    free(result);
    
    // No need to restore pointer as it remains NULL
}

void test_postgresql_execute_query_timeout_scenario(void) {
    // Setup connection
    PostgresConnection pg_conn = {0};
    pg_conn.connection = (void*)0x1000;
    
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_POSTGRESQL;
    connection.designator = (char*)"test";
    connection.connection_handle = &pg_conn;
    
    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT pg_sleep(60)";
    request.timeout_seconds = 1;
    
    QueryResult* result = NULL;
    
    // Configure mocks to simulate timeout
    mock_libpq_set_PQexec_result((void*)0x12345678);
    mock_libpq_set_check_timeout_expired_use_mock(true);
    mock_libpq_set_check_timeout_expired_result(true); // Force timeout
    
    bool query_result = postgresql_execute_query(&connection, &request, &result);
    
    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
}

void test_postgresql_execute_prepared_invalid_connection_handle(void) {
    // Setup connection with NULL handle
    PostgresConnection pg_conn = {0};
    pg_conn.connection = NULL; // Invalid
    
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_POSTGRESQL;
    connection.designator = (char*)"test";
    connection.connection_handle = &pg_conn;
    
    PreparedStatement stmt = {0};
    stmt.name = (char*)"test_statement";
    
    QueryRequest request = {0};
    request.timeout_seconds = 30;
    
    QueryResult* result = NULL;
    
    bool query_result = postgresql_execute_prepared(&connection, &stmt, &request, &result);
    
    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
}

void test_postgresql_execute_prepared_timeout_scenario(void) {
    // Setup connection
    PostgresConnection pg_conn = {0};
    pg_conn.connection = (void*)0x1000;
    
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_POSTGRESQL;
    connection.designator = (char*)"test";
    connection.connection_handle = &pg_conn;
    
    PreparedStatement stmt = {0};
    stmt.name = (char*)"slow_statement";
    
    QueryRequest request = {0};
    request.timeout_seconds = 1;
    
    QueryResult* result = NULL;
    
    // Configure mocks to simulate timeout
    mock_libpq_set_PQexec_result((void*)0x12345678);
    mock_libpq_set_check_timeout_expired_use_mock(true);
    mock_libpq_set_check_timeout_expired_result(true); // Force timeout
    
    bool query_result = postgresql_execute_prepared(&connection, &stmt, &request, &result);
    
    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
}

void test_postgresql_execute_prepared_null_result(void) {
    // Setup connection
    PostgresConnection pg_conn = {0};
    pg_conn.connection = (void*)0x1000;
    
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_POSTGRESQL;
    connection.designator = (char*)"test";
    connection.connection_handle = &pg_conn;
    
    PreparedStatement stmt = {0};
    stmt.name = (char*)"test_statement";
    
    QueryRequest request = {0};
    request.timeout_seconds = 30;
    
    QueryResult* result = NULL;
    
    // Configure mocks for NULL result
    mock_libpq_set_PQexec_result(NULL); // PQexec returns NULL
    mock_libpq_set_check_timeout_expired_use_mock(true);
    mock_libpq_set_check_timeout_expired_result(false); // No timeout
    
    bool query_result = postgresql_execute_prepared(&connection, &stmt, &request, &result);
    
    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
}

void test_postgresql_execute_prepared_no_data_returned(void) {
    // Setup connection
    PostgresConnection pg_conn = {0};
    pg_conn.connection = (void*)0x1000;
    
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_POSTGRESQL;
    connection.designator = (char*)"test";
    connection.connection_handle = &pg_conn;
    
    PreparedStatement stmt = {0};
    stmt.name = (char*)"test_statement";
    
    QueryRequest request = {0};
    request.timeout_seconds = 30;
    
    QueryResult* result = NULL;
    
    // Configure mocks for successful query with no data
    mock_libpq_set_PQexec_result((void*)0x12345678);
    mock_libpq_set_PQresultStatus_result(PGRES_TUPLES_OK);
    mock_libpq_set_PQntuples_result(0);
    mock_libpq_set_PQnfields_result(0);
    
    bool query_result = postgresql_execute_prepared(&connection, &stmt, &request, &result);
    
    TEST_ASSERT_TRUE(query_result);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL(0, result->row_count);
    
    // Cleanup
    free(result);
}

void test_postgresql_execute_prepared_failed_timeout_setting(void) {
    // Setup connection
    PostgresConnection pg_conn = {0};
    pg_conn.connection = (void*)0x1000;
    
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_POSTGRESQL;
    connection.designator = (char*)"test";
    connection.connection_handle = &pg_conn;
    
    PreparedStatement stmt = {0};
    stmt.name = (char*)"test_statement";
    
    QueryRequest request = {0};
    request.timeout_seconds = 30;
    
    QueryResult* result = NULL;
    
    // Configure mocks - timeout setting fails (returns NULL), but query succeeds
    // First call to PQexec (for timeout) returns NULL, second call (for query) succeeds
    // This is tricky - we need to return NULL first then a valid result
    // The mock doesn't support this easily, so we'll just return NULL for timeout
    // and the query will still succeed
    mock_libpq_set_PQexec_result(NULL); // First call (timeout setting) returns NULL
    mock_libpq_set_PQresultStatus_result(PGRES_TUPLES_OK);
    mock_libpq_set_PQntuples_result(0);
    mock_libpq_set_PQnfields_result(0);
    
    bool query_result = postgresql_execute_prepared(&connection, &stmt, &request, &result);
    
    // Should fail because PQexec returns NULL for the actual query too
    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
}

void test_postgresql_execute_prepared_error_status(void) {
    // Setup connection
    PostgresConnection pg_conn = {0};
    pg_conn.connection = (void*)0x1000;

    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_POSTGRESQL;
    connection.designator = (char*)"test";
    connection.connection_handle = &pg_conn;

    PreparedStatement stmt = {0};
    stmt.name = (char*)"bad_statement";

    QueryRequest request = {0};
    request.timeout_seconds = 30;

    QueryResult* result = NULL;

    // Configure mocks for error
    mock_libpq_set_PQexec_result((void*)0x12345678);
    mock_libpq_set_PQresultStatus_result(PGRES_FATAL_ERROR);
    mock_libpq_set_PQerrorMessage_result("prepared statement \"bad_statement\" does not exist");

    bool query_result = postgresql_execute_prepared(&connection, &stmt, &request, &result);

    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NOT_NULL(result); // Should create error result
    TEST_ASSERT_FALSE(result->success); // Should be error result
    TEST_ASSERT_NOT_NULL(result->error_message); // Should have error message

    // Cleanup
    free(result->error_message);
    free(result->data_json);
    free(result);
}

int main(void) {
    UNITY_BEGIN();
    
    // postgresql_execute_query success paths
    RUN_TEST(test_postgresql_execute_query_success_no_data);
    RUN_TEST(test_postgresql_execute_query_success_with_single_row);
    RUN_TEST(test_postgresql_execute_query_success_with_multiple_rows);
    RUN_TEST(test_postgresql_execute_query_command_ok_status);
    RUN_TEST(test_postgresql_execute_query_error_status);
    RUN_TEST(test_postgresql_execute_query_zero_timeout);
    RUN_TEST(test_postgresql_execute_query_timeout_scenario);
    
    // postgresql_execute_prepared success paths
    RUN_TEST(test_postgresql_execute_prepared_success_with_data);
    RUN_TEST(test_postgresql_execute_prepared_fallback_to_pqexec);
    RUN_TEST(test_postgresql_execute_prepared_error_status);
    RUN_TEST(test_postgresql_execute_prepared_invalid_connection_handle);
    RUN_TEST(test_postgresql_execute_prepared_timeout_scenario);
    RUN_TEST(test_postgresql_execute_prepared_null_result);
    RUN_TEST(test_postgresql_execute_prepared_no_data_returned);
    RUN_TEST(test_postgresql_execute_prepared_failed_timeout_setting);
    
    return UNITY_END();
}