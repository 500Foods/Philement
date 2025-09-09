/*
 * Unity Test File: postgresql_get_connection_string
 * This file contains unit tests for postgresql_get_connection_string() functionality
 */

#include "../../../../src/hydrogen.h"
#include "unity.h"
#include "../../../../src/database/database.h"

// Forward declarations for functions being tested
char* postgresql_get_connection_string(ConnectionConfig* config);

// Function prototypes for test functions
void test_postgresql_get_connection_string_null_config(void);
void test_postgresql_get_connection_string_with_connection_string(void);
void test_postgresql_get_connection_string_with_individual_fields(void);
void test_postgresql_get_connection_string_default_values(void);
void test_postgresql_get_connection_string_partial_config(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test functions
void test_postgresql_get_connection_string_null_config(void) {
    // Test null parameter handling
    char* result = postgresql_get_connection_string(NULL);
    TEST_ASSERT_NULL(result);
}

void test_postgresql_get_connection_string_with_connection_string(void) {
    // Test with connection_string field set
    ConnectionConfig config = {0};
    config.connection_string = (char*)"postgresql://user:pass@host:5432/db";

    char* result = postgresql_get_connection_string(&config);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("postgresql://user:pass@host:5432/db", result);

    free(result);
}

void test_postgresql_get_connection_string_with_individual_fields(void) {
    // Test with individual fields set
    ConnectionConfig config = {0};
    config.host = (char*)"localhost";
    config.port = 5432;
    config.database = (char*)"testdb";
    config.username = (char*)"testuser";
    config.password = (char*)"testpass";

    char* result = postgresql_get_connection_string(&config);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("postgresql://testuser:testpass@localhost:5432/testdb", result);

    free(result);
}

void test_postgresql_get_connection_string_default_values(void) {
    // Test with empty config - should use defaults
    ConnectionConfig config = {0};

    char* result = postgresql_get_connection_string(&config);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("postgresql://:@localhost:5432/postgres", result);

    free(result);
}

void test_postgresql_get_connection_string_partial_config(void) {
    // Test with partial config - some fields set, others default
    ConnectionConfig config = {0};
    config.host = (char*)"remotehost";
    config.database = (char*)"mydb";
    // username, password, port should use defaults

    char* result = postgresql_get_connection_string(&config);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("postgresql://:@remotehost:5432/mydb", result);

    free(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_postgresql_get_connection_string_null_config);
    RUN_TEST(test_postgresql_get_connection_string_with_connection_string);
    RUN_TEST(test_postgresql_get_connection_string_with_individual_fields);
    RUN_TEST(test_postgresql_get_connection_string_default_values);
    RUN_TEST(test_postgresql_get_connection_string_partial_config);

    return UNITY_END();
}