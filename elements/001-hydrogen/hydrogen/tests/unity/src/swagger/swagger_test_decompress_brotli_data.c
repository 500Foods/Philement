/*
 * Unity Test File: decompress_brotli_data Function Tests
 * This file contains unit tests for the decompress_brotli_data() function
 * from src/swagger/swagger.c
 *
 * Note: Comprehensive decompression testing is handled by the integration test (test_22_swagger.sh)
 * which tests actual file serving with real compressed payloads. These unit tests focus on
 * parameter validation and error handling.
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers
#include <src/swagger/swagger.h>

// Forward declaration for the function being tested
bool decompress_brotli_data(const uint8_t *compressed_data, size_t compressed_size,
                           uint8_t **decompressed_data, size_t *decompressed_size);

// Function prototypes for test functions
void test_decompress_brotli_data_null_compressed_data(void);
void test_decompress_brotli_data_null_decompressed_data(void);
void test_decompress_brotli_data_null_decompressed_size(void);
void test_decompress_brotli_data_invalid_compressed_data(void);

void setUp(void) {
    // No setup needed for these tests
}

void tearDown(void) {
    // No teardown needed for these tests
}

//=============================================================================
// Parameter Validation Tests
//=============================================================================

void test_decompress_brotli_data_null_compressed_data(void) {
    uint8_t *decompressed = NULL;
    size_t decompressed_size = 0;
    
    bool result = decompress_brotli_data(NULL, 100, &decompressed, &decompressed_size);
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(decompressed);
}

void test_decompress_brotli_data_null_decompressed_data(void) {
    uint8_t compressed[] = {0x0b, 0x00, 0x80};
    size_t decompressed_size = 0;
    
    bool result = decompress_brotli_data(compressed, sizeof(compressed), NULL, &decompressed_size);
    
    TEST_ASSERT_FALSE(result);
}

void test_decompress_brotli_data_null_decompressed_size(void) {
    uint8_t compressed[] = {0x0b, 0x00, 0x80};
    uint8_t *decompressed = NULL;
    
    bool result = decompress_brotli_data(compressed, sizeof(compressed), &decompressed, NULL);
    
    TEST_ASSERT_FALSE(result);
}

//=============================================================================
// Valid Decompression Tests
//=============================================================================

// Note: Actual decompression of real compressed data is tested via test_22_swagger.sh
// which runs the server and requests files from clients without brotli support.
// These unit tests focus on parameter validation and error handling only.

void test_decompress_brotli_data_invalid_compressed_data(void) {
    // Invalid brotli data (random bytes)
    uint8_t invalid_data[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t *decompressed = NULL;
    size_t decompressed_size = 0;
    
    bool result = decompress_brotli_data(invalid_data, sizeof(invalid_data), &decompressed, &decompressed_size);
    
    // Should fail gracefully
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(decompressed);
}

//=============================================================================
// Test Main Function
//=============================================================================

int main(void) {
    UNITY_BEGIN();
    
    // Parameter validation tests
    RUN_TEST(test_decompress_brotli_data_null_compressed_data);
    RUN_TEST(test_decompress_brotli_data_null_decompressed_data);
    RUN_TEST(test_decompress_brotli_data_null_decompressed_size);
    
    // Error handling tests
    RUN_TEST(test_decompress_brotli_data_invalid_compressed_data);
    
    return UNITY_END();
}