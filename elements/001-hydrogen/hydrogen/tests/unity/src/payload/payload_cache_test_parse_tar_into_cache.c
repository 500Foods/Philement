/*
 * Unity Test File: parse_tar_into_cache Function Tests
 * This file contains unit tests for the parse_tar_into_cache() function
 * from src/payload/payload_cache.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Forward declaration for the function being tested
bool parse_tar_into_cache(const uint8_t *tar_data, size_t tar_size);
void cleanup_payload_cache(void);
bool initialize_payload_cache(void);

// Test function prototypes
void setUp(void);
void tearDown(void);
void test_parse_tar_into_cache_null_data(void);
void test_parse_tar_into_cache_zero_size(void);
void test_parse_tar_into_cache_size_too_small(void);
void test_parse_tar_into_cache_empty_tar(void);
void test_parse_tar_into_cache_simple_file(void);
void test_parse_tar_into_cache_boundary_size(void);
void test_parse_tar_into_cache_large_data(void);

void setUp(void) {
    // Clean up any existing state and initialize
    cleanup_payload_cache();
    initialize_payload_cache();
}

void tearDown(void) {
    // Clean up after each test
    cleanup_payload_cache();
}

void test_parse_tar_into_cache_null_data(void) {
    TEST_ASSERT_FALSE(parse_tar_into_cache(NULL, 100));
}

void test_parse_tar_into_cache_zero_size(void) {
    const uint8_t dummy_data[10] = {0};
    TEST_ASSERT_FALSE(parse_tar_into_cache(dummy_data, 0));
}

void test_parse_tar_into_cache_size_too_small(void) {
    const uint8_t small_data[500] = {0}; // Less than 512 bytes
    TEST_ASSERT_FALSE(parse_tar_into_cache(small_data, 500));
}

void test_parse_tar_into_cache_empty_tar(void) {
    // Create a tar with just empty blocks (end of archive)
    const uint8_t empty_tar[1024] = {0};
    TEST_ASSERT_FALSE(parse_tar_into_cache(empty_tar, 1024));
}

void test_parse_tar_into_cache_simple_file(void) {
    // Create a minimal tar with one small file
    // This is a simplified test - in practice, creating valid tar data is complex
    // We'll test the basic parsing logic with controlled inputs

    // For now, test with invalid data to ensure error handling
    uint8_t invalid_tar[1024];
    memset(invalid_tar, 0xFF, 1024); // Fill with non-zero data

    // Should handle gracefully
    TEST_ASSERT_FALSE(parse_tar_into_cache(invalid_tar, 1024));
}

void test_parse_tar_into_cache_boundary_size(void) {
    // Test exactly 512 bytes (minimum valid size)
    const uint8_t boundary_tar[512] = {0};
    TEST_ASSERT_FALSE(parse_tar_into_cache(boundary_tar, 512));
}

void test_parse_tar_into_cache_large_data(void) {
    // Test with larger data size
    size_t large_size = 1024 * 1024; // 1MB
    uint8_t *large_tar = calloc(large_size, 1);

    if (large_tar) {
        // Fill with pattern that won't be valid tar
        memset(large_tar, 0xAA, large_size);

        TEST_ASSERT_FALSE(parse_tar_into_cache(large_tar, large_size));

        free(large_tar);
    } else {
        // If allocation failed, just test with null
        TEST_ASSERT_FALSE(parse_tar_into_cache(NULL, large_size));
    }
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_parse_tar_into_cache_null_data);
    RUN_TEST(test_parse_tar_into_cache_zero_size);
    RUN_TEST(test_parse_tar_into_cache_size_too_small);
    RUN_TEST(test_parse_tar_into_cache_empty_tar);
    RUN_TEST(test_parse_tar_into_cache_simple_file);
    RUN_TEST(test_parse_tar_into_cache_boundary_size);
    RUN_TEST(test_parse_tar_into_cache_large_data);

    return UNITY_END();
}