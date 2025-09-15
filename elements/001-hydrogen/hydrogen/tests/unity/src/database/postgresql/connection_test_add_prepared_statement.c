/*
 * Unity Test File: add_prepared_statement Function Tests
 * This file contains unit tests for the add_prepared_statement() function
 * from src/database/postgresql/connection.c
 */

// Standard project header plus Unity Framework header
#include "../../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers
#include "../../../../../src/database/postgresql/connection.h"

// Forward declarations for the functions being tested
PreparedStatementCache* create_prepared_statement_cache(void);
void destroy_prepared_statement_cache(PreparedStatementCache* cache);
bool add_prepared_statement(PreparedStatementCache* cache, const char* name);

// Function prototypes for test functions
void test_add_prepared_statement_null_cache(void);
void test_add_prepared_statement_null_name(void);
void test_add_prepared_statement_both_null(void);
void test_add_prepared_statement_success(void);
void test_add_prepared_statement_duplicate(void);
void test_add_prepared_statement_multiple(void);
void test_add_prepared_statement_empty_string(void);
void test_add_prepared_statement_long_name(void);
void test_add_prepared_statement_capacity_expansion(void);

// Test fixtures
void setUp(void) {
    // No setup needed
}

void tearDown(void) {
    // No cleanup needed
}

// Test functions
void test_add_prepared_statement_null_cache(void) {
    bool result = add_prepared_statement(NULL, "test_stmt");
    TEST_ASSERT_FALSE(result);
}

void test_add_prepared_statement_null_name(void) {
    PreparedStatementCache* cache = create_prepared_statement_cache();
    TEST_ASSERT_NOT_NULL(cache);

    bool result = add_prepared_statement(cache, NULL);
    TEST_ASSERT_FALSE(result);

    destroy_prepared_statement_cache(cache);
}

void test_add_prepared_statement_both_null(void) {
    bool result = add_prepared_statement(NULL, NULL);
    TEST_ASSERT_FALSE(result);
}

void test_add_prepared_statement_success(void) {
    PreparedStatementCache* cache = create_prepared_statement_cache();
    TEST_ASSERT_NOT_NULL(cache);

    bool result = add_prepared_statement(cache, "test_statement");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, cache->count);

    destroy_prepared_statement_cache(cache);
}

void test_add_prepared_statement_duplicate(void) {
    PreparedStatementCache* cache = create_prepared_statement_cache();
    TEST_ASSERT_NOT_NULL(cache);

    // Add first time
    bool result1 = add_prepared_statement(cache, "duplicate_stmt");
    TEST_ASSERT_TRUE(result1);
    TEST_ASSERT_EQUAL(1, cache->count);

    // Add same statement again
    bool result2 = add_prepared_statement(cache, "duplicate_stmt");
    TEST_ASSERT_TRUE(result2); // Should return true (already exists)
    TEST_ASSERT_EQUAL(1, cache->count); // Count should not increase

    destroy_prepared_statement_cache(cache);
}

void test_add_prepared_statement_multiple(void) {
    PreparedStatementCache* cache = create_prepared_statement_cache();
    TEST_ASSERT_NOT_NULL(cache);

    // Add multiple different statements
    bool result1 = add_prepared_statement(cache, "stmt1");
    bool result2 = add_prepared_statement(cache, "stmt2");
    bool result3 = add_prepared_statement(cache, "stmt3");

    TEST_ASSERT_TRUE(result1);
    TEST_ASSERT_TRUE(result2);
    TEST_ASSERT_TRUE(result3);
    TEST_ASSERT_EQUAL(3, cache->count);

    destroy_prepared_statement_cache(cache);
}

void test_add_prepared_statement_empty_string(void) {
    PreparedStatementCache* cache = create_prepared_statement_cache();
    TEST_ASSERT_NOT_NULL(cache);

    bool result = add_prepared_statement(cache, "");
    TEST_ASSERT_TRUE(result); // Empty string is still a valid name
    TEST_ASSERT_EQUAL(1, cache->count);

    destroy_prepared_statement_cache(cache);
}

void test_add_prepared_statement_long_name(void) {
    PreparedStatementCache* cache = create_prepared_statement_cache();
    TEST_ASSERT_NOT_NULL(cache);

    char long_name[256];
    memset(long_name, 'a', 255);
    long_name[255] = '\0';

    bool result = add_prepared_statement(cache, long_name);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, cache->count);

    destroy_prepared_statement_cache(cache);
}

void test_add_prepared_statement_capacity_expansion(void) {
    PreparedStatementCache* cache = create_prepared_statement_cache();
    TEST_ASSERT_NOT_NULL(cache);

    // Add enough statements to trigger capacity expansion (more than initial 16)
    char name[32];
    for (int i = 0; i < 20; i++) {
        snprintf(name, sizeof(name), "stmt_%d", i);
        bool result = add_prepared_statement(cache, name);
        TEST_ASSERT_TRUE(result);
    }

    TEST_ASSERT_EQUAL(20, cache->count);
    TEST_ASSERT_TRUE(cache->capacity >= 20); // Should have expanded

    destroy_prepared_statement_cache(cache);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_add_prepared_statement_null_cache);
    RUN_TEST(test_add_prepared_statement_null_name);
    RUN_TEST(test_add_prepared_statement_both_null);
    RUN_TEST(test_add_prepared_statement_success);
    RUN_TEST(test_add_prepared_statement_duplicate);
    RUN_TEST(test_add_prepared_statement_multiple);
    RUN_TEST(test_add_prepared_statement_empty_string);
    RUN_TEST(test_add_prepared_statement_long_name);
    RUN_TEST(test_add_prepared_statement_capacity_expansion);

    return UNITY_END();
}