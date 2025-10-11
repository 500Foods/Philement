/*
 * Unity Test File: Beryllium beryllium_free_stats Function Tests
 * This file contains comprehensive unit tests for the beryllium_free_stats() function
 * from src/print/beryllium.c
 *
 * Coverage Goals:
 * - Test memory cleanup functionality
 * - Test handling of NULL pointers
 * - Test cleanup of complex data structures
 * - Test proper resource release
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the beryllium module
#include "../../../../src/print/beryllium.h"

// Forward declaration for the function being tested
void beryllium_free_stats(BerylliumStats *stats);

void setUp(void) {
    // No setup needed for memory cleanup tests
}

void tearDown(void) {
    // No teardown needed for memory cleanup tests
}

// Function prototypes for test functions
void test_beryllium_free_stats_null_pointer(void);
void test_beryllium_free_stats_empty_stats(void);
void test_beryllium_free_stats_with_object_times_only(void);
void test_beryllium_free_stats_with_object_infos_only(void);
void test_beryllium_free_stats_complete_structure(void);
void test_beryllium_free_stats_partial_allocation(void);
void test_beryllium_free_stats_double_free_protection(void);
void test_beryllium_free_stats_large_structure(void);
void test_beryllium_free_stats_memory_cleanup_verification(void);

//=============================================================================
// Basic Memory Cleanup Tests
//=============================================================================

void test_beryllium_free_stats_null_pointer(void) {
    // Test that function handles NULL pointer gracefully
    beryllium_free_stats(NULL);
    // Should not crash - this is a success if we reach this point
    TEST_ASSERT_TRUE(true);
}

void test_beryllium_free_stats_empty_stats(void) {
    // Test with empty BerylliumStats structure
    BerylliumStats stats = {0};

    // Function should handle empty structure gracefully
    beryllium_free_stats(&stats);

    // Verify structure is properly reset
    TEST_ASSERT_NULL(stats.object_times);
    TEST_ASSERT_NULL(stats.object_infos);
    TEST_ASSERT_EQUAL_INT(0, stats.num_objects);
    TEST_ASSERT_EQUAL_INT(0, stats.layer_count_slicer);
    TEST_ASSERT_FALSE(stats.success);
}

void test_beryllium_free_stats_with_object_times_only(void) {
    // Test cleanup with only object_times allocated
    BerylliumStats stats = {0};
    const int TEST_MAX_LAYERS = 100; // Use a reasonable value for testing

    // Allocate object_times array
    stats.object_times = calloc((size_t)TEST_MAX_LAYERS, sizeof(double*));
    stats.layer_count_slicer = TEST_MAX_LAYERS;

    // Allocate a few layer arrays
    for (int i = 0; i < 5; i++) {
        stats.object_times[i] = calloc((size_t)10, sizeof(double));
    }

    // Free the structure
    beryllium_free_stats(&stats);

    // Verify cleanup
    TEST_ASSERT_NULL(stats.object_times);
    TEST_ASSERT_EQUAL_INT(0, stats.layer_count_slicer);
}

void test_beryllium_free_stats_with_object_infos_only(void) {
    // Test cleanup with only object_infos allocated
    BerylliumStats stats = {0};

    // Allocate object_infos array
    stats.num_objects = 3;
    stats.object_infos = calloc((size_t)3, sizeof(ObjectInfo));

    // Initialize object names
    stats.object_infos[0].name = strdup("object1");
    stats.object_infos[1].name = strdup("object2");
    stats.object_infos[2].name = strdup("object3");

    // Free the structure
    beryllium_free_stats(&stats);

    // Verify cleanup
    TEST_ASSERT_NULL(stats.object_infos);
    TEST_ASSERT_EQUAL_INT(0, stats.num_objects);
}

void test_beryllium_free_stats_complete_structure(void) {
    // Test cleanup with fully populated structure
    BerylliumStats stats = {0};
    const int TEST_MAX_LAYERS_COMPLETE = 50;
    const int NUM_OBJECTS = 5;

    // Allocate and populate object_times
    stats.object_times = calloc((size_t)TEST_MAX_LAYERS_COMPLETE, sizeof(double*));
    stats.layer_count_slicer = TEST_MAX_LAYERS_COMPLETE;

    for (int i = 0; i < TEST_MAX_LAYERS_COMPLETE; i++) {
        stats.object_times[i] = calloc((size_t)NUM_OBJECTS, sizeof(double));
        // Fill with some test data
        for (int j = 0; j < NUM_OBJECTS; j++) {
            stats.object_times[i][j] = (double)(i * NUM_OBJECTS + j);
        }
    }

    // Allocate and populate object_infos
    stats.num_objects = NUM_OBJECTS;
    stats.object_infos = calloc((size_t)NUM_OBJECTS, sizeof(ObjectInfo));

    for (int i = 0; i < NUM_OBJECTS; i++) {
        char name[32];
        sprintf(name, "object_%d", i);
        stats.object_infos[i].name = strdup(name);
        stats.object_infos[i].index = i;
    }

    // Set other fields
    stats.success = true;
    stats.total_lines = 1000;
    stats.gcode_lines = 800;

    // Free the structure
    beryllium_free_stats(&stats);

    // Verify complete cleanup
    TEST_ASSERT_NULL(stats.object_times);
    TEST_ASSERT_NULL(stats.object_infos);
    TEST_ASSERT_EQUAL_INT(0, stats.num_objects);
    TEST_ASSERT_EQUAL_INT(0, stats.layer_count_slicer);
    TEST_ASSERT_TRUE(stats.success); // Should be preserved (free function doesn't reset success flag)
}

void test_beryllium_free_stats_partial_allocation(void) {
    // Test cleanup with partially allocated structure
    BerylliumStats stats = {0};

    // Allocate object_times but not all layer arrays
    stats.object_times = calloc((size_t)10, sizeof(double*));
    stats.layer_count_slicer = 10;
    (void)stats.layer_count_slicer; // Suppress unused variable warning

    // Only allocate some layer arrays (simulating partial initialization)
    stats.object_times[0] = calloc((size_t)5, sizeof(double));
    stats.object_times[5] = calloc((size_t)3, sizeof(double));
    (void)stats.object_times; // Suppress unused variable warning

    // Leave others as NULL (which is valid)

    // Free the structure
    beryllium_free_stats(&stats);

    // Verify cleanup handles partial allocation correctly
    TEST_ASSERT_NULL(stats.object_times);
    TEST_ASSERT_EQUAL_INT(0, stats.layer_count_slicer);
}

void test_beryllium_free_stats_double_free_protection(void) {
    // Test that double-free is handled gracefully
    BerylliumStats stats = {0};

    // Allocate structure
    stats.object_times = calloc((size_t)5, sizeof(double*));
    stats.layer_count_slicer = 5;
    stats.object_times[0] = calloc((size_t)3, sizeof(double));

    stats.object_infos = calloc((size_t)2, sizeof(ObjectInfo));
    stats.num_objects = 2;
    stats.object_infos[0].name = strdup("test");

    // Free once
    beryllium_free_stats(&stats);

    // Free again (should be safe)
    beryllium_free_stats(&stats);

    // Verify structure remains in reset state
    TEST_ASSERT_NULL(stats.object_times);
    TEST_ASSERT_NULL(stats.object_infos);
    TEST_ASSERT_EQUAL_INT(0, stats.num_objects);
    TEST_ASSERT_EQUAL_INT(0, stats.layer_count_slicer);
}

void test_beryllium_free_stats_large_structure(void) {
    // Test cleanup with large structure
    BerylliumStats stats = {0};
    const int LARGE_LAYER_COUNT = 1000;
    const int LARGE_OBJECT_COUNT = 100;

    // Allocate large object_times structure
    stats.object_times = calloc((size_t)LARGE_LAYER_COUNT, sizeof(double*));
    stats.layer_count_slicer = LARGE_LAYER_COUNT;

    // Allocate arrays for every 10th layer (to save time while still testing)
    for (int i = 0; i < LARGE_LAYER_COUNT; i += 10) {
        stats.object_times[i] = calloc((size_t)LARGE_OBJECT_COUNT, sizeof(double));
    }

    // Allocate large object_infos structure
    stats.num_objects = LARGE_OBJECT_COUNT;
    stats.object_infos = calloc((size_t)LARGE_OBJECT_COUNT, sizeof(ObjectInfo));

    for (int i = 0; i < LARGE_OBJECT_COUNT; i++) {
        char name[64];
        sprintf(name, "large_test_object_%d", i);
        stats.object_infos[i].name = strdup(name);
    }

    // Free the large structure
    beryllium_free_stats(&stats);

    // Verify complete cleanup of large structure
    TEST_ASSERT_NULL(stats.object_times);
    TEST_ASSERT_NULL(stats.object_infos);
    TEST_ASSERT_EQUAL_INT(0, stats.num_objects);
    TEST_ASSERT_EQUAL_INT(0, stats.layer_count_slicer);
}

void test_beryllium_free_stats_memory_cleanup_verification(void) {
    // Test that memory is actually freed (best effort verification)
    BerylliumStats stats = {0};

    // Allocate some memory
    stats.object_times = calloc((size_t)10, sizeof(double*));
    stats.layer_count_slicer = 10;
    stats.object_times[0] = calloc((size_t)5, sizeof(double));

    stats.object_infos = calloc((size_t)3, sizeof(ObjectInfo));
    stats.num_objects = 3;
    (void)stats.num_objects; // Suppress unused variable warning
    stats.object_infos[0].name = strdup("verification_test");
    stats.object_infos[1].name = strdup("another_object");
    stats.object_infos[2].name = strdup("final_object");

    // Store pointers for verification
    double** original_object_times = stats.object_times;
    ObjectInfo* original_object_infos = stats.object_infos;

    // Verify pointers are not NULL before cleanup
    TEST_ASSERT_NOT_NULL(original_object_times);
    TEST_ASSERT_NOT_NULL(original_object_infos);

    // Free the structure
    beryllium_free_stats(&stats);

    // Verify pointers are NULL after cleanup
    TEST_ASSERT_NULL(stats.object_times);
    TEST_ASSERT_NULL(stats.object_infos);

    // Verify counters are reset
    TEST_ASSERT_EQUAL_INT(0, stats.num_objects);
    TEST_ASSERT_EQUAL_INT(0, stats.layer_count_slicer);
    TEST_ASSERT_FALSE(stats.success);
}

//=============================================================================
// Test Main Function
//=============================================================================

int main(void) {
    UNITY_BEGIN();

    // Basic cleanup tests
    RUN_TEST(test_beryllium_free_stats_null_pointer);
    RUN_TEST(test_beryllium_free_stats_empty_stats);

    // Component cleanup tests
    RUN_TEST(test_beryllium_free_stats_with_object_times_only);
    RUN_TEST(test_beryllium_free_stats_with_object_infos_only);
    RUN_TEST(test_beryllium_free_stats_complete_structure);

    // Edge case tests
    RUN_TEST(test_beryllium_free_stats_partial_allocation);
    RUN_TEST(test_beryllium_free_stats_double_free_protection);

    // Performance and scale tests
    RUN_TEST(test_beryllium_free_stats_large_structure);
    RUN_TEST(test_beryllium_free_stats_memory_cleanup_verification);

    return UNITY_END();
}
