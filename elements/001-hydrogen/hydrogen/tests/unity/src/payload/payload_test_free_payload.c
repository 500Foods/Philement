/*
 * Unity Test File: free_payload Function Tests
 * This file contains unit tests for the free_payload() function
 * from src/payload/payload.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Forward declaration for the function being tested
void free_payload(PayloadData *payload);
void test_free_payload_null_payload(void);
void test_free_payload_empty_payload(void);
void test_free_payload_with_data(void);
void test_free_payload_multiple_calls(void);
void test_free_payload_large_data(void);

void setUp(void) {
    // No setup needed for free_payload tests
}

void tearDown(void) {
    // No teardown needed for free_payload tests
}

void test_free_payload_null_payload(void) {
    // Should not crash with NULL pointer
    free_payload(NULL);
    TEST_ASSERT_TRUE(true); // If we reach here, it didn't crash
}

void test_free_payload_empty_payload(void) {
    PayloadData empty_payload = {0};
    free_payload(&empty_payload);
    TEST_ASSERT_NULL(empty_payload.data);
    TEST_ASSERT_EQUAL(0, empty_payload.size);
    TEST_ASSERT_FALSE(empty_payload.is_compressed);
}

void test_free_payload_with_data(void) {
    PayloadData test_payload_local = {0};
    test_payload_local.data = malloc(100);
    test_payload_local.size = 100;
    test_payload_local.is_compressed = true;
    
    free_payload(&test_payload_local);
    
    TEST_ASSERT_NULL(test_payload_local.data);
    TEST_ASSERT_EQUAL(0, test_payload_local.size);
    TEST_ASSERT_FALSE(test_payload_local.is_compressed);
}

void test_free_payload_multiple_calls(void) {
    PayloadData test_payload = {0};
    test_payload.data = malloc(50);
    test_payload.size = 50;
    test_payload.is_compressed = false;
    
    // First free should work normally
    free_payload(&test_payload);
    TEST_ASSERT_NULL(test_payload.data);
    TEST_ASSERT_EQUAL(0, test_payload.size);
    TEST_ASSERT_FALSE(test_payload.is_compressed);
    
    // Second free should be safe (no crash)
    free_payload(&test_payload);
    TEST_ASSERT_NULL(test_payload.data);
    TEST_ASSERT_EQUAL(0, test_payload.size);
    TEST_ASSERT_FALSE(test_payload.is_compressed);
}

void test_free_payload_large_data(void) {
    PayloadData large_payload = {0};
    large_payload.data = malloc(10000);
    large_payload.size = 10000;
    large_payload.is_compressed = true;
    
    free_payload(&large_payload);
    
    TEST_ASSERT_NULL(large_payload.data);
    TEST_ASSERT_EQUAL(0, large_payload.size);
    TEST_ASSERT_FALSE(large_payload.is_compressed);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_free_payload_null_payload);
    RUN_TEST(test_free_payload_empty_payload);
    RUN_TEST(test_free_payload_with_data);
    RUN_TEST(test_free_payload_multiple_calls);
    RUN_TEST(test_free_payload_large_data);
    
    return UNITY_END();
}
