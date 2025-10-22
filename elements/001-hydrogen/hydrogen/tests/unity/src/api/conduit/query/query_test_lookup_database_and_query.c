/*
 * Unity Test File: lookup_database_and_query
 * This file contains unit tests for lookup_database_and_query function
 * and related lookup functions in src/api/conduit/query/query.c
 */

#include <string.h>  // For memset

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Database includes for types
#include <src/database/database_cache.h>
#include <src/database/dbqueue/dbqueue.h>

// Include source header
#include <src/api/conduit/query/query.h>

// Enable mock database queue functions
#define USE_MOCK_DBQUEUE
#include <unity/mocks/mock_dbqueue.h>

// Mock globals for testing (since global_queue_manager is external)
static DatabaseQueue mock_db_queue;
static QueryTableCache mock_query_cache;
static QueryCacheEntry mock_cache_entry;
static DatabaseQueueManager* saved_global_manager = NULL;


void setUp(void) {
    // Reset all mocks
    mock_dbqueue_reset_all();

    // Save original global manager
    saved_global_manager = global_queue_manager;

    // Initialize mock data
    memset(&mock_db_queue, 0, sizeof(DatabaseQueue));
    mock_db_queue.query_cache = (QueryTableCache*)&mock_query_cache;
    memset(&mock_cache_entry, 0, sizeof(QueryCacheEntry));
    mock_cache_entry.sql_template = (char*)"SELECT * FROM test";
    mock_cache_entry.description = (char*)"Test query";
    mock_cache_entry.queue_type = (char*)"default";
    mock_cache_entry.timeout_seconds = 30;
    mock_cache_entry.query_ref = 42;  // Set the query_ref for the mock

    // Initialize mock query cache
    memset(&mock_query_cache, 0, sizeof(QueryTableCache));
    mock_query_cache.entries = calloc(2, sizeof(QueryCacheEntry*));
    if (mock_query_cache.entries) {
        mock_query_cache.entries[0] = &mock_cache_entry;
        mock_query_cache.entry_count = 1;
        mock_query_cache.capacity = 2;
    }

    // Create mock manager
    DatabaseQueueManager* mock_manager = calloc(1, sizeof(DatabaseQueueManager));
    if (mock_manager) {
        mock_manager->databases = calloc(1, sizeof(DatabaseQueue*));
        if (mock_manager->databases) {
            mock_db_queue.database_name = (char*)"testdb";  // Set database name
            mock_manager->databases[0] = &mock_db_queue;
            mock_manager->database_count = 1;
            mock_manager->max_databases = 1;
            global_queue_manager = mock_manager;
        } else {
            free(mock_manager);
        }
    }

    // Set up mock results for successful tests
    mock_dbqueue_set_get_database_result(&mock_db_queue);
    mock_dbqueue_set_query_cache_lookup_result(&mock_cache_entry);
}

void tearDown(void) {
    // Restore original global manager
    if (global_queue_manager) {
        if (global_queue_manager->databases) {
            free(global_queue_manager->databases);
        }
        free(global_queue_manager);
    }
    global_queue_manager = saved_global_manager;

    // Clean up mock query cache entries
    if (mock_query_cache.entries) {
        free(mock_query_cache.entries);
        mock_query_cache.entries = NULL;
    }
}

// Test successful lookup (both database and query found)
static void test_lookup_database_and_query_success(void);
static void test_lookup_database_and_query_db_not_found(void);
static void test_lookup_database_and_query_query_not_found(void);
static void test_lookup_database_and_query_null_params(void);
static void test_lookup_database_queue(void);
static void test_lookup_query_cache_entry_null_db(void);
static void test_lookup_query_cache_entry_null_cache(void);
static void test_lookup_query_cache_entry_success(void);

void test_lookup_database_and_query_success(void) {
    DatabaseQueue* db_queue = NULL;
    QueryCacheEntry* cache_entry = NULL;

    bool result = lookup_database_and_query(&db_queue, &cache_entry, "testdb", 42);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(db_queue);
    TEST_ASSERT_NOT_NULL(cache_entry);
    TEST_ASSERT_EQUAL_INT(42, cache_entry->query_ref);
}

void test_lookup_database_and_query_db_not_found(void) {
    // Set mock to return NULL for database lookup
    mock_dbqueue_set_get_database_result(NULL);

    DatabaseQueue* db_queue = NULL;
    QueryCacheEntry* cache_entry = NULL;

    bool result = lookup_database_and_query(&db_queue, &cache_entry, "nonexistent", 42);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(db_queue);
    TEST_ASSERT_NULL(cache_entry);
}

void test_lookup_database_and_query_query_not_found(void) {
    // Set mock to return database but NULL for query cache lookup
    mock_dbqueue_set_get_database_result(&mock_db_queue);
    mock_dbqueue_set_query_cache_lookup_result(NULL);

    DatabaseQueue* db_queue = NULL;
    QueryCacheEntry* cache_entry = NULL;

    bool result = lookup_database_and_query(&db_queue, &cache_entry, "testdb", 999);  // 999 not found

    TEST_ASSERT_FALSE(result);  // Hits the cache_entry NULL branch
    TEST_ASSERT_NOT_NULL(db_queue);  // db found
    TEST_ASSERT_NULL(cache_entry);
}

void test_lookup_database_and_query_null_params(void) {
    DatabaseQueue* db_queue = NULL;
    QueryCacheEntry* cache_entry = NULL;

    bool result1 = lookup_database_and_query(NULL, &cache_entry, "testdb", 42);
    TEST_ASSERT_FALSE(result1);

    bool result2 = lookup_database_and_query(&db_queue, NULL, "testdb", 42);
    TEST_ASSERT_FALSE(result2);

    bool result3 = lookup_database_and_query(&db_queue, &cache_entry, NULL, 42);
    TEST_ASSERT_FALSE(result3);

    bool result4 = lookup_database_and_query(NULL, NULL, NULL, 42);
    TEST_ASSERT_FALSE(result4);
}

void test_lookup_database_queue(void) {
    // Set mock to return the database queue
    mock_dbqueue_set_get_database_result(&mock_db_queue);

    DatabaseQueue* result = lookup_database_queue("testdb");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("testdb", result->database_name);
}

void test_lookup_query_cache_entry_null_db(void) {
    QueryCacheEntry* result = lookup_query_cache_entry(NULL, 42);
    TEST_ASSERT_NULL(result);
}

void test_lookup_query_cache_entry_null_cache(void) {
    // Set mock to return NULL for query cache lookup
    mock_dbqueue_set_query_cache_lookup_result(NULL);

    // Also set the mock query cache to have no entries
    mock_query_cache.entry_count = 0;

    QueryCacheEntry* result = lookup_query_cache_entry(&mock_db_queue, 42);
    TEST_ASSERT_NULL(result);

    // Restore entry count for other tests
    mock_query_cache.entry_count = 1;
}

void test_lookup_query_cache_entry_success(void) {
    // Set mock to return the cache entry
    mock_dbqueue_set_query_cache_lookup_result(&mock_cache_entry);

    QueryCacheEntry* result = lookup_query_cache_entry(&mock_db_queue, 42);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(42, result->query_ref);
    TEST_ASSERT_EQUAL_STRING("SELECT * FROM test", result->sql_template);
    TEST_ASSERT_EQUAL_STRING("Test query", result->description);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_lookup_database_and_query_success);
    RUN_TEST(test_lookup_database_and_query_db_not_found);
    RUN_TEST(test_lookup_database_and_query_query_not_found);
    RUN_TEST(test_lookup_database_and_query_null_params);
    RUN_TEST(test_lookup_database_queue);
    RUN_TEST(test_lookup_query_cache_entry_null_db);
    RUN_TEST(test_lookup_query_cache_entry_null_cache);
    RUN_TEST(test_lookup_query_cache_entry_success);

    return UNITY_END();
}