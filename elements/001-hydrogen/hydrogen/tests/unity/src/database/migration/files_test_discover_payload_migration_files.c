/*
 * Unity Test File: discover_payload_migration_files Function Tests
 * This file contains comprehensive unit tests for the discover_payload_migration_files() function
 * from src/database/migration/files.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers
#include <src/database/database.h>
#include <src/database/migration/migration.h>

// Forward declarations
bool discover_payload_migration_files(const char* migration_name, char*** migration_files,
                                     size_t* migration_count, size_t* files_capacity,
                                     const char* dqm_label);

// Test function prototypes
static void test_discover_payload_migration_files_null_migration_name(void);
static void test_discover_payload_migration_files_empty_migration_name(void);
static void test_discover_payload_migration_files_no_files(void);
static void test_discover_payload_migration_files_null_migration_files_ptr(void);
static void test_discover_payload_migration_files_null_migration_count_ptr(void);
static void test_discover_payload_migration_files_null_files_capacity_ptr(void);
static void test_discover_payload_migration_files_null_dqm_label(void);
static void test_discover_payload_migration_files_with_preallocated_files(void);
static void test_discover_payload_migration_files_large_migration_name(void);

void setUp(void) {
    // No setup needed
}

void tearDown(void) {
    // No teardown needed
}

// Test: Null migration name
static void test_discover_payload_migration_files_null_migration_name(void) {
    char** migration_files = NULL;
    size_t migration_count = 0;
    size_t files_capacity = 0;

    bool result = discover_payload_migration_files(NULL, &migration_files, &migration_count, &files_capacity, "test");
    TEST_ASSERT_FALSE(result);
}

// Test: Empty migration name
static void test_discover_payload_migration_files_empty_migration_name(void) {
    char** migration_files = NULL;
    size_t migration_count = 0;
    size_t files_capacity = 0;

    bool result = discover_payload_migration_files("", &migration_files, &migration_count, &files_capacity, "test");
    TEST_ASSERT_FALSE(result);
}

// Test: No files found
static void test_discover_payload_migration_files_no_files(void) {
    char** migration_files = NULL;
    size_t migration_count = 0;
    size_t files_capacity = 0;

    bool result = discover_payload_migration_files("nonexistent", &migration_files, &migration_count, &files_capacity, "test");
    TEST_ASSERT_FALSE(result);
}

// Test: Null migration_files pointer
static void test_discover_payload_migration_files_null_migration_files_ptr(void) {
    size_t migration_count = 0;
    size_t files_capacity = 0;

    bool result = discover_payload_migration_files("test", NULL, &migration_count, &files_capacity, "test");
    TEST_ASSERT_FALSE(result);
}

// Test: Null migration_count pointer
static void test_discover_payload_migration_files_null_migration_count_ptr(void) {
    char** migration_files = NULL;
    size_t files_capacity = 0;

    bool result = discover_payload_migration_files("test", &migration_files, NULL, &files_capacity, "test");
    TEST_ASSERT_FALSE(result);
}

// Test: Null files_capacity pointer
static void test_discover_payload_migration_files_null_files_capacity_ptr(void) {
    char** migration_files = NULL;
    size_t migration_count = 0;

    bool result = discover_payload_migration_files("test", &migration_files, &migration_count, NULL, "test");
    TEST_ASSERT_FALSE(result);
}

// Test: Null dqm_label
static void test_discover_payload_migration_files_null_dqm_label(void) {
    char** migration_files = NULL;
    size_t migration_count = 0;
    size_t files_capacity = 0;

    bool result = discover_payload_migration_files("test", &migration_files, &migration_count, &files_capacity, NULL);
    TEST_ASSERT_FALSE(result);
}

// Test: With preallocated files array
static void test_discover_payload_migration_files_with_preallocated_files(void) {
    char** migration_files = malloc(10 * sizeof(char*));
    TEST_ASSERT_NOT_NULL(migration_files);
    
    size_t migration_count = 0;
    size_t files_capacity = 10;

    discover_payload_migration_files("test", &migration_files, &migration_count, &files_capacity, "test");
    
    free(migration_files);
}

// Test: Large migration name
static void test_discover_payload_migration_files_large_migration_name(void) {
    char** migration_files = NULL;
    size_t migration_count = 0;
    size_t files_capacity = 0;
    
    char large_name[300];
    memset(large_name, 'a', sizeof(large_name) - 1);
    large_name[sizeof(large_name) - 1] = '\0';

    bool result = discover_payload_migration_files(large_name, &migration_files, &migration_count, &files_capacity, "test");
    TEST_ASSERT_FALSE(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_discover_payload_migration_files_null_migration_name);
    RUN_TEST(test_discover_payload_migration_files_empty_migration_name);
    RUN_TEST(test_discover_payload_migration_files_no_files);
    RUN_TEST(test_discover_payload_migration_files_null_migration_files_ptr);
    RUN_TEST(test_discover_payload_migration_files_null_migration_count_ptr);
    RUN_TEST(test_discover_payload_migration_files_null_files_capacity_ptr);
    RUN_TEST(test_discover_payload_migration_files_null_dqm_label);
    RUN_TEST(test_discover_payload_migration_files_with_preallocated_files);
    RUN_TEST(test_discover_payload_migration_files_large_migration_name);

    return UNITY_END();
}
