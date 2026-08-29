/*
 * Unity Test File: track_queue_entry_removed Function Tests
 * This file contains comprehensive unit tests for the track_queue_entry_removed() function
 * from src/utils/utils_queue.h
 *
 * Coverage Goals:
 * - Test that entry_count is decremented by exactly 1 per call
 * - Test the underflow guard: entry_count never goes below 0
 * - Test multiple consecutive decrements
 * - Test decrement from zero (should remain 0)
 * - Test that other queue fields are not affected
 * - Test increment-decrement interaction
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include the header for the function being tested
#include <src/utils/utils_queue.h>

// Forward declarations for the functions being tested
void track_queue_entry_removed(QueueMemoryMetrics *queue);
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
void test_track_queue_entry_removed_from_one(void);
void test_track_queue_entry_removed_single_decrement(void);
void test_track_queue_entry_removed_multiple_decrements(void);
void test_track_queue_entry_removed_from_zero(void);
void test_track_queue_entry_removed_does_not_go_negative(void);
void test_track_queue_entry_removed_alternating(void);
void test_track_queue_entry_removed_does_not_affect_other_fields(void);
void test_track_queue_entry_removed_large_count(void);

//=============================================================================
// Basic Decrement Tests
//=============================================================================

void test_track_queue_entry_removed_from_one(void) {
    // Start with entry_count at 1 and verify it goes to 0
    test_queue.entry_count = 1;

    track_queue_entry_removed(&test_queue);

    TEST_ASSERT_EQUAL(0, test_queue.entry_count);
}

void test_track_queue_entry_removed_single_decrement(void) {
    // Verify a single call decrements by exactly 1
    test_queue.entry_count = 5;

    track_queue_entry_removed(&test_queue);

    TEST_ASSERT_EQUAL(4, test_queue.entry_count);
}

void test_track_queue_entry_removed_multiple_decrements(void) {
    // Call the function multiple times and verify the count decreases
    test_queue.entry_count = 10;

    for (int i = 0; i < 5; i++) {
        track_queue_entry_removed(&test_queue);
    }

    TEST_ASSERT_EQUAL(5, test_queue.entry_count);
}

void test_track_queue_entry_removed_large_count(void) {
    // Test decrementing from a large count to verify no underflow issues
    test_queue.entry_count = 1000;

    for (size_t i = 0; i < 500; i++) {
        track_queue_entry_removed(&test_queue);
    }

    TEST_ASSERT_EQUAL(500, test_queue.entry_count);
}

//=============================================================================
// Underflow Guard Tests
//=============================================================================

void test_track_queue_entry_removed_from_zero(void) {
    // Decrementing from zero should not cause underflow
    test_queue.entry_count = 0;

    track_queue_entry_removed(&test_queue);

    TEST_ASSERT_EQUAL(0, test_queue.entry_count);
}

void test_track_queue_entry_removed_does_not_go_negative(void) {
    // Repeatedly decrement from zero — must stay at 0 (no unsigned underflow)
    test_queue.entry_count = 0;

    for (int i = 0; i < 10; i++) {
        track_queue_entry_removed(&test_queue);
    }

    TEST_ASSERT_EQUAL(0, test_queue.entry_count);
}

void test_track_queue_entry_removed_alternating(void) {
    // Interleave add and remove calls to verify balanced tracking
    test_queue.entry_count = 0;

    // Two added, one removed -> count should be 1
    track_queue_entry_removed(&test_queue);  // from 0, stays 0
    track_queue_entry_removed(&test_queue);  // from 0, stays 0

    track_queue_entry_added(&test_queue);
    track_queue_entry_added(&test_queue);
    track_queue_entry_added(&test_queue);    // count is now 3

    track_queue_entry_removed(&test_queue);  // count is now 2
    track_queue_entry_removed(&test_queue);  // count is now 1

    TEST_ASSERT_EQUAL(1, test_queue.entry_count);
}

//=============================================================================
// Field Isolation Tests
//=============================================================================

void test_track_queue_entry_removed_does_not_affect_other_fields(void) {
    // Verify that only entry_count is modified, not other fields
    test_queue.entry_count = 10;
    test_queue.block_count = 42;
    test_queue.total_allocation = 99999;
    test_queue.metrics.virtual_bytes = 5000;
    test_queue.metrics.resident_bytes = 4000;
    test_queue.limits.max_blocks = 100;
    test_queue.limits.block_limit = 50;

    track_queue_entry_removed(&test_queue);

    TEST_ASSERT_EQUAL(9, test_queue.entry_count);
    TEST_ASSERT_EQUAL(42, test_queue.block_count);
    TEST_ASSERT_EQUAL(99999, test_queue.total_allocation);
    TEST_ASSERT_EQUAL(5000, test_queue.metrics.virtual_bytes);
    TEST_ASSERT_EQUAL(4000, test_queue.metrics.resident_bytes);
    TEST_ASSERT_EQUAL(100, test_queue.limits.max_blocks);
    TEST_ASSERT_EQUAL(50, test_queue.limits.block_limit);
}

//=============================================================================
// Test Main Function
//=============================================================================

int main(void) {
    UNITY_BEGIN();

    // Basic decrement tests
    RUN_TEST(test_track_queue_entry_removed_from_one);
    RUN_TEST(test_track_queue_entry_removed_single_decrement);
    RUN_TEST(test_track_queue_entry_removed_multiple_decrements);
    RUN_TEST(test_track_queue_entry_removed_large_count);

    // Underflow guard tests
    RUN_TEST(test_track_queue_entry_removed_from_zero);
    RUN_TEST(test_track_queue_entry_removed_does_not_go_negative);
    RUN_TEST(test_track_queue_entry_removed_alternating);

    // Field isolation tests
    RUN_TEST(test_track_queue_entry_removed_does_not_affect_other_fields);

    return UNITY_END();
}
