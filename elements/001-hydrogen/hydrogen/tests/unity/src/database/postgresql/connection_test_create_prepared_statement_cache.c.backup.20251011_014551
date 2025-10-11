/*
 * Unity Test File: create_prepared_statement_cache Function Tests
 * This file contains unit tests for the create_prepared_statement_cache() function
 * from src/database/postgresql/connection.c
 */

// Standard project header plus Unity Framework header
#include "../../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers
#include "../../../../../src/database/postgresql/connection.h"

// Forward declaration for the function being tested
PreparedStatementCache* create_prepared_statement_cache(void);

// Function prototypes for test functions
void test_create_prepared_statement_cache_success(void);
void test_create_prepared_statement_cache_memory_allocation(void);

// Test fixtures
void setUp(void) {
    // No setup needed
}

void tearDown(void) {
    // No cleanup needed
}

// Test functions
void test_create_prepared_statement_cache_success(void) {
    PreparedStatementCache* cache = create_prepared_statement_cache();

    TEST_ASSERT_NOT_NULL(cache);
    TEST_ASSERT_NOT_NULL(cache->names);
    TEST_ASSERT_EQUAL(16, cache->capacity);
    TEST_ASSERT_EQUAL(0, cache->count);

    // Clean up
    destroy_prepared_statement_cache(cache);
}

void test_create_prepared_statement_cache_memory_allocation(void) {
    // This test verifies that the function handles memory allocation properly
    // We can't easily simulate malloc failure in a unit test, but we can verify
    // the structure is properly initialized when allocation succeeds

    PreparedStatementCache* cache = create_prepared_statement_cache();

    TEST_ASSERT_NOT_NULL(cache);
    if (cache) {
        TEST_ASSERT_NOT_NULL(cache->names);
        TEST_ASSERT_EQUAL(16, cache->capacity);
        TEST_ASSERT_EQUAL(0, cache->count);

        // Verify the mutex is initialized (we can't easily test this directly)
        // but we can at least verify the structure exists
        TEST_ASSERT_TRUE(cache->capacity > 0);
    }

    // Clean up
    destroy_prepared_statement_cache(cache);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_create_prepared_statement_cache_success);
    RUN_TEST(test_create_prepared_statement_cache_memory_allocation);

    return UNITY_END();
}