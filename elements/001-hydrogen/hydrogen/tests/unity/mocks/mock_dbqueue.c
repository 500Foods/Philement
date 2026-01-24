/*
 * Mock database queue functions for unit testing
 *
 * Implementation of mock database queue manager functions.
 */

#include "mock_dbqueue.h"
#include <stdlib.h>
#include <string.h>

// Static variables to hold mock results
static DatabaseQueue* mock_get_database_result = NULL;
static QueryCacheEntry* mock_query_cache_lookup_result = NULL;
static QueryCacheEntry* mock_query_cache_lookup_by_ref_and_type_result = NULL;
static bool mock_submit_query_result = true;
static json_t* mock_get_stats_json_result = NULL;
static DatabaseQuery last_submitted_query;
static bool mock_submit_query_called = false;

// Mock implementation of database_queue_manager_get_database
DatabaseQueue* mock_database_queue_manager_get_database(DatabaseQueueManager* manager, const char* name);

// Mock implementation of query_cache_lookup
QueryCacheEntry* mock_query_cache_lookup(QueryTableCache* cache, int query_ref, const char* dqm_label);

// Mock implementation of query_cache_lookup_by_ref_and_type
QueryCacheEntry* mock_query_cache_lookup_by_ref_and_type(QueryTableCache* cache, int query_ref, int query_type, const char* dqm_label);

// Mock implementation of database_queue_submit_query
bool mock_database_queue_submit_query(DatabaseQueue* queue, const DatabaseQuery* query);

// Mock implementation of database_queue_get_stats_json
json_t* mock_database_queue_get_stats_json(DatabaseQueue* db_queue);

// Mock control functions
void mock_dbqueue_set_get_database_result(DatabaseQueue* result);
void mock_dbqueue_set_query_cache_lookup_result(QueryCacheEntry* result);
void mock_dbqueue_set_query_cache_lookup_by_ref_and_type_result(QueryCacheEntry* result);
void mock_dbqueue_set_submit_query_result(bool result);
void mock_dbqueue_reset_all(void);

// Get last submitted query for verification
const DatabaseQuery* mock_dbqueue_get_last_submitted_query(void);

// Helper function to free any dynamically allocated memory in a DatabaseQuery
static void free_query_memory(DatabaseQuery* query) {
    if (query->query_id) {
        free((void*)query->query_id);
        query->query_id = NULL;
    }
    if (query->query_template) {
        free((void*)query->query_template);
        query->query_template = NULL;
    }
    if (query->parameter_json) {
        free((void*)query->parameter_json);
        query->parameter_json = NULL;
    }
    if (query->error_message) {
        free((void*)query->error_message);
        query->error_message = NULL;
    }
}

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

// Mock implementation of database_queue_get_stats_json
json_t* mock_database_queue_get_stats_json(DatabaseQueue* db_queue) {
    (void)db_queue;  // Unused parameter
    return mock_get_stats_json_result;
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

// Mock implementation of database_queue_submit_query
bool mock_database_queue_submit_query(DatabaseQueue* queue, const DatabaseQuery* query) {
    (void)queue; // Unused parameter

    mock_submit_query_called = true;

    // Free any existing memory in last_submitted_query
    free_query_memory(&last_submitted_query);

    // Copy query data for verification
    if (query) {
        last_submitted_query = *query;
        if (query->query_id) {
            char* dup = strdup(query->query_id);
            if (dup) {
                last_submitted_query.query_id = dup;
            }
            // else keep the copied pointer
        }
        if (query->query_template) {
            char* dup = strdup(query->query_template);
            if (dup) {
                last_submitted_query.query_template = dup;
            }
            // else keep the copied pointer
        }
        if (query->parameter_json) {
            char* dup = strdup(query->parameter_json);
            if (dup) {
                last_submitted_query.parameter_json = dup;
            }
            // else keep the copied pointer
        }
        if (query->error_message) {
            char* dup = strdup(query->error_message);
            if (dup) {
                last_submitted_query.error_message = dup;
            }
            // else keep the copied pointer
        }
    }
    return mock_submit_query_result;
}

// Get last submitted query for verification
const DatabaseQuery* mock_dbqueue_get_last_submitted_query(void) {
    return &last_submitted_query;
}

// Check if submit query was called
bool mock_dbqueue_submit_query_called(void) {
    return mock_submit_query_called;
}

// Mock control function to set submit query result
void mock_dbqueue_set_submit_query_result(bool result) {
    mock_submit_query_result = result;
}

// Mock control function to set get stats json result
void mock_dbqueue_set_get_stats_json_result(json_t* result) {
    mock_get_stats_json_result = result;
}

// Reset all mock state
void mock_dbqueue_reset_all(void) {
    mock_get_database_result = NULL;
    mock_query_cache_lookup_result = NULL;
    mock_query_cache_lookup_by_ref_and_type_result = NULL;
    mock_submit_query_result = true;
    mock_get_stats_json_result = NULL;
    mock_submit_query_called = false;
    free_query_memory(&last_submitted_query);
    memset(&last_submitted_query, 0, sizeof(DatabaseQuery));
}