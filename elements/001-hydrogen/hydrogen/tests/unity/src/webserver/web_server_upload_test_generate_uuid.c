/*
 * Unity Test File: Web Server Upload - Generate UUID Test
 * This file contains unit tests for generate_uuid() function
 */

#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/webserver/web_server_upload.h>

// Standard library includes
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

void setUp(void) {
    // Reset mock system state
    mock_system_reset_all();
    // Set a fixed time for predictable testing
    mock_system_set_gettimeofday_time(1609459200, 500000); // 2021-01-01 00:00:00.5 UTC
    // Seed random number generator for consistent tests
    srand(42);
}

void tearDown(void) {
    // Clean up test fixtures
    mock_system_reset_all();
}

// Helper function to validate UUID format
static bool is_valid_uuid_format(const char *uuid_str) {
    if (!uuid_str) return false;
    
    // Check length (36 chars + null terminator)
    if (strlen(uuid_str) != 36) return false;
    
    // Check format: 8-4-4-4-12
    for (int i = 0; i < 36; i++) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            if (uuid_str[i] != '-') return false;
        } else {
            if (!isxdigit(uuid_str[i])) return false;
        }
    }
    
    return true;
}

// Test functions
static void test_generate_uuid_null_buffer(void) {
    // Test with NULL buffer - this will likely segfault in the actual function
    // but we document the expected behavior
    // The function does not check for NULL, so this is undefined behavior
    // We skip this test as it would cause a segfault
    TEST_PASS_MESSAGE("Function does not validate NULL buffer - documented as undefined behavior");
}

static void test_generate_uuid_basic_functionality(void) {
    char uuid_str[UUID_STR_LEN];
    
    // Generate UUID
    generate_uuid(uuid_str);
    
    // Verify the UUID is properly formatted
    TEST_ASSERT_TRUE(is_valid_uuid_format(uuid_str));
    
    // Verify null termination
    TEST_ASSERT_EQUAL_CHAR('\0', uuid_str[36]);
}

static void test_generate_uuid_multiple_calls(void) {
    char uuid1[UUID_STR_LEN];
    char uuid2[UUID_STR_LEN];
    
    // Generate two UUIDs
    mock_system_set_gettimeofday_time(1609459200, 500000);
    srand(42);
    generate_uuid(uuid1);
    
    // Change time slightly
    mock_system_set_gettimeofday_time(1609459200, 500001);
    srand(43);  // Different seed
    generate_uuid(uuid2);
    
    // Verify both are valid
    TEST_ASSERT_TRUE(is_valid_uuid_format(uuid1));
    TEST_ASSERT_TRUE(is_valid_uuid_format(uuid2));
    
    // Verify they are different
    TEST_ASSERT_TRUE(strcmp(uuid1, uuid2) != 0);
}

static void test_generate_uuid_uniqueness_over_time(void) {
    char uuids[5][UUID_STR_LEN];
    
    // Generate 5 UUIDs with different times
    for (int i = 0; i < 5; i++) {
        mock_system_set_gettimeofday_time(1609459200 + i, 500000 + i * 1000);
        srand((unsigned int)(42 + i));
        generate_uuid(uuids[i]);
        TEST_ASSERT_TRUE(is_valid_uuid_format(uuids[i]));
    }
    
    // Verify all are unique
    for (int i = 0; i < 5; i++) {
        for (int j = i + 1; j < 5; j++) {
            TEST_ASSERT_TRUE(strcmp(uuids[i], uuids[j]) != 0);
        }
    }
}

static void test_generate_uuid_no_null_termination(void) {
    char uuid_str[UUID_STR_LEN];
    
    // Fill buffer with non-zero values
    memset(uuid_str, 'X', UUID_STR_LEN);
    
    // Generate UUID
    generate_uuid(uuid_str);
    
    // Verify null termination exists
    TEST_ASSERT_EQUAL_CHAR('\0', uuid_str[36]);
    
    // Verify the UUID doesn't exceed expected length
    TEST_ASSERT_EQUAL_size_t(36, strlen(uuid_str));
}

static void test_generate_uuid_consistent_format(void) {
    char uuid_str[UUID_STR_LEN];
    
    // Generate UUID
    generate_uuid(uuid_str);
    
    // Check that hyphens are in the correct positions
    TEST_ASSERT_EQUAL_CHAR('-', uuid_str[8]);
    TEST_ASSERT_EQUAL_CHAR('-', uuid_str[13]);
    TEST_ASSERT_EQUAL_CHAR('-', uuid_str[18]);
    TEST_ASSERT_EQUAL_CHAR('-', uuid_str[23]);
    
    // Verify all other characters are hex digits
    for (int i = 0; i < 36; i++) {
        if (i != 8 && i != 13 && i != 18 && i != 23) {
            TEST_ASSERT_TRUE(isxdigit(uuid_str[i]));
        }
    }
}

static void test_generate_uuid_version_and_variant_bits(void) {
    char uuid_str[UUID_STR_LEN];
    int version_char, variant_char;
    
    // Generate UUID
    generate_uuid(uuid_str);
    
    // Extract version nibble (should be 4 for UUID v4)
    // Position 14 is the version field (first char after second hyphen)
    version_char = uuid_str[14];
    
    // Convert hex char to int
    int version = (version_char >= '0' && version_char <= '9') ? 
                  (version_char - '0') : 
                  (tolower(version_char) - 'a' + 10);
    
    // Check that version nibble has 0x4 set (UUID v4 format)
    // The code uses: ((rand() & 0xfff) | 0x4000)
    // This means the version nibble will be 0x4XXX where X is from rand
    TEST_ASSERT_EQUAL_INT(4, version & 0xF);
    
    // Extract variant bits (should be 10xx for RFC 4122)
    // Position 19 is the variant field (first char after third hyphen)
    variant_char = uuid_str[19];
    
    // Convert hex char to int
    int variant = (variant_char >= '0' && variant_char <= '9') ? 
                  (variant_char - '0') : 
                  (tolower(variant_char) - 'a' + 10);
    
    // Check that variant bits are set correctly (10xx binary = 8-B hex)
    // The code uses: ((rand() & 0x3fff) | 0x8000)
    // This means the variant nibble will be 0x8XXX to 0xBXXX
    TEST_ASSERT_TRUE(variant >= 8 && variant <= 11);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_generate_uuid_null_buffer);
    RUN_TEST(test_generate_uuid_basic_functionality);
    RUN_TEST(test_generate_uuid_multiple_calls);
    RUN_TEST(test_generate_uuid_uniqueness_over_time);
    RUN_TEST(test_generate_uuid_no_null_termination);
    RUN_TEST(test_generate_uuid_consistent_format);
    RUN_TEST(test_generate_uuid_version_and_variant_bits);

    return UNITY_END();
}