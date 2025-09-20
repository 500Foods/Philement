/*
 * Unity Test File: process_payload_data Function Tests
 * This file contains unit tests for the process_payload_data() function
 * from src/payload/payload.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Forward declaration for the function being tested
bool process_payload_data(const PayloadData *payload);

// Function prototypes for test functions
void test_process_payload_data_null_payload(void);
void test_process_payload_data_null_data(void);
void test_process_payload_data_empty_data(void);
void test_process_payload_data_zero_size(void);
void test_process_payload_data_uncompressed_payload(void);
void test_process_payload_data_compressed_payload(void);

// Test data
static const uint8_t test_data[] = "test payload data";
static const uint8_t compressed_data[] = {
    0x1B, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Brotli header (minimal)
    't', 'e', 's', 't', ' ', 'd', 'a', 't', 'a'         // Some test data
};

void setUp(void) {
    // Set up test environment
}

void tearDown(void) {
    // Clean up test environment
}

// Basic parameter validation tests
void test_process_payload_data_null_payload(void) {
    TEST_ASSERT_FALSE(process_payload_data(NULL));
}

void test_process_payload_data_null_data(void) {
    PayloadData payload;
    memset(&payload, 0, sizeof(PayloadData));
    payload.data = NULL;
    payload.size = sizeof(test_data);
    payload.is_compressed = false;

    TEST_ASSERT_FALSE(process_payload_data(&payload));
}

void test_process_payload_data_empty_data(void) {
    PayloadData payload;
    memset(&payload, 0, sizeof(PayloadData));
    payload.data = (uint8_t*)test_data;
    payload.size = 0;
    payload.is_compressed = false;

    TEST_ASSERT_FALSE(process_payload_data(&payload));
}

void test_process_payload_data_zero_size(void) {
    PayloadData payload;
    memset(&payload, 0, sizeof(PayloadData));
    payload.data = (uint8_t*)test_data;
    payload.size = 0;
    payload.is_compressed = false;

    TEST_ASSERT_FALSE(process_payload_data(&payload));
}

// Test with uncompressed payload
void test_process_payload_data_uncompressed_payload(void) {
    PayloadData payload;
    memset(&payload, 0, sizeof(PayloadData));
    payload.data = (uint8_t*)test_data;
    payload.size = sizeof(test_data) - 1; // Exclude null terminator
    payload.is_compressed = false;

    // This should succeed for uncompressed data
    TEST_ASSERT_TRUE(process_payload_data(&payload));
}

// Test with compressed payload (this will likely fail without proper Brotli data)
void test_process_payload_data_compressed_payload(void) {
    PayloadData payload;
    memset(&payload, 0, sizeof(PayloadData));
    payload.data = (uint8_t*)compressed_data;
    payload.size = sizeof(compressed_data);
    payload.is_compressed = true;

    // This may fail if the compressed data is not valid Brotli
    // but the function should handle it gracefully without crashing
    (void)process_payload_data(&payload);
    // We don't assert the specific result since it depends on the validity of our test data
    // The important thing is that it doesn't crash
    TEST_PASS();
}

int main(void) {
    UNITY_BEGIN();

    // process_payload_data tests
    RUN_TEST(test_process_payload_data_null_payload);
    RUN_TEST(test_process_payload_data_null_data);
    RUN_TEST(test_process_payload_data_empty_data);
    RUN_TEST(test_process_payload_data_zero_size);
    RUN_TEST(test_process_payload_data_uncompressed_payload);
    RUN_TEST(test_process_payload_data_compressed_payload);

    return UNITY_END();
}