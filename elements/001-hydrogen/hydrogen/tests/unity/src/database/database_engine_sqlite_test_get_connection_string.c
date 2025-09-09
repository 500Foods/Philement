/*
 * Unity Test File: sqlite_get_connection_string
 * This file contains unit tests for sqlite_get_connection_string() functionality
 */

#include "../../../../src/hydrogen.h"
#include "unity.h"
#include "../../../../src/database/database.h"

// Forward declarations for functions being tested
char* sqlite_get_connection_string(ConnectionConfig* config);

// Function prototypes for test functions
void test_sqlite_get_connection_string_null_config(void);
void test_sqlite_get_connection_string_with_connection_string(void);
void test_sqlite_get_connection_string_with_database(void);
void test_sqlite_get_connection_string_default_memory(void);
void test_sqlite_get_connection_string_connection_string_priority(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test functions
void test_sqlite_get_connection_string_null_config(void) {
    // Test null parameter handling
    char* result = sqlite_get_connection_string(NULL);
    TEST_ASSERT_NULL(result);
}

void test_sqlite_get_connection_string_with_connection_string(void) {
    // Test with connection_string field set
    ConnectionConfig config = {0};
    config.connection_string = (char*)"test.db";

    char* result = sqlite_get_connection_string(&config);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("test.db", result);

    free(result);
}

void test_sqlite_get_connection_string_with_database(void) {
    // Test with database field set
    ConnectionConfig config = {0};
    config.database = (char*)"mydb.sqlite";

    char* result = sqlite_get_connection_string(&config);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("mydb.sqlite", result);

    free(result);
}

void test_sqlite_get_connection_string_default_memory(void) {
    // Test default to :memory: when no fields set
    ConnectionConfig config = {0};

    char* result = sqlite_get_connection_string(&config);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING(":memory:", result);

    free(result);
}

void test_sqlite_get_connection_string_connection_string_priority(void) {
    // Test that connection_string takes priority over database
    ConnectionConfig config = {0};
    config.database = (char*)"mydb.sqlite";
    config.connection_string = (char*)"override.db";

    char* result = sqlite_get_connection_string(&config);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("override.db", result);

    free(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_sqlite_get_connection_string_null_config);
    RUN_TEST(test_sqlite_get_connection_string_with_connection_string);
    RUN_TEST(test_sqlite_get_connection_string_with_database);
    RUN_TEST(test_sqlite_get_connection_string_default_memory);
    RUN_TEST(test_sqlite_get_connection_string_connection_string_priority);

    return UNITY_END();
}