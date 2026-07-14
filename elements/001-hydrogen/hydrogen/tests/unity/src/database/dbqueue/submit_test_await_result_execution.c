/*
 * Unity Test File: submit_test_await_result_execution
 *
 * Exercises the REAL implementation of database_queue_await_result()
 * (submit.c), which was previously only exercised through the mock in
 * submit_test_await_result_affected_rows.c (compiled with
 * USE_MOCK_DBQUEUE). The synchronous wait inside database_queue_await_result
 * cannot be driven reliably from a unit test (the producer thread that
 * signals the pending result is starved while the caller blocks in
 * pthread_cond_timedwait in this environment), so this file covers the
 * parts that do NOT require a signalling producer thread:
 *
 *   - Parameter validation (NULL db_queue / NULL query_id)  -> lines 317-320
 *   - The timeout branch (no pending ever signalled)         -> lines 350-366
 *
 * The remaining signal-driven conversion logic (QueryResult -> DatabaseQuery
 * for both the success and NULL-result cases) was extracted into the
 * non-static helper database_query_build_from_result(), which is tested
 * directly and without threading in submit_test_build_from_result.c.
 */

#include <src/hydrogen.h>
#include <unity.h>

// Standard includes
#include <stdlib.h>
#include <string.h>

// Project includes
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/database.h>
#include <src/database/database_pending.h>

// Forward declarations (required for -Wmissing-prototypes)
void test_await_result_null_queue_returns_null(void);
void test_await_result_null_query_id_returns_null(void);
void test_await_result_timeout_returns_null(void);

// -------------------------------------------------------------------------
// Minimal DatabaseQueue construction
//
// database_queue_await_result only ever reads db_queue->database_name,
// db_queue->queue_number and db_queue->tags (via database_queue_generate_label)
// and db_queue->dqm_stats.total_timeouts (via database_queue_record_timeout).
// A zeroed struct with just those fields populated is therefore sufficient
// and avoids spawning any worker/heartbeat threads.
// -------------------------------------------------------------------------
static DatabaseQueue* make_minimal_queue(void) {
    DatabaseQueue* q = calloc(1, sizeof(DatabaseQueue));
    if (!q) return NULL;
    q->database_name = strdup("await_test_db");
    q->queue_number = 0;
    q->tags = strdup("SMFC");
    q->is_lead_queue = false;
    q->queue = NULL;
    return q;
}

static void free_minimal_queue(DatabaseQueue* q) {
    if (!q) return;
    free(q->database_name);
    free(q->tags);
    free(q);
}

void setUp(void) {
    // The global pending manager is lazily created on first use.
    get_pending_result_manager();
}

void tearDown(void) {
    // No per-test cleanup required.
}

void test_await_result_null_queue_returns_null(void) {
    DatabaseQuery* result = database_queue_await_result(NULL, "null_queue_qid", 5);
    TEST_ASSERT_NULL(result);
}

void test_await_result_null_query_id_returns_null(void) {
    DatabaseQueue* q = make_minimal_queue();
    TEST_ASSERT_NOT_NULL(q);

    DatabaseQuery* result = database_queue_await_result(q, NULL, 5);
    TEST_ASSERT_NULL(result);

    free_minimal_queue(q);
}

// timeout_seconds=0 yields an immediate timeout in pending_result_wait,
// so no producer thread is needed. Exercises the timed-out branch
// (pending_result_is_timed_out -> database_queue_record_timeout), which
// returns NULL.
void test_await_result_timeout_returns_null(void) {
    DatabaseQueue* q = make_minimal_queue();
    TEST_ASSERT_NOT_NULL(q);

    DatabaseQuery* result = database_queue_await_result(q, "await_timeout", 0);
    TEST_ASSERT_NULL(result);

    free_minimal_queue(q);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_await_result_null_queue_returns_null);
    RUN_TEST(test_await_result_null_query_id_returns_null);
    RUN_TEST(test_await_result_timeout_returns_null);

    return UNITY_END();
}
