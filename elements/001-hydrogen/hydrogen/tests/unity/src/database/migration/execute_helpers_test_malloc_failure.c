/*
 * Unity Test File: execute malloc failure tests
 * This file contains unit tests for memory allocation failure paths in execute.c
 * Uses mock_system to simulate malloc failures
 */

#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/migration/migration.h>

// Forward declarations for functions being tested
char* copy_sql_from_lua(const char* sql_result, size_t sql_length, const char* dqm_label);

// Forward declarations for test functions
void test_copy_sql_from_lua_malloc_failure(void);
void test_copy_sql_from_lua_malloc_success_after_failure(void);
void test_copy_sql_from_lua_large_allocation_failure(void);

// Test setup and teardown
void setUp(void) {
    mock_system_reset_all();
}

void tearDown(void) {
    mock_system_reset_all();
}

// ===== MALLOC FAILURE TESTS =====

void test_copy_sql_from_lua_malloc_failure(void) {
    const char* sql = "SELECT * FROM test;";
    size_t length = strlen(sql);
    
    // Make malloc fail on first call
    mock_system_set_malloc_failure(1);
    
    char* result = copy_sql_from_lua(sql, length, "test-label");
    
    // Should return NULL when malloc fails
    TEST_ASSERT_NULL(result);
}

void test_copy_sql_from_lua_malloc_success_after_failure(void) {
    const char* sql = "SELECT * FROM test;";
    size_t length = strlen(sql);
    
    // Make malloc fail on first call
    mock_system_set_malloc_failure(1);
    
    // First call should fail
    char* result1 = copy_sql_from_lua(sql, length, "test-label");
    TEST_ASSERT_NULL(result1);
    
    // Second call should succeed (mock resets after first failure)
    char* result2 = copy_sql_from_lua(sql, length, "test-label");
    TEST_ASSERT_NOT_NULL(result2);
    TEST_ASSERT_EQUAL_STRING(sql, result2);
    
    free(result2);
}

void test_copy_sql_from_lua_large_allocation_failure(void) {
    // Create a large SQL statement
    size_t large_size = 100000;
    char* large_sql = malloc(large_size + 1);
    TEST_ASSERT_NOT_NULL(large_sql);
    
    memset(large_sql, 'X', large_size);
    large_sql[large_size] = '\0';
    
    // Make malloc fail
    mock_system_set_malloc_failure(1);
    
    char* result = copy_sql_from_lua(large_sql, large_size, "test-label");
    
    // Should return NULL when malloc fails
    TEST_ASSERT_NULL(result);
    
    free(large_sql);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_copy_sql_from_lua_malloc_failure);
    RUN_TEST(test_copy_sql_from_lua_malloc_success_after_failure);
    RUN_TEST(test_copy_sql_from_lua_large_allocation_failure);
    
    return UNITY_END();
}