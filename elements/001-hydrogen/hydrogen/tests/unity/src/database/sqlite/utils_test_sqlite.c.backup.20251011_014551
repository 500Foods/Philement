/*
 * Unity Test File: SQLite Utility Functions
 * This file contains unit tests for SQLite utility functions
 */

#include "../../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../../src/database/database.h"
#include "../../../../../src/database/sqlite/utils.h"

// Forward declarations for functions being tested
char* sqlite_get_connection_string(const ConnectionConfig* config);
bool sqlite_validate_connection_string(const char* connection_string);
char* sqlite_escape_string(const DatabaseHandle* connection, const char* input);

// Function prototypes for test functions
void test_sqlite_get_connection_string_null_config(void);
void test_sqlite_get_connection_string_with_config(void);
void test_sqlite_validate_connection_string_null(void);
void test_sqlite_validate_connection_string_empty(void);
void test_sqlite_validate_connection_string_valid(void);
void test_sqlite_validate_connection_string_memory(void);
void test_sqlite_escape_string_null_connection(void);
void test_sqlite_escape_string_null_input(void);
void test_sqlite_escape_string_wrong_engine_type(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test sqlite_get_connection_string
void test_sqlite_get_connection_string_null_config(void) {
    char* result = sqlite_get_connection_string(NULL);
    TEST_ASSERT_NULL(result);
}

void test_sqlite_get_connection_string_with_config(void) {
    ConnectionConfig config = {0};
    config.database = strdup("test.db");
    config.host = strdup("localhost");
    config.port = 0; // SQLite doesn't use ports
    config.username = NULL; // SQLite doesn't use username
    config.password = NULL; // SQLite doesn't use password

    char* result = sqlite_get_connection_string(&config);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("test.db", result);

    free(result);
    free(config.database);
    free(config.host);
}

// Test sqlite_validate_connection_string
void test_sqlite_validate_connection_string_null(void) {
    bool result = sqlite_validate_connection_string(NULL);
    TEST_ASSERT_FALSE(result);
}

void test_sqlite_validate_connection_string_empty(void) {
    bool result = sqlite_validate_connection_string("");
    TEST_ASSERT_FALSE(result);
}

void test_sqlite_validate_connection_string_valid(void) {
    bool result = sqlite_validate_connection_string("test.db");
    TEST_ASSERT_TRUE(result);
}

void test_sqlite_validate_connection_string_memory(void) {
    bool result = sqlite_validate_connection_string(":memory:");
    TEST_ASSERT_TRUE(result);
}


// Test sqlite_escape_string
void test_sqlite_escape_string_null_connection(void) {
    char* result = sqlite_escape_string(NULL, "test");
    TEST_ASSERT_NULL(result);
}

void test_sqlite_escape_string_null_input(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    char* result = sqlite_escape_string(&connection, NULL);
    TEST_ASSERT_NULL(result);
}

void test_sqlite_escape_string_wrong_engine_type(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_POSTGRESQL; // Wrong engine type
    char* result = sqlite_escape_string(&connection, "test");
    TEST_ASSERT_NULL(result);
}

int main(void) {
    UNITY_BEGIN();

    // Test sqlite_get_connection_string
    RUN_TEST(test_sqlite_get_connection_string_null_config);
    RUN_TEST(test_sqlite_get_connection_string_with_config);

    // Test sqlite_validate_connection_string
    RUN_TEST(test_sqlite_validate_connection_string_null);
    RUN_TEST(test_sqlite_validate_connection_string_empty);
    RUN_TEST(test_sqlite_validate_connection_string_valid);
    RUN_TEST(test_sqlite_validate_connection_string_memory);

    // Test sqlite_escape_string
    RUN_TEST(test_sqlite_escape_string_null_connection);
    RUN_TEST(test_sqlite_escape_string_null_input);
    RUN_TEST(test_sqlite_escape_string_wrong_engine_type);

    return UNITY_END();
}