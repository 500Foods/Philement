/*
 * Unity Test File: Configuration Format Section Header Tests
 * This file contains unit tests for the format_section_header function
 * from src/config/config.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/config/config.h>

// Forward declarations for functions being tested
void format_section_header(char* buffer, size_t size, const char* letter, const char* name);

// Forward declarations for test functions
void test_format_section_header_null_buffer(void);
void test_format_section_header_null_letter(void);
void test_format_section_header_null_name(void);
void test_format_section_header_size_too_small(void);
void test_format_section_header_valid_input(void);

// Test setup and teardown
void setUp(void) {
    // No setup needed
}

void tearDown(void) {
    // No cleanup needed
}

// ===== EDGE CASE TESTS =====

void test_format_section_header_null_buffer(void) {
    // Test with NULL buffer - should handle gracefully without crashing
    format_section_header(NULL, 256, "A", "Server");
    // No assertions needed - function should not crash
}

void test_format_section_header_null_letter(void) {
    // Test with NULL letter - should set buffer to empty and return
    char buffer[256] = {0};
    format_section_header(buffer, sizeof(buffer), NULL, "Server");

    TEST_ASSERT_EQUAL_STRING("", buffer);
}

void test_format_section_header_null_name(void) {
    // Test with NULL name - should set buffer to empty and return
    char buffer[256] = {0};
    format_section_header(buffer, sizeof(buffer), "A", NULL);

    TEST_ASSERT_EQUAL_STRING("", buffer);
}

void test_format_section_header_size_too_small(void) {
    // Test with size <= strlen(LOG_LINE_BREAK) - should set buffer to empty
    char buffer[256] = {0};
    // strlen(LOG_LINE_BREAK) is 192 (64 em-dashes * 3 bytes each)
    format_section_header(buffer, 10, "A", "Server");

    TEST_ASSERT_EQUAL_STRING("", buffer);
}

// ===== NORMAL OPERATION TESTS =====

void test_format_section_header_valid_input(void) {
    // Test with valid parameters - should produce a formatted header
    char buffer[256] = {0};
    format_section_header(buffer, sizeof(buffer), "A", "Server");

    // Buffer should not be empty
    TEST_ASSERT_TRUE(strlen(buffer) > 0);

    // Should contain the section letter (uppercased)
    TEST_ASSERT_NOT_NULL(strstr(buffer, "A"));

    // Should contain the section name
    TEST_ASSERT_NOT_NULL(strstr(buffer, "Server"));
}

// ===== MAIN TEST RUNNER =====

int main(void) {
    UNITY_BEGIN();

    // Edge case tests
    RUN_TEST(test_format_section_header_null_buffer);
    RUN_TEST(test_format_section_header_null_letter);
    RUN_TEST(test_format_section_header_null_name);
    RUN_TEST(test_format_section_header_size_too_small);

    // Normal operation tests
    RUN_TEST(test_format_section_header_valid_input);

    return UNITY_END();
}
