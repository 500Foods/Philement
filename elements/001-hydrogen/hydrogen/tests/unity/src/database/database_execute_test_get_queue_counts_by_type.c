/*
 * Unity Test File: database_execute_test_get_queue_counts_by_type
 * This file contains unit tests for database_get_queue_counts_by_type function
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include "../../../../src/database/database.h"

// Forward declarations for functions being tested
void database_get_queue_counts_by_type(int* lead_count, int* slow_count, int* medium_count, int* fast_count, int* cache_count);

// Test function prototypes
void test_database_get_queue_counts_by_type_basic_functionality(void);
void test_database_get_queue_counts_by_type_null_pointers(void);
void test_database_get_queue_counts_by_type_uninitialized_subsystem(void);

void setUp(void) {
    // Set up test fixtures, if any
    // Initialize database subsystem for testing
    database_subsystem_init();
}

void tearDown(void) {
    // Clean up test fixtures, if any
    database_subsystem_shutdown();
}

// Test database_get_queue_counts_by_type function
void test_database_get_queue_counts_by_type_basic_functionality(void) {
    // Test basic functionality with valid parameters
    int lead_count, slow_count, medium_count, fast_count, cache_count;
    database_get_queue_counts_by_type(&lead_count, &slow_count, &medium_count, &fast_count, &cache_count);

    // Should all be 0 as no queues are set up
    TEST_ASSERT_EQUAL(0, lead_count);
    TEST_ASSERT_EQUAL(0, slow_count);
    TEST_ASSERT_EQUAL(0, medium_count);
    TEST_ASSERT_EQUAL(0, fast_count);
    TEST_ASSERT_EQUAL(0, cache_count);
}

void test_database_get_queue_counts_by_type_null_pointers(void) {
    // Test with NULL pointers (should not crash)
    database_get_queue_counts_by_type(NULL, NULL, NULL, NULL, NULL);
    TEST_PASS(); // Should not crash
}

void test_database_get_queue_counts_by_type_uninitialized_subsystem(void) {
    // Test with uninitialized subsystem
    database_subsystem_shutdown(); // Ensure subsystem is not initialized
    int lead_count, slow_count, medium_count, fast_count, cache_count;
    database_get_queue_counts_by_type(&lead_count, &slow_count, &medium_count, &fast_count, &cache_count);

    // Should all be 0
    TEST_ASSERT_EQUAL(0, lead_count);
    TEST_ASSERT_EQUAL(0, slow_count);
    TEST_ASSERT_EQUAL(0, medium_count);
    TEST_ASSERT_EQUAL(0, fast_count);
    TEST_ASSERT_EQUAL(0, cache_count);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_get_queue_counts_by_type_basic_functionality);
    RUN_TEST(test_database_get_queue_counts_by_type_null_pointers);
    RUN_TEST(test_database_get_queue_counts_by_type_uninitialized_subsystem);

    return UNITY_END();
}