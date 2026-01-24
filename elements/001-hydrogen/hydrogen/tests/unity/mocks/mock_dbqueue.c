/*
 * Mock database queue functions for unit testing
 *
 * Implementation of mock database queue manager functions.
 */

#include "mock_dbqueue.h"
#include <stdlib.h>

// Static variables to hold mock results
static DatabaseQueue* mock_get_database_result = NULL;
static QueryCacheEntry* mock_query_cache_lookup_result = NULL;
static QueryCacheEntry* mock_query_cache_lookup_by_ref_and_type_result = NULL;

// Mock implementation of database_queue_manager_get_database
DatabaseQueue* mock_database_queue_manager_get_database(DatabaseQueueManager* manager, const char* name);

// Mock implementation of query_cache_lookup
QueryCacheEntry* mock_query_cache_lookup(QueryTableCache* cache, int query_ref, const char* dqm_label);

// Mock implementation of query_cache_lookup_by_ref_and_type
QueryCacheEntry* mock_query_cache_lookup_by_ref_and_type(QueryTableCache* cache, int query_ref, int query_type, const char* dqm_label);

// Mock control functions
void mock_dbqueue_set_get_database_result(DatabaseQueue* result);
void mock_dbqueue_set_query_cache_lookup_result(QueryCacheEntry* result);
void mock_dbqueue_reset_all(void);

// Mock implementation of database_queue_manager_get_database
DatabaseQueue* mock_database_queue_manager_get_database(DatabaseQueueManager* manager, const char* name) {
    (void)manager;  // Unused parameter
    (void)name;     // Unused parameter
    return mock_get_database_result;
}

// Mock implementation of query_cache_lookup
QueryCacheEntry* mock_query_cache_lookup(QueryTableCache* cache, int query_ref, const char* dqm_label) {
    (void)cache;       // Unused parameter
    (void)query_ref;   // Unused parameter
    (void)dqm_label;   // Unused parameter
    return mock_query_cache_lookup_result;
}

// Mock implementation of query_cache_lookup_by_ref_and_type
QueryCacheEntry* mock_query_cache_lookup_by_ref_and_type(QueryTableCache* cache, int query_ref, int query_type, const char* dqm_label) {
    (void)cache;       // Unused parameter
    (void)query_ref;   // Unused parameter
    (void)query_type;  // Unused parameter
    (void)dqm_label;   // Unused parameter
    return mock_query_cache_lookup_by_ref_and_type_result;
}

// Mock control functions

// Set the result for database_queue_manager_get_database
void mock_dbqueue_set_get_database_result(DatabaseQueue* result) {
    mock_get_database_result = result;
}

// Set the result for query_cache_lookup
void mock_dbqueue_set_query_cache_lookup_result(QueryCacheEntry* result) {
    mock_query_cache_lookup_result = result;
}

// Reset all mock state
void mock_dbqueue_reset_all(void) {
    mock_get_database_result = NULL;
    mock_query_cache_lookup_result = NULL;
    mock_query_cache_lookup_by_ref_and_type_result = NULL;
}