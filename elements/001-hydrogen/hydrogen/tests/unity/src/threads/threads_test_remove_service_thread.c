/*
 * Unity Test File: remove_service_thread Function Tests
 * This file contains unit tests for the remove_service_thread() function
 * from src/threads/threads.c
 *
 * Coverage Goals:
 * - Test thread removal with various scenarios
 * - Test edge cases and boundary conditions
 * - Test array compaction after removal
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Forward declarations for the functions being tested
void init_service_threads(ServiceThreads *threads, const char* subsystem_name);
void add_service_thread(ServiceThreads *threads, pthread_t thread_id);
void remove_service_thread(ServiceThreads *threads, pthread_t thread_id);

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
void test_remove_service_thread_valid_removal(void);
void test_remove_service_thread_nonexistent_thread(void);
void test_remove_service_thread_null_thread_id(void);
void test_remove_service_thread_empty_list(void);
void test_remove_service_thread_remove_first(void);
void test_remove_service_thread_remove_middle(void);
void test_remove_service_thread_remove_last(void);
void test_remove_service_thread_duplicate_threads(void);
void test_remove_service_thread_array_compaction(void);
void test_remove_service_thread_tid_array_update(void);
void test_remove_service_thread_metrics_array_update(void);
void test_remove_service_thread_service_totals_unchanged(void);
void test_remove_service_thread_boundary_values(void);
void test_remove_service_thread_multiple_removals(void);
void test_remove_service_thread_remove_all(void);

//=============================================================================
// Basic Thread Removal Tests
//=============================================================================

void test_remove_service_thread_valid_removal(void) {
    // Test removing a thread that exists
    pthread_t test_thread_id = (pthread_t)42;
    add_service_thread(&test_threads, test_thread_id);

    TEST_ASSERT_EQUAL(1, test_threads.thread_count);

    remove_service_thread(&test_threads, test_thread_id);

    TEST_ASSERT_EQUAL(0, test_threads.thread_count);
}

void test_remove_service_thread_nonexistent_thread(void) {
    // Test removing a thread that doesn't exist
    add_service_thread(&test_threads, (pthread_t)1);
    add_service_thread(&test_threads, (pthread_t)2);

    TEST_ASSERT_EQUAL(2, test_threads.thread_count);

    remove_service_thread(&test_threads, (pthread_t)999);  // Non-existent

    // Should remain unchanged
    TEST_ASSERT_EQUAL(2, test_threads.thread_count);
}

void test_remove_service_thread_null_thread_id(void) {
    // Test removing a null thread ID
    add_service_thread(&test_threads, (pthread_t)1);
    add_service_thread(&test_threads, (pthread_t)2);

    remove_service_thread(&test_threads, 0);  // Should not crash

    TEST_ASSERT_EQUAL(2, test_threads.thread_count);
}

void test_remove_service_thread_empty_list(void) {
    // Test removing from an empty thread list
    remove_service_thread(&test_threads, (pthread_t)123);

    // Should not crash and remain empty
    TEST_ASSERT_EQUAL(0, test_threads.thread_count);
}

void test_remove_service_thread_remove_first(void) {
    // Test removing the first thread in the array
    add_service_thread(&test_threads, (pthread_t)1);
    add_service_thread(&test_threads, (pthread_t)2);
    add_service_thread(&test_threads, (pthread_t)3);

    TEST_ASSERT_EQUAL(3, test_threads.thread_count);

    remove_service_thread(&test_threads, (pthread_t)1);

    TEST_ASSERT_EQUAL(2, test_threads.thread_count);
    TEST_ASSERT_EQUAL((pthread_t)3, test_threads.thread_ids[0]);  // Last moved to first
    TEST_ASSERT_EQUAL((pthread_t)2, test_threads.thread_ids[1]);
}

void test_remove_service_thread_remove_middle(void) {
    // Test removing a thread from the middle of the array
    add_service_thread(&test_threads, (pthread_t)1);
    add_service_thread(&test_threads, (pthread_t)2);
    add_service_thread(&test_threads, (pthread_t)3);
    add_service_thread(&test_threads, (pthread_t)4);

    TEST_ASSERT_EQUAL(4, test_threads.thread_count);

    remove_service_thread(&test_threads, (pthread_t)2);

    TEST_ASSERT_EQUAL(3, test_threads.thread_count);
    // Verify the last thread moved to position 1
    TEST_ASSERT_EQUAL((pthread_t)1, test_threads.thread_ids[0]);
    TEST_ASSERT_EQUAL((pthread_t)4, test_threads.thread_ids[1]);
    TEST_ASSERT_EQUAL((pthread_t)3, test_threads.thread_ids[2]);
}

void test_remove_service_thread_remove_last(void) {
    // Test removing the last thread in the array
    add_service_thread(&test_threads, (pthread_t)1);
    add_service_thread(&test_threads, (pthread_t)2);
    add_service_thread(&test_threads, (pthread_t)3);

    TEST_ASSERT_EQUAL(3, test_threads.thread_count);

    remove_service_thread(&test_threads, (pthread_t)3);

    TEST_ASSERT_EQUAL(2, test_threads.thread_count);
    TEST_ASSERT_EQUAL((pthread_t)1, test_threads.thread_ids[0]);
    TEST_ASSERT_EQUAL((pthread_t)2, test_threads.thread_ids[1]);
}

void test_remove_service_thread_duplicate_threads(void) {
    // Test removing one instance of duplicate threads
    pthread_t test_thread_id = (pthread_t)42;
    add_service_thread(&test_threads, test_thread_id);
    add_service_thread(&test_threads, test_thread_id);  // Duplicate
    add_service_thread(&test_threads, (pthread_t)99);

    TEST_ASSERT_EQUAL(3, test_threads.thread_count);

    remove_service_thread(&test_threads, test_thread_id);

    // Should remove only the first instance
    TEST_ASSERT_EQUAL(2, test_threads.thread_count);
    // The last thread should have moved up
    TEST_ASSERT_EQUAL((pthread_t)99, test_threads.thread_ids[0]);
    TEST_ASSERT_EQUAL(test_thread_id, test_threads.thread_ids[1]);
}

//=============================================================================
// Thread Array Management Tests
//=============================================================================

void test_remove_service_thread_array_compaction(void) {
    // Test that array is properly compacted after removal
    add_service_thread(&test_threads, (pthread_t)1);
    add_service_thread(&test_threads, (pthread_t)2);
    add_service_thread(&test_threads, (pthread_t)3);
    add_service_thread(&test_threads, (pthread_t)4);
    add_service_thread(&test_threads, (pthread_t)5);

    remove_service_thread(&test_threads, (pthread_t)3);

    TEST_ASSERT_EQUAL(4, test_threads.thread_count);

    // Verify array is contiguous
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_NOT_EQUAL(0, test_threads.thread_ids[i]);  // No gaps
    }

    // Verify specific positions
    TEST_ASSERT_EQUAL((pthread_t)1, test_threads.thread_ids[0]);
    TEST_ASSERT_EQUAL((pthread_t)2, test_threads.thread_ids[1]);
    TEST_ASSERT_EQUAL((pthread_t)5, test_threads.thread_ids[2]);  // Moved from position 4
    TEST_ASSERT_EQUAL((pthread_t)4, test_threads.thread_ids[3]);  // Moved from position 3
}

void test_remove_service_thread_tid_array_update(void) {
    // Test that TID array is also properly updated
    add_service_thread(&test_threads, (pthread_t)1);
    add_service_thread(&test_threads, (pthread_t)2);
    add_service_thread(&test_threads, (pthread_t)3);

    remove_service_thread(&test_threads, (pthread_t)2);

    TEST_ASSERT_EQUAL(2, test_threads.thread_count);
    // Verify TID array is also compacted
    TEST_ASSERT_NOT_EQUAL(0, test_threads.thread_tids[0]);
    TEST_ASSERT_NOT_EQUAL(0, test_threads.thread_tids[1]);
}

void test_remove_service_thread_metrics_array_update(void) {
    // Test that metrics array is also properly updated
    add_service_thread(&test_threads, (pthread_t)1);
    add_service_thread(&test_threads, (pthread_t)2);
    add_service_thread(&test_threads, (pthread_t)3);

    // Set some metrics values to verify they're moved
    test_threads.thread_metrics[2].virtual_bytes = 1000;
    test_threads.thread_metrics[2].resident_bytes = 2000;

    remove_service_thread(&test_threads, (pthread_t)2);

    TEST_ASSERT_EQUAL(2, test_threads.thread_count);
    // Verify metrics were moved from position 2 to position 1
    TEST_ASSERT_EQUAL(1000, test_threads.thread_metrics[1].virtual_bytes);
    TEST_ASSERT_EQUAL(2000, test_threads.thread_metrics[1].resident_bytes);
}

void test_remove_service_thread_service_totals_unchanged(void) {
    // Test that service totals are not affected by thread removal
    add_service_thread(&test_threads, (pthread_t)1);
    add_service_thread(&test_threads, (pthread_t)2);

    test_threads.virtual_memory = 1000;
    test_threads.resident_memory = 2000;
    test_threads.memory_percent = 5.0;

    remove_service_thread(&test_threads, (pthread_t)1);

    // Service totals should remain unchanged
    TEST_ASSERT_EQUAL(1000, test_threads.virtual_memory);
    TEST_ASSERT_EQUAL(2000, test_threads.resident_memory);
    TEST_ASSERT_EQUAL(5.0, test_threads.memory_percent);
}

//=============================================================================
// Edge Case and Boundary Tests
//=============================================================================

void test_remove_service_thread_boundary_values(void) {
    // Test removing with various boundary thread IDs
    add_service_thread(&test_threads, (pthread_t)1);
    add_service_thread(&test_threads, (pthread_t)INT_MAX);
    add_service_thread(&test_threads, (pthread_t)0);
    add_service_thread(&test_threads, (pthread_t)-1);

    TEST_ASSERT_EQUAL(4, test_threads.thread_count);

    // Remove boundary values
    remove_service_thread(&test_threads, (pthread_t)INT_MAX);
    TEST_ASSERT_EQUAL(3, test_threads.thread_count);

    remove_service_thread(&test_threads, (pthread_t)0);
    TEST_ASSERT_EQUAL(2, test_threads.thread_count);

    remove_service_thread(&test_threads, (pthread_t)-1);
    TEST_ASSERT_EQUAL(1, test_threads.thread_count);

    remove_service_thread(&test_threads, (pthread_t)1);
    TEST_ASSERT_EQUAL(0, test_threads.thread_count);
}

void test_remove_service_thread_multiple_removals(void) {
    // Test multiple removals from the same array
    add_service_thread(&test_threads, (pthread_t)1);
    add_service_thread(&test_threads, (pthread_t)2);
    add_service_thread(&test_threads, (pthread_t)3);
    add_service_thread(&test_threads, (pthread_t)4);
    add_service_thread(&test_threads, (pthread_t)5);

    TEST_ASSERT_EQUAL(5, test_threads.thread_count);

    // Remove multiple threads - track how array compaction affects remaining elements
    remove_service_thread(&test_threads, (pthread_t)2);  // Remove 2: [1,2,3,4,5] -> [1,5,3,4]
    TEST_ASSERT_EQUAL(4, test_threads.thread_count);

    remove_service_thread(&test_threads, (pthread_t)4);  // Remove 4: [1,5,3,4] -> [1,5,3]
    TEST_ASSERT_EQUAL(3, test_threads.thread_count);

    remove_service_thread(&test_threads, (pthread_t)1);  // Remove 1: [1,5,3] -> [3,5]
    TEST_ASSERT_EQUAL(2, test_threads.thread_count);

    // The count should be 2 - array compaction reduces the count
    TEST_ASSERT_EQUAL(2, test_threads.thread_count);

    // Verify remaining threads - array compaction works correctly
    // After removing 2,4,1 from [1,2,3,4,5], we should get [3,5] due to compaction
    // The last element moves to fill gaps, so we end up with [3,5]
    TEST_ASSERT_EQUAL((pthread_t)3, test_threads.thread_ids[0]);
    TEST_ASSERT_EQUAL((pthread_t)5, test_threads.thread_ids[1]);
}

void test_remove_service_thread_remove_all(void) {
    // Test removing all threads one by one
    add_service_thread(&test_threads, (pthread_t)1);
    add_service_thread(&test_threads, (pthread_t)2);
    add_service_thread(&test_threads, (pthread_t)3);

    TEST_ASSERT_EQUAL(3, test_threads.thread_count);

    remove_service_thread(&test_threads, (pthread_t)1);
    TEST_ASSERT_EQUAL(2, test_threads.thread_count);

    remove_service_thread(&test_threads, (pthread_t)2);
    TEST_ASSERT_EQUAL(1, test_threads.thread_count);

    remove_service_thread(&test_threads, (pthread_t)3);
    TEST_ASSERT_EQUAL(0, test_threads.thread_count);

    // Try to remove from empty list
    remove_service_thread(&test_threads, (pthread_t)123);
    TEST_ASSERT_EQUAL(0, test_threads.thread_count);
}

//=============================================================================
// Test Main Function
//=============================================================================

int main(void) {
    UNITY_BEGIN();

    // Basic thread removal tests
    RUN_TEST(test_remove_service_thread_valid_removal);
    RUN_TEST(test_remove_service_thread_nonexistent_thread);
    RUN_TEST(test_remove_service_thread_null_thread_id);
    RUN_TEST(test_remove_service_thread_empty_list);
    RUN_TEST(test_remove_service_thread_remove_first);
    RUN_TEST(test_remove_service_thread_remove_middle);
    RUN_TEST(test_remove_service_thread_remove_last);
    RUN_TEST(test_remove_service_thread_duplicate_threads);

    // Thread array management tests
    RUN_TEST(test_remove_service_thread_array_compaction);
    RUN_TEST(test_remove_service_thread_tid_array_update);
    RUN_TEST(test_remove_service_thread_metrics_array_update);
    RUN_TEST(test_remove_service_thread_service_totals_unchanged);

    // Edge case and boundary tests
    RUN_TEST(test_remove_service_thread_boundary_values);
    RUN_TEST(test_remove_service_thread_multiple_removals);
    RUN_TEST(test_remove_service_thread_remove_all);

    return UNITY_END();
}
