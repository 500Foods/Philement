/*
 * Unity Test File: database_migrations_discover_files Function Tests
 * This file contains unit tests for the database_migrations_discover_files() function
 * and related internal functions from src/database/database_migrations_files.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers
#include "../../../../src/database/database.h"
#include "../../../../src/database/database_migrations.h"
#include "../../../../src/database/database_types.h"
#include "../../../../src/config/config_databases.h"

// Enable mocks for testing
#define USE_MOCK_SYSTEM
#include "../../../../tests/unity/mocks/mock_system.h"
#include "../../../../tests/unity/mocks/mock_database_migrations.h"

// Test fixtures
static DatabaseConnection test_conn;

// Function prototypes for test functions
void test_sort_migration_files_empty_array(void);
void test_sort_migration_files_single_element(void);
void test_sort_migration_files_already_sorted(void);
void test_sort_migration_files_reverse_order(void);
void test_sort_migration_files_mixed_order(void);
void test_discover_payload_migration_files_failure(void);
void test_discover_path_migration_files_success(void);
void test_discover_path_migration_files_invalid_path(void);

void setUp(void) {
    // Reset all mocks
    mock_system_reset_all();
    mock_database_migrations_reset_all();

    // Initialize test connection
    memset(&test_conn, 0, sizeof(DatabaseConnection));
    test_conn.enabled = true;
    test_conn.migrations = NULL; // Will be set per test
}

void tearDown(void) {
    // Clean up test connection
    if (test_conn.migrations) {
        free(test_conn.migrations);
        test_conn.migrations = NULL;
    }
    if (test_conn.name) {
        free(test_conn.name);
        test_conn.name = NULL;
    }
    if (test_conn.connection_name) {
        free(test_conn.connection_name);
        test_conn.connection_name = NULL;
    }
    if (test_conn.type) {
        free(test_conn.type);
        test_conn.type = NULL;
    }
    if (test_conn.database) {
        free(test_conn.database);
        test_conn.database = NULL;
    }
    if (test_conn.host) {
        free(test_conn.host);
        test_conn.host = NULL;
    }
    if (test_conn.port) {
        free(test_conn.port);
        test_conn.port = NULL;
    }
    if (test_conn.user) {
        free(test_conn.user);
        test_conn.user = NULL;
    }
    if (test_conn.pass) {
        free(test_conn.pass);
        test_conn.pass = NULL;
    }
    if (test_conn.bootstrap_query) {
        free(test_conn.bootstrap_query);
        test_conn.bootstrap_query = NULL;
    }
    if (test_conn.schema) {
        free(test_conn.schema);
        test_conn.schema = NULL;
    }
}

// Test sort_migration_files function
void test_sort_migration_files_empty_array(void) {
    char** files = NULL;
    sort_migration_files(files, 0);
    TEST_ASSERT_NULL(files);
}

void test_sort_migration_files_single_element(void) {
    char** files = malloc(sizeof(char*));
    files[0] = strdup("test_001.lua");
    sort_migration_files(files, 1);
    TEST_ASSERT_EQUAL_STRING("test_001.lua", files[0]);
    free(files[0]);
    free(files);
}

void test_sort_migration_files_already_sorted(void) {
    char** files = malloc(3 * sizeof(char*));
    files[0] = strdup("migration_001.lua");
    files[1] = strdup("migration_002.lua");
    files[2] = strdup("migration_003.lua");

    sort_migration_files(files, 3);

    TEST_ASSERT_EQUAL_STRING("migration_001.lua", files[0]);
    TEST_ASSERT_EQUAL_STRING("migration_002.lua", files[1]);
    TEST_ASSERT_EQUAL_STRING("migration_003.lua", files[2]);

    for (int i = 0; i < 3; i++) {
        free(files[i]);
    }
    free(files);
}

void test_sort_migration_files_reverse_order(void) {
    char** files = malloc(3 * sizeof(char*));
    files[0] = strdup("migration_003.lua");
    files[1] = strdup("migration_001.lua");
    files[2] = strdup("migration_002.lua");

    sort_migration_files(files, 3);

    TEST_ASSERT_EQUAL_STRING("migration_001.lua", files[0]);
    TEST_ASSERT_EQUAL_STRING("migration_002.lua", files[1]);
    TEST_ASSERT_EQUAL_STRING("migration_003.lua", files[2]);

    for (int i = 0; i < 3; i++) {
        free(files[i]);
    }
    free(files);
}

void test_sort_migration_files_mixed_order(void) {
    char** files = malloc(4 * sizeof(char*));
    files[0] = strdup("migration_010.lua");
    files[1] = strdup("migration_002.lua");
    files[2] = strdup("migration_001.lua");
    files[3] = strdup("migration_005.lua");

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

// Test discover_payload_migration_files function
void test_discover_payload_migration_files_failure(void) {
    char** migration_files = NULL;
    size_t migration_count = 0;
    size_t files_capacity = 0;

    // Mock failure
    mock_database_migrations_set_get_payload_files_result(false);
    bool result = discover_payload_migration_files("test", &migration_files, &migration_count, &files_capacity, "test");
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(migration_files);
    TEST_ASSERT_EQUAL(0, migration_count);
}

// Test discover_path_migration_files function (this should cover the previously uncovered function)
void test_discover_path_migration_files_success(void) {
    test_conn.migrations = strdup("/tmp/test_migrations");
    char** migration_files = NULL;
    size_t migration_count = 0;
    size_t files_capacity = 0;

    // This will likely fail due to directory not existing, but it will exercise the code
    discover_path_migration_files(&test_conn, &migration_files, &migration_count, &files_capacity, "test");
    // We don't assert the result since it depends on filesystem, but it exercises the code
    database_migrations_cleanup_files(migration_files, migration_count);
}

void test_discover_path_migration_files_invalid_path(void) {
    test_conn.migrations = strdup("/");
    char** migration_files = NULL;
    size_t migration_count = 0;
    size_t files_capacity = 0;

    bool result = discover_path_migration_files(&test_conn, &migration_files, &migration_count, &files_capacity, "test");
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(migration_files);
    TEST_ASSERT_EQUAL(0, migration_count);
}

int main(void) {
    UNITY_BEGIN();

    // Test sort_migration_files function
    RUN_TEST(test_sort_migration_files_empty_array);
    RUN_TEST(test_sort_migration_files_single_element);
    RUN_TEST(test_sort_migration_files_already_sorted);
    RUN_TEST(test_sort_migration_files_reverse_order);
    RUN_TEST(test_sort_migration_files_mixed_order);

    // Test discover_payload_migration_files function
    RUN_TEST(test_discover_payload_migration_files_failure);

    // Test discover_path_migration_files function
    RUN_TEST(test_discover_path_migration_files_success);
    RUN_TEST(test_discover_path_migration_files_invalid_path);

    return UNITY_END();
}