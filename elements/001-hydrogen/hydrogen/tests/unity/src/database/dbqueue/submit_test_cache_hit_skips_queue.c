/*
 * Unity Test File: submit_test_cache_hit_skips_queue
 *
 * Integration test that verifies database_queue_submit_query() (in
 * src/database/dbqueue/submit.c) transparently uses the global
 * query-result cache for cache-type queries and skips enqueuing on a hit.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/database_pending.h>
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/dbqueue/query_result_cache.h>
#include <src/queue/queue.h>

// Forward declarations for test functions
void test_database_queue_submit_query_cache_hit_skips_queue(void);

void setUp(void) {
    if (!queue_system_initialized) {
        queue_system_init();
    }
    database_queue_system_init();
    query_result_cache_clear(query_result_cache_get_global());
}

void tearDown(void) {
    database_queue_system_destroy();
}

void test_database_queue_submit_query_cache_hit_skips_queue(void) {
    DatabaseQueueManager* manager = database_queue_manager_create(4);
    TEST_ASSERT_NOT_NULL(manager);

    DatabaseQueue* worker = database_queue_create_worker("testdb", "sqlite:///tmp/test.db", QUEUE_TYPE_CACHE, NULL);
    TEST_ASSERT_NOT_NULL(worker);
    TEST_ASSERT_FALSE(worker->is_lead_queue);
    TEST_ASSERT_EQUAL_STRING(QUEUE_TYPE_CACHE, worker->queue_type);

    bool added = database_queue_manager_add_database(manager, worker);
    TEST_ASSERT_TRUE(added);

    const char* query_id = "cache_query_1";
    const char* sql = "SELECT * FROM lookups WHERE lookup_id = :LOOKUPID";
    const char* params = "{\"LOOKUPID\":46}";
    const char* data_json = "[{\"lookup_id\":46,\"key_idx\":0}]";

    // Register the pending result that the caller would be waiting on.
    PendingResultManager* pending_mgr = get_pending_result_manager();
    TEST_ASSERT_NOT_NULL(pending_mgr);
    PendingQueryResult* pending = pending_result_register(pending_mgr, query_id, 5, NULL);
    TEST_ASSERT_NOT_NULL(pending);

    // Pre-populate the global cache as if a previous worker execution stored it.
    TEST_ASSERT_TRUE(query_result_cache_put(query_result_cache_get_global(),
                                             worker->database_name,
                                             sql,
                                             params,
                                             data_json,
                                             1, 2, 0, 1));
    TEST_ASSERT_EQUAL_size_t(1, query_result_cache_entry_count(query_result_cache_get_global()));

    // Build the cache-type query.
    DatabaseQuery query = {
        .query_id = strdup(query_id),
        .query_template = strdup(sql),
        .parameter_json = strdup(params),
        .queue_type_hint = DB_QUEUE_CACHE,
        .submitted_at = 0,
        .processed_at = 0,
        .retry_count = 0,
        .error_message = NULL
    };

    // Submit should succeed but should NOT enqueue the query because the cache is hit.
    bool submit_result = database_queue_submit_query(worker, &query);
    TEST_ASSERT_TRUE(submit_result);
    TEST_ASSERT_EQUAL_size_t(0, queue_size(worker->queue));

    // The pending result must have been signaled by the cache hit path.
    int wait_result = pending_result_wait(pending, NULL);
    TEST_ASSERT_EQUAL(0, wait_result);
    TEST_ASSERT_TRUE(pending_result_is_completed(pending));

    QueryResult* result = pending_result_get(pending);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL_STRING(data_json, result->data_json);
    TEST_ASSERT_EQUAL_size_t(1, result->row_count);
    TEST_ASSERT_EQUAL_size_t(2, result->column_count);

    // Cleanup
    pending_result_unregister(pending_mgr, pending, NULL);
    free(query.query_id);
    free(query.query_template);
    free(query.parameter_json);
    database_queue_manager_destroy(manager);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_submit_query_cache_hit_skips_queue);

    return UNITY_END();
}
