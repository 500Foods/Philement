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
 *   - The worker claims waiter fields live from the scoreboard at
 *     terminal status (scoreboard_claim_waiter), so attach after
 *     job start still works if it happens before claim.
 *
 * Phase 12 only logs a marker (the real H_Handle signal is Phase
 * 13). The test pins the "the worker does fire the hook on
 * completion" property by asserting on the log line via
 * mock_logging_message_contains.
 *
 * Signal tests use a short busy-loop body so the worker is still
 * RUNNING when attach runs. Instant `return 1` can complete before
 * attach under suite load; live claim fixes mid-run attach, not
 * attach-after-terminal (callers must check status after attach).
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
void test_worker_signal_uses_live_claim(void);

#define POLL_TIMEOUT_MS 5000
#define POLL_SLEEP_USEC 1000

// Keep the worker busy long enough that attach reliably runs before
// terminal claim under concurrent suite load.
#define LUA_BUSY_OK \
    "local n = 0\n" \
    "for i = 1, 2000000 do n = n + 1 end\n" \
    "return 1\n"

#define LUA_BUSY_FAIL \
    "local n = 0\n" \
    "for i = 1, 2000000 do n = n + 1 end\n" \
    "error('intentional failure')\n"

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

    int sentinel = 0;
    char* id = NULL;
    submit_and_attach_while_running("ok", LUA_BUSY_OK,
                                    &sentinel, NULL, &id);

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

    int sentinel = 0;
    char* id = NULL;
    submit_and_attach_while_running("boom", LUA_BUSY_FAIL,
                                    &sentinel, &sentinel, &id);

    TEST_ASSERT_EQUAL(SCOREBOARD_JOB_FAILED, wait_for_terminal(id));

    TEST_ASSERT_TRUE(mock_logging_message_contains("would signal waiter"));

    free(id);
}

// Live claim: attach while RUNNING, then after COMPLETED the
// scoreboard still holds the waiter fields and the worker logged
// the signal marker from scoreboard_claim_waiter.
void test_worker_signal_uses_live_claim(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(2));
    mock_logging_reset_all();

    static int sentinel;
    sentinel = 42;
    char* id = NULL;
    submit_and_attach_while_running("ok", LUA_BUSY_OK,
                                    &sentinel, NULL, &id);

    TEST_ASSERT_EQUAL(SCOREBOARD_JOB_COMPLETED, wait_for_terminal(id));

    // The scoreboard entry is still in the scoreboard after
    // completion (append-only). Waiter fields survive; claim marks
    // waiter_signaled so a second claim is a no-op.
    ScoreboardEntry* e = scoreboard_find(scripting_scoreboard, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_TRUE(e->has_waiter);
    TEST_ASSERT_TRUE(e->waiter_signaled);
    TEST_ASSERT_EQUAL_PTR(&sentinel, e->waiter_handle);
    TEST_ASSERT_NULL(e->result_ref);
    scoreboard_entry_free(e);

    void* handle = NULL;
    void* result = NULL;
    TEST_ASSERT_FALSE(scoreboard_claim_waiter(scripting_scoreboard, id,
                                              &handle, &result));

    TEST_ASSERT_TRUE(mock_logging_message_contains("would signal waiter"));

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
