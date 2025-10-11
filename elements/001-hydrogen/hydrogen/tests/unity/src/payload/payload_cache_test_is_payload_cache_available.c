/*
 * Unity Test File: is_payload_cache_available Function Tests
 * This file contains unit tests for the is_payload_cache_available() function
 * from src/payload/payload_cache.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Forward declaration for the function being tested
bool is_payload_cache_available(void);
void cleanup_payload_cache(void);
bool initialize_payload_cache(void);

// Extern declaration for global cache (for testing)
extern PayloadCache global_payload_cache;

// Test function prototypes
void setUp(void);
void tearDown(void);
void test_is_payload_cache_available_not_initialized(void);
void test_is_payload_cache_available_initialized_only(void);
void test_is_payload_cache_available_fully_available(void);
void test_is_payload_cache_available_after_cleanup(void);

void setUp(void) {
    // Clean up any existing state
    cleanup_payload_cache();
}

void tearDown(void) {
    // Clean up after each test
    cleanup_payload_cache();
}

void test_is_payload_cache_available_not_initialized(void) {
    // Cache should not be available when not initialized
    TEST_ASSERT_FALSE(is_payload_cache_available());
}

void test_is_payload_cache_available_initialized_only(void) {
    // Initialize but don't load payload
    initialize_payload_cache();

    // Should be false because is_available is false
    TEST_ASSERT_FALSE(is_payload_cache_available());
}

void test_is_payload_cache_available_fully_available(void) {
    // Initialize cache
    initialize_payload_cache();

    // Manually set availability (simulating successful payload load)
    // Note: This is testing the logic, not the full load process
    global_payload_cache.is_available = true;

    TEST_ASSERT_TRUE(is_payload_cache_available());
}

void test_is_payload_cache_available_after_cleanup(void) {
    // Initialize and make available
    initialize_payload_cache();
    global_payload_cache.is_available = true;

    TEST_ASSERT_TRUE(is_payload_cache_available());

    // Clean up
    cleanup_payload_cache();

    TEST_ASSERT_FALSE(is_payload_cache_available());
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_is_payload_cache_available_not_initialized);
    RUN_TEST(test_is_payload_cache_available_initialized_only);
    RUN_TEST(test_is_payload_cache_available_fully_available);
    RUN_TEST(test_is_payload_cache_available_after_cleanup);

    return UNITY_END();
}