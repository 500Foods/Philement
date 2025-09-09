/*
 * Unity Test File: sqlite_escape_string
 * This file contains unit tests for sqlite_escape_string() functionality
 */

#include "../../../../src/hydrogen.h"
#include "unity.h"
#include "../../../../src/database/database.h"

// Forward declarations for functions being tested
char* sqlite_escape_string(DatabaseHandle* connection, const char* input);

// Function prototypes for test functions
void test_sqlite_escape_string_null_connection(void);
void test_sqlite_escape_string_null_input(void);
void test_sqlite_escape_string_wrong_engine_type(void);
void test_sqlite_escape_string_no_quotes(void);
void test_sqlite_escape_string_with_single_quote(void);
void test_sqlite_escape_string_multiple_quotes(void);
void test_sqlite_escape_string_empty_string(void);
void test_sqlite_escape_string_only_quote(void);
void test_sqlite_escape_string_quotes_at_start_and_end(void);

// Mock database handle for testing
static DatabaseHandle mock_connection = {
    .engine_type = DB_ENGINE_SQLITE
};

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test functions
void test_sqlite_escape_string_null_connection(void) {
    // Test null connection parameter
    char* result = sqlite_escape_string(NULL, "test");
    TEST_ASSERT_NULL(result);
}

void test_sqlite_escape_string_null_input(void) {
    // Test null input parameter
    char* result = sqlite_escape_string(&mock_connection, NULL);
    TEST_ASSERT_NULL(result);
}

void test_sqlite_escape_string_wrong_engine_type(void) {
    // Test with wrong engine type
    DatabaseHandle wrong_engine = {.engine_type = DB_ENGINE_POSTGRESQL};
    char* result = sqlite_escape_string(&wrong_engine, "test");
    TEST_ASSERT_NULL(result);
}

void test_sqlite_escape_string_no_quotes(void) {
    // Test string with no single quotes
    char* result = sqlite_escape_string(&mock_connection, "hello world");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("hello world", result);
    free(result);
}

void test_sqlite_escape_string_with_single_quote(void) {
    // Test string with single quote
    char* result = sqlite_escape_string(&mock_connection, "don't");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("don''t", result);
    free(result);
}

void test_sqlite_escape_string_multiple_quotes(void) {
    // Test string with multiple single quotes
    char* result = sqlite_escape_string(&mock_connection, "O'Reilly's book");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("O''Reilly''s book", result);
    free(result);
}

void test_sqlite_escape_string_empty_string(void) {
    // Test empty string
    char* result = sqlite_escape_string(&mock_connection, "");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("", result);
    free(result);
}

void test_sqlite_escape_string_only_quote(void) {
    // Test string that is only a single quote
    char* result = sqlite_escape_string(&mock_connection, "'");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("''", result);
    free(result);
}

void test_sqlite_escape_string_quotes_at_start_and_end(void) {
    // Test quotes at start and end
    char* result = sqlite_escape_string(&mock_connection, "'test'");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("''test''", result);
    free(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_sqlite_escape_string_null_connection);
    RUN_TEST(test_sqlite_escape_string_null_input);
    RUN_TEST(test_sqlite_escape_string_wrong_engine_type);
    RUN_TEST(test_sqlite_escape_string_no_quotes);
    RUN_TEST(test_sqlite_escape_string_with_single_quote);
    RUN_TEST(test_sqlite_escape_string_multiple_quotes);
    RUN_TEST(test_sqlite_escape_string_empty_string);
    RUN_TEST(test_sqlite_escape_string_only_quote);
    RUN_TEST(test_sqlite_escape_string_quotes_at_start_and_end);

    return UNITY_END();
}