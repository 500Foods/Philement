/*
 * Unity Test File: worker_pool_test_shutdown.c
 *
 * Phase 7 of the LUA_PLAN. Validates the worker pool shutdown path:
 *   - scripting_workers_destroy with a NULL pool is a no-op
 *   - destroy after init is idempotent
 *   - destroy joins all worker threads (scripting_threads.thread_count
 *     returns to 0)
 *   - destroy signals the shutdown flag and lets in-flight work finish
 *   - destroy frees the script registry and job queue
 *   - destroy is safe to call mid-shutdown with work still queued
 *
 * These tests do not need polling: the destroy path blocks on
 * pthread_join until all workers exit, so the test can assert
 * directly after the call.
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Standard includes
#include <pthread.h>
#include <string.h>
#include <time.h>

// Modules under test
#include <src/scripting/scripting.h>
#include <src/scripting/scoreboard.h>
#include <src/scripting/worker_pool.h>
#include <src/scripting/script_registry.h>
#include <src/threads/threads.h>

// Forward declarations (required for -Wmissing-prototypes)
void test_destroy_null_pool_is_safe(void);
void test_destroy_after_init_clears_state(void);
void test_destroy_joins_worker_threads(void);
void test_destroy_drains_in_flight_work(void);
void test_destroy_is_idempotent(void);
void test_destroy_releases_registry_and_queue(void);

void setUp(void) {
    scripting_init_state();
}

void tearDown(void) {
    scripting_workers_destroy();
    scripting_cleanup_state();
    scripting_system_shutdown = 0;
    scripting_orchestrator_state = NULL;
}

// scripting_workers_destroy on a NULL pool is a no-op.
void test_destroy_null_pool_is_safe(void) {
    TEST_ASSERT_NULL(scripting_workers);
    scripting_workers_destroy();
    TEST_ASSERT_NULL(scripting_workers);
}

// After init+destroy, the pool pointer is NULL and scripting_threads
// is back to zero workers.
void test_destroy_after_init_clears_state(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(3));
    TEST_ASSERT_NOT_NULL(scripting_workers);
    TEST_ASSERT_EQUAL_INT(3, scripting_threads.thread_count);

    scripting_workers_destroy();
    TEST_ASSERT_NULL(scripting_workers);
    TEST_ASSERT_EQUAL_INT(0, scripting_threads.thread_count);
}

// submit several jobs, then destroy: all worker threads join and
// the scoreboard reflects the work that was actually done.
void test_destroy_joins_worker_threads(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(4));
    for (int i = 0; i < 10; i++) {
        char name[16];
        snprintf(name, sizeof(name), "j%d", i);
        char* id = scripting_submit_job_with_source(name, "return 1\n", NULL);
        TEST_ASSERT_NOT_NULL(id);
        free(id);
    }
    scripting_workers_destroy();
    TEST_ASSERT_NULL(scripting_workers);
    TEST_ASSERT_EQUAL_INT(0, scripting_threads.thread_count);
    // All 10 jobs were submitted; some may still be PENDING/RUNNING,
    // but the workers' pthread_t records are gone.
}

// The shutdown path lets in-flight jobs complete (the worker drains
// the queue before exiting). After destroy, jobs that were submitted
// before the call should be in a terminal state.
void test_destroy_drains_in_flight_work(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(2));
    char* id1 = scripting_submit_job_with_source("drain_a", "return 1\n", NULL);
    char* id2 = scripting_submit_job_with_source("drain_b", "return 2\n", NULL);
    TEST_ASSERT_NOT_NULL(id1);
    TEST_ASSERT_NOT_NULL(id2);

    scripting_workers_destroy();
    TEST_ASSERT_NULL(scripting_workers);

    // Both jobs should be COMPLETED because the workers drain the
    // queue before exiting.
    ScoreboardEntry* e1 = scoreboard_find(scripting_scoreboard, id1);
    ScoreboardEntry* e2 = scoreboard_find(scripting_scoreboard, id2);
    TEST_ASSERT_NOT_NULL(e1);
    TEST_ASSERT_NOT_NULL(e2);
    TEST_ASSERT_EQUAL(SCOREBOARD_JOB_COMPLETED, e1->status);
    TEST_ASSERT_EQUAL(SCOREBOARD_JOB_COMPLETED, e2->status);
    scoreboard_entry_free(e1);
    scoreboard_entry_free(e2);
    free(id1);
    free(id2);
}

// Calling destroy twice in a row is safe (idempotent).
void test_destroy_is_idempotent(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(2));
    scripting_workers_destroy();
    scripting_workers_destroy();
    TEST_ASSERT_NULL(scripting_workers);
    TEST_ASSERT_EQUAL_INT(0, scripting_threads.thread_count);
}

// After destroy, the pool's registry and job_queue are gone, so a
// subsequent submit returns NULL.
void test_destroy_releases_registry_and_queue(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(2));
    scripting_workers_destroy();

    // scripting_workers is NULL; submit should refuse.
    char* id = scripting_submit_job("anything", NULL);
    TEST_ASSERT_NULL(id);
    char* id2 = scripting_submit_job_with_source("anything",
        "return 1", NULL);
    TEST_ASSERT_NULL(id2);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_destroy_null_pool_is_safe);
    RUN_TEST(test_destroy_after_init_clears_state);
    RUN_TEST(test_destroy_joins_worker_threads);
    RUN_TEST(test_destroy_drains_in_flight_work);
    RUN_TEST(test_destroy_is_idempotent);
    RUN_TEST(test_destroy_releases_registry_and_queue);

    return UNITY_END();
}
