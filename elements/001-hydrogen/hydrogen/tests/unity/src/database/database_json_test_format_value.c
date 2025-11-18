/*
 * Unity Test File: Database JSON Format Value Function Tests
 * Tests for database_json_format_value function in database_json.c
 */

/*
 * CHANGELOG
 * 2025-11-18: Created comprehensive unit tests for database_json_format_value
 *             Tests parameter validation, null handling, numeric formatting,
 *             string escaping, and edge cases for 75%+ coverage
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/database/database_json.h>

// Forward declarations for functions being tested
bool database_json_format_value(const char* column_name, const char* value, bool is_numeric, bool is_null,
                               char* output, size_t output_size, size_t* written);

// Test function declarations
void test_database_json_format_value_null_column_name(void);
void test_database_json_format_value_null_output(void);
void test_database_json_format_value_null_written(void);
void test_database_json_format_value_null_value(void);
void test_database_json_format_value_null_value_numeric(void);
void test_database_json_format_value_is_null_true(void);
void test_database_json_format_value_is_null_true_with_value(void);
void test_database_json_format_value_numeric_true_valid_value(void);
void test_database_json_format_value_numeric_true_zero_value(void);
void test_database_json_format_value_numeric_true_negative_value(void);
void test_database_json_format_value_numeric_true_empty_string(void);
void test_database_json_format_value_numeric_scientific_notation(void);
void test_database_json_format_value_numeric_large_number(void);
void test_database_json_format_value_string_simple(void);
void test_database_json_format_value_string_with_quotes(void);
void test_database_json_format_value_string_with_newlines(void);
void test_database_json_format_value_string_with_tabs(void);
void test_database_json_format_value_string_with_backslashes(void);
void test_database_json_format_value_string_unicode(void);
void test_database_json_format_value_string_empty(void);
void test_database_json_format_value_output_size_too_small(void);
void test_database_json_format_value_numeric_output_size_too_small(void);
void test_database_json_format_value_null_output_size_too_small(void);
void test_database_json_format_value_column_name_with_quotes(void);
void test_database_json_format_value_empty_column_name(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// ============================================================================
// Tests for database_json_format_value
// ============================================================================

void test_database_json_format_value_null_column_name(void) {
    char output[100];
    size_t written = 0;
    bool result = database_json_format_value(NULL, "test", false, false, output, sizeof(output), &written);
    TEST_ASSERT_FALSE(result);
}

void test_database_json_format_value_null_output(void) {
    size_t written = 0;
    bool result = database_json_format_value("column", "test", false, false, NULL, 100, &written);
    TEST_ASSERT_FALSE(result);
}

void test_database_json_format_value_null_written(void) {
    char output[100];
    bool result = database_json_format_value("column", "test", false, false, output, sizeof(output), NULL);
    TEST_ASSERT_FALSE(result);
}

void test_database_json_format_value_null_value(void) {
    char output[100];
    size_t written = 0;
    bool result = database_json_format_value("column", NULL, false, false, output, sizeof(output), &written);
    TEST_ASSERT_FALSE(result); // Function returns false for NULL value in string mode
}

void test_database_json_format_value_null_value_numeric(void) {
    char output[100];
    size_t written = 0;
    bool result = database_json_format_value("column", NULL, true, false, output, sizeof(output), &written);
    TEST_ASSERT_FALSE(result); // Function returns false for NULL value in numeric mode
}

void test_database_json_format_value_is_null_true(void) {
    char output[100];
    size_t written = 0;
    bool result = database_json_format_value("column", "some_value", false, true, output, sizeof(output), &written);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("\"column\":null", output);
    TEST_ASSERT_EQUAL(13, written); // "column":null = 13 chars
}

void test_database_json_format_value_is_null_true_with_value(void) {
    char output[100];
    size_t written = 0;
    bool result = database_json_format_value("column", "ignored", true, true, output, sizeof(output), &written);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("\"column\":null", output);
    TEST_ASSERT_EQUAL(13, written);
}

void test_database_json_format_value_numeric_true_valid_value(void) {
    char output[100];
    size_t written = 0;
    bool result = database_json_format_value("age", "25", true, false, output, sizeof(output), &written);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("\"age\":25", output);
    TEST_ASSERT_EQUAL(8, written); // "age":25 = 8 chars
}

void test_database_json_format_value_numeric_true_zero_value(void) {
    char output[100];
    size_t written = 0;
    bool result = database_json_format_value("count", "0", true, false, output, sizeof(output), &written);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("\"count\":0", output);
    TEST_ASSERT_EQUAL(9, written);
}

void test_database_json_format_value_numeric_true_negative_value(void) {
    char output[100];
    size_t written = 0;
    bool result = database_json_format_value("balance", "-100.50", true, false, output, sizeof(output), &written);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("\"balance\":-100.50", output);
    TEST_ASSERT_EQUAL(17, written);
}

void test_database_json_format_value_numeric_true_empty_string(void) {
    char output[100];
    size_t written = 0;
    bool result = database_json_format_value("count", "", true, false, output, sizeof(output), &written);
    TEST_ASSERT_FALSE(result); // Function returns false for empty string in numeric mode
}

void test_database_json_format_value_string_simple(void) {
    char output[100];
    size_t written = 0;
    bool result = database_json_format_value("name", "John", false, false, output, sizeof(output), &written);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("\"name\":\"John\"", output);
    TEST_ASSERT_EQUAL(13, written);
}

void test_database_json_format_value_string_with_quotes(void) {
    char output[100];
    size_t written = 0;
    bool result = database_json_format_value("message", "He said \"Hello\"", false, false, output, sizeof(output), &written);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("\"message\":\"He said \\\"Hello\\\"\"", output);
    TEST_ASSERT_EQUAL(29, written);
}

void test_database_json_format_value_string_with_newlines(void) {
    char output[100];
    size_t written = 0;
    bool result = database_json_format_value("text", "Line1\nLine2", false, false, output, sizeof(output), &written);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("\"text\":\"Line1\\nLine2\"", output);
    TEST_ASSERT_EQUAL(21, written);
}

void test_database_json_format_value_string_with_tabs(void) {
    char output[100];
    size_t written = 0;
    bool result = database_json_format_value("data", "Col1\tCol2", false, false, output, sizeof(output), &written);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("\"data\":\"Col1\\tCol2\"", output);
    TEST_ASSERT_EQUAL(19, written);
}

void test_database_json_format_value_string_with_backslashes(void) {
    char output[100];
    size_t written = 0;
    bool result = database_json_format_value("path", "C:\\temp\\file.txt", false, false, output, sizeof(output), &written);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("\"path\":\"C:\\\\temp\\\\file.txt\"", output);
    TEST_ASSERT_EQUAL(27, written);
}

void test_database_json_format_value_output_size_too_small(void) {
    char output[10]; // Too small for "column":"value"
    size_t written = 0;
    bool result = database_json_format_value("column", "value", false, false, output, sizeof(output), &written);
    TEST_ASSERT_FALSE(result);
}

void test_database_json_format_value_numeric_output_size_too_small(void) {
    char output[5]; // Too small for "n":123
    size_t written = 0;
    bool result = database_json_format_value("n", "123", true, false, output, sizeof(output), &written);
    TEST_ASSERT_FALSE(result);
}

void test_database_json_format_value_null_output_size_too_small(void) {
    char output[5]; // Too small for "c":null
    size_t written = 0;
    bool result = database_json_format_value("c", "value", false, true, output, sizeof(output), &written);
    TEST_ASSERT_FALSE(result);
}

void test_database_json_format_value_column_name_with_quotes(void) {
    char output[100];
    size_t written = 0;
    // Column names with quotes should be handled (though unusual)
    bool result = database_json_format_value("col\"name", "value", false, false, output, sizeof(output), &written);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("\"col\"name\":\"value\"", output);
    TEST_ASSERT_EQUAL(18, written);
}

void test_database_json_format_value_empty_column_name(void) {
    char output[100];
    size_t written = 0;
    bool result = database_json_format_value("", "value", false, false, output, sizeof(output), &written);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("\"value\"", output);
    TEST_ASSERT_EQUAL(7, written);
}

void test_database_json_format_value_numeric_scientific_notation(void) {
    char output[100];
    size_t written = 0;
    bool result = database_json_format_value("scientific", "1.23e-4", true, false, output, sizeof(output), &written);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("\"scientific\":1.23e-4", output);
    TEST_ASSERT_EQUAL(20, written);
}

void test_database_json_format_value_string_unicode(void) {
    char output[100];
    size_t written = 0;
    bool result = database_json_format_value("unicode", "café", false, false, output, sizeof(output), &written);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("\"unicode\":\"café\"", output);
    TEST_ASSERT_EQUAL(17, written);
}

void test_database_json_format_value_numeric_large_number(void) {
    char output[100];
    size_t written = 0;
    bool result = database_json_format_value("big_num", "123456789012345678901234567890", true, false, output, sizeof(output), &written);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("\"big_num\":123456789012345678901234567890", output);
    TEST_ASSERT_EQUAL(40, written);
}

void test_database_json_format_value_string_empty(void) {
    char output[100];
    size_t written = 0;
    bool result = database_json_format_value("empty", "", false, false, output, sizeof(output), &written);
    TEST_ASSERT_FALSE(result); // Function returns false for empty string in string mode
}

int main(void) {
    UNITY_BEGIN();

    // Parameter validation tests
    RUN_TEST(test_database_json_format_value_null_column_name);
    RUN_TEST(test_database_json_format_value_null_output);
    RUN_TEST(test_database_json_format_value_null_written);
    RUN_TEST(test_database_json_format_value_null_value);
    RUN_TEST(test_database_json_format_value_null_value_numeric);

    // NULL handling tests
    RUN_TEST(test_database_json_format_value_is_null_true);
    RUN_TEST(test_database_json_format_value_is_null_true_with_value);

    // Numeric formatting tests
    RUN_TEST(test_database_json_format_value_numeric_true_valid_value);
    RUN_TEST(test_database_json_format_value_numeric_true_zero_value);
    RUN_TEST(test_database_json_format_value_numeric_true_negative_value);
    RUN_TEST(test_database_json_format_value_numeric_true_empty_string);
    RUN_TEST(test_database_json_format_value_numeric_scientific_notation);
    RUN_TEST(test_database_json_format_value_numeric_large_number);

    // String formatting tests
    RUN_TEST(test_database_json_format_value_string_simple);
    RUN_TEST(test_database_json_format_value_string_with_quotes);
    RUN_TEST(test_database_json_format_value_string_with_newlines);
    RUN_TEST(test_database_json_format_value_string_with_tabs);
    RUN_TEST(test_database_json_format_value_string_with_backslashes);
    RUN_TEST(test_database_json_format_value_string_unicode);
    RUN_TEST(test_database_json_format_value_string_empty);

    // Edge case and error tests
    RUN_TEST(test_database_json_format_value_output_size_too_small);
    RUN_TEST(test_database_json_format_value_numeric_output_size_too_small);
    RUN_TEST(test_database_json_format_value_null_output_size_too_small);
    RUN_TEST(test_database_json_format_value_column_name_with_quotes);
    RUN_TEST(test_database_json_format_value_empty_column_name);

    return UNITY_END();
}