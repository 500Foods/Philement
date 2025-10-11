/*
 * Unity Test File: initialize_payload_cache Function Tests
 * This file contains unit tests for the initialize_payload_cache() function
 * from src/payload/payload_cache.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Forward declaration for the function being tested
bool initialize_payload_cache(void);
void cleanup_payload_cache(void);
bool is_payload_cache_available(void);

// Test function prototypes
void setUp(void);
void tearDown(void);
void test_initialize_payload_cache_basic(void);
void test_initialize_payload_cache_multiple_calls(void);
void test_initialize_payload_cache_after_cleanup(void);

void setUp(void) {
    // Clean up any existing state
    cleanup_payload_cache();
}

void tearDown(void) {
    // Clean up after each test
    cleanup_payload_cache();
}

void test_initialize_payload_cache_basic(void) {
    bool result = initialize_payload_cache();
    TEST_ASSERT_TRUE(result);

    // Verify cache is initialized (but not available since no payload loaded)
    TEST_ASSERT_FALSE(is_payload_cache_available());
}

void test_initialize_payload_cache_multiple_calls(void) {
    // First call should succeed
    bool result1 = initialize_payload_cache();
    TEST_ASSERT_TRUE(result1);

    // Second call should also succeed (idempotent)
    bool result2 = initialize_payload_cache();
    TEST_ASSERT_TRUE(result2);

    // Cache should be initialized but not available (no payload loaded)
    TEST_ASSERT_FALSE(is_payload_cache_available());
}

void test_initialize_payload_cache_after_cleanup(void) {
    // Initialize first
    bool init_result = initialize_payload_cache();
    TEST_ASSERT_TRUE(init_result);
    TEST_ASSERT_FALSE(is_payload_cache_available()); // Not available until payload loaded

    // Clean up
    cleanup_payload_cache();
    TEST_ASSERT_FALSE(is_payload_cache_available());

    // Re-initialize should work
    bool reinit_result = initialize_payload_cache();
    TEST_ASSERT_TRUE(reinit_result);
    TEST_ASSERT_FALSE(is_payload_cache_available()); // Still not available
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_initialize_payload_cache_basic);
    RUN_TEST(test_initialize_payload_cache_multiple_calls);
    RUN_TEST(test_initialize_payload_cache_after_cleanup);

    return UNITY_END();
}