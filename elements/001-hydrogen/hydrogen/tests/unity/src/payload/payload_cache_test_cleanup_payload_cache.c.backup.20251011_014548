/*
 * Unity Test File: cleanup_payload_cache Function Tests
 * This file contains unit tests for the cleanup_payload_cache() function
 * from src/payload/payload_cache.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Forward declaration for the function being tested
void cleanup_payload_cache(void);
bool initialize_payload_cache(void);
bool is_payload_cache_available(void);

// Extern declaration for global cache (for testing)
extern PayloadCache global_payload_cache;

// Test function prototypes
void setUp(void);
void tearDown(void);
void test_cleanup_payload_cache_empty_cache(void);
void test_cleanup_payload_cache_initialized_only(void);
void test_cleanup_payload_cache_with_files(void);
void test_cleanup_payload_cache_multiple_calls(void);

void setUp(void) {
    // Clean up any existing state
    cleanup_payload_cache();
}

void tearDown(void) {
    // Clean up after each test
    cleanup_payload_cache();
}

void test_cleanup_payload_cache_empty_cache(void) {
    // Clean up empty cache should not crash
    cleanup_payload_cache();
    TEST_ASSERT_TRUE(true); // If we reach here, it didn't crash
}

void test_cleanup_payload_cache_initialized_only(void) {
    // Initialize cache
    initialize_payload_cache();

    // Clean up
    cleanup_payload_cache();

    // Verify cache is reset
    TEST_ASSERT_FALSE(is_payload_cache_available());
}

void test_cleanup_payload_cache_with_files(void) {
    // Initialize cache
    initialize_payload_cache();

    // Manually add some files to simulate loaded cache
    global_payload_cache.num_files = 2;
    global_payload_cache.capacity = 16;
    global_payload_cache.files = calloc(16, sizeof(PayloadFile));

    // Add some dummy files
    global_payload_cache.files[0].name = strdup("test1.txt");
    global_payload_cache.files[0].data = malloc(100);
    global_payload_cache.files[0].size = 100;

    global_payload_cache.files[1].name = strdup("test2.txt");
    global_payload_cache.files[1].data = malloc(200);
    global_payload_cache.files[1].size = 200;

    // Clean up should free everything
    cleanup_payload_cache();

    // Verify cache is reset
    TEST_ASSERT_FALSE(is_payload_cache_available());
}

void test_cleanup_payload_cache_multiple_calls(void) {
    // Initialize and load some data
    initialize_payload_cache();
    global_payload_cache.num_files = 1;
    global_payload_cache.capacity = 16;
    global_payload_cache.files = calloc(16, sizeof(PayloadFile));
    global_payload_cache.files[0].name = strdup("test.txt");

    // First cleanup
    cleanup_payload_cache();
    TEST_ASSERT_FALSE(is_payload_cache_available());

    // Second cleanup should be safe
    cleanup_payload_cache();
    TEST_ASSERT_FALSE(is_payload_cache_available());
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_cleanup_payload_cache_empty_cache);
    RUN_TEST(test_cleanup_payload_cache_initialized_only);
    RUN_TEST(test_cleanup_payload_cache_with_files);
    RUN_TEST(test_cleanup_payload_cache_multiple_calls);

    return UNITY_END();
}