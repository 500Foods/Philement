/*
 * Unity Test File: cleanup_files Function Tests
 * This file contains comprehensive unit tests for the cleanup_files() function
 * from src/database/migration/files.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers
#include <src/database/database.h>
#include <src/database/migration/migration.h>

// Forward declarations
void cleanup_files(char** migration_files, size_t migration_count);

// Test: cleanup_files with NULL pointer and zero count
void test_cleanup_files_null_pointer_zero_count(void);

// Test: cleanup_files with NULL pointer and nonzero count
void test_cleanup_files_null_pointer_nonzero_count(void);

// Test: cleanup_files with single file
void test_cleanup_files_single_file(void);

// Test: cleanup_files with multiple files
void test_cleanup_files_multiple_files(void);

// Test: cleanup_files with many files
void test_cleanup_files_many_files(void);

// Test: cleanup_files with zero count and valid pointer
void test_cleanup_files_zero_count_valid_pointer(void);

void setUp(void) {
    // No setup needed for cleanup_files tests
}

void tearDown(void) {
    // No teardown needed for cleanup_files tests
}

void test_cleanup_files_null_pointer_zero_count(void) {
    // Should not crash with NULL pointer and zero count
    cleanup_files(NULL, 0);
    TEST_ASSERT_TRUE(true);  // If we get here, no crash occurred
}

void test_cleanup_files_null_pointer_nonzero_count(void) {
    // Should not crash with NULL pointer even if count is nonzero
    cleanup_files(NULL, 5);
    TEST_ASSERT_TRUE(true);  // If we get here, no crash occurred
}

void test_cleanup_files_single_file(void) {
    char** files = malloc(sizeof(char*));
    TEST_ASSERT_NOT_NULL(files);

    files[0] = strdup("migration_001.lua");
    TEST_ASSERT_NOT_NULL(files[0]);

    cleanup_files(files, 1);
    // After cleanup, memory should be freed (we can't directly test this,
    // but we verify the function doesn't crash)
    TEST_ASSERT_TRUE(true);
}

void test_cleanup_files_multiple_files(void) {
    char** files = malloc(3 * sizeof(char*));
    TEST_ASSERT_NOT_NULL(files);

    files[0] = strdup("migration_001.lua");
    files[1] = strdup("migration_002.lua");
    files[2] = strdup("migration_003.lua");
    TEST_ASSERT_NOT_NULL(files[0]);
    TEST_ASSERT_NOT_NULL(files[1]);
    TEST_ASSERT_NOT_NULL(files[2]);

    cleanup_files(files, 3);
    TEST_ASSERT_TRUE(true);
}

void test_cleanup_files_many_files(void) {
    char** files = malloc(10 * sizeof(char*));
    TEST_ASSERT_NOT_NULL(files);

    for (int i = 0; i < 10; i++) {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "migration_%03d.lua", i + 1);
        files[i] = strdup(buffer);
        TEST_ASSERT_NOT_NULL(files[i]);
    }

    cleanup_files(files, 10);
    TEST_ASSERT_TRUE(true);
}

void test_cleanup_files_zero_count_valid_pointer(void) {
    char** files = malloc(5 * sizeof(char*));
    TEST_ASSERT_NOT_NULL(files);

    // Allocate some files but tell cleanup to free zero files
    files[0] = strdup("migration_001.lua");
    TEST_ASSERT_NOT_NULL(files[0]);

    cleanup_files(files, 0);
    // The pointer should still be freed even with zero count
    TEST_ASSERT_TRUE(true);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_cleanup_files_null_pointer_zero_count);
    RUN_TEST(test_cleanup_files_null_pointer_nonzero_count);
    RUN_TEST(test_cleanup_files_single_file);
    RUN_TEST(test_cleanup_files_multiple_files);
    RUN_TEST(test_cleanup_files_many_files);
    RUN_TEST(test_cleanup_files_zero_count_valid_pointer);

    return UNITY_END();
}
