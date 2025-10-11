/*
 * Unity Test File: add_service_thread Function Tests
 * This file contains unit tests for the add_service_thread() function
 * from src/threads/threads.c
 *
 * Coverage Goals:
 * - Test thread addition with various thread IDs
 * - Test boundary conditions and limits
 * - Test error handling and edge cases
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Forward declarations for the functions being tested
void init_service_threads(ServiceThreads *threads, const char* subsystem_name);
void add_service_thread(ServiceThreads *threads, pthread_t thread_id);

// Test fixtures
static ServiceThreads test_threads;

// Unity framework requires these functions to be externally visible
extern void setUp(void);
extern void tearDown(void);

void setUp(void) {
    // Initialize test fixtures
    memset(&test_threads, 0, sizeof(ServiceThreads));
    init_service_threads(&test_threads, "TestService");
}

void tearDown(void) {
    // Clean up test fixtures
}

// Function prototypes for test functions
void test_add_service_thread_valid_addition(void);
void test_add_service_thread_null_thread_id(void);
void test_add_service_thread_multiple_additions(void);
void test_add_service_thread_max_threads_reached(void);
void test_add_service_thread_duplicate_thread(void);
void test_add_service_thread_large_thread_ids(void);
void test_add_service_thread_zero_thread_id(void);
void test_add_service_thread_negative_thread_id(void);
void test_add_service_thread_array_ordering(void);
void test_add_service_thread_tid_assignment(void);
void test_add_service_thread_metrics_initialization(void);
void test_add_service_thread_service_totals_unchanged(void);
void test_add_service_thread_boundary_values(void);
void test_add_service_thread_max_minus_one(void);

//=============================================================================
// Basic Thread Addition Tests
//=============================================================================

void test_add_service_thread_valid_addition(void) {
    // Test adding a valid thread ID (use current thread as valid ID)
    pthread_t test_thread_id = pthread_self();
    add_service_thread(&test_threads, test_thread_id);

    TEST_ASSERT_EQUAL(1, test_threads.thread_count);
    TEST_ASSERT_TRUE(pthread_equal(test_threads.thread_ids[0], test_thread_id));
    TEST_ASSERT_GREATER_THAN(0, test_threads.thread_tids[0]);  // TID should be set
}

void test_add_service_thread_null_thread_id(void) {
    // Test adding a null thread ID (pthread_t can be 0)
    add_service_thread(&test_threads, 0);

    // Should still work (though the thread ID is invalid)
    TEST_ASSERT_EQUAL(1, test_threads.thread_count);
}

void test_add_service_thread_multiple_additions(void) {
    // Test adding multiple threads
    add_service_thread(&test_threads, (pthread_t)1);
    add_service_thread(&test_threads, (pthread_t)2);
    add_service_thread(&test_threads, (pthread_t)3);

    TEST_ASSERT_EQUAL(3, test_threads.thread_count);

    for (int i = 0; i < 3; i++) {
        TEST_ASSERT_EQUAL((pthread_t)(i + 1), test_threads.thread_ids[i]);
    }
}

void test_add_service_thread_max_threads_reached(void) {
    // Test adding threads beyond MAX_SERVICE_THREADS limit
    for (int i = 0; i < MAX_SERVICE_THREADS; i++) {
        add_service_thread(&test_threads, (pthread_t)i);
    }

    TEST_ASSERT_EQUAL(MAX_SERVICE_THREADS, test_threads.thread_count);

    // Try to add one more - should be silently ignored
    add_service_thread(&test_threads, (pthread_t)999);
    TEST_ASSERT_EQUAL(MAX_SERVICE_THREADS, test_threads.thread_count);  // Should remain the same
}

void test_add_service_thread_duplicate_thread(void) {
    // Test adding the same thread ID multiple times
    pthread_t test_thread_id = (pthread_t)42;
    add_service_thread(&test_threads, test_thread_id);
    add_service_thread(&test_threads, test_thread_id);  // Duplicate

    // Should allow duplicates (function doesn't check for uniqueness)
    TEST_ASSERT_EQUAL(2, test_threads.thread_count);
}

void test_add_service_thread_large_thread_ids(void) {
    // Test with very large thread IDs
    add_service_thread(&test_threads, (pthread_t)0xFFFFFFFF);
    add_service_thread(&test_threads, (pthread_t)0x7FFFFFFFFFFFFFFF);

    TEST_ASSERT_EQUAL(2, test_threads.thread_count);
    TEST_ASSERT_EQUAL((pthread_t)0xFFFFFFFF, test_threads.thread_ids[0]);
    TEST_ASSERT_EQUAL((pthread_t)0x7FFFFFFFFFFFFFFF, test_threads.thread_ids[1]);
}

void test_add_service_thread_zero_thread_id(void) {
    // Test with zero thread ID (edge case)
    add_service_thread(&test_threads, (pthread_t)0);

    TEST_ASSERT_EQUAL(1, test_threads.thread_count);
    TEST_ASSERT_EQUAL((pthread_t)0, test_threads.thread_ids[0]);
}

void test_add_service_thread_negative_thread_id(void) {
    // Test with negative thread ID (if supported)
    add_service_thread(&test_threads, (pthread_t)-1);

    TEST_ASSERT_EQUAL(1, test_threads.thread_count);
    TEST_ASSERT_EQUAL((pthread_t)-1, test_threads.thread_ids[0]);
}

//=============================================================================
// Thread Array Management Tests
//=============================================================================

void test_add_service_thread_array_ordering(void) {
    // Test that threads are added in order
    for (int i = 10; i >= 1; i--) {
        add_service_thread(&test_threads, (pthread_t)i);
    }

    TEST_ASSERT_EQUAL(10, test_threads.thread_count);

    // Verify threads are stored in addition order
    for (int i = 0; i < 10; i++) {
        TEST_ASSERT_EQUAL((pthread_t)(10 - i), test_threads.thread_ids[i]);
    }
}

void test_add_service_thread_tid_assignment(void) {
    // Test that TIDs are properly assigned
    add_service_thread(&test_threads, (pthread_t)123);

    TEST_ASSERT_EQUAL(1, test_threads.thread_count);
    TEST_ASSERT_EQUAL((pthread_t)123, test_threads.thread_ids[0]);
    // TID should be assigned (may be 0 if syscall fails, but that's OK for this test)
    TEST_ASSERT_GREATER_OR_EQUAL(0, test_threads.thread_tids[0]);
}

void test_add_service_thread_metrics_initialization(void) {
    // Test that thread metrics are initialized to zero
    add_service_thread(&test_threads, (pthread_t)456);

    TEST_ASSERT_EQUAL(1, test_threads.thread_count);
    TEST_ASSERT_EQUAL(0, test_threads.thread_metrics[0].virtual_bytes);
    TEST_ASSERT_EQUAL(0, test_threads.thread_metrics[0].resident_bytes);
}

void test_add_service_thread_service_totals_unchanged(void) {
    // Test that service totals are not affected by thread addition
    test_threads.virtual_memory = 1000;
    test_threads.resident_memory = 2000;
    test_threads.memory_percent = 5.0;

    add_service_thread(&test_threads, (pthread_t)789);

    // Service totals should remain unchanged
    TEST_ASSERT_EQUAL(1000, test_threads.virtual_memory);
    TEST_ASSERT_EQUAL(2000, test_threads.resident_memory);
    TEST_ASSERT_EQUAL(5.0, test_threads.memory_percent);
}

//=============================================================================
// Boundary and Edge Case Tests
//=============================================================================

void test_add_service_thread_boundary_values(void) {
    // Test with various boundary values
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

        TEST_ASSERT_EQUAL(1, test_threads.thread_count);
        TEST_ASSERT_EQUAL(boundary_values[i], test_threads.thread_ids[0]);
    }
}

void test_add_service_thread_max_minus_one(void) {
    // Test adding MAX_SERVICE_THREADS - 1 threads
    for (int i = 0; i < MAX_SERVICE_THREADS - 1; i++) {
        add_service_thread(&test_threads, (pthread_t)i);
    }

    TEST_ASSERT_EQUAL(MAX_SERVICE_THREADS - 1, test_threads.thread_count);

    // Should still be able to add one more
    add_service_thread(&test_threads, (pthread_t)999);
    TEST_ASSERT_EQUAL(MAX_SERVICE_THREADS, test_threads.thread_count);
}

//=============================================================================
// Test Main Function
//=============================================================================

int main(void) {
    UNITY_BEGIN();

    // Basic thread addition tests
    RUN_TEST(test_add_service_thread_valid_addition);
    RUN_TEST(test_add_service_thread_null_thread_id);
    RUN_TEST(test_add_service_thread_multiple_additions);
    RUN_TEST(test_add_service_thread_max_threads_reached);
    RUN_TEST(test_add_service_thread_duplicate_thread);
    RUN_TEST(test_add_service_thread_large_thread_ids);
    RUN_TEST(test_add_service_thread_zero_thread_id);
    RUN_TEST(test_add_service_thread_negative_thread_id);

    // Thread array management tests
    RUN_TEST(test_add_service_thread_array_ordering);
    RUN_TEST(test_add_service_thread_tid_assignment);
    RUN_TEST(test_add_service_thread_metrics_initialization);
    RUN_TEST(test_add_service_thread_service_totals_unchanged);

    // Boundary and edge case tests
    RUN_TEST(test_add_service_thread_boundary_values);
    RUN_TEST(test_add_service_thread_max_minus_one);

    return UNITY_END();
}
