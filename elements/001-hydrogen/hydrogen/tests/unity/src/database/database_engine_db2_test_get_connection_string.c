/*
 * Unity Test File: db2_get_connection_string
 * This file contains unit tests for db2_get_connection_string() functionality
 */

#include "../../../../src/hydrogen.h"
#include "unity.h"
#include "../../../../src/database/database.h"

// Forward declarations for functions being tested
char* db2_get_connection_string(ConnectionConfig* config);

// Function prototypes for test functions
void test_db2_get_connection_string_null_config(void);
void test_db2_get_connection_string_with_connection_string(void);
void test_db2_get_connection_string_with_database(void);
void test_db2_get_connection_string_default_database(void);
void test_db2_get_connection_string_connection_string_priority(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test functions
void test_db2_get_connection_string_null_config(void) {
    // Test null parameter handling
    char* result = db2_get_connection_string(NULL);
    TEST_ASSERT_NULL(result);
}

void test_db2_get_connection_string_with_connection_string(void) {
    // Test with connection_string field set
    ConnectionConfig config = {0};
    config.connection_string = (char*)"MYDB";

    char* result = db2_get_connection_string(&config);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("MYDB", result);

    free(result);
}

void test_db2_get_connection_string_with_database(void) {
    // Test with database field set
    ConnectionConfig config = {0};
    config.database = (char*)"TESTDB";

    char* result = db2_get_connection_string(&config);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("TESTDB", result);

    free(result);
}

void test_db2_get_connection_string_default_database(void) {
    // Test with empty config - should use default "SAMPLE"
    ConnectionConfig config = {0};

    char* result = db2_get_connection_string(&config);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("SAMPLE", result);

    free(result);
}

void test_db2_get_connection_string_connection_string_priority(void) {
    // Test that connection_string takes priority over database
    ConnectionConfig config = {0};
    config.database = (char*)"TESTDB";
    config.connection_string = (char*)"PRIORITYDB";

    char* result = db2_get_connection_string(&config);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("PRIORITYDB", result);

    free(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_db2_get_connection_string_null_config);
    RUN_TEST(test_db2_get_connection_string_with_connection_string);
    RUN_TEST(test_db2_get_connection_string_with_database);
    RUN_TEST(test_db2_get_connection_string_default_database);
    RUN_TEST(test_db2_get_connection_string_connection_string_priority);

    return UNITY_END();
}