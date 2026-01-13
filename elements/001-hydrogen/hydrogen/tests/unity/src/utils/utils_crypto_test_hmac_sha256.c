/*
 * Unity Test File: utils_hmac_sha256()
 * This file contains unit tests for HMAC-SHA256 operations
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/utils/utils_crypto.h>
#include <string.h>
#include <openssl/sha.h>

// Forward declaration for function being tested
unsigned char* utils_hmac_sha256(const unsigned char* data, size_t data_len,
                                  const char* key, size_t key_len,
                                  unsigned int* output_len);

// Function prototypes for test functions
void test_hmac_sha256_basic(void);
void test_hmac_sha256_empty_data(void);
void test_hmac_sha256_empty_key(void);
void test_hmac_sha256_deterministic(void);
void test_hmac_sha256_different_keys(void);
void test_hmac_sha256_different_data(void);
void test_hmac_sha256_null_data(void);
void test_hmac_sha256_null_key(void);
void test_hmac_sha256_null_output_len(void);
void test_hmac_sha256_binary_data(void);
void test_hmac_sha256_binary_key(void);
void test_hmac_sha256_long_key(void);
void test_hmac_sha256_large_data(void);
void test_hmac_sha256_output_length_constant(void);
void test_hmac_sha256_special_characters_data(void);
void test_hmac_sha256_special_characters_key(void);
void test_hmac_sha256_utf8_data(void);
void test_hmac_sha256_output_is_raw_bytes(void);
void test_hmac_sha256_zero_length_data(void);
void test_hmac_sha256_zero_length_key(void);

void setUp(void) {
    // No setup needed
}

void tearDown(void) {
    // No teardown needed
}

// Test basic functionality
void test_hmac_sha256_basic(void) {
    const char* data = "message";
    const char* key = "secret";
    unsigned int output_len = 0;
    
    unsigned char* result = utils_hmac_sha256((const unsigned char*)data, strlen(data),
                                              key, strlen(key),
                                              &output_len);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(SHA256_DIGEST_LENGTH, output_len);
    
    free(result);
}

void test_hmac_sha256_empty_data(void) {
    const char* data = "";
    const char* key = "secret";
    unsigned int output_len = 0;
    
    unsigned char* result = utils_hmac_sha256((const unsigned char*)data, strlen(data),
                                              key, strlen(key),
                                              &output_len);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(SHA256_DIGEST_LENGTH, output_len);
    
    free(result);
}

void test_hmac_sha256_empty_key(void) {
    const char* data = "message";
    const char* key = "";
    unsigned int output_len = 0;
    
    unsigned char* result = utils_hmac_sha256((const unsigned char*)data, strlen(data),
                                              key, strlen(key),
                                              &output_len);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(SHA256_DIGEST_LENGTH, output_len);
    
    free(result);
}

// Test deterministic behavior
void test_hmac_sha256_deterministic(void) {
    const char* data = "test message";
    const char* key = "test key";
    unsigned int output_len1 = 0;
    unsigned int output_len2 = 0;
    
    unsigned char* result1 = utils_hmac_sha256((const unsigned char*)data, strlen(data),
                                               key, strlen(key),
                                               &output_len1);
    unsigned char* result2 = utils_hmac_sha256((const unsigned char*)data, strlen(data),
                                               key, strlen(key),
                                               &output_len2);
    
    TEST_ASSERT_NOT_NULL(result1);
    TEST_ASSERT_NOT_NULL(result2);
    TEST_ASSERT_EQUAL(output_len1, output_len2);
    
    // Same inputs should produce same HMAC
    TEST_ASSERT_EQUAL_MEMORY(result1, result2, output_len1);
    
    free(result1);
    free(result2);
}

// Test different keys produce different HMACs
void test_hmac_sha256_different_keys(void) {
    const char* data = "message";
    const char* key1 = "key1";
    const char* key2 = "key2";
    unsigned int output_len1 = 0;
    unsigned int output_len2 = 0;
    
    unsigned char* result1 = utils_hmac_sha256((const unsigned char*)data, strlen(data),
                                               key1, strlen(key1),
                                               &output_len1);
    unsigned char* result2 = utils_hmac_sha256((const unsigned char*)data, strlen(data),
                                               key2, strlen(key2),
                                               &output_len2);
    
    TEST_ASSERT_NOT_NULL(result1);
    TEST_ASSERT_NOT_NULL(result2);
    
    // Different keys should produce different HMACs
    bool different = false;
    for (unsigned int i = 0; i < output_len1; i++) {
        if (result1[i] != result2[i]) {
            different = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(different);
    
    free(result1);
    free(result2);
}

// Test different data produces different HMACs
void test_hmac_sha256_different_data(void) {
    const char* data1 = "message1";
    const char* data2 = "message2";
    const char* key = "secret";
    unsigned int output_len1 = 0;
    unsigned int output_len2 = 0;
    
    unsigned char* result1 = utils_hmac_sha256((const unsigned char*)data1, strlen(data1),
                                               key, strlen(key),
                                               &output_len1);
    unsigned char* result2 = utils_hmac_sha256((const unsigned char*)data2, strlen(data2),
                                               key, strlen(key),
                                               &output_len2);
    
    TEST_ASSERT_NOT_NULL(result1);
    TEST_ASSERT_NOT_NULL(result2);
    
    // Different data should produce different HMACs
    bool different = false;
    for (unsigned int i = 0; i < output_len1; i++) {
        if (result1[i] != result2[i]) {
            different = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(different);
    
    free(result1);
    free(result2);
}

// Test null parameter handling
void test_hmac_sha256_null_data(void) {
    const char* key = "secret";
    unsigned int output_len = 0;
    
    unsigned char* result = utils_hmac_sha256(NULL, 10, key, strlen(key), &output_len);
    TEST_ASSERT_NULL(result);
}

void test_hmac_sha256_null_key(void) {
    const char* data = "message";
    unsigned int output_len = 0;
    
    unsigned char* result = utils_hmac_sha256((const unsigned char*)data, strlen(data),
                                              NULL, 10, &output_len);
    TEST_ASSERT_NULL(result);
}

void test_hmac_sha256_null_output_len(void) {
    const char* data = "message";
    const char* key = "secret";
    
    unsigned char* result = utils_hmac_sha256((const unsigned char*)data, strlen(data),
                                              key, strlen(key), NULL);
    TEST_ASSERT_NULL(result);
}

// Test binary data
void test_hmac_sha256_binary_data(void) {
    unsigned char data[] = {0x00, 0x01, 0x02, 0x03, 0xFF, 0xFE};
    const char* key = "binary-key";
    unsigned int output_len = 0;
    
    unsigned char* result = utils_hmac_sha256(data, sizeof(data),
                                              key, strlen(key),
                                              &output_len);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(SHA256_DIGEST_LENGTH, output_len);
    
    free(result);
}

void test_hmac_sha256_binary_key(void) {
    const char* data = "message";
    unsigned char key[] = {0x00, 0x01, 0x02, 0x03, 0xFF, 0xFE};
    unsigned int output_len = 0;
    
    unsigned char* result = utils_hmac_sha256((const unsigned char*)data, strlen(data),
                                              (const char*)key, sizeof(key),
                                              &output_len);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(SHA256_DIGEST_LENGTH, output_len);
    
    free(result);
}

// Test long key
void test_hmac_sha256_long_key(void) {
    const char* data = "message";
    char long_key[256];
    memset(long_key, 'K', sizeof(long_key));
    unsigned int output_len = 0;
    
    unsigned char* result = utils_hmac_sha256((const unsigned char*)data, strlen(data),
                                              long_key, sizeof(long_key),
                                              &output_len);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(SHA256_DIGEST_LENGTH, output_len);
    
    free(result);
}

// Test large data
void test_hmac_sha256_large_data(void) {
    size_t data_size = 10000;
    unsigned char* large_data = malloc(data_size);
    TEST_ASSERT_NOT_NULL(large_data);
    
    // Fill with pattern
    for (size_t i = 0; i < data_size; i++) {
        large_data[i] = (unsigned char)(i % 256);
    }
    
    const char* key = "secret";
    unsigned int output_len = 0;
    
    unsigned char* result = utils_hmac_sha256(large_data, data_size,
                                              key, strlen(key),
                                              &output_len);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(SHA256_DIGEST_LENGTH, output_len);
    
    free(large_data);
    free(result);
}

// Test output length is always SHA256_DIGEST_LENGTH
void test_hmac_sha256_output_length_constant(void) {
    const char* messages[] = {"short", "medium length message", "very long message with lots of data"};
    const char* key = "secret";
    
    for (size_t i = 0; i < 3; i++) {
        unsigned int output_len = 0;
        unsigned char* result = utils_hmac_sha256((const unsigned char*)messages[i], strlen(messages[i]),
                                                  key, strlen(key),
                                                  &output_len);
        
        TEST_ASSERT_NOT_NULL(result);
        TEST_ASSERT_EQUAL(SHA256_DIGEST_LENGTH, output_len);
        
        free(result);
    }
}

// Test special characters in data
void test_hmac_sha256_special_characters_data(void) {
    const char* data = "!@#$%^&*()_+-=[]{}|;':\"<>?,./";
    const char* key = "secret";
    unsigned int output_len = 0;
    
    unsigned char* result = utils_hmac_sha256((const unsigned char*)data, strlen(data),
                                              key, strlen(key),
                                              &output_len);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(SHA256_DIGEST_LENGTH, output_len);
    
    free(result);
}

// Test special characters in key
void test_hmac_sha256_special_characters_key(void) {
    const char* data = "message";
    const char* key = "!@#$%^&*()_+-=[]{}|;':\"<>?,./";
    unsigned int output_len = 0;
    
    unsigned char* result = utils_hmac_sha256((const unsigned char*)data, strlen(data),
                                              key, strlen(key),
                                              &output_len);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(SHA256_DIGEST_LENGTH, output_len);
    
    free(result);
}

// Test UTF-8 data
void test_hmac_sha256_utf8_data(void) {
    // UTF-8 encoding of "こんにちは" (hello in Japanese)
    unsigned char utf8[] = {0xE3, 0x81, 0x93, 0xE3, 0x82, 0x93,
                           0xE3, 0x81, 0xAB, 0xE3, 0x81, 0xA1,
                           0xE3, 0x81, 0xAF};
    const char* key = "secret";
    unsigned int output_len = 0;
    
    unsigned char* result = utils_hmac_sha256(utf8, sizeof(utf8),
                                              key, strlen(key),
                                              &output_len);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(SHA256_DIGEST_LENGTH, output_len);
    
    free(result);
}

// Test that output is raw bytes (not base64 encoded)
void test_hmac_sha256_output_is_raw_bytes(void) {
    const char* data = "message";
    const char* key = "secret";
    unsigned int output_len = 0;
    
    unsigned char* result = utils_hmac_sha256((const unsigned char*)data, strlen(data),
                                              key, strlen(key),
                                              &output_len);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(SHA256_DIGEST_LENGTH, output_len);
    
    // Result should be raw bytes, may contain any value 0-255
    // Just verify we got the expected length
    TEST_ASSERT_EQUAL(32, output_len);
    
    free(result);
}

// Test zero-length data
void test_hmac_sha256_zero_length_data(void) {
    const char* key = "secret";
    unsigned int output_len = 0;
    
    unsigned char* result = utils_hmac_sha256((const unsigned char*)"", 0,
                                              key, strlen(key),
                                              &output_len);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(SHA256_DIGEST_LENGTH, output_len);
    
    free(result);
}

// Test zero-length key
void test_hmac_sha256_zero_length_key(void) {
    const char* data = "message";
    unsigned int output_len = 0;
    
    unsigned char* result = utils_hmac_sha256((const unsigned char*)data, strlen(data),
                                              "", 0,
                                              &output_len);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(SHA256_DIGEST_LENGTH, output_len);
    
    free(result);
}

int main(void) {
    UNITY_BEGIN();
    
    // Basic functionality tests
    RUN_TEST(test_hmac_sha256_basic);
    RUN_TEST(test_hmac_sha256_empty_data);
    RUN_TEST(test_hmac_sha256_empty_key);
    RUN_TEST(test_hmac_sha256_deterministic);
    
    // Different inputs tests
    RUN_TEST(test_hmac_sha256_different_keys);
    RUN_TEST(test_hmac_sha256_different_data);
    
    // Parameter validation tests
    RUN_TEST(test_hmac_sha256_null_data);
    RUN_TEST(test_hmac_sha256_null_key);
    RUN_TEST(test_hmac_sha256_null_output_len);
    
    // Binary data tests
    RUN_TEST(test_hmac_sha256_binary_data);
    RUN_TEST(test_hmac_sha256_binary_key);
    
    // Length variation tests
    RUN_TEST(test_hmac_sha256_long_key);
    RUN_TEST(test_hmac_sha256_large_data);
    RUN_TEST(test_hmac_sha256_zero_length_data);
    RUN_TEST(test_hmac_sha256_zero_length_key);
    
    // Output tests
    RUN_TEST(test_hmac_sha256_output_length_constant);
    RUN_TEST(test_hmac_sha256_output_is_raw_bytes);
    
    // Special character tests
    RUN_TEST(test_hmac_sha256_special_characters_data);
    RUN_TEST(test_hmac_sha256_special_characters_key);
    RUN_TEST(test_hmac_sha256_utf8_data);
    
    return UNITY_END();
}
