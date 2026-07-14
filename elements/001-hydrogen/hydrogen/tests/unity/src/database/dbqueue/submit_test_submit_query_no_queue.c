/*
 * Unity Test File: submit_test_submit_query_no_queue
 *
 * Exercises database_queue_submit_query() (submit.c) branch at lines 235-236
 * ("No queue available for query"), which was uncovered by both Unity and
 * blackbox suites. The branch is reached when the queue has no underlying
 * enqueue target and the query is neither a cache hit nor routed to a child
 * queue. A synthetic (calloc'd) DatabaseQueue with queue == NULL is used so
 * no worker/heartbeat threads are spawned.
 *
 * Note: the cache-hit signal-failure branch (line 184) is intentionally not
 * covered here. That path does result->data_json = json_dumps(...) (allocated
 * by jansson's real malloc) and then database_engine_cleanup_result() frees it
 * via the globally-mocked free(). Under USE_MOCK_SYSTEM that malloc/free
 * mismatch aborts, and the path is already exercised by the blackbox suite, so
 * it is treated as an irreducible Unity floor.
 */

#include <src/hydrogen.h>
#include <unity.h>

// Standard includes
#include <stdlib.h>
#include <string.h>

// Project includes
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/database.h>

// Forward declarations (required for -Wmissing-prototypes)
void test_submit_query_no_queue_available(void);

// Build a minimally-populated DatabaseQueue. submit_query only reads
// database_name (cache key), is_lead_queue, and queue (NULL here) on the
// branches exercised by this test.
static DatabaseQueue* make_minimal_queue(const char* name) {
    DatabaseQueue* q = calloc(1, sizeof(DatabaseQueue));
    if (!q) return NULL;
    q->database_name = strdup(name);
    q->queue_number = 0;
    q->tags = NULL;
    q->is_lead_queue = false;
    q->queue = NULL;
    return q;
}

static void free_minimal_queue(DatabaseQueue* q) {
    if (!q) return;
    free(q->database_name);
    free(q);
}

void setUp(void) {
    // No setup required
}

void tearDown(void) {
    // No teardown required
}

// Covers lines 235-236: no queue available for the query.
void test_submit_query_no_queue_available(void) {
    DatabaseQueue* q = make_minimal_queue("noqueue_db");
    TEST_ASSERT_NOT_NULL(q);

    DatabaseQuery query = {0};
    query.query_template = strdup("SELECT 1");

    bool result = database_queue_submit_query(q, &query);
    TEST_ASSERT_FALSE(result);

    free(query.query_template);
    free_minimal_queue(q);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_submit_query_no_queue_available);

    return UNITY_END();
}
