/*
 * Unity Test File: mysql_get_connection_string
 * This file contains unit tests for mysql_get_connection_string() functionality
 */

#include "../../../../src/hydrogen.h"
#include "unity.h"
#include "../../../../src/database/database.h"

// Forward declarations for functions being tested
char* mysql_get_connection_string(ConnectionConfig* config);

// Function prototypes for test functions
void test_mysql_get_connection_string_null_config(void);
void test_mysql_get_connection_string_with_connection_string(void);
void test_mysql_get_connection_string_with_individual_fields(void);
void test_mysql_get_connection_string_default_values(void);
void test_mysql_get_connection_string_partial_config(void);
void test_mysql_get_connection_string_custom_port(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test functions
void test_mysql_get_connection_string_null_config(void) {
    // Test null parameter handling
    char* result = mysql_get_connection_string(NULL);
    TEST_ASSERT_NULL(result);
}

void test_mysql_get_connection_string_with_connection_string(void) {
    // Test with connection_string field set
    ConnectionConfig config = {0};
    config.connection_string = (char*)"mysql://user:pass@host:3306/db";

    char* result = mysql_get_connection_string(&config);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("mysql://user:pass@host:3306/db", result);

    free(result);
}

void test_mysql_get_connection_string_with_individual_fields(void) {
    // Test with individual fields set
    ConnectionConfig config = {0};
    config.host = (char*)"localhost";
    config.port = 3306;
    config.database = (char*)"testdb";
    config.username = (char*)"testuser";
    config.password = (char*)"testpass";

    char* result = mysql_get_connection_string(&config);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("mysql://testuser:testpass@localhost:3306/testdb", result);

    free(result);
}

void test_mysql_get_connection_string_default_values(void) {
    // Test with empty config - should use defaults
    ConnectionConfig config = {0};

    char* result = mysql_get_connection_string(&config);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("mysql://:@localhost:3306/", result);

    free(result);
}

void test_mysql_get_connection_string_partial_config(void) {
    // Test with partial config - some fields set, others default
    ConnectionConfig config = {0};
    config.host = (char*)"remotehost";
    config.database = (char*)"mydb";
    // username, password, port should use defaults

    char* result = mysql_get_connection_string(&config);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("mysql://:@remotehost:3306/mydb", result);

    free(result);
}

void test_mysql_get_connection_string_custom_port(void) {
    // Test with custom port
    ConnectionConfig config = {0};
    config.host = (char*)"localhost";
    config.port = 3307;
    config.database = (char*)"testdb";

    char* result = mysql_get_connection_string(&config);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("mysql://:@localhost:3307/testdb", result);

    free(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mysql_get_connection_string_null_config);
    RUN_TEST(test_mysql_get_connection_string_with_connection_string);
    RUN_TEST(test_mysql_get_connection_string_with_individual_fields);
    RUN_TEST(test_mysql_get_connection_string_default_values);
    RUN_TEST(test_mysql_get_connection_string_partial_config);
    RUN_TEST(test_mysql_get_connection_string_custom_port);

    return UNITY_END();
}