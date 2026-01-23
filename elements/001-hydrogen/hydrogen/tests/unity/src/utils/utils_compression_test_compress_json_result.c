/*
 * Unity Test File: Compression Utilities - compress_json_result Tests
 * This file contains unit tests for compress_json_result function
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include the header for the function being tested
#include <src/utils/utils_compression.h>

// Standard C libraries
#include <stdlib.h>
#include <string.h>

// Function prototypes
void test_compress_json_result_null_data(void);
void test_compress_json_result_zero_size(void);
void test_compress_json_result_null_compressed_data(void);
void test_compress_json_result_null_compressed_size(void);
void test_compress_json_result_success(void);
void test_compress_json_result_small_json(void);
void test_compress_json_result_large_json(void);

// Test data
static const char* test_json = "{\"test\": \"data\", \"number\": 123, \"array\": [1,2,3]}";
static const size_t test_json_size = 54; // strlen(test_json)

void setUp(void) {
    // No setup needed for compression tests
}

void tearDown(void) {
    // No cleanup needed for compression tests
}

// Test null json_data parameter
void test_compress_json_result_null_data(void) {
    unsigned char* compressed_data = NULL;
    size_t compressed_size = 0;

    bool result = compress_json_result(NULL, test_json_size, &compressed_data, &compressed_size);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(compressed_data);
    TEST_ASSERT_EQUAL(0, compressed_size);
}

// Test zero json_size parameter
void test_compress_json_result_zero_size(void) {
    unsigned char* compressed_data = NULL;
    size_t compressed_size = 0;

    bool result = compress_json_result(test_json, 0, &compressed_data, &compressed_size);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(compressed_data);
    TEST_ASSERT_EQUAL(0, compressed_size);
}

// Test null compressed_data parameter
void test_compress_json_result_null_compressed_data(void) {
    size_t compressed_size = 0;

    bool result = compress_json_result(test_json, test_json_size, NULL, &compressed_size);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(0, compressed_size);
}

// Test null compressed_size parameter
void test_compress_json_result_null_compressed_size(void) {
    unsigned char* compressed_data = NULL;

    bool result = compress_json_result(test_json, test_json_size, &compressed_data, NULL);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(compressed_data);
}

// Test successful compression
void test_compress_json_result_success(void) {
    unsigned char* compressed_data = NULL;
    size_t compressed_size = 0;

    bool result = compress_json_result(test_json, test_json_size, &compressed_data, &compressed_size);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(compressed_data);
    TEST_ASSERT_GREATER_THAN(0, compressed_size);

    // Verify compressed data is not the same as original
    TEST_ASSERT_FALSE(memcmp(compressed_data, test_json, compressed_size) == 0);

    // Clean up
    free(compressed_data);
}

// Test compression with small JSON
void test_compress_json_result_small_json(void) {
    const char* small_json = "{}";
    const size_t small_size = 2;
    unsigned char* compressed_data = NULL;
    size_t compressed_size = 0;

    bool result = compress_json_result(small_json, small_size, &compressed_data, &compressed_size);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(compressed_data);
    TEST_ASSERT_GREATER_THAN(0, compressed_size);

    // Clean up
    free(compressed_data);
}

// Test compression with large JSON
void test_compress_json_result_large_json(void) {
    // Create a larger JSON string
    const char* large_json = "{\"data\": \"";
    const size_t base_size = strlen(large_json);
    const size_t padding_size = 1000;
    char* padded_json = malloc(base_size + padding_size + 3); // +3 for "}\n"
    TEST_ASSERT_NOT_NULL(padded_json);

    strcpy(padded_json, large_json);
    memset(padded_json + base_size, 'x', padding_size);
    strcpy(padded_json + base_size + padding_size, "\"}");

    const size_t large_size = strlen(padded_json);
    unsigned char* compressed_data = NULL;
    size_t compressed_size = 0;

    bool result = compress_json_result(padded_json, large_size, &compressed_data, &compressed_size);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(compressed_data);
    TEST_ASSERT_GREATER_THAN(0, compressed_size);

    // Clean up
    free(compressed_data);
    free(padded_json);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_compress_json_result_null_data);
    RUN_TEST(test_compress_json_result_zero_size);
    RUN_TEST(test_compress_json_result_null_compressed_data);
    RUN_TEST(test_compress_json_result_null_compressed_size);
    RUN_TEST(test_compress_json_result_success);
    RUN_TEST(test_compress_json_result_small_json);
    RUN_TEST(test_compress_json_result_large_json);

    return UNITY_END();
}