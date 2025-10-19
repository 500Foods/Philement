/*
 * Unity Test File: discover_path_migration_files Function Tests with Real Files
 * This file tests the discover_path_migration_files() function with actual migration files
 * to improve code coverage for the file discovery logic.
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers
#include <src/database/database.h>
#include <src/database/migration/migration.h>
#include <sys/stat.h>

// Forward declarations
bool discover_path_migration_files(const DatabaseConnection* conn_config, char*** migration_files,
                                 size_t* migration_count, size_t* files_capacity,
                                 const char* dqm_label);

// Test function prototypes
static void test_discover_path_migration_files_with_valid_files(void);
static void test_discover_path_migration_files_with_mixed_files(void);
static void test_discover_path_migration_files_with_no_matching_files(void);
static void test_discover_path_migration_files_capacity_expansion(void);

// Test fixtures
static const char* test_dir = "/tmp/hydrogen_test_migrations";
static DatabaseConnection test_conn;

void setUp(void) {
    memset(&test_conn, 0, sizeof(DatabaseConnection));
    
    // Create test directory
    mkdir(test_dir, 0755);
}

void tearDown(void) {
    if (test_conn.migrations) {
        free(test_conn.migrations);
        test_conn.migrations = NULL;
    }
    
    // Clean up test files
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "rm -rf %s", test_dir);
    system(cmd);
}

// Helper function to create a test file
static void create_test_file(const char* filename) {
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/%s", test_dir, filename);
    FILE* f = fopen(filepath, "w");
    if (f) {
        fprintf(f, "-- Test migration file\n");
        fclose(f);
    }
}

// Test: Discover valid migration files
static void test_discover_path_migration_files_with_valid_files(void) {
    // Create test migration files
    create_test_file("testmig_001.lua");
    create_test_file("testmig_002.lua");
    create_test_file("testmig_003.lua");
    
    // Set up connection to point to test directory
    char migration_path[512];
    snprintf(migration_path, sizeof(migration_path), "%s/testmig", test_dir);
    test_conn.migrations = strdup(migration_path);
    
    char** migration_files = NULL;
    size_t migration_count = 0;
    size_t files_capacity = 0;
    
    bool result = discover_path_migration_files(&test_conn, &migration_files, &migration_count, &files_capacity, "test");
    
    // Should succeed and find 3 files
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(3, migration_count);
    TEST_ASSERT_NOT_NULL(migration_files);
    
    // Clean up
    for (size_t i = 0; i < migration_count; i++) {
        free(migration_files[i]);
    }
    free(migration_files);
}

// Test: Discover files with mixed valid and invalid files
static void test_discover_path_migration_files_with_mixed_files(void) {
    // Create mix of valid and invalid files
    create_test_file("testmig_001.lua");
    create_test_file("testmig_002.lua");
    create_test_file("testmig_999.lua");
    create_test_file("other_001.lua");        // Wrong prefix
    create_test_file("testmig.lua");          // No number (no underscore after prefix)
    create_test_file("testmig_abc.lua");      // Non-numeric but valid length (function doesn't validate numeric)
    create_test_file("testmig_001.txt");      // Wrong extension
    create_test_file("testmig_1234567.lua");  // Too long (7 digits, max is 6)
    
    char migration_path[512];
    snprintf(migration_path, sizeof(migration_path), "%s/testmig", test_dir);
    test_conn.migrations = strdup(migration_path);
    
    char** migration_files = NULL;
    size_t migration_count = 0;
    size_t files_capacity = 0;
    
    bool result = discover_path_migration_files(&test_conn, &migration_files, &migration_count, &files_capacity, "test");
    
    // Should succeed and find 4 files (001, 002, 999, abc - function doesn't validate numeric content)
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(4, migration_count);
    
    // Clean up
    for (size_t i = 0; i < migration_count; i++) {
        free(migration_files[i]);
    }
    free(migration_files);
}

// Test: Capacity expansion with many files
static void test_discover_path_migration_files_capacity_expansion(void) {
    // Create many migration files to trigger capacity expansion (initial capacity is 10)
    for (int i = 1; i <= 15; i++) {
        char filename[64];
        snprintf(filename, sizeof(filename), "testmig_%03d.lua", i);
        create_test_file(filename);
    }
    
    char migration_path[512];
    snprintf(migration_path, sizeof(migration_path), "%s/testmig", test_dir);
    test_conn.migrations = strdup(migration_path);
    
    char** migration_files = NULL;
    size_t migration_count = 0;
    size_t files_capacity = 0;
    
    bool result = discover_path_migration_files(&test_conn, &migration_files, &migration_count, &files_capacity, "test");
    
    // Should succeed and find all 15 files, triggering capacity expansion
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(15, migration_count);
    TEST_ASSERT_GREATER_OR_EQUAL(15, files_capacity);
    
    // Clean up
    for (size_t i = 0; i < migration_count; i++) {
        free(migration_files[i]);
    }
    free(migration_files);
}

// Test: No matching files (but directory exists)
static void test_discover_path_migration_files_with_no_matching_files(void) {
    // Create non-matching files
    create_test_file("other_001.lua");
    create_test_file("different_002.lua");
    
    char migration_path[512];
    snprintf(migration_path, sizeof(migration_path), "%s/testmig", test_dir);
    test_conn.migrations = strdup(migration_path);
    
    char** migration_files = NULL;
    size_t migration_count = 0;
    size_t files_capacity = 0;
    
    bool result = discover_path_migration_files(&test_conn, &migration_files, &migration_count, &files_capacity, "test");
    
    // Should succeed but find 0 files
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(0, migration_count);
    TEST_ASSERT_NULL(migration_files);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_discover_path_migration_files_with_valid_files);
    RUN_TEST(test_discover_path_migration_files_with_mixed_files);
    RUN_TEST(test_discover_path_migration_files_with_no_matching_files);
    RUN_TEST(test_discover_path_migration_files_capacity_expansion);
    
    return UNITY_END();
}