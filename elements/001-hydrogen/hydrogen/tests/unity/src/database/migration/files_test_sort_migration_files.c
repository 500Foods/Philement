/*
 * Unity Test File: sort_migration_files Function Tests
 * This file contains comprehensive unit tests for the sort_migration_files() function
 * from src/database/migration/files.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers
#include <src/database/database.h>
#include <src/database/migration/migration.h>

// Forward declarations
void sort_migration_files(char** migration_files, size_t migration_count);

// Test function prototypes
void test_sort_migration_files_empty_array(void);
void test_sort_migration_files_single_element(void);
void test_sort_migration_files_already_sorted(void);
void test_sort_migration_files_reverse_order(void);
void test_sort_migration_files_mixed_order(void);
void test_sort_migration_files_different_prefixes(void);
void test_sort_migration_files_large_numbers(void);
void test_sort_migration_files_two_elements_swap(void);
void test_sort_migration_files_two_elements_sorted(void);
void test_sort_migration_files_no_underscore(void);
void test_sort_migration_files_many_elements(void);

void setUp(void) {
    // No setup needed for sort_migration_files tests
}

void tearDown(void) {
    // No teardown needed for sort_migration_files tests
}

// Test: Empty array (migration_count = 0)
void test_sort_migration_files_empty_array(void) {
    char** files = NULL;
    sort_migration_files(files, 0);
    TEST_ASSERT_NULL(files);
}

// Test: Single element (migration_count = 1)
void test_sort_migration_files_single_element(void) {
    char** files = malloc(sizeof(char*));
    TEST_ASSERT_NOT_NULL(files);

    files[0] = strdup("test_001.lua");
    TEST_ASSERT_NOT_NULL(files[0]);

    sort_migration_files(files, 1);
    TEST_ASSERT_EQUAL_STRING("test_001.lua", files[0]);

    free(files[0]);
    free(files);
}

// Test: Already sorted array
void test_sort_migration_files_already_sorted(void) {
    char** files = malloc(3 * sizeof(char*));
    TEST_ASSERT_NOT_NULL(files);

    files[0] = strdup("migration_001.lua");
    files[1] = strdup("migration_002.lua");
    files[2] = strdup("migration_003.lua");
    TEST_ASSERT_NOT_NULL(files[0]);
    TEST_ASSERT_NOT_NULL(files[1]);
    TEST_ASSERT_NOT_NULL(files[2]);

    sort_migration_files(files, 3);

    TEST_ASSERT_EQUAL_STRING("migration_001.lua", files[0]);
    TEST_ASSERT_EQUAL_STRING("migration_002.lua", files[1]);
    TEST_ASSERT_EQUAL_STRING("migration_003.lua", files[2]);

    for (int i = 0; i < 3; i++) {
        free(files[i]);
    }
    free(files);
}

// Test: Reverse order (requires swaps)
void test_sort_migration_files_reverse_order(void) {
    char** files = malloc(3 * sizeof(char*));
    TEST_ASSERT_NOT_NULL(files);

    files[0] = strdup("migration_003.lua");
    files[1] = strdup("migration_002.lua");
    files[2] = strdup("migration_001.lua");
    TEST_ASSERT_NOT_NULL(files[0]);
    TEST_ASSERT_NOT_NULL(files[1]);
    TEST_ASSERT_NOT_NULL(files[2]);

    sort_migration_files(files, 3);

    TEST_ASSERT_EQUAL_STRING("migration_001.lua", files[0]);
    TEST_ASSERT_EQUAL_STRING("migration_002.lua", files[1]);
    TEST_ASSERT_EQUAL_STRING("migration_003.lua", files[2]);

    for (int i = 0; i < 3; i++) {
        free(files[i]);
    }
    free(files);
}

// Test: Mixed order with larger numbers
void test_sort_migration_files_mixed_order(void) {
    char** files = malloc(4 * sizeof(char*));
    TEST_ASSERT_NOT_NULL(files);

    files[0] = strdup("migration_010.lua");
    files[1] = strdup("migration_002.lua");
    files[2] = strdup("migration_001.lua");
    files[3] = strdup("migration_005.lua");
    TEST_ASSERT_NOT_NULL(files[0]);
    TEST_ASSERT_NOT_NULL(files[1]);
    TEST_ASSERT_NOT_NULL(files[2]);
    TEST_ASSERT_NOT_NULL(files[3]);

    sort_migration_files(files, 4);

    TEST_ASSERT_EQUAL_STRING("migration_001.lua", files[0]);
    TEST_ASSERT_EQUAL_STRING("migration_002.lua", files[1]);
    TEST_ASSERT_EQUAL_STRING("migration_005.lua", files[2]);
    TEST_ASSERT_EQUAL_STRING("migration_010.lua", files[3]);

    for (int i = 0; i < 4; i++) {
        free(files[i]);
    }
    free(files);
}

// Test: Files with different prefixes but same numbers
void test_sort_migration_files_different_prefixes(void) {
    char** files = malloc(3 * sizeof(char*));
    TEST_ASSERT_NOT_NULL(files);

    files[0] = strdup("app_migration_003.lua");
    files[1] = strdup("app_migration_001.lua");
    files[2] = strdup("app_migration_002.lua");
    TEST_ASSERT_NOT_NULL(files[0]);
    TEST_ASSERT_NOT_NULL(files[1]);
    TEST_ASSERT_NOT_NULL(files[2]);

    sort_migration_files(files, 3);

    TEST_ASSERT_EQUAL_STRING("app_migration_001.lua", files[0]);
    TEST_ASSERT_EQUAL_STRING("app_migration_002.lua", files[1]);
    TEST_ASSERT_EQUAL_STRING("app_migration_003.lua", files[2]);

    for (int i = 0; i < 3; i++) {
        free(files[i]);
    }
    free(files);
}

// Test: Large numbers (up to 6 digits)
void test_sort_migration_files_large_numbers(void) {
    char** files = malloc(3 * sizeof(char*));
    TEST_ASSERT_NOT_NULL(files);

    files[0] = strdup("migration_999999.lua");
    files[1] = strdup("migration_000001.lua");
    files[2] = strdup("migration_100000.lua");
    TEST_ASSERT_NOT_NULL(files[0]);
    TEST_ASSERT_NOT_NULL(files[1]);
    TEST_ASSERT_NOT_NULL(files[2]);

    sort_migration_files(files, 3);

    TEST_ASSERT_EQUAL_STRING("migration_000001.lua", files[0]);
    TEST_ASSERT_EQUAL_STRING("migration_100000.lua", files[1]);
    TEST_ASSERT_EQUAL_STRING("migration_999999.lua", files[2]);

    for (int i = 0; i < 3; i++) {
        free(files[i]);
    }
    free(files);
}

// Test: Two elements requiring swap
void test_sort_migration_files_two_elements_swap(void) {
    char** files = malloc(2 * sizeof(char*));
    TEST_ASSERT_NOT_NULL(files);

    files[0] = strdup("migration_002.lua");
    files[1] = strdup("migration_001.lua");
    TEST_ASSERT_NOT_NULL(files[0]);
    TEST_ASSERT_NOT_NULL(files[1]);

    sort_migration_files(files, 2);

    TEST_ASSERT_EQUAL_STRING("migration_001.lua", files[0]);
    TEST_ASSERT_EQUAL_STRING("migration_002.lua", files[1]);

    for (int i = 0; i < 2; i++) {
        free(files[i]);
    }
    free(files);
}

// Test: Two elements already sorted
void test_sort_migration_files_two_elements_sorted(void) {
    char** files = malloc(2 * sizeof(char*));
    TEST_ASSERT_NOT_NULL(files);

    files[0] = strdup("migration_001.lua");
    files[1] = strdup("migration_002.lua");
    TEST_ASSERT_NOT_NULL(files[0]);
    TEST_ASSERT_NOT_NULL(files[1]);

    sort_migration_files(files, 2);

    TEST_ASSERT_EQUAL_STRING("migration_001.lua", files[0]);
    TEST_ASSERT_EQUAL_STRING("migration_002.lua", files[1]);

    for (int i = 0; i < 2; i++) {
        free(files[i]);
    }
    free(files);
}

// Test: Files with no underscore (edge case - strrchr returns NULL)
void test_sort_migration_files_no_underscore(void) {
    char** files = malloc(2 * sizeof(char*));
    TEST_ASSERT_NOT_NULL(files);

    files[0] = strdup("migration.lua");
    files[1] = strdup("test.lua");
    TEST_ASSERT_NOT_NULL(files[0]);
    TEST_ASSERT_NOT_NULL(files[1]);

    // This should not crash even though strrchr returns NULL
    sort_migration_files(files, 2);

    // Files should remain in original order since num1 and num2 are both 0
    TEST_ASSERT_EQUAL_STRING("migration.lua", files[0]);
    TEST_ASSERT_EQUAL_STRING("test.lua", files[1]);

    for (int i = 0; i < 2; i++) {
        free(files[i]);
    }
    free(files);
}

// Test: Complex scenario with many elements
void test_sort_migration_files_many_elements(void) {
    char** files = malloc(6 * sizeof(char*));
    TEST_ASSERT_NOT_NULL(files);

    files[0] = strdup("migration_050.lua");
    files[1] = strdup("migration_010.lua");
    files[2] = strdup("migration_100.lua");
    files[3] = strdup("migration_005.lua");
    files[4] = strdup("migration_075.lua");
    files[5] = strdup("migration_001.lua");
    for (int i = 0; i < 6; i++) {
        TEST_ASSERT_NOT_NULL(files[i]);
    }

    sort_migration_files(files, 6);

    TEST_ASSERT_EQUAL_STRING("migration_001.lua", files[0]);
    TEST_ASSERT_EQUAL_STRING("migration_005.lua", files[1]);
    TEST_ASSERT_EQUAL_STRING("migration_010.lua", files[2]);
    TEST_ASSERT_EQUAL_STRING("migration_050.lua", files[3]);
    TEST_ASSERT_EQUAL_STRING("migration_075.lua", files[4]);
    TEST_ASSERT_EQUAL_STRING("migration_100.lua", files[5]);

    for (int i = 0; i < 6; i++) {
        free(files[i]);
    }
    free(files);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_sort_migration_files_empty_array);
    RUN_TEST(test_sort_migration_files_single_element);
    RUN_TEST(test_sort_migration_files_already_sorted);
    RUN_TEST(test_sort_migration_files_reverse_order);
    RUN_TEST(test_sort_migration_files_mixed_order);
    RUN_TEST(test_sort_migration_files_different_prefixes);
    RUN_TEST(test_sort_migration_files_large_numbers);
    RUN_TEST(test_sort_migration_files_two_elements_swap);
    RUN_TEST(test_sort_migration_files_two_elements_sorted);
    RUN_TEST(test_sort_migration_files_no_underscore);
    RUN_TEST(test_sort_migration_files_many_elements);

    return UNITY_END();
}
