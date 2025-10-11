/*
 * Unity Test File: MySQL Utility Functions
 * This file contains unit tests for MySQL utility functions
 */

#include "../../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../../src/database/database.h"
#include "../../../../../src/database/mysql/utils.h"

// Forward declarations for functions being tested
char* mysql_get_connection_string(const ConnectionConfig* config);
bool mysql_validate_connection_string(const char* connection_string);
char* mysql_escape_string(const DatabaseHandle* connection, const char* input);

// Function prototypes for test functions
void test_mysql_get_connection_string_null_config(void);
void test_mysql_get_connection_string_with_config(void);
void test_mysql_validate_connection_string_null(void);
void test_mysql_validate_connection_string_empty(void);
void test_mysql_validate_connection_string_valid(void);
void test_mysql_validate_connection_string_invalid(void);
void test_mysql_escape_string_null_connection(void);
void test_mysql_escape_string_null_input(void);
void test_mysql_escape_string_wrong_engine_type(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test mysql_get_connection_string
void test_mysql_get_connection_string_null_config(void) {
    char* result = mysql_get_connection_string(NULL);
    TEST_ASSERT_NULL(result);
}

void test_mysql_get_connection_string_with_config(void) {
    ConnectionConfig config = {0};
    config.database = strdup("testdb");
    config.host = strdup("localhost");
    config.port = 3306;
    config.username = strdup("testuser");
    config.password = strdup("testpass");

    char* result = mysql_get_connection_string(&config);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "testdb"));
    TEST_ASSERT_NOT_NULL(strstr(result, "localhost"));
    TEST_ASSERT_NOT_NULL(strstr(result, "3306"));
    TEST_ASSERT_NOT_NULL(strstr(result, "testuser"));
    TEST_ASSERT_NOT_NULL(strstr(result, "testpass"));

    free(result);
    free(config.database);
    free(config.host);
    free(config.username);
    free(config.password);
}

// Test mysql_validate_connection_string
void test_mysql_validate_connection_string_null(void) {
    bool result = mysql_validate_connection_string(NULL);
    TEST_ASSERT_FALSE(result);
}

void test_mysql_validate_connection_string_empty(void) {
    bool result = mysql_validate_connection_string("");
    TEST_ASSERT_FALSE(result);
}

void test_mysql_validate_connection_string_valid(void) {
    bool result = mysql_validate_connection_string("mysql://user:pass@host:3306/db");
    TEST_ASSERT_TRUE(result);
}

void test_mysql_validate_connection_string_invalid(void) {
    bool result = mysql_validate_connection_string("postgresql://user:pass@host:5432/db");
    TEST_ASSERT_FALSE(result);
}

// Test mysql_escape_string
void test_mysql_escape_string_null_connection(void) {
    char* result = mysql_escape_string(NULL, "test");
    TEST_ASSERT_NULL(result);
}

void test_mysql_escape_string_null_input(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MYSQL;
    char* result = mysql_escape_string(&connection, NULL);
    TEST_ASSERT_NULL(result);
}

void test_mysql_escape_string_wrong_engine_type(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE; // Wrong engine type
    char* result = mysql_escape_string(&connection, "test");
    TEST_ASSERT_NULL(result);
}

int main(void) {
    UNITY_BEGIN();

    // Test mysql_get_connection_string
    RUN_TEST(test_mysql_get_connection_string_null_config);
    RUN_TEST(test_mysql_get_connection_string_with_config);

    // Test mysql_validate_connection_string
    RUN_TEST(test_mysql_validate_connection_string_null);
    RUN_TEST(test_mysql_validate_connection_string_empty);
    RUN_TEST(test_mysql_validate_connection_string_valid);
    RUN_TEST(test_mysql_validate_connection_string_invalid);

    // Test mysql_escape_string
    RUN_TEST(test_mysql_escape_string_null_connection);
    RUN_TEST(test_mysql_escape_string_null_input);
    RUN_TEST(test_mysql_escape_string_wrong_engine_type);

    return UNITY_END();
}