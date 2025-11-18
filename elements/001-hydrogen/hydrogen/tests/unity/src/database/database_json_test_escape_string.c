/*
 * Unity Test File: Database JSON Escape String Function Tests
 * Tests for database_json_escape_string function in database_json.c
 */

/*
 * CHANGELOG
 * 2025-11-18: Created comprehensive unit tests for database_json_escape_string
 *             Tests parameter validation, normal operation, edge cases, and error scenarios
 *             Aims for 75%+ coverage of the function
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/database/database_json.h>

// Forward declarations for functions being tested
int database_json_escape_string(const char* input, char* output, size_t output_size);

// Test function declarations
void test_database_json_escape_string_null_input(void);
void test_database_json_escape_string_null_output(void);
void test_database_json_escape_string_zero_output_size(void);
void test_database_json_escape_string_output_size_too_small(void);
void test_database_json_escape_string_empty_string(void);
void test_database_json_escape_string_no_special_chars(void);
void test_database_json_escape_string_double_quotes(void);
void test_database_json_escape_string_backslashes(void);
void test_database_json_escape_string_newlines(void);
void test_database_json_escape_string_carriage_returns(void);
void test_database_json_escape_string_tabs(void);
void test_database_json_escape_string_mixed_special_chars(void);
void test_database_json_escape_string_multiple_escapes(void);
void test_database_json_escape_string_exact_fit(void);
void test_database_json_escape_string_barely_too_small(void);
void test_database_json_escape_string_special_char_at_boundary(void);
void test_database_json_escape_string_unicode_passthrough(void);
void test_database_json_escape_string_control_chars(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// ============================================================================
// Tests for database_json_escape_string
// ============================================================================

void test_database_json_escape_string_null_input(void) {
    char output[100];
    int result = database_json_escape_string(NULL, output, sizeof(output));
    TEST_ASSERT_EQUAL(-1, result);
}

void test_database_json_escape_string_null_output(void) {
    int result = database_json_escape_string("test", NULL, 100);
    TEST_ASSERT_EQUAL(-1, result);
}

void test_database_json_escape_string_zero_output_size(void) {
    char output[100];
    int result = database_json_escape_string("test", output, 0);
    TEST_ASSERT_EQUAL(-1, result);
}

void test_database_json_escape_string_output_size_too_small(void) {
    char output[2]; // Only space for null terminator
    int result = database_json_escape_string("test", output, sizeof(output));
    TEST_ASSERT_EQUAL(-1, result);
}

void test_database_json_escape_string_empty_string(void) {
    char output[100];
    int result = database_json_escape_string("", output, sizeof(output));
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL_STRING("", output);
}

void test_database_json_escape_string_no_special_chars(void) {
    char output[100];
    int result = database_json_escape_string("Hello World", output, sizeof(output));
    TEST_ASSERT_EQUAL(11, result);
    TEST_ASSERT_EQUAL_STRING("Hello World", output);
}

void test_database_json_escape_string_double_quotes(void) {
    char output[100];
    int result = database_json_escape_string("Say \"Hello\"", output, sizeof(output));
    TEST_ASSERT_EQUAL(13, result); // 12 chars + null terminator, but return value is chars written
    TEST_ASSERT_EQUAL_STRING("Say \\\"Hello\\\"", output);
}

void test_database_json_escape_string_backslashes(void) {
    char output[100];
    int result = database_json_escape_string("C:\\path\\file", output, sizeof(output));
    TEST_ASSERT_EQUAL(14, result);
    TEST_ASSERT_EQUAL_STRING("C:\\\\path\\\\file", output);
}

void test_database_json_escape_string_newlines(void) {
    char output[100];
    int result = database_json_escape_string("Line1\nLine2", output, sizeof(output));
    TEST_ASSERT_EQUAL(12, result);
    TEST_ASSERT_EQUAL_STRING("Line1\\nLine2", output);
}

void test_database_json_escape_string_carriage_returns(void) {
    char output[100];
    int result = database_json_escape_string("Line1\rLine2", output, sizeof(output));
    TEST_ASSERT_EQUAL(12, result);
    TEST_ASSERT_EQUAL_STRING("Line1\\rLine2", output);
}

void test_database_json_escape_string_tabs(void) {
    char output[100];
    int result = database_json_escape_string("Col1\tCol2", output, sizeof(output));
    TEST_ASSERT_EQUAL(10, result);
    TEST_ASSERT_EQUAL_STRING("Col1\\tCol2", output);
}

void test_database_json_escape_string_mixed_special_chars(void) {
    char output[100];
    int result = database_json_escape_string("Test\n\"Quote\"\t\\Slash", output, sizeof(output));
    TEST_ASSERT_EQUAL(24, result);
    TEST_ASSERT_EQUAL_STRING("Test\\n\\\"Quote\\\"\\t\\\\Slash", output);
}

void test_database_json_escape_string_multiple_escapes(void) {
    char output[100];
    int result = database_json_escape_string("\"\"\\\\", output, sizeof(output));
    TEST_ASSERT_EQUAL(8, result);
    TEST_ASSERT_EQUAL_STRING("\\\"\\\"\\\\\\\\", output);
}

void test_database_json_escape_string_exact_fit(void) {
    char output[6]; // "test" + null = 5 chars needed
    int result = database_json_escape_string("test", output, sizeof(output));
    TEST_ASSERT_EQUAL(4, result);
    TEST_ASSERT_EQUAL_STRING("test", output);
}

void test_database_json_escape_string_barely_too_small(void) {
    char output[4]; // Only 4 bytes, but "test" needs 5 (4 chars + null)
    int result = database_json_escape_string("test", output, sizeof(output));
    TEST_ASSERT_EQUAL(-1, result); // Should indicate buffer too small
}

void test_database_json_escape_string_special_char_at_boundary(void) {
    char output[10]; // Can fit "test\\n" (7 chars + null)
    int result = database_json_escape_string("test\n", output, sizeof(output));
    TEST_ASSERT_EQUAL(6, result);
    TEST_ASSERT_EQUAL_STRING("test\\n", output);
}

void test_database_json_escape_string_unicode_passthrough(void) {
    char output[100];
    // Test that Unicode characters pass through unchanged
    // "café" is 5 bytes in UTF-8 (é takes 2 bytes)
    int result = database_json_escape_string("café", output, sizeof(output));
    TEST_ASSERT_EQUAL(5, result);
    TEST_ASSERT_EQUAL_STRING("café", output);
}

void test_database_json_escape_string_control_chars(void) {
    char output[100];
    // Test control characters (should be escaped if needed, but our function only escapes specific ones)
    char input[10];
    input[0] = 'a';
    input[1] = 0x01; // SOH control character
    input[2] = 'b';
    input[3] = '\0';

    int result = database_json_escape_string(input, output, sizeof(output));
    TEST_ASSERT_EQUAL(3, result); // Control chars should pass through unchanged in our implementation
    TEST_ASSERT_EQUAL_UINT8('a', output[0]);
    TEST_ASSERT_EQUAL_UINT8(0x01, output[1]);
    TEST_ASSERT_EQUAL_UINT8('b', output[2]);
    TEST_ASSERT_EQUAL_UINT8('\0', output[3]);
}

int main(void) {
    UNITY_BEGIN();

    // Parameter validation tests
    RUN_TEST(test_database_json_escape_string_null_input);
    RUN_TEST(test_database_json_escape_string_null_output);
    RUN_TEST(test_database_json_escape_string_zero_output_size);
    RUN_TEST(test_database_json_escape_string_output_size_too_small);

    // Normal operation tests
    RUN_TEST(test_database_json_escape_string_empty_string);
    RUN_TEST(test_database_json_escape_string_no_special_chars);
    RUN_TEST(test_database_json_escape_string_double_quotes);
    RUN_TEST(test_database_json_escape_string_backslashes);
    RUN_TEST(test_database_json_escape_string_newlines);
    RUN_TEST(test_database_json_escape_string_carriage_returns);
    RUN_TEST(test_database_json_escape_string_tabs);
    RUN_TEST(test_database_json_escape_string_mixed_special_chars);
    RUN_TEST(test_database_json_escape_string_multiple_escapes);

    // Edge case tests
    RUN_TEST(test_database_json_escape_string_exact_fit);
    RUN_TEST(test_database_json_escape_string_barely_too_small);
    RUN_TEST(test_database_json_escape_string_special_char_at_boundary);
    RUN_TEST(test_database_json_escape_string_unicode_passthrough);
    RUN_TEST(test_database_json_escape_string_control_chars);

    return UNITY_END();
}