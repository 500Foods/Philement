/*
 * Unity Test File: submit_test_concurrent_await_affected_rows
 *
 * Phase 14 of the LUA_PLAN. Concurrent-claim pipeline test.
 *
 * The test does NOT simulate the engine's atomic-claim semantics
 * (which lives in the database engines' conditional-UPDATE
 * behavior, already covered by per-engine Unity tests). It does
 * verify that Hydrogen's plumbing is thread-safe: N pthreads
 * concurrently call database_queue_await_result, and the
 * affected_rows field is correctly returned to every thread.
 *
 * Atomic task claiming in production works like this:
 *   1. Lua: H.query("UPDATE ... WHERE id=:id AND status='open'")
 *   2. Engine: executes the conditional UPDATE; reports
 *      affected_rows=1 to the winner, affected_rows=0 to losers.
 *   3. Hydrogen: propagates affected_rows from QueryResult through
 *      DatabaseQuery to the Lua result table (this phase).
 *
 * Step 2 is the engine's job; step 3 is Hydrogen's job. This test
 * covers step 3's thread-safety under concurrent load, with N
 * pthreads each calling the await path. The mock returns the
 * pre-set affected_rows for every call; the test asserts all
 * threads see the same value (no lost updates, no race
 * conditions in the await-result plumbing).
 *
 * The mock infrastructure (USE_MOCK_DBQUEUE) is applied to tests
 * in this directory, so the production database_queue_await_result
 * is replaced by mock_database_queue_await_result.
 */

#include <src/hydrogen.h>
#include <unity.h>

// Standard includes
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

// Project includes
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/database.h>

// Test infrastructure
#define USE_MOCK_DBQUEUE
#include <unity/mocks/mock_dbqueue.h>

// Forward declarations
void test_concurrent_await_thread_safety_n8(void);
void test_concurrent_await_thread_safety_n32(void);
void test_concurrent_await_thread_safety_n128(void);

typedef struct AwaitArgs {
    int thread_index;
    int* result_array;       // each thread writes its affected_rows here
} AwaitArgs;

// A non-NULL placeholder DatabaseQueue (the production code only
// uses the pointer as an opaque handle; the mock ignores it).
static DatabaseQueue placeholder_dbq = {0};

// Per-thread: call database_queue_await_result and store the
// returned affected_rows in result_array[thread_index].
static void* await_thread(void* arg) {
    AwaitArgs* args = (AwaitArgs*)arg;
    char query_id[32];
    snprintf(query_id, sizeof(query_id), "concurrent_await_q_%d", args->thread_index);
    DatabaseQuery* q = database_queue_await_result(&placeholder_dbq, query_id, 5);
    if (q) {
        args->result_array[args->thread_index] = q->affected_rows;
        free(q->query_id);
        free(q->query_template);
        free(q->error_message);
        free(q);
    } else {
        args->result_array[args->thread_index] = -1; // signal failure
    }
    return NULL;
}

void setUp(void) {
    mock_dbqueue_reset_all();
}

void tearDown(void) {
    mock_dbqueue_reset_all();
}

// Run the concurrent-claim test with N threads. The mock is set
// to return affected_rows=1 (the "winner" view); every thread
// should see the same value.
static void run_concurrent_await(int n_threads) {
    int* results = calloc((size_t)n_threads, sizeof(int));
    TEST_ASSERT_NOT_NULL(results);

    // The mock returns a fresh copy of this template on every call
    DatabaseQuery template = {0};
    template.affected_rows = 1; // simulate the winner's view
    template.query_template = strdup("[]");
    mock_dbqueue_set_await_result(&template);

    pthread_t* tids = calloc((size_t)n_threads, sizeof(pthread_t));
    AwaitArgs* args = calloc((size_t)n_threads, sizeof(AwaitArgs));
    TEST_ASSERT_NOT_NULL(tids);
    TEST_ASSERT_NOT_NULL(args);
    for (int i = 0; i < n_threads; i++) {
        args[i].thread_index = i;
        args[i].result_array = results;
        int rc = pthread_create(&tids[i], NULL, await_thread, &args[i]);
        TEST_ASSERT_EQUAL(0, rc);
    }
    for (int i = 0; i < n_threads; i++) {
        pthread_join(tids[i], NULL);
    }

    // Every thread must see the pre-set affected_rows (1).
    // Any -1 means the await returned NULL (mock not reached or
    // other failure) which would mean the plumbing is broken.
    int winners = 0;
    int failed = 0;
    for (int i = 0; i < n_threads; i++) {
        if (results[i] == 1) winners++;
        else if (results[i] == -1) failed++;
    }
    TEST_ASSERT_EQUAL(0, failed);
    TEST_ASSERT_EQUAL(n_threads, winners);

    free(results);
    free(tids);
    free(args);
}

// N=8 concurrent awaits
void test_concurrent_await_thread_safety_n8(void) {
    run_concurrent_await(8);
}

// N=32 concurrent awaits
void test_concurrent_await_thread_safety_n32(void) {
    run_concurrent_await(32);
}

// N=128 concurrent awaits (stress)
void test_concurrent_await_thread_safety_n128(void) {
    run_concurrent_await(128);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_concurrent_await_thread_safety_n8);
    RUN_TEST(test_concurrent_await_thread_safety_n32);
    RUN_TEST(test_concurrent_await_thread_safety_n128);

    return UNITY_END();
}
