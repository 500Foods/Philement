/*
 * Unity Test File: utils_random_bytes()
 * This file contains unit tests for cryptographically secure random byte generation
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/utils/utils_crypto.h>
#include <string.h>

// Forward declaration for function being tested
bool utils_random_bytes(unsigned char* buffer, size_t length);


// Function prototypes for test functions
void test_random_bytes_basic(void);
void test_random_bytes_small_buffer(void);
void test_random_bytes_medium_buffer(void);
void test_random_bytes_large_buffer(void);
void test_random_bytes_null_buffer(void);
void test_random_bytes_zero_length(void);
void test_random_bytes_different_calls_produce_different_data(void);
void test_random_bytes_multiple_calls(void);
void test_random_bytes_length_1(void);
void test_random_bytes_length_16(void);
void test_random_bytes_length_32(void);
void test_random_bytes_length_64(void);
void test_random_bytes_length_128(void);
void test_random_bytes_length_256(void);
void test_random_bytes_value_distribution(void);
void test_random_bytes_fills_entire_buffer(void);
void test_random_bytes_sequential_calls(void);
void test_random_bytes_token_generation(void);
void test_random_bytes_session_id_generation(void);
void test_random_bytes_nonce_generation(void);
void test_random_bytes_key_generation(void);
void test_random_bytes_very_large_buffer(void);
void test_random_bytes_respects_buffer_bounds(void);
void test_random_bytes_basic(void);
void test_random_bytes_small_buffer(void);
void test_random_bytes_medium_buffer(void);
void test_random_bytes_large_buffer(void);
void test_random_bytes_null_buffer(void);
void test_random_bytes_zero_length(void);
void test_random_bytes_different_calls_produce_different_data(void);
void test_random_bytes_multiple_calls(void);
void test_random_bytes_length_1(void);
void test_random_bytes_length_16(void);
void test_random_bytes_length_32(void);
void test_random_bytes_length_64(void);
void test_random_bytes_length_128(void);
void test_random_bytes_length_256(void);
void test_random_bytes_value_distribution(void);
void test_random_bytes_fills_entire_buffer(void);
void test_random_bytes_sequential_calls(void);
void test_random_bytes_token_generation(void);
void test_random_bytes_session_id_generation(void);
void test_random_bytes_nonce_generation(void);
void test_random_bytes_key_generation(void);
void test_random_bytes_very_large_buffer(void);
void test_random_bytes_respects_buffer_bounds(void);

void setUp(void) {
    // No setup needed
}


// Function prototypes for test functions
void test_random_bytes_basic(void);
void test_random_bytes_small_buffer(void);
void test_random_bytes_medium_buffer(void);
void test_random_bytes_large_buffer(void);
void test_random_bytes_null_buffer(void);
void test_random_bytes_zero_length(void);
void test_random_bytes_different_calls_produce_different_data(void);
void test_random_bytes_multiple_calls(void);
void test_random_bytes_length_1(void);
void test_random_bytes_length_16(void);
void test_random_bytes_length_32(void);
void test_random_bytes_length_64(void);
void test_random_bytes_length_128(void);
void test_random_bytes_length_256(void);
void test_random_bytes_value_distribution(void);
void test_random_bytes_fills_entire_buffer(void);
void test_random_bytes_sequential_calls(void);
void test_random_bytes_token_generation(void);
void test_random_bytes_session_id_generation(void);
void test_random_bytes_nonce_generation(void);
void test_random_bytes_key_generation(void);
void test_random_bytes_very_large_buffer(void);
void test_random_bytes_respects_buffer_bounds(void);

void tearDown(void) {
    // No teardown needed
}

// Test basic functionality
void test_random_bytes_basic(void) {
    unsigned char buffer[32];
    memset(buffer, 0, sizeof(buffer));
    
    bool result = utils_random_bytes(buffer, sizeof(buffer));
    
    TEST_ASSERT_TRUE(result);
    
    // Verify buffer was modified (extremely unlikely to be all zeros)
    bool has_nonzero = false;
    for (size_t i = 0; i < sizeof(buffer); i++) {
        if (buffer[i] != 0) {
            has_nonzero = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(has_nonzero);
}

void test_random_bytes_small_buffer(void) {
    unsigned char buffer[1];
    
    bool result = utils_random_bytes(buffer, sizeof(buffer));
    TEST_ASSERT_TRUE(result);
}

void test_random_bytes_medium_buffer(void) {
    unsigned char buffer[64];
    
    bool result = utils_random_bytes(buffer, sizeof(buffer));
    TEST_ASSERT_TRUE(result);
}

void test_random_bytes_large_buffer(void) {
    size_t size = 1024;
    unsigned char* buffer = malloc(size);
    TEST_ASSERT_NOT_NULL(buffer);
    
    bool result = utils_random_bytes(buffer, size);
    TEST_ASSERT_TRUE(result);
    
    free(buffer);
}

// Test null parameter handling
void test_random_bytes_null_buffer(void) {
    bool result = utils_random_bytes(NULL, 32);
    TEST_ASSERT_FALSE(result);
}

void test_random_bytes_zero_length(void) {
    unsigned char buffer[32];
    bool result = utils_random_bytes(buffer, 0);
    TEST_ASSERT_FALSE(result);
}

// Test randomness properties
void test_random_bytes_different_calls_produce_different_data(void) {
    unsigned char buffer1[32];
    unsigned char buffer2[32];
    
    bool result1 = utils_random_bytes(buffer1, sizeof(buffer1));
    bool result2 = utils_random_bytes(buffer2, sizeof(buffer2));
    
    TEST_ASSERT_TRUE(result1);
    TEST_ASSERT_TRUE(result2);
    
    // Two calls should produce different random data
    bool different = false;
    for (size_t i = 0; i < sizeof(buffer1); i++) {
        if (buffer1[i] != buffer2[i]) {
            different = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(different);
}

void test_random_bytes_multiple_calls(void) {
    unsigned char buffers[10][32];
    
    // Generate 10 random buffers
    for (int i = 0; i < 10; i++) {
        bool result = utils_random_bytes(buffers[i], sizeof(buffers[i]));
        TEST_ASSERT_TRUE(result);
    }
    
    // Verify they're all different from each other
    for (int i = 0; i < 10; i++) {
        for (int j = i + 1; j < 10; j++) {
            bool different = false;
            for (size_t k = 0; k < sizeof(buffers[i]); k++) {
                if (buffers[i][k] != buffers[j][k]) {
                    different = true;
                    break;
                }
            }
            TEST_ASSERT_TRUE(different);
        }
    }
}

// Test various buffer sizes
void test_random_bytes_length_1(void) {
    unsigned char buffer[1];
    bool result = utils_random_bytes(buffer, 1);
    TEST_ASSERT_TRUE(result);
}

void test_random_bytes_length_16(void) {
    unsigned char buffer[16];
    bool result = utils_random_bytes(buffer, 16);
    TEST_ASSERT_TRUE(result);
}

void test_random_bytes_length_32(void) {
    unsigned char buffer[32];
    bool result = utils_random_bytes(buffer, 32);
    TEST_ASSERT_TRUE(result);
}

void test_random_bytes_length_64(void) {
    unsigned char buffer[64];
    bool result = utils_random_bytes(buffer, 64);
    TEST_ASSERT_TRUE(result);
}

void test_random_bytes_length_128(void) {
    unsigned char buffer[128];
    bool result = utils_random_bytes(buffer, 128);
    TEST_ASSERT_TRUE(result);
}

void test_random_bytes_length_256(void) {
    unsigned char buffer[256];
    bool result = utils_random_bytes(buffer, 256);
    TEST_ASSERT_TRUE(result);
}

// Test that random bytes span full range
void test_random_bytes_value_distribution(void) {
    // Generate lots of random bytes to check distribution
    size_t size = 10000;
    unsigned char* buffer = malloc(size);
    TEST_ASSERT_NOT_NULL(buffer);
    
    bool result = utils_random_bytes(buffer, size);
    TEST_ASSERT_TRUE(result);
    
    // Count occurrences of different byte ranges
    int low_count = 0;    // 0x00-0x3F
    int mid_count = 0;    // 0x40-0xBF
    int high_count = 0;   // 0xC0-0xFF
    
    for (size_t i = 0; i < size; i++) {
        if (buffer[i] < 0x40) {
            low_count++;
        } else if (buffer[i] < 0xC0) {
            mid_count++;
        } else {
            high_count++;
        }
    }
    
    // Each range should have roughly 25%, 50%, 25% of values
    // We'll be lenient and check they're all > 10% (1000)
    TEST_ASSERT_GREATER_THAN(1000, low_count);
    TEST_ASSERT_GREATER_THAN(1000, mid_count);
    TEST_ASSERT_GREATER_THAN(1000, high_count);
    
    free(buffer);
}

// Test buffer is fully filled
void test_random_bytes_fills_entire_buffer(void) {
    unsigned char buffer[64];
    memset(buffer, 0, sizeof(buffer));
    
    bool result = utils_random_bytes(buffer, sizeof(buffer));
    TEST_ASSERT_TRUE(result);
    
    // Check that multiple bytes were changed (not just the first few)
    // Count how many bytes are non-zero across the buffer
    int nonzero_count = 0;
    for (size_t i = 0; i < sizeof(buffer); i++) {
        if (buffer[i] != 0) {
            nonzero_count++;
        }
    }
    
    // Should have changed most of the buffer (>50%)
    TEST_ASSERT_GREATER_THAN(32, nonzero_count);
}

// Test sequential calls work correctly
void test_random_bytes_sequential_calls(void) {
    unsigned char buffer1[32];
    unsigned char buffer2[32];
    unsigned char buffer3[32];
    
    bool result1 = utils_random_bytes(buffer1, sizeof(buffer1));
    bool result2 = utils_random_bytes(buffer2, sizeof(buffer2));
    bool result3 = utils_random_bytes(buffer3, sizeof(buffer3));
    
    TEST_ASSERT_TRUE(result1);
    TEST_ASSERT_TRUE(result2);
    TEST_ASSERT_TRUE(result3);
    
    // All three should be different
    TEST_ASSERT_NOT_EQUAL(0, memcmp(buffer1, buffer2, sizeof(buffer1)));
    TEST_ASSERT_NOT_EQUAL(0, memcmp(buffer2, buffer3, sizeof(buffer2)));
    TEST_ASSERT_NOT_EQUAL(0, memcmp(buffer1, buffer3, sizeof(buffer1)));
}

// Test typical use cases
void test_random_bytes_token_generation(void) {
    // Generate a typical random token (16 bytes)
    unsigned char token[16];
    bool result = utils_random_bytes(token, sizeof(token));
    TEST_ASSERT_TRUE(result);
}

void test_random_bytes_session_id_generation(void) {
    // Generate a typical session ID (32 bytes)
    unsigned char session_id[32];
    bool result = utils_random_bytes(session_id, sizeof(session_id));
    TEST_ASSERT_TRUE(result);
}

void test_random_bytes_nonce_generation(void) {
    // Generate a typical nonce (12 bytes for AES-GCM)
    unsigned char nonce[12];
    bool result = utils_random_bytes(nonce, sizeof(nonce));
    TEST_ASSERT_TRUE(result);
}

void test_random_bytes_key_generation(void) {
    // Generate a typical encryption key (32 bytes for AES-256)
    unsigned char key[32];
    bool result = utils_random_bytes(key, sizeof(key));
    TEST_ASSERT_TRUE(result);
}

// Test very large buffer
void test_random_bytes_very_large_buffer(void) {
    size_t size = 100000;  // 100KB
    unsigned char* buffer = malloc(size);
    TEST_ASSERT_NOT_NULL(buffer);
    
    bool result = utils_random_bytes(buffer, size);
    TEST_ASSERT_TRUE(result);
    
    // Sample check: verify different sections have non-zero bytes
    bool start_has_data = false;
    bool middle_has_data = false;
    bool end_has_data = false;
    
    for (size_t i = 0; i < 100; i++) {
        if (buffer[i] != 0) start_has_data = true;
        if (buffer[size/2 + i] != 0) middle_has_data = true;
        if (buffer[size - 100 + i] != 0) end_has_data = true;
    }
    
    TEST_ASSERT_TRUE(start_has_data);
    TEST_ASSERT_TRUE(middle_has_data);
    TEST_ASSERT_TRUE(end_has_data);
    
    free(buffer);
}

// Test that function doesn't write beyond buffer bounds
void test_random_bytes_respects_buffer_bounds(void) {
    unsigned char buffer[32 + 2];  // Add sentinel bytes
    buffer[0] = 0xAA;              // Sentinel before
    buffer[32 + 1] = 0xBB;         // Sentinel after
    
    bool result = utils_random_bytes(buffer + 1, 32);
    TEST_ASSERT_TRUE(result);
    
    // Check sentinels weren't modified
    TEST_ASSERT_EQUAL(0xAA, buffer[0]);
    TEST_ASSERT_EQUAL(0xBB, buffer[32 + 1]);
}

int main(void) {
    UNITY_BEGIN();
    
    // Basic functionality tests
    RUN_TEST(test_random_bytes_basic);
    RUN_TEST(test_random_bytes_small_buffer);
    RUN_TEST(test_random_bytes_medium_buffer);
    RUN_TEST(test_random_bytes_large_buffer);
    
    // Parameter validation tests
    RUN_TEST(test_random_bytes_null_buffer);
    RUN_TEST(test_random_bytes_zero_length);
    
    // Randomness property tests
    RUN_TEST(test_random_bytes_different_calls_produce_different_data);
    RUN_TEST(test_random_bytes_multiple_calls);
    RUN_TEST(test_random_bytes_sequential_calls);
    
    // Various buffer size tests
    RUN_TEST(test_random_bytes_length_1);
    RUN_TEST(test_random_bytes_length_16);
    RUN_TEST(test_random_bytes_length_32);
    RUN_TEST(test_random_bytes_length_64);
    RUN_TEST(test_random_bytes_length_128);
    RUN_TEST(test_random_bytes_length_256);
    
    // Distribution tests
    RUN_TEST(test_random_bytes_value_distribution);
    RUN_TEST(test_random_bytes_fills_entire_buffer);
    
    // Use case tests
    RUN_TEST(test_random_bytes_token_generation);
    RUN_TEST(test_random_bytes_session_id_generation);
    RUN_TEST(test_random_bytes_nonce_generation);
    RUN_TEST(test_random_bytes_key_generation);
    
    // Edge case tests
    RUN_TEST(test_random_bytes_very_large_buffer);
    RUN_TEST(test_random_bytes_respects_buffer_bounds);
    
    return UNITY_END();
}
