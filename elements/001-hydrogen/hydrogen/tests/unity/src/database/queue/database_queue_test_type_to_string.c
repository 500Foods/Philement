/*
 * Unity Test File: database_queue_type_to_string
 * This file contains unit tests for database_queue_type_to_string functionality
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/queue/database_queue.h>

// Forward declarations for functions being tested
const char* database_queue_type_to_string(int queue_type);

// Function prototypes for test functions
void test_database_queue_type_to_string_slow(void);
void test_database_queue_type_to_string_medium(void);
void test_database_queue_type_to_string_fast(void);
void test_database_queue_type_to_string_cache(void);
void test_database_queue_type_to_string_invalid(void);
void test_database_queue_type_to_string_boundary_values(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test functions
void test_database_queue_type_to_string_slow(void) {
    // Test SLOW queue type
    const char* result = database_queue_type_to_string(DB_QUEUE_SLOW);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("slow", result);
}

void test_database_queue_type_to_string_medium(void) {
    // Test MEDIUM queue type
    const char* result = database_queue_type_to_string(DB_QUEUE_MEDIUM);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("medium", result);
}

void test_database_queue_type_to_string_fast(void) {
    // Test FAST queue type
    const char* result = database_queue_type_to_string(DB_QUEUE_FAST);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("fast", result);
}

void test_database_queue_type_to_string_cache(void) {
    // Test CACHE queue type
    const char* result = database_queue_type_to_string(DB_QUEUE_CACHE);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("cache", result);
}

void test_database_queue_type_to_string_invalid(void) {
    // Test invalid queue type
    const char* result = database_queue_type_to_string(999);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("unknown", result);
}

void test_database_queue_type_to_string_boundary_values(void) {
    // Test boundary values
    const char* result_neg = database_queue_type_to_string(-1);
    TEST_ASSERT_NOT_NULL(result_neg);
    TEST_ASSERT_EQUAL_STRING("unknown", result_neg);

    const char* result_max = database_queue_type_to_string(DB_QUEUE_MAX_TYPES);
    TEST_ASSERT_NOT_NULL(result_max);
    TEST_ASSERT_EQUAL_STRING("unknown", result_max);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_type_to_string_slow);
    RUN_TEST(test_database_queue_type_to_string_medium);
    RUN_TEST(test_database_queue_type_to_string_fast);
    RUN_TEST(test_database_queue_type_to_string_cache);
    RUN_TEST(test_database_queue_type_to_string_invalid);
    RUN_TEST(test_database_queue_type_to_string_boundary_values);

    return UNITY_END();
}