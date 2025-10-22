/*
 * Unity Test File: lookup_query_cache_entry
 * This file contains unit tests for lookup_query_cache_entry function in query.c
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary database headers
#include <src/database/database_cache.h>

// Include source header
#include <src/api/conduit/query/query.h>

// Global test fixtures
static QueryTableCache* g_cache = NULL;
static DatabaseQueue* g_db_queue = NULL;
static QueryCacheEntry* g_entry = NULL;

void setUp(void) {
    // Create test fixtures
    g_cache = query_cache_create();
    TEST_ASSERT_NOT_NULL(g_cache);

    g_entry = query_cache_entry_create(1, "SELECT 1", "test description", "select", 30);
    TEST_ASSERT_NOT_NULL(g_entry);

    bool added = query_cache_add_entry(g_cache, g_entry);
    TEST_ASSERT_TRUE(added);

    g_db_queue = malloc(sizeof(DatabaseQueue));
    TEST_ASSERT_NOT_NULL(g_db_queue);
    memset(g_db_queue, 0, sizeof(DatabaseQueue));
    g_db_queue->query_cache = g_cache;
}

void tearDown(void) {
    if (g_cache) {
        query_cache_destroy(g_cache);
        g_cache = NULL;
    }
    if (g_db_queue) {
        free(g_db_queue);
        g_db_queue = NULL;
    }
    g_entry = NULL;
}

// Test with NULL db_queue
static void test_lookup_query_cache_entry_null_db_queue(void) {
    DatabaseQueue* db_queue = NULL;
    int query_ref = 1;

    QueryCacheEntry* result = lookup_query_cache_entry(db_queue, query_ref);

    TEST_ASSERT_NULL(result);
}

// Test with db_queue but NULL query_cache
static void test_lookup_query_cache_entry_null_query_cache(void) {
    DatabaseQueue db_queue_local = {0}; // query_cache is NULL
    int query_ref = 1;

    QueryCacheEntry* result = lookup_query_cache_entry(&db_queue_local, query_ref);

    TEST_ASSERT_NULL(result);
}

// Test with valid db_queue and query_cache, valid query_ref
static void test_lookup_query_cache_entry_valid(void) {
    int query_ref = 1;

    QueryCacheEntry* result = lookup_query_cache_entry(g_db_queue, query_ref);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(1, result->query_ref);
    TEST_ASSERT_EQUAL_STRING("SELECT 1", result->sql_template);
    TEST_ASSERT_EQUAL_STRING("test description", result->description);
    TEST_ASSERT_EQUAL_STRING("select", result->queue_type);
}

// Test with valid db_queue and query_cache, query_ref 0
static void test_lookup_query_cache_entry_query_ref_zero(void) {
    int query_ref = 0;

    QueryCacheEntry* result = lookup_query_cache_entry(g_db_queue, query_ref);

    TEST_ASSERT_NULL(result);
}

// Test with valid db_queue and query_cache, negative query_ref
static void test_lookup_query_cache_entry_negative_query_ref(void) {
    int query_ref = -1;

    QueryCacheEntry* result = lookup_query_cache_entry(g_db_queue, query_ref);

    TEST_ASSERT_NULL(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_lookup_query_cache_entry_null_db_queue);
    RUN_TEST(test_lookup_query_cache_entry_null_query_cache);
    RUN_TEST(test_lookup_query_cache_entry_valid);
    RUN_TEST(test_lookup_query_cache_entry_query_ref_zero);
    RUN_TEST(test_lookup_query_cache_entry_negative_query_ref);

    return UNITY_END();
}