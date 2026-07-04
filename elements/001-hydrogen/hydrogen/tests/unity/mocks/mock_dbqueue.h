/*
 * Mock database queue functions for unit testing
 *
 * This file provides mock implementations of database queue manager functions
 * to enable testing of database operations without real database connections.
 */

#ifndef MOCK_DBQUEUE_H
#define MOCK_DBQUEUE_H

#include <src/database/dbqueue/dbqueue.h>

// Mock function declarations - these will override the real ones when USE_MOCK_DBQUEUE is defined
#ifdef USE_MOCK_DBQUEUE

// Override database queue manager functions with our mocks
#define database_queue_manager_get_database mock_database_queue_manager_get_database
#define query_cache_lookup mock_query_cache_lookup
#define query_cache_lookup_by_ref_and_type mock_query_cache_lookup_by_ref_and_type
#define database_queue_submit_query mock_database_queue_submit_query
#define database_queue_get_stats_json mock_database_queue_get_stats_json
// Phase 14: mock database_queue_await_result so H.wait can be tested
// without a real DQM. The mock returns a pre-set DatabaseQuery whose
// affected_rows, data_json, and error_message are controlled by the
// test via mock_dbqueue_set_await_result.
#define database_queue_await_result mock_database_queue_await_result

// Always declare mock function prototypes for the .c file
DatabaseQueue* mock_database_queue_manager_get_database(DatabaseQueueManager* manager, const char* name);
QueryCacheEntry* mock_query_cache_lookup(QueryTableCache* cache, int query_ref, const char* dqm_label);
QueryCacheEntry* mock_query_cache_lookup_by_ref_and_type(QueryTableCache* cache, int query_ref, int query_type, const char* dqm_label);
bool mock_database_queue_submit_query(DatabaseQueue* queue, const DatabaseQuery* query);
json_t* mock_database_queue_get_stats_json(DatabaseQueue* db_queue);
DatabaseQuery* mock_database_queue_await_result(DatabaseQueue* db_queue, const char* query_id, int timeout_seconds);

#endif // USE_MOCK_DBQUEUE

// Mock control functions for tests - always available
void mock_dbqueue_set_get_database_result(DatabaseQueue* result);
void mock_dbqueue_set_query_cache_lookup_result(QueryCacheEntry* result);
void mock_dbqueue_set_query_cache_lookup_by_ref_and_type_result(QueryCacheEntry* result);
void mock_dbqueue_set_submit_query_result(bool result);
void mock_dbqueue_set_get_stats_json_result(json_t* result);
// Phase 14: set the DatabaseQuery* that mock_database_queue_await_result
// will return. The mock copies the struct (deep-copies the strdup'd
// fields) and frees its own copy on the next call or on reset.
void mock_dbqueue_set_await_result(const DatabaseQuery* result);
void mock_dbqueue_reset_all(void);
const DatabaseQuery* mock_dbqueue_get_last_submitted_query(void);
bool mock_dbqueue_submit_query_called(void);
int mock_dbqueue_get_await_call_count(void);

#endif // MOCK_DBQUEUE_H