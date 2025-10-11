/*
 * Unity Test File: destroy_prepared_statement_cache Function Tests
 * This file contains unit tests for the destroy_prepared_statement_cache() function
 * from src/database/postgresql/connection.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers
#include <src/database/postgresql/connection.h>

// Forward declarations for the functions being tested
PreparedStatementCache* create_prepared_statement_cache(void);
void destroy_prepared_statement_cache(PreparedStatementCache* cache);
bool add_prepared_statement(PreparedStatementCache* cache, const char* name);

// Function prototypes for test functions
void test_destroy_prepared_statement_cache_null_pointer(void);
void test_destroy_prepared_statement_cache_empty_cache(void);
void test_destroy_prepared_statement_cache_with_statements(void);
void test_destroy_prepared_statement_cache_multiple_calls(void);

// Test fixtures
void setUp(void) {
    // No setup needed
}

void tearDown(void) {
    // No cleanup needed
}

// Test functions
void test_destroy_prepared_statement_cache_null_pointer(void) {
    // Test that function handles NULL pointer gracefully
    destroy_prepared_statement_cache(NULL);
    // Should not crash - if we get here, test passes
    TEST_ASSERT_TRUE(true);
}

void test_destroy_prepared_statement_cache_empty_cache(void) {
    PreparedStatementCache* cache = create_prepared_statement_cache();
    TEST_ASSERT_NOT_NULL(cache);

    // Destroy empty cache
    destroy_prepared_statement_cache(cache);
    // Should not crash - if we get here, test passes
    TEST_ASSERT_TRUE(true);
}

void test_destroy_prepared_statement_cache_with_statements(void) {
    PreparedStatementCache* cache = create_prepared_statement_cache();
    TEST_ASSERT_NOT_NULL(cache);

    // Add some statements
    bool result1 = add_prepared_statement(cache, "stmt1");
    bool result2 = add_prepared_statement(cache, "stmt2");
    TEST_ASSERT_TRUE(result1);
    TEST_ASSERT_TRUE(result2);
    TEST_ASSERT_EQUAL(2, cache->count);

    // Destroy cache with statements
    destroy_prepared_statement_cache(cache);
    // Should not crash - if we get here, test passes
    TEST_ASSERT_TRUE(true);
}

void test_destroy_prepared_statement_cache_multiple_calls(void) {
    PreparedStatementCache* cache = create_prepared_statement_cache();
    TEST_ASSERT_NOT_NULL(cache);

    // Add a statement
    bool result = add_prepared_statement(cache, "test_stmt");
    TEST_ASSERT_TRUE(result);

    // Destroy cache
    destroy_prepared_statement_cache(cache);

    // Note: We cannot safely test calling destroy again on the same pointer
    // as it would cause undefined behavior. The function should handle NULL gracefully.
    destroy_prepared_statement_cache(NULL);
    // Should not crash - if we get here, test passes
    TEST_ASSERT_TRUE(true);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_destroy_prepared_statement_cache_null_pointer);
    RUN_TEST(test_destroy_prepared_statement_cache_empty_cache);
    RUN_TEST(test_destroy_prepared_statement_cache_with_statements);
    RUN_TEST(test_destroy_prepared_statement_cache_multiple_calls);

    return UNITY_END();
}