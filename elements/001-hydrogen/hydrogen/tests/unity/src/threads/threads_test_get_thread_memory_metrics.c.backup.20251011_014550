/*
 * Unity Test File: get_thread_memory_metrics Function Tests
 * This file contains unit tests for the get_thread_memory_metrics() function
 * from src/threads/threads.c
 *
 * Coverage Goals:
 * - Test memory metrics retrieval for various scenarios
 * - Test edge cases and boundary conditions
 * - Test proper metrics return values
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Forward declarations for the functions being tested
ThreadMemoryMetrics get_thread_memory_metrics(ServiceThreads *threads, pthread_t thread_id);

// Test fixtures
static ServiceThreads test_threads;

// Unity framework requires these functions to be externally visible
extern void setUp(void);
extern void tearDown(void);

void setUp(void) {
    // Initialize test fixtures using memset (same as working test)
    memset(&test_threads, 0, sizeof(ServiceThreads));
}

void tearDown(void) {
    // Clean up test fixtures
}

// Function prototypes for test functions
void test_get_thread_memory_metrics_null_threads(void);
void test_get_thread_memory_metrics_empty_threads(void);
void test_get_thread_memory_metrics_nonexistent_thread(void);
void test_get_thread_memory_metrics_existing_thread(void);
void test_get_thread_memory_metrics_with_current_thread(void);
void test_get_thread_memory_metrics_first_thread(void);
void test_get_thread_memory_metrics_middle_thread(void);
void test_get_thread_memory_metrics_last_thread(void);
void test_get_thread_memory_metrics_zero_metrics(void);
void test_get_thread_memory_metrics_exact_match(void);
void test_get_thread_memory_metrics_boundary_thread_ids(void);
void test_get_thread_memory_metrics_duplicate_threads(void);
void test_get_thread_memory_metrics_large_values(void);
void test_get_thread_memory_metrics_structure_copy(void);

//=============================================================================
// Basic Memory Metrics Retrieval Tests
//=============================================================================

void test_get_thread_memory_metrics_null_threads(void) {
    // Test with NULL threads parameter
    ThreadMemoryMetrics metrics = get_thread_memory_metrics(NULL, pthread_self());

    TEST_ASSERT_EQUAL(0, metrics.virtual_bytes);
    TEST_ASSERT_EQUAL(0, metrics.resident_bytes);
}

void test_get_thread_memory_metrics_empty_threads(void) {
    // Test getting metrics from empty thread list
    ThreadMemoryMetrics metrics = get_thread_memory_metrics(&test_threads, (pthread_t)123);

    TEST_ASSERT_EQUAL(0, metrics.virtual_bytes);
    TEST_ASSERT_EQUAL(0, metrics.resident_bytes);
}

void test_get_thread_memory_metrics_nonexistent_thread(void) {
    // Test getting metrics for a thread that doesn't exist
    add_service_thread(&test_threads, (pthread_t)1);
    add_service_thread(&test_threads, (pthread_t)2);

    ThreadMemoryMetrics metrics = get_thread_memory_metrics(&test_threads, (pthread_t)999);

    TEST_ASSERT_EQUAL(0, metrics.virtual_bytes);
    TEST_ASSERT_EQUAL(0, metrics.resident_bytes);
}

void test_get_thread_memory_metrics_existing_thread(void) {
    // Test getting metrics for a thread that exists
    pthread_t test_thread_id = (pthread_t)42;
    add_service_thread(&test_threads, test_thread_id);

    // Set some test metrics
    test_threads.thread_metrics[0].virtual_bytes = 1000;
    test_threads.thread_metrics[0].resident_bytes = 2000;

    ThreadMemoryMetrics metrics = get_thread_memory_metrics(&test_threads, test_thread_id);

    TEST_ASSERT_EQUAL(1000, metrics.virtual_bytes);
    TEST_ASSERT_EQUAL(2000, metrics.resident_bytes);
}

void test_get_thread_memory_metrics_with_current_thread(void) {
    // Test with the current thread to avoid potential issues with fake thread IDs
    pthread_t current_thread = pthread_self();
    add_service_thread(&test_threads, current_thread);

    // Set some test metrics
    test_threads.thread_metrics[0].virtual_bytes = 1500;
    test_threads.thread_metrics[0].resident_bytes = 2500;

    ThreadMemoryMetrics metrics = get_thread_memory_metrics(&test_threads, current_thread);

    TEST_ASSERT_EQUAL(1500, metrics.virtual_bytes);
    TEST_ASSERT_EQUAL(2500, metrics.resident_bytes);
}

void test_get_thread_memory_metrics_first_thread(void) {
    // Test getting metrics for the first thread in the array
    add_service_thread(&test_threads, (pthread_t)1);
    add_service_thread(&test_threads, (pthread_t)2);
    add_service_thread(&test_threads, (pthread_t)3);

    // Set metrics for first thread
    test_threads.thread_metrics[0].virtual_bytes = 100;
    test_threads.thread_metrics[0].resident_bytes = 200;

    ThreadMemoryMetrics metrics = get_thread_memory_metrics(&test_threads, (pthread_t)1);

    TEST_ASSERT_EQUAL(100, metrics.virtual_bytes);
    TEST_ASSERT_EQUAL(200, metrics.resident_bytes);
}

void test_get_thread_memory_metrics_middle_thread(void) {
    // Test getting metrics for a middle thread in the array
    add_service_thread(&test_threads, (pthread_t)1);
    add_service_thread(&test_threads, (pthread_t)2);
    add_service_thread(&test_threads, (pthread_t)3);
    add_service_thread(&test_threads, (pthread_t)4);

    // Set metrics for middle thread
    test_threads.thread_metrics[2].virtual_bytes = 300;
    test_threads.thread_metrics[2].resident_bytes = 400;

    ThreadMemoryMetrics metrics = get_thread_memory_metrics(&test_threads, (pthread_t)3);

    TEST_ASSERT_EQUAL(300, metrics.virtual_bytes);
    TEST_ASSERT_EQUAL(400, metrics.resident_bytes);
}

void test_get_thread_memory_metrics_last_thread(void) {
    // Test getting metrics for the last thread in the array
    add_service_thread(&test_threads, (pthread_t)1);
    add_service_thread(&test_threads, (pthread_t)2);
    add_service_thread(&test_threads, (pthread_t)3);

    // Set metrics for last thread
    test_threads.thread_metrics[2].virtual_bytes = 500;
    test_threads.thread_metrics[2].resident_bytes = 600;

    ThreadMemoryMetrics metrics = get_thread_memory_metrics(&test_threads, (pthread_t)3);

    TEST_ASSERT_EQUAL(500, metrics.virtual_bytes);
    TEST_ASSERT_EQUAL(600, metrics.resident_bytes);
}

void test_get_thread_memory_metrics_zero_metrics(void) {
    // Test getting metrics that are zero
    pthread_t test_thread_id = (pthread_t)123;
    add_service_thread(&test_threads, test_thread_id);

    // Metrics should be initialized to zero by add_service_thread
    ThreadMemoryMetrics metrics = get_thread_memory_metrics(&test_threads, test_thread_id);

    TEST_ASSERT_EQUAL(0, metrics.virtual_bytes);
    TEST_ASSERT_EQUAL(0, metrics.resident_bytes);
}

//=============================================================================
// Thread ID Matching Tests
//=============================================================================

void test_get_thread_memory_metrics_exact_match(void) {
    // Test exact thread ID matching
    add_service_thread(&test_threads, (pthread_t)1);
    add_service_thread(&test_threads, (pthread_t)2);

    // Set different metrics for each thread
    test_threads.thread_metrics[0].virtual_bytes = 1000;
    test_threads.thread_metrics[0].resident_bytes = 2000;
    test_threads.thread_metrics[1].virtual_bytes = 3000;
    test_threads.thread_metrics[1].resident_bytes = 4000;

    ThreadMemoryMetrics metrics1 = get_thread_memory_metrics(&test_threads, (pthread_t)1);
    ThreadMemoryMetrics metrics2 = get_thread_memory_metrics(&test_threads, (pthread_t)2);

    TEST_ASSERT_EQUAL(1000, metrics1.virtual_bytes);
    TEST_ASSERT_EQUAL(2000, metrics1.resident_bytes);
    TEST_ASSERT_EQUAL(3000, metrics2.virtual_bytes);
    TEST_ASSERT_EQUAL(4000, metrics2.resident_bytes);
}

void test_get_thread_memory_metrics_boundary_thread_ids(void) {
    // Test with boundary thread ID values
    const pthread_t boundary_values[] = {
        (pthread_t)0,
        (pthread_t)1,
        (pthread_t)INT_MAX,
        (pthread_t)UINT_MAX,
        (pthread_t)-1,
        (pthread_t)INT_MIN
    };

    for (size_t i = 0; i < sizeof(boundary_values) / sizeof(boundary_values[0]); i++) {
        // Reset for each test
        init_service_threads(&test_threads, "TestService");
        add_service_thread(&test_threads, boundary_values[i]);

        // Set some test metrics
        test_threads.thread_metrics[0].virtual_bytes = 100 + i;
        test_threads.thread_metrics[0].resident_bytes = 200 + i;

        ThreadMemoryMetrics metrics = get_thread_memory_metrics(&test_threads, boundary_values[i]);

        TEST_ASSERT_EQUAL(100 + i, metrics.virtual_bytes);
        TEST_ASSERT_EQUAL(200 + i, metrics.resident_bytes);
    }
}

//=============================================================================
// Duplicate Thread Tests
//=============================================================================

void test_get_thread_memory_metrics_duplicate_threads(void) {
    // Test behavior with duplicate thread IDs
    pthread_t test_thread_id = (pthread_t)42;
    add_service_thread(&test_threads, test_thread_id);
    add_service_thread(&test_threads, test_thread_id);  // Duplicate

    // Set different metrics for each instance
    test_threads.thread_metrics[0].virtual_bytes = 1000;
    test_threads.thread_metrics[0].resident_bytes = 2000;
    test_threads.thread_metrics[1].virtual_bytes = 3000;
    test_threads.thread_metrics[1].resident_bytes = 4000;

    // Should return the first match
    ThreadMemoryMetrics metrics = get_thread_memory_metrics(&test_threads, test_thread_id);

    TEST_ASSERT_EQUAL(1000, metrics.virtual_bytes);
    TEST_ASSERT_EQUAL(2000, metrics.resident_bytes);
}

//=============================================================================
// Metrics Structure Tests
//=============================================================================

void test_get_thread_memory_metrics_large_values(void) {
    // Test with large metric values
    pthread_t test_thread_id = (pthread_t)123;
    add_service_thread(&test_threads, test_thread_id);

    test_threads.thread_metrics[0].virtual_bytes = 0x7FFFFFFFFFFFFFFF;
    test_threads.thread_metrics[0].resident_bytes = 0x7FFFFFFFFFFFFFFF;

    ThreadMemoryMetrics metrics = get_thread_memory_metrics(&test_threads, test_thread_id);

    TEST_ASSERT_EQUAL(0x7FFFFFFFFFFFFFFF, metrics.virtual_bytes);
    TEST_ASSERT_EQUAL(0x7FFFFFFFFFFFFFFF, metrics.resident_bytes);
}

void test_get_thread_memory_metrics_structure_copy(void) {
    // Test that the function returns a proper copy of the metrics
    pthread_t test_thread_id = (pthread_t)456;
    add_service_thread(&test_threads, test_thread_id);

    test_threads.thread_metrics[0].virtual_bytes = 777;
    test_threads.thread_metrics[0].resident_bytes = 888;

    ThreadMemoryMetrics metrics = get_thread_memory_metrics(&test_threads, test_thread_id);

    TEST_ASSERT_EQUAL(777, metrics.virtual_bytes);
    TEST_ASSERT_EQUAL(888, metrics.resident_bytes);

    // Modify the returned metrics (should not affect original)
    metrics.virtual_bytes = 999;
    metrics.resident_bytes = 111;

    // Original should be unchanged
    TEST_ASSERT_EQUAL(777, test_threads.thread_metrics[0].virtual_bytes);
    TEST_ASSERT_EQUAL(888, test_threads.thread_metrics[0].resident_bytes);
}

int main(void) {
    UNITY_BEGIN();

    // Run basic tests that don't involve system calls
    RUN_TEST(test_get_thread_memory_metrics_null_threads);
    RUN_TEST(test_get_thread_memory_metrics_empty_threads);

    // Test with current thread (safer than fake thread IDs)
    RUN_TEST(test_get_thread_memory_metrics_with_current_thread);

    // Test basic functionality with real thread ID
    RUN_TEST(test_get_thread_memory_metrics_existing_thread);

    // Test basic array operations (safe)
    RUN_TEST(test_get_thread_memory_metrics_zero_metrics);
    RUN_TEST(test_get_thread_memory_metrics_exact_match);
    RUN_TEST(test_get_thread_memory_metrics_structure_copy);

    // Test more complex scenarios that use add_service_thread
    RUN_TEST(test_get_thread_memory_metrics_nonexistent_thread);
    RUN_TEST(test_get_thread_memory_metrics_first_thread);
    RUN_TEST(test_get_thread_memory_metrics_middle_thread);
    RUN_TEST(test_get_thread_memory_metrics_last_thread);

    // Test advanced scenarios
    RUN_TEST(test_get_thread_memory_metrics_boundary_thread_ids);
    RUN_TEST(test_get_thread_memory_metrics_duplicate_threads);
    RUN_TEST(test_get_thread_memory_metrics_large_values);

    return UNITY_END();
}
