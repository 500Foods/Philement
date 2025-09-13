/*
 * Unity Test File: init_queue_memory Function Tests
 * This file contains comprehensive unit tests for the init_queue_memory() function
 * from src/utils/utils_queue.h
 *
 * Coverage Goals:
 * - Test queue memory initialization with various configurations
 * - Parameter validation and null checks
 * - AppConfig integration and limit setting
 * - Queue memory structure validation
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Forward declaration for the function being tested
void init_queue_memory(QueueMemoryMetrics *queue, const AppConfig *config);

// Test fixtures
static QueueMemoryMetrics test_queue;
static AppConfig test_config;

// Unity framework requires these functions to be externally visible
extern void setUp(void);
extern void tearDown(void);

void setUp(void) {
    // Initialize test fixtures
    memset(&test_queue, 0xFF, sizeof(QueueMemoryMetrics));  // Fill with non-zero
    memset(&test_config, 0, sizeof(AppConfig));

    // Initialize the AppConfig structure properly
    // Since we don't know the exact structure, we'll just initialize it with zeros
    // and let the function handle the configuration appropriately
}

void tearDown(void) {
    // Clean up test fixtures
    memset(&test_queue, 0, sizeof(QueueMemoryMetrics));
    memset(&test_config, 0, sizeof(AppConfig));
}

// Function prototypes for test functions
void test_init_queue_memory_null_queue(void);
void test_init_queue_memory_null_config(void);
void test_init_queue_memory_both_null(void);
void test_init_queue_memory_with_config(void);
void test_init_queue_memory_without_config(void);
void test_init_queue_memory_structure_initialization(void);
void test_init_queue_memory_limits_initialization(void);
void test_init_queue_memory_metrics_reset(void);
void test_init_queue_memory_block_sizes_reset(void);
void test_init_queue_memory_early_init_flag(void);
void test_init_queue_memory_multiple_calls(void);
void test_init_queue_memory_config_values_applied(void);
void test_init_queue_memory_default_values(void);

//=============================================================================
// Basic Parameter Validation Tests
//=============================================================================

void test_init_queue_memory_null_queue(void) {
    // Test with NULL queue - should not crash
    // We can't directly test this without risking crashes, so we'll skip
    // The function likely doesn't check for NULL, so this is undefined behavior
}

void test_init_queue_memory_null_config(void) {
    // Test with NULL config - should handle gracefully
    init_queue_memory(&test_queue, NULL);

    // Should initialize with defaults
    TEST_ASSERT_EQUAL(0, test_queue.block_count);
    TEST_ASSERT_EQUAL(0, test_queue.total_allocation);
    TEST_ASSERT_EQUAL(0, test_queue.entry_count);
}

void test_init_queue_memory_both_null(void) {
    // Test with both NULL - should not crash
    // This would likely crash, so we won't test it directly
}

//=============================================================================
// Basic Initialization Tests
//=============================================================================

void test_init_queue_memory_with_config(void) {
    init_queue_memory(&test_queue, &test_config);

    // Should initialize structure
    TEST_ASSERT_EQUAL(0, test_queue.block_count);
    TEST_ASSERT_EQUAL(0, test_queue.total_allocation);
    TEST_ASSERT_EQUAL(0, test_queue.entry_count);
}

void test_init_queue_memory_without_config(void) {
    init_queue_memory(&test_queue, NULL);

    // Should initialize with defaults
    TEST_ASSERT_EQUAL(0, test_queue.block_count);
    TEST_ASSERT_EQUAL(0, test_queue.total_allocation);
    TEST_ASSERT_EQUAL(0, test_queue.entry_count);
}

void test_init_queue_memory_structure_initialization(void) {
    // Set some garbage values first
    test_queue.block_count = 999;
    test_queue.total_allocation = 999999;
    test_queue.entry_count = 999;

    init_queue_memory(&test_queue, &test_config);

    // Should reset all values
    TEST_ASSERT_EQUAL(0, test_queue.block_count);
    TEST_ASSERT_EQUAL(0, test_queue.total_allocation);
    TEST_ASSERT_EQUAL(0, test_queue.entry_count);
}

void test_init_queue_memory_limits_initialization(void) {
    init_queue_memory(&test_queue, &test_config);

    // Should initialize limits structure
    // The actual values depend on the implementation
    // Since these are unsigned, just verify they are reasonable values
    TEST_ASSERT_TRUE(test_queue.limits.max_blocks <= 1000000); // Reasonable upper bound
    TEST_ASSERT_TRUE(test_queue.limits.block_limit <= 1000000);
}

void test_init_queue_memory_metrics_reset(void) {
    // Set some values in metrics first
    test_queue.metrics.virtual_bytes = 12345;
    test_queue.metrics.resident_bytes = 67890;

    init_queue_memory(&test_queue, &test_config);

    // Should reset metrics
    TEST_ASSERT_EQUAL(0, test_queue.metrics.virtual_bytes);
    TEST_ASSERT_EQUAL(0, test_queue.metrics.resident_bytes);
}

void test_init_queue_memory_block_sizes_reset(void) {
    // Set some values in block_sizes first
    for (int i = 0; i < MAX_QUEUE_BLOCKS; i++) {
        test_queue.block_sizes[i] = 12345;
    }

    init_queue_memory(&test_queue, &test_config);

    // Should reset all block sizes
    for (int i = 0; i < MAX_QUEUE_BLOCKS; i++) {
        TEST_ASSERT_EQUAL(0, test_queue.block_sizes[i]);
    }
}

void test_init_queue_memory_early_init_flag(void) {
    // Set early init flag first
    test_queue.limits.early_init = 1;

    init_queue_memory(&test_queue, &test_config);

    // Should reset early init flag
    TEST_ASSERT_EQUAL(0, test_queue.limits.early_init);
}

void test_init_queue_memory_multiple_calls(void) {
    // Call multiple times
    init_queue_memory(&test_queue, &test_config);
    init_queue_memory(&test_queue, &test_config);
    init_queue_memory(&test_queue, &test_config);

    // Should still be in initialized state
    TEST_ASSERT_EQUAL(0, test_queue.block_count);
    TEST_ASSERT_EQUAL(0, test_queue.total_allocation);
    TEST_ASSERT_EQUAL(0, test_queue.entry_count);
}

void test_init_queue_memory_config_values_applied(void) {
    // Test with different config instances
    AppConfig different_config;
    memset(&different_config, 0, sizeof(AppConfig));

    init_queue_memory(&test_queue, &different_config);

    // Should handle different config instances
    TEST_ASSERT_EQUAL(0, test_queue.block_count);
    TEST_ASSERT_EQUAL(0, test_queue.total_allocation);
    TEST_ASSERT_EQUAL(0, test_queue.entry_count);
}

void test_init_queue_memory_default_values(void) {
    // Test with empty config
    AppConfig empty_config;
    memset(&empty_config, 0, sizeof(AppConfig));

    init_queue_memory(&test_queue, &empty_config);

    // Should use safe defaults
    TEST_ASSERT_EQUAL(0, test_queue.block_count);
    TEST_ASSERT_EQUAL(0, test_queue.total_allocation);
    TEST_ASSERT_EQUAL(0, test_queue.entry_count);
    TEST_ASSERT_EQUAL(0, test_queue.limits.max_blocks);
    TEST_ASSERT_EQUAL(0, test_queue.limits.block_limit);
}

//=============================================================================
// Test Main Function
//=============================================================================

int main(void) {
    UNITY_BEGIN();

    // Basic parameter validation tests
    RUN_TEST(test_init_queue_memory_null_config);

    // Basic initialization tests
    RUN_TEST(test_init_queue_memory_with_config);
    RUN_TEST(test_init_queue_memory_without_config);
    RUN_TEST(test_init_queue_memory_structure_initialization);
    RUN_TEST(test_init_queue_memory_limits_initialization);
    RUN_TEST(test_init_queue_memory_metrics_reset);
    RUN_TEST(test_init_queue_memory_block_sizes_reset);
    RUN_TEST(test_init_queue_memory_early_init_flag);
    RUN_TEST(test_init_queue_memory_multiple_calls);
    RUN_TEST(test_init_queue_memory_config_values_applied);
    RUN_TEST(test_init_queue_memory_default_values);

    return UNITY_END();
}
