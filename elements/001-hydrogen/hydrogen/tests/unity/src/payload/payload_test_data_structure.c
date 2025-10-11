/*
 * Unity Test File: PayloadData Structure Tests
 * This file contains unit tests for the PayloadData structure
 * from src/payload/payload.h
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

void test_payload_data_structure_initialization(void);
void test_payload_data_structure_assignment(void);
void test_payload_data_structure_size_field(void);
void test_payload_data_structure_compression_flag(void);
void test_payload_data_structure_data_pointer(void);
void test_payload_marker_constant(void);
void test_payload_marker_length(void);
void setUp(void) {
    // No setup needed for structure tests
}

void tearDown(void) {
    // No teardown needed for structure tests
}

// Test PayloadData structure
void test_payload_data_structure_initialization(void) {
    PayloadData payload = {0};
    TEST_ASSERT_NULL(payload.data);
    TEST_ASSERT_EQUAL(0, payload.size);
    TEST_ASSERT_FALSE(payload.is_compressed);
}

void test_payload_data_structure_assignment(void) {
    PayloadData payload = {0};
    uint8_t test_data[] = {1, 2, 3, 4, 5};
    
    payload.data = malloc(sizeof(test_data));
    memcpy(payload.data, test_data, sizeof(test_data));
    payload.size = sizeof(test_data);
    payload.is_compressed = true;
    
    TEST_ASSERT_NOT_NULL(payload.data);
    TEST_ASSERT_EQUAL(sizeof(test_data), payload.size);
    TEST_ASSERT_TRUE(payload.is_compressed);
    TEST_ASSERT_EQUAL_MEMORY(test_data, payload.data, sizeof(test_data));
    
    free(payload.data);
}

void test_payload_data_structure_size_field(void) {
    PayloadData payload = {0};
    
    // Test various size values
    payload.size = 0;
    TEST_ASSERT_EQUAL(0, payload.size);
    
    payload.size = 1024;
    TEST_ASSERT_EQUAL(1024, payload.size);
    
    payload.size = SIZE_MAX;
    TEST_ASSERT_EQUAL(SIZE_MAX, payload.size);
}

void test_payload_data_structure_compression_flag(void) {
    PayloadData payload = {0};
    
    // Test false state
    payload.is_compressed = false;
    TEST_ASSERT_FALSE(payload.is_compressed);
    
    // Test true state
    payload.is_compressed = true;
    TEST_ASSERT_TRUE(payload.is_compressed);
    
    // Test toggle
    payload.is_compressed = !payload.is_compressed;
    TEST_ASSERT_FALSE(payload.is_compressed);
}

void test_payload_data_structure_data_pointer(void) {
    PayloadData payload = {0};
    uint8_t *test_ptr = malloc(10);
    
    // Test NULL assignment
    payload.data = NULL;
    TEST_ASSERT_NULL(payload.data);
    
    // Test valid pointer assignment
    payload.data = test_ptr;
    TEST_ASSERT_EQUAL_PTR(test_ptr, payload.data);
    
    free(test_ptr);
}

// Test PAYLOAD_MARKER constant
void test_payload_marker_constant(void) {
    TEST_ASSERT_NOT_NULL(PAYLOAD_MARKER);
    TEST_ASSERT_TRUE(strlen(PAYLOAD_MARKER) > 0);
    TEST_ASSERT_EQUAL_STRING("<<< HERE BE ME TREASURE >>>", PAYLOAD_MARKER);
}

void test_payload_marker_length(void) {
    size_t expected_length = strlen("<<< HERE BE ME TREASURE >>>");
    size_t actual_length = strlen(PAYLOAD_MARKER);
    TEST_ASSERT_EQUAL(expected_length, actual_length);
}

int main(void) {
    UNITY_BEGIN();
    
    // PayloadData structure tests
    RUN_TEST(test_payload_data_structure_initialization);
    RUN_TEST(test_payload_data_structure_assignment);
    RUN_TEST(test_payload_data_structure_size_field);
    RUN_TEST(test_payload_data_structure_compression_flag);
    RUN_TEST(test_payload_data_structure_data_pointer);
    
    // PAYLOAD_MARKER tests
    RUN_TEST(test_payload_marker_constant);
    RUN_TEST(test_payload_marker_length);
    
    return UNITY_END();
}
