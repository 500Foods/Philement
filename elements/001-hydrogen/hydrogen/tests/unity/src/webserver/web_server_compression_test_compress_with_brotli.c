/*
 * Unity Test File: Web Server Compression - Compress With Brotli Test
 * This file contains unit tests for compress_with_brotli() function
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/webserver/web_server_compression.h>

// Standard library includes
#include <stdlib.h>
#include <string.h>

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test functions
static void test_compress_with_brotli_null_input(void) {
    // Test null input parameter
    uint8_t *output = NULL;
    size_t output_size = 0;
    TEST_ASSERT_FALSE(compress_with_brotli(NULL, 10, &output, &output_size));
}

static void test_compress_with_brotli_zero_input_size(void) {
    // Test zero input size
    const uint8_t input[] = "test";
    uint8_t *output = NULL;
    size_t output_size = 0;
    TEST_ASSERT_FALSE(compress_with_brotli(input, 0, &output, &output_size));
}

static void test_compress_with_brotli_null_output(void) {
    // Test null output parameter
    const uint8_t input[] = "test";
    size_t output_size = 0;
    TEST_ASSERT_FALSE(compress_with_brotli(input, sizeof(input), NULL, &output_size));
}

static void test_compress_with_brotli_null_output_size(void) {
    // Test null output_size parameter
    const uint8_t input[] = "test";
    uint8_t *output = NULL;
    TEST_ASSERT_FALSE(compress_with_brotli(input, sizeof(input), &output, NULL));
}

static void test_compress_with_brotli_small_data(void) {
    // Test compression with small data
    const uint8_t input[] = "Hello World";
    uint8_t *output = NULL;
    size_t output_size = 0;

    TEST_ASSERT_TRUE(compress_with_brotli(input, strlen((char*)input), &output, &output_size));
    TEST_ASSERT_NOT_NULL(output);
    TEST_ASSERT_GREATER_THAN(0, output_size);

    // Verify compressed data is different from input
    TEST_ASSERT_TRUE(output_size != strlen((char*)input));

    free(output);
}

static void test_compress_with_brotli_empty_string(void) {
    // Test compression with empty string (zero size)
    const uint8_t input[] = "";
    uint8_t *output = NULL;
    size_t output_size = 0;

    TEST_ASSERT_FALSE(compress_with_brotli(input, 0, &output, &output_size));
}

static void test_compress_with_brotli_large_data(void) {
    // Test compression with larger data
    const char *test_data = "This is a test string for Brotli compression. It contains some repetitive text that should compress well. This is a test string for Brotli compression.";
    const uint8_t *input = (const uint8_t *)test_data;
    uint8_t *output = NULL;
    size_t output_size = 0;

    TEST_ASSERT_TRUE(compress_with_brotli(input, strlen(test_data), &output, &output_size));
    TEST_ASSERT_NOT_NULL(output);
    TEST_ASSERT_GREATER_THAN(0, output_size);

    // Compressed size should be smaller than original for compressible data
    TEST_ASSERT_LESS_THAN(strlen(test_data), output_size);

    free(output);
}

static void test_compress_with_brotli_binary_data(void) {
    // Test compression with binary data
    const uint8_t input[] = {0x00, 0x01, 0x02, 0x03, 0xFF, 0xFE, 0xFD, 0xFC};
    uint8_t *output = NULL;
    size_t output_size = 0;

    TEST_ASSERT_TRUE(compress_with_brotli(input, sizeof(input), &output, &output_size));
    TEST_ASSERT_NOT_NULL(output);
    TEST_ASSERT_GREATER_THAN(0, output_size);

    free(output);
}

static void test_compress_with_brotli_repetitive_data(void) {
    // Test compression with highly repetitive data (should compress very well)
    uint8_t input[1024];
    memset(input, 'A', sizeof(input)); // Fill with repetitive data

    uint8_t *output = NULL;
    size_t output_size = 0;

    TEST_ASSERT_TRUE(compress_with_brotli(input, sizeof(input), &output, &output_size));
    TEST_ASSERT_NOT_NULL(output);
    TEST_ASSERT_GREATER_THAN(0, output_size);

    // Should compress much better than original
    TEST_ASSERT_LESS_THAN(sizeof(input) / 10, output_size);

    free(output);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_compress_with_brotli_null_input);
    RUN_TEST(test_compress_with_brotli_zero_input_size);
    RUN_TEST(test_compress_with_brotli_null_output);
    RUN_TEST(test_compress_with_brotli_null_output_size);
    RUN_TEST(test_compress_with_brotli_small_data);
    RUN_TEST(test_compress_with_brotli_empty_string);
    RUN_TEST(test_compress_with_brotli_large_data);
    RUN_TEST(test_compress_with_brotli_binary_data);
    RUN_TEST(test_compress_with_brotli_repetitive_data);

    return UNITY_END();
}