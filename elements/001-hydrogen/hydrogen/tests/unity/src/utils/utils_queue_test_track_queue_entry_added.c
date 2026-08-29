/*
 * Unity Test File: track_queue_entry_added Function Tests
 * This file contains comprehensive unit tests for the track_queue_entry_added() function
 * from src/utils/utils_queue.h
 *
 * Coverage Goals:
 * - Test that entry_count is incremented by exactly 1 per call
 * - Test multiple consecutive increments
 * - Test increment from a non-zero starting value
 * - Test that other queue fields are not affected
 * - Test increment-decrement interaction
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include the header for the function being tested
#include <src/utils/utils_queue.h>

// Forward declaration for the function being tested
void track_queue_entry_added(QueueMemoryMetrics *queue);

// Test fixtures
static QueueMemoryMetrics test_queue;

// Unity framework requires these functions to be externally visible
extern void setUp(void);
extern void tearDown(void);

void setUp(void) {
    // Initialize test queue with zeros
    memset(&test_queue, 0, sizeof(QueueMemoryMetrics));
}

void tearDown(void) {
    // Clean up test fixtures
    memset(&test_queue, 0, sizeof(QueueMemoryMetrics));
}

// Forward declarations for test functions
void test_track_queue_entry_added_from_zero(void);
void test_track_queue_entry_added_single_increment(void);
void test_track_queue_entry_added_multiple_increments(void);
void test_track_queue_entry_added_from_nonzero(void);
void test_track_queue_entry_added_does_not_affect_other_fields(void);
void test_track_queue_entry_added_large_count(void);
void test_track_queue_entry_added_preserves_block_data(void);

//=============================================================================
// Basic Increment Tests
//=============================================================================

void test_track_queue_entry_added_from_zero(void) {
    // Start with entry_count at 0 and verify it goes to 1
    TEST_ASSERT_EQUAL(0, test_queue.entry_count);

    track_queue_entry_added(&test_queue);

    TEST_ASSERT_EQUAL(1, test_queue.entry_count);
}

void test_track_queue_entry_added_single_increment(void) {
    // Verify a single call increments by exactly 1
    test_queue.entry_count = 5;

    track_queue_entry_added(&test_queue);

    TEST_ASSERT_EQUAL(6, test_queue.entry_count);
}

void test_track_queue_entry_added_multiple_increments(void) {
    // Call the function multiple times and verify the count accumulates
    test_queue.entry_count = 0;

    for (int i = 0; i < 10; i++) {
        track_queue_entry_added(&test_queue);
    }

    TEST_ASSERT_EQUAL(10, test_queue.entry_count);
}

void test_track_queue_entry_added_from_nonzero(void) {
    // Start from a non-zero value and verify the increment
    test_queue.entry_count = 100;

    track_queue_entry_added(&test_queue);

    TEST_ASSERT_EQUAL(101, test_queue.entry_count);
}

void test_track_queue_entry_added_large_count(void) {
    // Test incrementing to a large count to verify no overflow issues
    test_queue.entry_count = 0;

    for (size_t i = 0; i < 1000; i++) {
        track_queue_entry_added(&test_queue);
    }

    TEST_ASSERT_EQUAL(1000, test_queue.entry_count);
}

//=============================================================================
// Field Isolation Tests
//=============================================================================

void test_track_queue_entry_added_does_not_affect_other_fields(void) {
    // Verify that only entry_count is modified, not other fields
    test_queue.block_count = 42;
    test_queue.total_allocation = 99999;
    test_queue.metrics.virtual_bytes = 5000;
    test_queue.metrics.resident_bytes = 4000;
    test_queue.limits.max_blocks = 100;
    test_queue.limits.block_limit = 50;

    track_queue_entry_added(&test_queue);

    TEST_ASSERT_EQUAL(1, test_queue.entry_count);
    TEST_ASSERT_EQUAL(42, test_queue.block_count);
    TEST_ASSERT_EQUAL(99999, test_queue.total_allocation);
    TEST_ASSERT_EQUAL(5000, test_queue.metrics.virtual_bytes);
    TEST_ASSERT_EQUAL(4000, test_queue.metrics.resident_bytes);
    TEST_ASSERT_EQUAL(100, test_queue.limits.max_blocks);
    TEST_ASSERT_EQUAL(50, test_queue.limits.block_limit);
}

void test_track_queue_entry_added_preserves_block_data(void) {
    // Verify that block_sizes array and block_count are untouched
    for (int i = 0; i < MAX_QUEUE_BLOCKS; i++) {
        test_queue.block_sizes[i] = (size_t)(i + 1);
    }
    test_queue.block_count = MAX_QUEUE_BLOCKS;

    track_queue_entry_added(&test_queue);

    TEST_ASSERT_EQUAL(1, test_queue.entry_count);
    TEST_ASSERT_EQUAL(MAX_QUEUE_BLOCKS, test_queue.block_count);
    for (int i = 0; i < MAX_QUEUE_BLOCKS; i++) {
        TEST_ASSERT_EQUAL(i + 1, test_queue.block_sizes[i]);
    }
}

//=============================================================================
// Test Main Function
//=============================================================================

int main(void) {
    UNITY_BEGIN();

    // Basic increment tests
    RUN_TEST(test_track_queue_entry_added_from_zero);
    RUN_TEST(test_track_queue_entry_added_single_increment);
    RUN_TEST(test_track_queue_entry_added_multiple_increments);
    RUN_TEST(test_track_queue_entry_added_from_nonzero);
    RUN_TEST(test_track_queue_entry_added_large_count);

    // Field isolation tests
    RUN_TEST(test_track_queue_entry_added_does_not_affect_other_fields);
    RUN_TEST(test_track_queue_entry_added_preserves_block_data);

    return UNITY_END();
}
