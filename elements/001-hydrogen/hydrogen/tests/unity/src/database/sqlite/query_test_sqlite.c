/*
 * Unity Test File: SQLite Query Functions
 * This file contains unit tests for SQLite query functions
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/database/database.h>
#include <src/database/sqlite/query.h>

// Forward declarations for functions being tested
bool sqlite_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result);
bool sqlite_execute_prepared(DatabaseHandle* connection, const PreparedStatement* stmt, QueryRequest* request, QueryResult** result);

// Function prototypes for test functions
void test_sqlite_execute_query_null_connection(void);
void test_sqlite_execute_query_null_request(void);
void test_sqlite_execute_query_null_result_ptr(void);
void test_sqlite_execute_query_wrong_engine_type(void);
void test_sqlite_execute_prepared_null_connection(void);
void test_sqlite_execute_prepared_null_stmt(void);
void test_sqlite_execute_prepared_null_request(void);
void test_sqlite_execute_prepared_null_result_ptr(void);
void test_sqlite_execute_prepared_wrong_engine_type(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test sqlite_execute_query
void test_sqlite_execute_query_null_connection(void) {
    QueryRequest request = {0};
    QueryResult* result = NULL;
    bool query_result = sqlite_execute_query(NULL, &request, &result);
    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
}

void test_sqlite_execute_query_null_request(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    QueryResult* result = NULL;
    bool query_result = sqlite_execute_query(&connection, NULL, &result);
    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
}

void test_sqlite_execute_query_null_result_ptr(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    QueryRequest request = {0};
    bool query_result = sqlite_execute_query(&connection, &request, NULL);
    TEST_ASSERT_FALSE(query_result);
}

void test_sqlite_execute_query_wrong_engine_type(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_POSTGRESQL; // Wrong engine type
    QueryRequest request = {0};
    QueryResult* result = NULL;
    bool query_result = sqlite_execute_query(&connection, &request, &result);
    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
}

// Test sqlite_execute_prepared
void test_sqlite_execute_prepared_null_connection(void) {
    PreparedStatement stmt = {0};
    QueryRequest request = {0};
    QueryResult* result = NULL;
    bool query_result = sqlite_execute_prepared(NULL, &stmt, &request, &result);
    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
}

void test_sqlite_execute_prepared_null_stmt(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    QueryRequest request = {0};
    QueryResult* result = NULL;
    bool query_result = sqlite_execute_prepared(&connection, NULL, &request, &result);
    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
}

void test_sqlite_execute_prepared_null_request(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    PreparedStatement stmt = {0};
    QueryResult* result = NULL;
    bool query_result = sqlite_execute_prepared(&connection, &stmt, NULL, &result);
    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
}

void test_sqlite_execute_prepared_null_result_ptr(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    PreparedStatement stmt = {0};
    QueryRequest request = {0};
    bool query_result = sqlite_execute_prepared(&connection, &stmt, &request, NULL);
    TEST_ASSERT_FALSE(query_result);
}

void test_sqlite_execute_prepared_wrong_engine_type(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_POSTGRESQL; // Wrong engine type
    PreparedStatement stmt = {0};
    QueryRequest request = {0};
    QueryResult* result = NULL;
    bool query_result = sqlite_execute_prepared(&connection, &stmt, &request, &result);
    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
}

int main(void) {
    UNITY_BEGIN();

    // Test sqlite_execute_query
    RUN_TEST(test_sqlite_execute_query_null_connection);
    RUN_TEST(test_sqlite_execute_query_null_request);
    RUN_TEST(test_sqlite_execute_query_null_result_ptr);
    RUN_TEST(test_sqlite_execute_query_wrong_engine_type);

    // Test sqlite_execute_prepared
    RUN_TEST(test_sqlite_execute_prepared_null_connection);
    RUN_TEST(test_sqlite_execute_prepared_null_stmt);
    RUN_TEST(test_sqlite_execute_prepared_null_request);
    RUN_TEST(test_sqlite_execute_prepared_null_result_ptr);
    RUN_TEST(test_sqlite_execute_prepared_wrong_engine_type);

    return UNITY_END();
}