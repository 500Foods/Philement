/*
 * Unity Test File: remove_prepared_statement Function Tests
 * This file contains unit tests for the remove_prepared_statement() function
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
bool remove_prepared_statement(PreparedStatementCache* cache, const char* name);

// Function prototypes for test functions
void test_remove_prepared_statement_null_cache(void);
void test_remove_prepared_statement_null_name(void);
void test_remove_prepared_statement_both_null(void);
void test_remove_prepared_statement_empty_cache(void);
void test_remove_prepared_statement_success(void);
void test_remove_prepared_statement_nonexistent(void);
void test_remove_prepared_statement_multiple(void);
void test_remove_prepared_statement_first(void);
void test_remove_prepared_statement_last(void);
void test_remove_prepared_statement_duplicate_calls(void);

// Test fixtures
void setUp(void) {
    // No setup needed
}

void tearDown(void) {
    // No cleanup needed
}

// Test functions
void test_remove_prepared_statement_null_cache(void) {
    bool result = remove_prepared_statement(NULL, "test_stmt");
    TEST_ASSERT_FALSE(result);
}

void test_remove_prepared_statement_null_name(void) {
    PreparedStatementCache* cache = create_prepared_statement_cache();
    TEST_ASSERT_NOT_NULL(cache);

    bool result = remove_prepared_statement(cache, NULL);
    TEST_ASSERT_FALSE(result);

    destroy_prepared_statement_cache(cache);
}

void test_remove_prepared_statement_both_null(void) {
    bool result = remove_prepared_statement(NULL, NULL);
    TEST_ASSERT_FALSE(result);
}

void test_remove_prepared_statement_empty_cache(void) {
    PreparedStatementCache* cache = create_prepared_statement_cache();
    TEST_ASSERT_NOT_NULL(cache);

    bool result = remove_prepared_statement(cache, "nonexistent");
    TEST_ASSERT_FALSE(result);

    destroy_prepared_statement_cache(cache);
}

void test_remove_prepared_statement_success(void) {
    PreparedStatementCache* cache = create_prepared_statement_cache();
    TEST_ASSERT_NOT_NULL(cache);

    // Add a statement
    bool add_result = add_prepared_statement(cache, "test_stmt");
    TEST_ASSERT_TRUE(add_result);
    TEST_ASSERT_EQUAL(1, cache->count);

    // Remove the statement
    bool remove_result = remove_prepared_statement(cache, "test_stmt");
    TEST_ASSERT_TRUE(remove_result);
    TEST_ASSERT_EQUAL(0, cache->count);

    destroy_prepared_statement_cache(cache);
}

void test_remove_prepared_statement_nonexistent(void) {
    PreparedStatementCache* cache = create_prepared_statement_cache();
    TEST_ASSERT_NOT_NULL(cache);

    // Add a statement
    bool add_result = add_prepared_statement(cache, "existing_stmt");
    TEST_ASSERT_TRUE(add_result);
    TEST_ASSERT_EQUAL(1, cache->count);

    // Try to remove a different statement
    bool remove_result = remove_prepared_statement(cache, "nonexistent");
    TEST_ASSERT_FALSE(remove_result);
    TEST_ASSERT_EQUAL(1, cache->count); // Count should remain the same

    destroy_prepared_statement_cache(cache);
}

void test_remove_prepared_statement_multiple(void) {
    PreparedStatementCache* cache = create_prepared_statement_cache();
    TEST_ASSERT_NOT_NULL(cache);

    // Add multiple statements
    add_prepared_statement(cache, "stmt1");
    add_prepared_statement(cache, "stmt2");
    add_prepared_statement(cache, "stmt3");
    TEST_ASSERT_EQUAL(3, cache->count);

    // Remove middle statement
    bool result = remove_prepared_statement(cache, "stmt2");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(2, cache->count);

    // Verify remaining statements still exist
    TEST_ASSERT_EQUAL_STRING("stmt1", cache->names[0]);
    TEST_ASSERT_EQUAL_STRING("stmt3", cache->names[1]);

    destroy_prepared_statement_cache(cache);
}

void test_remove_prepared_statement_first(void) {
    PreparedStatementCache* cache = create_prepared_statement_cache();
    TEST_ASSERT_NOT_NULL(cache);

    // Add multiple statements
    add_prepared_statement(cache, "stmt1");
    add_prepared_statement(cache, "stmt2");
    add_prepared_statement(cache, "stmt3");
    TEST_ASSERT_EQUAL(3, cache->count);

    // Remove first statement
    bool result = remove_prepared_statement(cache, "stmt1");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(2, cache->count);

    // Verify remaining statements shifted
    TEST_ASSERT_EQUAL_STRING("stmt2", cache->names[0]);
    TEST_ASSERT_EQUAL_STRING("stmt3", cache->names[1]);

    destroy_prepared_statement_cache(cache);
}

void test_remove_prepared_statement_last(void) {
    PreparedStatementCache* cache = create_prepared_statement_cache();
    TEST_ASSERT_NOT_NULL(cache);

    // Add multiple statements
    add_prepared_statement(cache, "stmt1");
    add_prepared_statement(cache, "stmt2");
    add_prepared_statement(cache, "stmt3");
    TEST_ASSERT_EQUAL(3, cache->count);

    // Remove last statement
    bool result = remove_prepared_statement(cache, "stmt3");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(2, cache->count);

    // Verify first two statements remain
    TEST_ASSERT_EQUAL_STRING("stmt1", cache->names[0]);
    TEST_ASSERT_EQUAL_STRING("stmt2", cache->names[1]);

    destroy_prepared_statement_cache(cache);
}

void test_remove_prepared_statement_duplicate_calls(void) {
    PreparedStatementCache* cache = create_prepared_statement_cache();
    TEST_ASSERT_NOT_NULL(cache);

    // Add a statement
    add_prepared_statement(cache, "test_stmt");
    TEST_ASSERT_EQUAL(1, cache->count);

    // Remove it once
    bool result1 = remove_prepared_statement(cache, "test_stmt");
    TEST_ASSERT_TRUE(result1);
    TEST_ASSERT_EQUAL(0, cache->count);

    // Try to remove it again
    bool result2 = remove_prepared_statement(cache, "test_stmt");
    TEST_ASSERT_FALSE(result2);
    TEST_ASSERT_EQUAL(0, cache->count);

    destroy_prepared_statement_cache(cache);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_remove_prepared_statement_null_cache);
    RUN_TEST(test_remove_prepared_statement_null_name);
    RUN_TEST(test_remove_prepared_statement_both_null);
    RUN_TEST(test_remove_prepared_statement_empty_cache);
    RUN_TEST(test_remove_prepared_statement_success);
    RUN_TEST(test_remove_prepared_statement_nonexistent);
    RUN_TEST(test_remove_prepared_statement_multiple);
    RUN_TEST(test_remove_prepared_statement_first);
    RUN_TEST(test_remove_prepared_statement_last);
    RUN_TEST(test_remove_prepared_statement_duplicate_calls);

    return UNITY_END();
}