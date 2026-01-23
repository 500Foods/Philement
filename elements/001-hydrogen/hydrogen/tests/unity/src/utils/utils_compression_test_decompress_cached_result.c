/*
 * Unity Test File: Compression Utilities - decompress_cached_result Tests
 * This file contains unit tests for decompress_cached_result function
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include the header for the function being tested
#include <src/utils/utils_compression.h>

// Standard C libraries for test data
#include <stdlib.h>
#include <string.h>

// Function prototypes
void test_decompress_cached_result_null_compressed_data(void);
void test_decompress_cached_result_zero_compressed_size(void);
void test_decompress_cached_result_null_decompressed_data(void);
void test_decompress_cached_result_null_decompressed_size(void);
void test_decompress_cached_result_success_round_trip(void);
void test_decompress_cached_result_invalid_data(void);
void test_decompress_cached_result_small_data(void);
void test_decompress_cached_result_large_invalid_data(void);

// Test data
static const char* test_json = "{\"test\": \"data\", \"number\": 123, \"array\": [1,2,3]}";
static const size_t test_json_size = 54; // strlen(test_json)

void setUp(void) {
    // No setup needed for decompression tests
}

void tearDown(void) {
    // No cleanup needed for decompression tests
}

// Test null compressed_data parameter
void test_decompress_cached_result_null_compressed_data(void) {
    char* decompressed_data = NULL;
    size_t decompressed_size = 0;

    bool result = decompress_cached_result(NULL, 100, &decompressed_data, &decompressed_size);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(decompressed_data);
    TEST_ASSERT_EQUAL(0, decompressed_size);
}

// Test zero compressed_size parameter
void test_decompress_cached_result_zero_compressed_size(void) {
    const unsigned char dummy_data[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    char* decompressed_data = NULL;
    size_t decompressed_size = 0;

    bool result = decompress_cached_result(dummy_data, 0, &decompressed_data, &decompressed_size);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(decompressed_data);
    TEST_ASSERT_EQUAL(0, decompressed_size);
}

// Test null decompressed_data parameter
void test_decompress_cached_result_null_decompressed_data(void) {
    const unsigned char dummy_data[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    size_t decompressed_size = 0;

    bool result = decompress_cached_result(dummy_data, 10, NULL, &decompressed_size);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(0, decompressed_size);
}

// Test null decompressed_size parameter
void test_decompress_cached_result_null_decompressed_size(void) {
    const unsigned char dummy_data[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    char* decompressed_data = NULL;

    bool result = decompress_cached_result(dummy_data, 10, &decompressed_data, NULL);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(decompressed_data);
}

// Test successful compression and decompression round trip
void test_decompress_cached_result_success_round_trip(void) {
    // First compress the test data
    unsigned char* compressed_data = NULL;
    size_t compressed_size = 0;

    bool compress_result = compress_json_result(test_json, test_json_size, &compressed_data, &compressed_size);
    TEST_ASSERT_TRUE(compress_result);
    TEST_ASSERT_NOT_NULL(compressed_data);
    TEST_ASSERT_GREATER_THAN(0, compressed_size);

    // Now decompress it
    char* decompressed_data = NULL;
    size_t decompressed_size = 0;

    bool decompress_result = decompress_cached_result(compressed_data, compressed_size, &decompressed_data, &decompressed_size);

    TEST_ASSERT_TRUE(decompress_result);
    TEST_ASSERT_NOT_NULL(decompressed_data);
    TEST_ASSERT_GREATER_THAN(0, decompressed_size);

    // Verify the decompressed data matches the original
    TEST_ASSERT_EQUAL(test_json_size, decompressed_size);
    TEST_ASSERT_EQUAL_MEMORY(test_json, decompressed_data, test_json_size);

    // Clean up
    free(compressed_data);
    free(decompressed_data);
}

// Test decompression with invalid compressed data
void test_decompress_cached_result_invalid_data(void) {
    const unsigned char invalid_data[10] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    char* decompressed_data = NULL;
    size_t decompressed_size = 0;

    bool result = decompress_cached_result(invalid_data, 10, &decompressed_data, &decompressed_size);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(decompressed_data);
    TEST_ASSERT_EQUAL(0, decompressed_size);
}

// Test decompression with small compressed data
void test_decompress_cached_result_small_data(void) {
   const  unsigned char small_data[1] = {0};
    char* decompressed_data = NULL;
    size_t decompressed_size = 0;

    bool result = decompress_cached_result(small_data, 1, &decompressed_data, &decompressed_size);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(decompressed_data);
    TEST_ASSERT_EQUAL(0, decompressed_size);
}

// Test decompression with large compressed data (simulate corrupted data)
void test_decompress_cached_result_large_invalid_data(void) {
    // Create a large buffer with invalid data
    const size_t large_size = 10000;
    unsigned char* large_invalid_data = malloc(large_size);
    TEST_ASSERT_NOT_NULL(large_invalid_data);

    // Fill with pattern that should fail decompression
    memset(large_invalid_data, 0xAA, large_size);

    char* decompressed_data = NULL;
    size_t decompressed_size = 0;

    bool result = decompress_cached_result(large_invalid_data, large_size, &decompressed_data, &decompressed_size);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(decompressed_data);
    TEST_ASSERT_EQUAL(0, decompressed_size);

    // Clean up
    free(large_invalid_data);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_decompress_cached_result_null_compressed_data);
    RUN_TEST(test_decompress_cached_result_zero_compressed_size);
    RUN_TEST(test_decompress_cached_result_null_decompressed_data);
    RUN_TEST(test_decompress_cached_result_null_decompressed_size);
    RUN_TEST(test_decompress_cached_result_success_round_trip);
    RUN_TEST(test_decompress_cached_result_invalid_data);
    RUN_TEST(test_decompress_cached_result_small_data);
    RUN_TEST(test_decompress_cached_result_large_invalid_data);

    return UNITY_END();
}