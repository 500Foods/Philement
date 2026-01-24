/*
 * Unity Test File: database_operations_test_lookup_database_and_public_query.c
 * This file contains unit tests for the lookup_database_and_public_query function
 */

// Standard includes
#include <src/hydrogen.h>
#include <unity.h>

// Enable mocks for unit testing
#define USE_MOCK_DBQUEUE
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_dbqueue.h>
#include <unity/mocks/mock_system.h>

// Include source headers
#include <src/api/conduit/conduit_helpers.h>

// Forward declarations for functions being tested
bool lookup_database_and_public_query(DatabaseQueue** db_queue, QueryCacheEntry** cache_entry,
                                       const char* database, int query_ref);

void setUp(void) {
    // Reset all mocks to default state
    mock_dbqueue_reset_all();
    mock_system_reset_all();
}

void tearDown(void) {
    // Clean up after each test
    mock_dbqueue_reset_all();
    mock_system_reset_all();
}

// Test NULL pointer parameters
static void test_lookup_database_and_public_query_null_db_queue(void) {
    QueryCacheEntry* cache_entry = NULL;
    bool result = lookup_database_and_public_query(NULL, &cache_entry, "test_db", 123);
    TEST_ASSERT_FALSE(result);
}

static void test_lookup_database_and_public_query_null_cache_entry(void) {
    DatabaseQueue* db_queue = NULL;
    bool result = lookup_database_and_public_query(&db_queue, NULL, "test_db", 123);
    TEST_ASSERT_FALSE(result);
}

static void test_lookup_database_and_public_query_null_database(void) {
    DatabaseQueue* db_queue = NULL;
    QueryCacheEntry* cache_entry = NULL;
    bool result = lookup_database_and_public_query(&db_queue, &cache_entry, NULL, 123);
    TEST_ASSERT_FALSE(result);
}

// Test case where database lookup fails
static void test_lookup_database_and_public_query_database_not_found(void) {
    // Mock database lookup to return NULL
    mock_dbqueue_set_get_database_result(NULL);

    DatabaseQueue* db_queue = NULL;
    QueryCacheEntry* cache_entry = NULL;
    bool result = lookup_database_and_public_query(&db_queue, &cache_entry, "nonexistent_db", 123);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(db_queue);
    TEST_ASSERT_NULL(cache_entry);
}

// Test case where database exists but query_cache is NULL
static void test_lookup_database_and_public_query_query_cache_null(void) {
    // Create a mock database queue with NULL query_cache
    DatabaseQueue* mock_dbq = calloc(1, sizeof(DatabaseQueue));
    TEST_ASSERT_NOT_NULL(mock_dbq);
    mock_dbq->query_cache = NULL;  // This should trigger the uncovered lines

    mock_dbqueue_set_get_database_result(mock_dbq);

    DatabaseQueue* db_queue = NULL;
    QueryCacheEntry* cache_entry = NULL;
    bool result = lookup_database_and_public_query(&db_queue, &cache_entry, "test_db", 123);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NOT_NULL(db_queue);
    TEST_ASSERT_NULL(cache_entry);

    free(mock_dbq);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_lookup_database_and_public_query_null_db_queue);
    RUN_TEST(test_lookup_database_and_public_query_null_cache_entry);
    RUN_TEST(test_lookup_database_and_public_query_null_database);
    RUN_TEST(test_lookup_database_and_public_query_database_not_found);
    RUN_TEST(test_lookup_database_and_public_query_query_cache_null);

    return UNITY_END();
}