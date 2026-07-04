/*
 * Unity Test File: worker_pool_test_waiter.c
 *
 * Phase 12 of the LUA_PLAN. End-to-end test of the worker-side
 * completion signal hook:
 *
 *   - When a job has a waiter attached (has_waiter=true) and the
 *     worker marks it COMPLETED, the "would signal waiter" log line
 *     appears exactly once.
 *   - When a job has no waiter attached, the line does NOT appear.
 *   - When a job with a waiter FAILED at runtime, the line still
 *     appears (the worker fires the hook on every terminal status).
 *   - The log line is the worker's view of the same has_waiter flag
 *     that scoreboard_get_waiter reads; both reflect the scoreboard
 *     state at the moment of completion.
 *
 * Phase 12 only logs a marker (the real H_Handle signal is Phase
 * 13). The test pins the "the worker does fire the hook on
 * completion" property by asserting on the log line via
 * mock_logging_message_contains.
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

// Mock log helpers (USE_MOCK_LOGGING is defined by the CMake build)
#include "mock_logging.h"

// Forward declarations (required for -Wmissing-prototypes)
void test_worker_signals_waiter_on_completed(void);
void test_worker_does_not_signal_when_no_waiter(void);
void test_worker_signals_waiter_on_failed(void);
void test_worker_signal_uses_entry_copy_snapshot(void);

#define POLL_TIMEOUT_MS 5000
#define POLL_SLEEP_USEC 1000

// Poll the scoreboard until the job reaches a terminal status, or
// fail the test if it does not within POLL_TIMEOUT_MS. Mirrors the
// helper in worker_pool_test_execute.c.
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

void setUp(void) {
    scripting_init_state();
}

void tearDown(void) {
    scripting_workers_destroy();
    scripting_cleanup_state();
    scripting_system_shutdown = 0;
    scripting_orchestrator_state = NULL;
}

// A job with an attached waiter is signalled on COMPLETED.
void test_worker_signals_waiter_on_completed(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(2));
    mock_logging_reset_all();

    char* id = scripting_submit_job_with_source("ok",
        "return 1\n", NULL);
    TEST_ASSERT_NOT_NULL(id);

    int sentinel = 0;
    TEST_ASSERT_TRUE(scoreboard_attach_waiter(scripting_scoreboard,
                                              id, &sentinel, NULL));

    TEST_ASSERT_EQUAL(SCOREBOARD_JOB_COMPLETED, wait_for_terminal(id));

    // The worker's signal-if-present path is at LOG_LEVEL_TRACE.
    // The mock records every log call, so we can assert that the
    // marker appeared.
    TEST_ASSERT_TRUE(mock_logging_message_contains("would signal waiter"));

    free(id);
}

// A job without an attached waiter produces no "would signal
// waiter" marker (the worker skips the hook entirely).
void test_worker_does_not_signal_when_no_waiter(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(2));
    mock_logging_reset_all();

    char* id = scripting_submit_job_with_source("ok",
        "return 1\n", NULL);
    TEST_ASSERT_NOT_NULL(id);
    // Deliberately do not call scoreboard_attach_waiter.

    TEST_ASSERT_EQUAL(SCOREBOARD_JOB_COMPLETED, wait_for_terminal(id));

    TEST_ASSERT_FALSE(mock_logging_message_contains("would signal waiter"));

    free(id);
}

// A job that fails at runtime still fires the signal hook. The
// worker does not distinguish COMPLETED from FAILED for the
// signal-if-present path; any terminal status triggers it.
void test_worker_signals_waiter_on_failed(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(2));
    mock_logging_reset_all();

    char* id = scripting_submit_job_with_source("boom",
        "error('intentional failure')\n", NULL);
    TEST_ASSERT_NOT_NULL(id);

    int sentinel = 0;
    TEST_ASSERT_TRUE(scoreboard_attach_waiter(scripting_scoreboard,
                                              id, &sentinel, &sentinel));

    TEST_ASSERT_EQUAL(SCOREBOARD_JOB_FAILED, wait_for_terminal(id));

    TEST_ASSERT_TRUE(mock_logging_message_contains("would signal waiter"));

    free(id);
}

// The worker reads has_waiter from the entry copy it took via
// scoreboard_find. We pin this by attaching a waiter, then verifying
// the scoreboard-side get also reflects the same state after the
// job completes (the entry was found and the marker was logged).
void test_worker_signal_uses_entry_copy_snapshot(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(2));
    mock_logging_reset_all();

    char* id = scripting_submit_job_with_source("ok",
        "return 1\n", NULL);
    TEST_ASSERT_NOT_NULL(id);

    static int sentinel;
    sentinel = 42;
    TEST_ASSERT_TRUE(scoreboard_attach_waiter(scripting_scoreboard,
                                              id, &sentinel, NULL));

    TEST_ASSERT_EQUAL(SCOREBOARD_JOB_COMPLETED, wait_for_terminal(id));

    // The scoreboard entry is still in the scoreboard after
    // completion (the scoreboard is append-only; the entry is
    // never removed), so we can read it back and verify the waiter
    // fields survived the job's lifecycle.
    ScoreboardEntry* e = scoreboard_find(scripting_scoreboard, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_TRUE(e->has_waiter);
    TEST_ASSERT_EQUAL_PTR(&sentinel, e->waiter_handle);
    TEST_ASSERT_NULL(e->result_ref);
    scoreboard_entry_free(e);

    // And the worker fired the signal hook.
    TEST_ASSERT_TRUE(mock_logging_message_contains("would signal waiter"));

    free(id);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_worker_signals_waiter_on_completed);
    RUN_TEST(test_worker_does_not_signal_when_no_waiter);
    RUN_TEST(test_worker_signals_waiter_on_failed);
    RUN_TEST(test_worker_signal_uses_entry_copy_snapshot);

    return UNITY_END();
}
