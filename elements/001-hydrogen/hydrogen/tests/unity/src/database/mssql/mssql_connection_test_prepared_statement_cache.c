/*
 * Unity Test File: MSSQL prepared statement cache
 * Tests mssql_create_prepared_statement_cache() and mssql_destroy_prepared_statement_cache()
 */

#include <src/hydrogen.h>
#include <unity.h>

#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

#include <src/database/database.h>
#include <src/database/mssql/connection.h>
#include <src/database/mssql/types.h>

void setUp(void);
void tearDown(void);
void test_mssql_create_cache_success(void);
void test_mssql_create_cache_failure_alloc_names(void);
void test_mssql_create_cache_failure_alloc_cache(void);
void test_mssql_destroy_cache_null(void);
void test_mssql_destroy_cache_empty(void);
void test_mssql_destroy_cache_with_entries(void);
void test_mssql_destroy_cache_with_capacity_entries(void);

void setUp(void) {
    mock_system_reset_all();
}

void tearDown(void) {
}

void test_mssql_create_cache_success(void) {
    PreparedStatementCache* cache = mssql_create_prepared_statement_cache();
    TEST_ASSERT_NOT_NULL(cache);
    TEST_ASSERT_NOT_NULL(cache->names);
    TEST_ASSERT_EQUAL(0, cache->count);
    TEST_ASSERT_EQUAL(16, cache->capacity);
    pthread_mutex_destroy(&cache->lock);
    free(cache->names);
    free(cache);
}

void test_mssql_create_cache_failure_alloc_names(void) {
    mock_system_set_calloc_failure(2);
    PreparedStatementCache* cache = mssql_create_prepared_statement_cache();
    TEST_ASSERT_NULL(cache);
    mock_system_set_calloc_failure(0);
}

void test_mssql_create_cache_failure_alloc_cache(void) {
    mock_system_set_calloc_failure(1);
    PreparedStatementCache* cache = mssql_create_prepared_statement_cache();
    TEST_ASSERT_NULL(cache);
    mock_system_set_calloc_failure(0);
}

void test_mssql_destroy_cache_null(void) {
    mssql_destroy_prepared_statement_cache(NULL);
    TEST_PASS();
}

void test_mssql_destroy_cache_empty(void) {
    PreparedStatementCache* cache = mssql_create_prepared_statement_cache();
    TEST_ASSERT_NOT_NULL(cache);
    mssql_destroy_prepared_statement_cache(cache);
    TEST_PASS();
}

void test_mssql_destroy_cache_with_entries(void) {
    PreparedStatementCache* cache = mssql_create_prepared_statement_cache();
    TEST_ASSERT_NOT_NULL(cache);

    cache->names[0] = strdup("stmt_one");
    cache->names[1] = strdup("stmt_two");
    cache->count = 2;

    mssql_destroy_prepared_statement_cache(cache);
    TEST_PASS();
}

void test_mssql_destroy_cache_with_capacity_entries(void) {
    PreparedStatementCache* cache = mssql_create_prepared_statement_cache();
    TEST_ASSERT_NOT_NULL(cache);

    cache->names[0] = strdup("stmt_a");
    free(cache->names[0]);
    cache->names[0] = NULL;
    cache->count = 0;

    mssql_destroy_prepared_statement_cache(cache);
    TEST_PASS();
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mssql_create_cache_success);
    RUN_TEST(test_mssql_create_cache_failure_alloc_names);
    RUN_TEST(test_mssql_destroy_cache_null);
    RUN_TEST(test_mssql_destroy_cache_empty);
    RUN_TEST(test_mssql_destroy_cache_with_entries);
    RUN_TEST(test_mssql_destroy_cache_with_capacity_entries);

    return UNITY_END();
}
