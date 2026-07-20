/*
 * Unity Test File: http_pool_test_should_exit.c
 *
 * Drives scripting_http_worker_should_exit() directly across all its
 * branches:
 *   - shutdown not requested                         -> false (176-177)
 *   - shutdown requested, pool/queue NULL            -> true  (179-180)
 *   - shutdown requested, queue present but empty    -> true  (182)
 *   - shutdown requested, queue non-empty            -> false (182 negated)
 *
 * The pool is hand-built with a real queue but no worker threads, so
 * the test is fully deterministic (no thread can drain the queue).
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Standard includes
#include <string.h>
#include <stdlib.h>

// Module under test
#include <src/scripting/http_pool.h>
#include <src/queue/queue.h>

// Forward declarations
void test_should_exit_not_requested(void);
void test_should_exit_pool_null(void);
void test_should_exit_queue_empty(void);
void test_should_exit_queue_nonempty(void);

// Build a minimal pool with a real queue and no worker threads. The
// worker_count field only influences scripting_http_pool_destroy's
// join loop, which is a no-op at 0.
static ScriptingHttpPool* make_pool(int worker_count) {
    ScriptingHttpPool* pool = calloc(1, sizeof(ScriptingHttpPool));
    TEST_ASSERT_NOT_NULL(pool);
    pool->worker_count = worker_count;
    pool->worker_tids = NULL;
    QueueAttributes attrs = {0};
    pool->http_queue = queue_create_with_label(SCRIPTING_HTTP_QUEUE_NAME,
                                               &attrs, SR_SCRIPTING);
    TEST_ASSERT_NOT_NULL(pool->http_queue);
    return pool;
}

void setUp(void) {
    scripting_system_shutdown = 0;
}

void tearDown(void) {
    scripting_system_shutdown = 0;
    scripting_http_pool_destroy();
}

// No shutdown requested: should_exit returns false regardless of queue.
void test_should_exit_not_requested(void) {
    scripting_system_shutdown = 0;
    ScriptingHttpPool* pool = make_pool(0);
    TEST_ASSERT_FALSE(scripting_http_worker_should_exit(pool));
    queue_release(pool->http_queue);
    free(pool);
}

// Shutdown requested and pool is NULL: should_exit returns true.
void test_should_exit_pool_null(void) {
    scripting_system_shutdown = 1;
    TEST_ASSERT_TRUE(scripting_http_worker_should_exit(NULL));
}

// Shutdown requested, queue present but empty: should_exit returns true.
void test_should_exit_queue_empty(void) {
    scripting_system_shutdown = 1;
    ScriptingHttpPool* pool = make_pool(0);
    TEST_ASSERT_TRUE(scripting_http_worker_should_exit(pool));
    queue_release(pool->http_queue);
    free(pool);
}

// Shutdown requested, queue non-empty: should_exit returns false so the
// workers drain before exiting.
void test_should_exit_queue_nonempty(void) {
    scripting_system_shutdown = 1;
    ScriptingHttpPool* pool = make_pool(0);
    int dummy = 0;
    TEST_ASSERT_TRUE(queue_enqueue(pool->http_queue,
                                   (const char*)&dummy, sizeof(dummy), 0));
    TEST_ASSERT_FALSE(scripting_http_worker_should_exit(pool));
    // Drain so the queue is empty before we release it.
    size_t sz = 0;
    int prio = 0;
    char* item = queue_dequeue_nonblocking(pool->http_queue, &sz, &prio);
    TEST_ASSERT_NOT_NULL(item);
    free(item);
    queue_release(pool->http_queue);
    free(pool);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_should_exit_not_requested);
    RUN_TEST(test_should_exit_pool_null);
    RUN_TEST(test_should_exit_queue_empty);
    RUN_TEST(test_should_exit_queue_nonempty);
    return UNITY_END();
}
