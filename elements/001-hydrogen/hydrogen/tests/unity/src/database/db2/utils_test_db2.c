/*
 * Unity Test File: DB2 Utils Functions
 * This file contains unit tests for DB2 utility functions
 */

#include "../../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../../src/database/database.h"
#include "../../../../../src/database/db2/utils.h"

// Forward declarations for functions being tested
char* db2_get_connection_string(const ConnectionConfig* config);
bool db2_validate_connection_string(const char* connection_string);
char* db2_escape_string(const DatabaseHandle* connection, const char* input);

// Function prototypes for test functions
void test_db2_get_connection_string_null_config(void);
void test_db2_get_connection_string_with_config(void);
void test_db2_validate_connection_string_null(void);
void test_db2_validate_connection_string_empty(void);
void test_db2_validate_connection_string_valid(void);
void test_db2_validate_connection_string_invalid(void);
void test_db2_escape_string_null_connection(void);
void test_db2_escape_string_null_input(void);
void test_db2_escape_string_wrong_engine(void);
void test_db2_escape_string_no_quotes(void);
void test_db2_escape_string_with_quotes(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test db2_get_connection_string
void test_db2_get_connection_string_null_config(void) {
    char* result = db2_get_connection_string(NULL);
    TEST_ASSERT_NULL(result);
}

void test_db2_get_connection_string_with_config(void) {
    ConnectionConfig config = {0};
    config.database = strdup("testdb");
    config.host = strdup("localhost");
    config.port = 50000;
    config.username = strdup("testuser");
    config.password = strdup("testpass");

    char* result = db2_get_connection_string(&config);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "DATABASE=testdb"));
    TEST_ASSERT_NOT_NULL(strstr(result, "HOSTNAME=localhost"));
    TEST_ASSERT_NOT_NULL(strstr(result, "PORT=50000"));
    TEST_ASSERT_NOT_NULL(strstr(result, "UID=testuser"));
    TEST_ASSERT_NOT_NULL(strstr(result, "PWD=testpass"));

    free(result);
    free(config.database);
    free(config.host);
    free(config.username);
    free(config.password);
}

// Test db2_validate_connection_string
void test_db2_validate_connection_string_null(void) {
    bool result = db2_validate_connection_string(NULL);
    TEST_ASSERT_FALSE(result);
}

void test_db2_validate_connection_string_empty(void) {
    bool result = db2_validate_connection_string("");
    TEST_ASSERT_FALSE(result);
}

void test_db2_validate_connection_string_valid(void) {
    bool result = db2_validate_connection_string("DATABASE=test;HOSTNAME=localhost;");
    TEST_ASSERT_TRUE(result);
}

void test_db2_validate_connection_string_invalid(void) {
    bool result = db2_validate_connection_string("INVALID=string");
    TEST_ASSERT_FALSE(result);
}

// Test db2_escape_string
void test_db2_escape_string_null_connection(void) {
    char* result = db2_escape_string(NULL, "test");
    TEST_ASSERT_NULL(result);
}

void test_db2_escape_string_null_input(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    char* result = db2_escape_string(&connection, NULL);
    TEST_ASSERT_NULL(result);
}

void test_db2_escape_string_wrong_engine(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE; // Wrong engine
    char* result = db2_escape_string(&connection, "test");
    TEST_ASSERT_NULL(result);
}

void test_db2_escape_string_no_quotes(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    char* result = db2_escape_string(&connection, "hello world");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("hello world", result);
    free(result);
}

void test_db2_escape_string_with_quotes(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    char* result = db2_escape_string(&connection, "don't worry");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("don''t worry", result);
    free(result);
}

int main(void) {
    UNITY_BEGIN();

    // Test db2_get_connection_string
    RUN_TEST(test_db2_get_connection_string_null_config);
    RUN_TEST(test_db2_get_connection_string_with_config);

    // Test db2_validate_connection_string
    RUN_TEST(test_db2_validate_connection_string_null);
    RUN_TEST(test_db2_validate_connection_string_empty);
    RUN_TEST(test_db2_validate_connection_string_valid);
    RUN_TEST(test_db2_validate_connection_string_invalid);

    // Test db2_escape_string
    RUN_TEST(test_db2_escape_string_null_connection);
    RUN_TEST(test_db2_escape_string_null_input);
    RUN_TEST(test_db2_escape_string_wrong_engine);
    RUN_TEST(test_db2_escape_string_no_quotes);
    RUN_TEST(test_db2_escape_string_with_quotes);

    return UNITY_END();
}