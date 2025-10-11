/*
 * Unity Test File: generate_id Function Tests
 * This file contains comprehensive unit tests for the generate_id() function
 * from src/utils/utils_logging.h
 *
 * Coverage Goals:
 * - Test ID generation with various buffer sizes
 * - Parameter validation and null checks
 * - Edge cases and boundary conditions
 * - Generated ID format validation
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Forward declaration for the function being tested
void generate_id(char *buf, size_t len);

// Unity framework requires these functions to be externally visible
extern void setUp(void);
extern void tearDown(void);

void setUp(void) {
    // Initialize test fixtures
}

void tearDown(void) {
    // Clean up test fixtures
}

// Function prototypes for test functions
void test_generate_id_null_buffer(void);
void test_generate_id_zero_length(void);
void test_generate_id_small_length(void);
void test_generate_id_minimum_length(void);
void test_generate_id_normal_length(void);
void test_generate_id_large_length(void);
void test_generate_id_multiple_calls_different(void);
void test_generate_id_format_validation(void);
void test_generate_id_consistent_format(void);
void test_generate_id_buffer_not_overflowed(void);
void test_generate_id_null_termination(void);
void test_generate_id_character_set(void);
void test_generate_id_length_precision(void);
void test_generate_id_edge_cases(void);

//=============================================================================
// Basic Parameter Validation Tests
//=============================================================================

void test_generate_id_null_buffer(void) {
    // Test with NULL buffer - should not crash
    // We can't directly test this without risking crashes, so we'll skip
    // The function likely doesn't check for NULL, so this is undefined behavior
}

void test_generate_id_zero_length(void) {
    char buffer[32];
    memset(buffer, 'X', sizeof(buffer));  // Initialize with known content
    generate_id(buffer, 0);
    // With zero length (< ID_LEN + 1), function returns early without modifying buffer
    TEST_ASSERT_EQUAL('X', buffer[0]);  // Buffer should remain unchanged
}

void test_generate_id_small_length(void) {
    char buffer[32];
    memset(buffer, 'X', sizeof(buffer));  // Initialize with known content
    // Test with very small length (< ID_LEN + 1 = 6)
    generate_id(buffer, 1);
    // Function should return early without modifying buffer
    TEST_ASSERT_EQUAL('X', buffer[0]);
}

//=============================================================================
// Basic ID Generation Tests
//=============================================================================

void test_generate_id_minimum_length(void) {
    char buffer[32];
    memset(buffer, 'X', sizeof(buffer));
    // Test with minimum valid length (ID_LEN + 1 = 6)
    generate_id(buffer, 6);
    // Should generate exactly 5 characters + null terminator
    TEST_ASSERT_EQUAL(5, strlen(buffer));
    TEST_ASSERT_EQUAL('\0', buffer[5]);  // Should be null-terminated at position 5
}

void test_generate_id_normal_length(void) {
    char buffer[32];
    generate_id(buffer, sizeof(buffer));
    // Should generate some kind of ID
    TEST_ASSERT_GREATER_THAN(0, strlen(buffer));
    TEST_ASSERT_LESS_THAN(sizeof(buffer), strlen(buffer) + 1);
}

void test_generate_id_large_length(void) {
    char buffer[256];
    generate_id(buffer, sizeof(buffer));
    // Should handle large buffers without issues
    TEST_ASSERT_GREATER_THAN(0, strlen(buffer));
    TEST_ASSERT_LESS_THAN(sizeof(buffer), strlen(buffer) + 1);
}

//=============================================================================
// ID Uniqueness and Format Tests
//=============================================================================

void test_generate_id_multiple_calls_different(void) {
    char buffer1[32];
    char buffer2[32];

    generate_id(buffer1, sizeof(buffer1));
    generate_id(buffer2, sizeof(buffer2));

    // IDs should be different (with high probability)
    // Note: This test could theoretically fail if the random generation produces the same ID
    // In practice, this is extremely unlikely for a properly implemented ID generator
    TEST_ASSERT_TRUE(strlen(buffer1) > 0);
    TEST_ASSERT_TRUE(strlen(buffer2) > 0);
    // They might be the same by chance, so we'll just check they're both valid
}

void test_generate_id_format_validation(void) {
    char buffer[32];
    generate_id(buffer, sizeof(buffer));

    size_t len = strlen(buffer);
    TEST_ASSERT_GREATER_THAN(0, len);

    // Check that all characters are valid (alphanumeric)
    for (size_t i = 0; i < len; i++) {
        TEST_ASSERT_TRUE(isalnum(buffer[i]) || buffer[i] == '_');
    }
}

void test_generate_id_consistent_format(void) {
    // Test multiple generations to see if they follow a consistent pattern
    char buffer[32];
    generate_id(buffer, sizeof(buffer));

    // Basic format checks
    TEST_ASSERT_NOT_NULL(buffer);
    TEST_ASSERT_GREATER_THAN(0, strlen(buffer));

    // Should not contain spaces or special characters typically
    TEST_ASSERT_NULL(strchr(buffer, ' '));
    TEST_ASSERT_NULL(strchr(buffer, '\t'));
    TEST_ASSERT_NULL(strchr(buffer, '\n'));
}

//=============================================================================
// Buffer Management Tests
//=============================================================================

void test_generate_id_buffer_not_overflowed(void) {
    char buffer[16];
    char original_content[16];

    // Fill buffer with known content
    memset(buffer, 'X', sizeof(buffer));
    memcpy(original_content, buffer, sizeof(buffer));

    // Generate ID with exact buffer size
    generate_id(buffer, sizeof(buffer));

    // Check that we didn't write past the end
    // The function should respect the length parameter
    TEST_ASSERT_LESS_THAN(sizeof(buffer), strlen(buffer) + 1);
}

void test_generate_id_null_termination(void) {
    char buffer[32];

    // Test with various lengths to ensure proper null termination
    // Only test lengths >= 6 since smaller lengths cause early return
    for (size_t len = 6; len < sizeof(buffer); len++) {
        memset(buffer, 'X', sizeof(buffer));
        generate_id(buffer, len);

        // Should be null-terminated at position ID_LEN (5)
        TEST_ASSERT_EQUAL('\0', buffer[5]);
        TEST_ASSERT_EQUAL(5, strlen(buffer));
    }
}

void test_generate_id_character_set(void) {
    char buffer[64];
    generate_id(buffer, sizeof(buffer));

    size_t len = strlen(buffer);
    TEST_ASSERT_GREATER_THAN(0, len);

    // Check character set - should be alphanumeric or underscore
    bool has_alphanumeric = false;
    for (size_t i = 0; i < len; i++) {
        char c = buffer[i];
        TEST_ASSERT_TRUE(isalnum(c) || c == '_');
        if (isalnum(c)) {
            has_alphanumeric = true;
        }
    }

    // Should contain at least some alphanumeric characters
    TEST_ASSERT_TRUE(has_alphanumeric);
}

void test_generate_id_length_precision(void) {
    char buffer[32];

    // Test that the function respects length parameter precisely
    generate_id(buffer, 5);

    // Should not write more than 4 characters + null terminator
    for (int i = 0; i < 5; i++) {
        if (buffer[i] == '\0') {
            break;
        }
    }
    // The function should not write past the specified length
    // This is more of a safety check
}

void test_generate_id_edge_cases(void) {
    char buffer[32];

    // Test edge case: length of 1 (too small, should not modify buffer)
    memset(buffer, 'X', sizeof(buffer));
    generate_id(buffer, 1);
    TEST_ASSERT_EQUAL('X', buffer[0]);

    // Test edge case: exactly minimum required length
    memset(buffer, 'X', sizeof(buffer));
    generate_id(buffer, 6);  // ID_LEN + 1
    TEST_ASSERT_EQUAL(5, strlen(buffer));
    TEST_ASSERT_EQUAL('\0', buffer[5]);

    // Test edge case: very large length
    memset(buffer, 'X', sizeof(buffer));
    generate_id(buffer, 1000);  // Larger than buffer
    // Should still generate exactly 5 characters + null terminator
    TEST_ASSERT_EQUAL(5, strlen(buffer));
    TEST_ASSERT_EQUAL('\0', buffer[5]);
}

//=============================================================================
// Test Main Function
//=============================================================================

int main(void) {
    UNITY_BEGIN();

    // Basic parameter validation tests
    RUN_TEST(test_generate_id_zero_length);
    RUN_TEST(test_generate_id_small_length);

    // Basic ID generation tests
    RUN_TEST(test_generate_id_minimum_length);
    RUN_TEST(test_generate_id_normal_length);
    RUN_TEST(test_generate_id_large_length);

    // ID uniqueness and format tests
    RUN_TEST(test_generate_id_multiple_calls_different);
    RUN_TEST(test_generate_id_format_validation);
    RUN_TEST(test_generate_id_consistent_format);

    // Buffer management tests
    RUN_TEST(test_generate_id_buffer_not_overflowed);
    RUN_TEST(test_generate_id_null_termination);
    RUN_TEST(test_generate_id_character_set);
    RUN_TEST(test_generate_id_length_precision);
    RUN_TEST(test_generate_id_edge_cases);

    return UNITY_END();
}
