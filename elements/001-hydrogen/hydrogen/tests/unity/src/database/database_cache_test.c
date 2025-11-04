/*
 * Unity Tests for Query Table Cache (QTC)
 *
 * Tests the database cache functionality for storing and retrieving query templates.
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

#include "../../../../src/database/database_cache.h"

// Test function prototypes
void test_query_cache_create_destroy(void);
void test_query_cache_entry_create(void);
void test_query_cache_entry_create_null_strings(void);
void test_query_cache_add_entry(void);
void test_query_cache_lookup_not_found(void);
void test_query_cache_usage_update(void);
void test_query_cache_stats(void);
void test_query_cache_resize(void);
void test_query_cache_concurrent_access(void);

// Test fixtures
static QueryTableCache* test_cache = NULL;

void setUp(void) {
    test_cache = query_cache_create(NULL);
    TEST_ASSERT_NOT_NULL(test_cache);
}

void tearDown(void) {
    if (test_cache) {
        query_cache_destroy(test_cache, NULL);
        test_cache = NULL;
    }
}

// Test cache creation and destruction
void test_query_cache_create_destroy(void) {
    QueryTableCache* cache = query_cache_create(NULL);
    TEST_ASSERT_NOT_NULL(cache);
    TEST_ASSERT_EQUAL(0, query_cache_get_entry_count(cache));

    query_cache_destroy(cache, NULL);
}

// Test entry creation
void test_query_cache_entry_create(void) {
    QueryCacheEntry* entry = query_cache_entry_create(
        123, 999, "SELECT * FROM users WHERE id = :userId", "Get user by ID", "fast", 30, NULL);

    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_EQUAL(123, entry->query_ref);
    TEST_ASSERT_EQUAL_STRING("SELECT * FROM users WHERE id = :userId", entry->sql_template);
    TEST_ASSERT_EQUAL_STRING("Get user by ID", entry->description);
    TEST_ASSERT_EQUAL_STRING("fast", entry->queue_type);
    TEST_ASSERT_EQUAL(30, entry->timeout_seconds);
    TEST_ASSERT_EQUAL(0, entry->usage_count);

    query_cache_entry_destroy(entry);
}

// Test entry creation with NULL strings
void test_query_cache_entry_create_null_strings(void) {
    QueryCacheEntry* entry = query_cache_entry_create(456, 999, NULL, NULL, NULL, 10, NULL);

    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_EQUAL(456, entry->query_ref);
    TEST_ASSERT_NULL(entry->sql_template);
    TEST_ASSERT_NULL(entry->description);
    TEST_ASSERT_NULL(entry->queue_type);
    TEST_ASSERT_EQUAL(10, entry->timeout_seconds);

    query_cache_entry_destroy(entry);
}

// Test adding entries to cache
void test_query_cache_add_entry(void) {
    QueryCacheEntry* entry = query_cache_entry_create(
        789, 999, "SELECT * FROM products", "Get all products", "medium", 60, NULL);

    TEST_ASSERT_TRUE(query_cache_add_entry(test_cache, entry, NULL));
    TEST_ASSERT_EQUAL(1, query_cache_get_entry_count(test_cache));

    // Verify entry can be looked up
    QueryCacheEntry* found = query_cache_lookup(test_cache, 789, NULL);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL(789, found->query_ref);
    TEST_ASSERT_EQUAL_STRING("SELECT * FROM products", found->sql_template);
}

// Test lookup of non-existent entry
void test_query_cache_lookup_not_found(void) {
    QueryCacheEntry* found = query_cache_lookup(test_cache, 999, NULL);
    TEST_ASSERT_NULL(found);
}

// Test usage statistics update
void test_query_cache_usage_update(void) {
    QueryCacheEntry* entry = query_cache_entry_create(
        111, 999, "SELECT COUNT(*) FROM users", "Count users", "fast", 5, NULL);

    TEST_ASSERT_TRUE(query_cache_add_entry(test_cache, entry, NULL));

    // Initial usage should be 0
    QueryCacheEntry* found = query_cache_lookup(test_cache, 111, NULL);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL(1, found->usage_count); // Incremented by lookup

    // Update usage
    query_cache_update_usage(test_cache, 111, NULL);

    // Lookup again to check increment
    found = query_cache_lookup(test_cache, 111, NULL);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL(4, found->usage_count); // +1 for lookup, +1 for internal lookup in update, +1 for update, +1 for this lookup
}

// Test cache statistics
void test_query_cache_stats(void) {
    // Add a few entries
    QueryCacheEntry* entry1 = query_cache_entry_create(1, 999, "SELECT 1", "Test 1", "fast", 10, NULL);
    QueryCacheEntry* entry2 = query_cache_entry_create(2, 999, "SELECT 2", "Test 2", "slow", 20, NULL);

    TEST_ASSERT_TRUE(query_cache_add_entry(test_cache, entry1, NULL));
    TEST_ASSERT_TRUE(query_cache_add_entry(test_cache, entry2, NULL));

    TEST_ASSERT_EQUAL(2, query_cache_get_entry_count(test_cache));

    // Lookup entries to generate usage
    query_cache_lookup(test_cache, 1, NULL);
    query_cache_lookup(test_cache, 1, NULL);
    query_cache_lookup(test_cache, 2, NULL);

    // Check stats string contains expected information
    char stats_buffer[256];
    query_cache_get_stats(test_cache, stats_buffer, sizeof(stats_buffer));

    // Just check that we get a non-empty stats string
    TEST_ASSERT_GREATER_THAN(0, strlen(stats_buffer));
}

// Test cache resize (adding more entries than initial capacity)
void test_query_cache_resize(void) {
    // Add more entries than initial capacity (64)
    for (int i = 0; i < 100; i++) {
        char sql[50];
        char desc[50];
        snprintf(sql, sizeof(sql), "SELECT %d", i);
        snprintf(desc, sizeof(desc), "Query %d", i);

        QueryCacheEntry* entry = query_cache_entry_create(i, 999, sql, desc, "fast", 30, NULL);
        TEST_ASSERT_TRUE(query_cache_add_entry(test_cache, entry, NULL));
    }

    TEST_ASSERT_EQUAL(100, query_cache_get_entry_count(test_cache));

    // Verify all entries can be looked up
    for (int i = 0; i < 100; i++) {
        QueryCacheEntry* found = query_cache_lookup(test_cache, i, NULL);
        TEST_ASSERT_NOT_NULL(found);
        TEST_ASSERT_EQUAL(i, found->query_ref);
    }
}

// Test concurrent access (basic smoke test)
void test_query_cache_concurrent_access(void) {
    // This is a basic test - full concurrency testing would require threads
    QueryCacheEntry* entry = query_cache_entry_create(999, 999, "SELECT 1", "Concurrent test", "fast", 1, NULL);
    TEST_ASSERT_TRUE(query_cache_add_entry(test_cache, entry, NULL));

    // Multiple lookups (simulating concurrent reads)
    for (int i = 0; i < 10; i++) {
        QueryCacheEntry* found = query_cache_lookup(test_cache, 999, NULL);
        TEST_ASSERT_NOT_NULL(found);
        TEST_ASSERT_EQUAL(999, found->query_ref);
    }
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_query_cache_create_destroy);
    RUN_TEST(test_query_cache_entry_create);
    RUN_TEST(test_query_cache_entry_create_null_strings);
    RUN_TEST(test_query_cache_add_entry);
    RUN_TEST(test_query_cache_lookup_not_found);
    RUN_TEST(test_query_cache_usage_update);
    RUN_TEST(test_query_cache_stats);
    RUN_TEST(test_query_cache_resize);
    RUN_TEST(test_query_cache_concurrent_access);

    return UNITY_END();
}