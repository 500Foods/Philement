/*
 * Unity Test File: worker_pool_test_waiter.c
 *
 * Phase 12 of the LUA_PLAN. End-to-end test of the worker-side
 * completion signal hook:
 *
 *   - When a job has a waiter attached and the worker marks it
 *     COMPLETED, scoreboard_claim_waiter sets waiter_signaled.
 *   - When a job has no waiter, waiter_signaled stays false.
 *   - FAILED jobs claim the waiter the same way as COMPLETED.
 *   - Claim is one-shot: a second scoreboard_claim_waiter is false.
 *
 * Assertions use scoreboard waiter_signaled (the production claim
 * path), not mock log history. Idle workers emit high-volume MUTEX
 * logs that used to overflow a small mock history and produce flaky
 * false negatives under suite load.
 *
 * Attach runs while the job is still PENDING/RUNNING so claim at
 * terminal status observes the waiter. Callers that attach after
 * terminal must check status themselves (Phase 13 H.wait).
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Standard includes
#include <string.h>
#include <time.h>
#include <unistd.h>

// Modules under test
#include <src/scripting/scripting.h>
#include <src/scripting/scoreboard.h>
#include <src/scripting/worker_pool.h>
#include <src/threads/threads.h>

// Forward declarations (required for -Wmissing-prototypes)
void test_worker_signals_waiter_on_completed(void);
void test_worker_does_not_signal_when_no_waiter(void);
void test_worker_signals_waiter_on_failed(void);
void test_worker_signal_uses_live_claim(void);

#define POLL_TIMEOUT_MS 5000
#define POLL_SLEEP_USEC 1000

// Short busy body so attach can run before terminal claim under load.
// Keep this modest: longer loops only increase idle-worker log noise.
#define LUA_BUSY_OK \
    "local n = 0\n" \
    "for i = 1, 500000 do n = n + 1 end\n" \
    "return 1\n"

#define LUA_BUSY_FAIL \
    "local n = 0\n" \
    "for i = 1, 500000 do n = n + 1 end\n" \
    "error('intentional failure')\n"

// Poll the scoreboard until the job reaches a terminal status, or
// fail the test if it does not within POLL_TIMEOUT_MS.
static ScoreboardJobStatus wait_for_terminal(const char* job_id) {
    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);
    while (1) {
        ScoreboardEntry* e = scoreboard_find(scripting_scoreboard, job_id);
        if (e) {
            ScoreboardJobStatus s = e->status;
            scoreboard_entry_free(e);
            if (s == SCOREBOARD_JOB_COMPLETED
                || s == SCOREBOARD_JOB_FAILED
                || s == SCOREBOARD_JOB_KILLED) {
                return s;
            }
        }
        clock_gettime(CLOCK_MONOTONIC, &now);
        long elapsed_ms = (now.tv_sec - start.tv_sec) * 1000
                        + (now.tv_nsec - start.tv_nsec) / 1000000;
        if (elapsed_ms >= POLL_TIMEOUT_MS) {
            TEST_FAIL_MESSAGE("Timed out waiting for job to reach terminal status");
        }
        usleep(POLL_SLEEP_USEC);
    }
}

// Submit then attach while the job is still non-terminal so the
// worker's live claim observes the waiter.
static void submit_and_attach_while_running(const char* name,
                                            const char* source,
                                            void* handle,
                                            void* result_ref,
                                            char** out_id) {
    char* id = scripting_submit_job_with_source(name, source, NULL);
    TEST_ASSERT_NOT_NULL(id);

    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);
    while (1) {
        ScoreboardEntry* e = scoreboard_find(scripting_scoreboard, id);
        if (e) {
            ScoreboardJobStatus s = e->status;
            scoreboard_entry_free(e);
            if (s == SCOREBOARD_JOB_PENDING || s == SCOREBOARD_JOB_RUNNING) {
                TEST_ASSERT_TRUE(scoreboard_attach_waiter(scripting_scoreboard,
                                                          id, handle, result_ref));
                *out_id = id;
                return;
            }
            if (s == SCOREBOARD_JOB_COMPLETED
                || s == SCOREBOARD_JOB_FAILED
                || s == SCOREBOARD_JOB_KILLED) {
                free(id);
                TEST_FAIL_MESSAGE("Job reached terminal before waiter attach");
            }
        }
        clock_gettime(CLOCK_MONOTONIC, &now);
        long elapsed_ms = (now.tv_sec - start.tv_sec) * 1000
                        + (now.tv_nsec - start.tv_nsec) / 1000000;
        if (elapsed_ms >= POLL_TIMEOUT_MS) {
            free(id);
            TEST_FAIL_MESSAGE("Timed out waiting to attach waiter while running");
        }
        usleep(POLL_SLEEP_USEC);
    }
}

static void assert_waiter_claimed(const char* job_id, void* expected_handle,
                                  void* expected_result, bool expect_claimed) {
    ScoreboardEntry* e = scoreboard_find(scripting_scoreboard, job_id);
    TEST_ASSERT_NOT_NULL(e);
    if (expect_claimed) {
        TEST_ASSERT_TRUE(e->has_waiter);
        TEST_ASSERT_TRUE(e->waiter_signaled);
        TEST_ASSERT_EQUAL_PTR(expected_handle, e->waiter_handle);
        TEST_ASSERT_EQUAL_PTR(expected_result, e->result_ref);
    } else {
        TEST_ASSERT_FALSE(e->has_waiter);
        TEST_ASSERT_FALSE(e->waiter_signaled);
        TEST_ASSERT_NULL(e->waiter_handle);
        TEST_ASSERT_NULL(e->result_ref);
    }
    scoreboard_entry_free(e);
}

void setUp(void) {
    scripting_init_state();
}

void tearDown(void) {
    scripting_workers_destroy();
    scripting_cleanup_state();
    scripting_system_shutdown = 0;
    scripting_orchestrator_state = NULL;
}

// A job with an attached waiter is claimed on COMPLETED.
void test_worker_signals_waiter_on_completed(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(2));

    int sentinel = 0;
    char* id = NULL;
    submit_and_attach_while_running("ok", LUA_BUSY_OK,
                                    &sentinel, NULL, &id);

    TEST_ASSERT_EQUAL(SCOREBOARD_JOB_COMPLETED, wait_for_terminal(id));
    assert_waiter_claimed(id, &sentinel, NULL, true);

    free(id);
}

// A job without an attached waiter is never claimed.
void test_worker_does_not_signal_when_no_waiter(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(2));

    char* id = scripting_submit_job_with_source("ok",
        "return 1\n", NULL);
    TEST_ASSERT_NOT_NULL(id);

    TEST_ASSERT_EQUAL(SCOREBOARD_JOB_COMPLETED, wait_for_terminal(id));
    assert_waiter_claimed(id, NULL, NULL, false);

    free(id);
}

// A job that fails at runtime still claims the waiter.
void test_worker_signals_waiter_on_failed(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(2));

    int sentinel = 0;
    char* id = NULL;
    submit_and_attach_while_running("boom", LUA_BUSY_FAIL,
                                    &sentinel, &sentinel, &id);

    TEST_ASSERT_EQUAL(SCOREBOARD_JOB_FAILED, wait_for_terminal(id));
    assert_waiter_claimed(id, &sentinel, &sentinel, true);

    free(id);
}

// Live claim is one-shot: after the worker claims, a second claim fails.
void test_worker_signal_uses_live_claim(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(2));

    static int sentinel;
    sentinel = 42;
    char* id = NULL;
    submit_and_attach_while_running("ok", LUA_BUSY_OK,
                                    &sentinel, NULL, &id);

    TEST_ASSERT_EQUAL(SCOREBOARD_JOB_COMPLETED, wait_for_terminal(id));
    assert_waiter_claimed(id, &sentinel, NULL, true);

    void* handle = NULL;
    void* result = NULL;
    TEST_ASSERT_FALSE(scoreboard_claim_waiter(scripting_scoreboard, id,
                                              &handle, &result));

    free(id);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_worker_signals_waiter_on_completed);
    RUN_TEST(test_worker_does_not_signal_when_no_waiter);
    RUN_TEST(test_worker_signals_waiter_on_failed);
    RUN_TEST(test_worker_signal_uses_live_claim);

    return UNITY_END();
}
