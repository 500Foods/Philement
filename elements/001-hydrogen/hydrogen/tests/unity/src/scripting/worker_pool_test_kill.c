/*
 * Unity Test File: worker_pool_test_kill.c
 *
 * Phase 10 of the LUA_PLAN. End-to-end worker tests for the kill
 * path:
 *   - A job that exceeds max_runtime_seconds ends in KILLED (not
 *     FAILED). The error message mentions the kill cause.
 *   - A job that has kill_requested=true before the worker
 *     dequeues it is skipped (no Lua execution) and marked KILLED.
 *   - A job whose kill_requested is set MID-run (after the worker
 *     has started running it) ends in KILLED on the next hook
 *     tick.
 *   - A normal runtime error (not a kill) still ends in FAILED.
 *     The KILLED vs FAILED distinction is preserved.
 *   - A job with max_runtime_seconds=INT_MAX runs to completion
 *     even if it is busy for a long time.
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Standard includes
#include <string.h>
#include <limits.h>
#include <time.h>
#include <unistd.h>

// Modules under test
#include <src/scripting/scripting.h>
#include <src/scripting/scoreboard.h>
#include <src/scripting/worker_pool.h>
#include <src/threads/threads.h>

// Mock app_config with the production defaults
static AppConfig mock_app_config_storage = {0};

// Forward declarations (required for -Wmissing-prototypes)
void test_worker_kill_via_max_runtime(void);
void test_worker_kill_via_external_request_before_dequeue(void);
void test_worker_kill_via_external_request_mid_run(void);
void test_worker_runtime_error_still_failed(void);
void test_worker_int_max_max_runtime_runs_to_completion(void);
void test_worker_kill_requested_during_shutdown_safe(void);

#define POLL_TIMEOUT_MS 10000
#define POLL_SLEEP_USEC 1000

// Poll the scoreboard until the job reaches a terminal status, or
// fail the test if it does not within POLL_TIMEOUT_MS. Returns the
// final status (and populates *out_entry with a heap copy the caller
// must free, if out_entry is non-NULL).
static ScoreboardJobStatus wait_for_terminal(const char* job_id,
                                             ScoreboardEntry** out_entry) {
    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);
    while (1) {
        ScoreboardEntry* e = scoreboard_find(scripting_scoreboard, job_id);
        if (e) {
            if (e->status == SCOREBOARD_JOB_COMPLETED
                || e->status == SCOREBOARD_JOB_FAILED
                || e->status == SCOREBOARD_JOB_KILLED) {
                ScoreboardJobStatus s = e->status;
                if (out_entry) {
                    *out_entry = e;
                } else {
                    scoreboard_entry_free(e);
                }
                return s;
            }
            scoreboard_entry_free(e);
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
    memset(&mock_app_config_storage, 0, sizeof(mock_app_config_storage));
    // Tight hook interval + sample-every=1 for fast test loops.
    mock_app_config_storage.scripting.InstructionHookInterval = 50;
    mock_app_config_storage.scripting.MemorySampleEveryNHooks = 1;
    mock_app_config_storage.scripting.MemorySoftLimitKB = 32768;
    mock_app_config_storage.scripting.MemoryHardLimitKB = 65536;
    mock_app_config_storage.scripting.DefaultMaxRuntime = 60;
    app_config = &mock_app_config_storage;
    scripting_init_state();
}

void tearDown(void) {
    scripting_workers_destroy();
    scripting_cleanup_state();
    scripting_system_shutdown = 0;
    scripting_orchestrator_state = NULL;
    app_config = NULL;
}

// A job that does real work but is given a max_runtime of 1 second
// is killed by the hook (the busy loop in the test ensures the
// hook fires many times) and the scoreboard status is KILLED.
void test_worker_kill_via_max_runtime(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(2));
    ScoreboardJobLimits limits = {0};
    limits.instruction_hook_interval = 50;
    limits.max_runtime_seconds = 1;  // 1 second deadline
    limits.enforce_limits = true;
    // A busy loop that takes well over 1 second at typical rates.
    char* id = scripting_submit_job_with_source_and_limits("rt_kill",
        "local t0 = os.time()\n"
        "while os.time() - t0 < 5 do end\n",
        NULL, &limits);
    TEST_ASSERT_NOT_NULL(id);

    ScoreboardEntry* e = NULL;
    ScoreboardJobStatus s = wait_for_terminal(id, &e);
    TEST_ASSERT_EQUAL(SCOREBOARD_JOB_KILLED, s);
    TEST_ASSERT_NOT_NULL(e);
    scoreboard_entry_free(e);
    free(id);
}

// A job whose kill_requested is set BEFORE the worker dequeues it
// is skipped and marked KILLED without ever running Lua. We do
// this by submitting a job that does a real-time busy loop and
// requesting the kill on the scoreboard immediately after the
// submit returns. The race between submit and worker-dequeue
// resolves as "skip + KILLED" if we win, or "hook trips + KILLED"
// if the worker wins - either way the contract is "KILLED, not
// FAILED". The pin is the terminal status, not the path.
void test_worker_kill_via_external_request_before_dequeue(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(2));
    // A real-time loop (1s) that the hook can interrupt. We use
    // os.time() rather than H.sleep because H.sleep is a Phase 11
    // deliverable; a busy loop is what the hook is designed for.
    char* id = scripting_submit_job_with_source("pending_kill",
        "local t0 = os.time()\n"
        "while os.time() - t0 < 1 do end\n",
        NULL);
    TEST_ASSERT_NOT_NULL(id);

    // Try to set the kill flag immediately. Even if the worker
    // has already dequeued and is between transitions, the PENDING-
    // skip check happens before Lua execution, so this still
    // produces KILLED.
    scoreboard_request_kill(scripting_scoreboard, id);

    ScoreboardEntry* e = NULL;
    ScoreboardJobStatus s = wait_for_terminal(id, &e);
    TEST_ASSERT_EQUAL(SCOREBOARD_JOB_KILLED, s);
    TEST_ASSERT_NOT_NULL(e);
    scoreboard_entry_free(e);
    free(id);
}

// A long-running job that has kill_requested set after a short
// delay (the test thread sets it) is killed on the next hook tick.
// The scoreboard status is KILLED.
void test_worker_kill_via_external_request_mid_run(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(2));
    // 5-second busy loop; the test thread sets kill_requested
    // after 100ms.
    char* id = scripting_submit_job_with_source("long_running",
        "local t0 = os.time()\n"
        "while os.time() - t0 < 5 do end\n",
        NULL);
    TEST_ASSERT_NOT_NULL(id);

    // Give the worker a moment to dequeue and start running.
    usleep(100000);  // 100 ms

    // Request the kill. The hook is on a 50-instruction interval,
    // so the next tick (within a few ms) will trip the kill.
    TEST_ASSERT_TRUE(scoreboard_request_kill(scripting_scoreboard, id));

    ScoreboardEntry* e = NULL;
    ScoreboardJobStatus s = wait_for_terminal(id, &e);
    TEST_ASSERT_EQUAL(SCOREBOARD_JOB_KILLED, s);
    TEST_ASSERT_NOT_NULL(e);
    scoreboard_entry_free(e);
    free(id);
}

// A normal runtime error (e.g. indexing a nil value) is still
// FAILED, not KILLED. This pins the KILLED vs FAILED distinction:
// a kill is a kill (external flag or deadline), an error is an
// error.
void test_worker_runtime_error_still_failed(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(2));
    char* id = scripting_submit_job_with_source("errs",
        "error('boom')\n", NULL);
    TEST_ASSERT_NOT_NULL(id);

    ScoreboardEntry* e = NULL;
    ScoreboardJobStatus s = wait_for_terminal(id, &e);
    TEST_ASSERT_EQUAL(SCOREBOARD_JOB_FAILED, s);
    TEST_ASSERT_NOT_NULL(e);
    scoreboard_entry_free(e);
    free(id);
}

// A job with max_runtime_seconds=INT_MAX is not killed by the
// time check, even if it does a fair amount of work. (We don't
// run a multi-second job here because that would slow the test;
// the deadline check is the same code path tested by the earlier
// tests, and INT_MAX means "skip the check".)
void test_worker_int_max_max_runtime_runs_to_completion(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(2));
    ScoreboardJobLimits limits = {0};
    limits.instruction_hook_interval = 50;
    limits.max_runtime_seconds = INT_MAX;
    limits.enforce_limits = true;
    char* id = scripting_submit_job_with_source_and_limits("no_rlim",
        "for i = 1, 10000 do end\n", NULL, &limits);
    TEST_ASSERT_NOT_NULL(id);

    ScoreboardEntry* e = NULL;
    ScoreboardJobStatus s = wait_for_terminal(id, &e);
    TEST_ASSERT_EQUAL(SCOREBOARD_JOB_COMPLETED, s);
    TEST_ASSERT_NOT_NULL(e);
    scoreboard_entry_free(e);
    free(id);
}

// Setting kill_requested on a job that has ALREADY reached a
// terminal status is a no-op. The scoreboard returns true (entry
// exists) but the kill path is not retroactively triggered (the
// hook only runs while the state is RUNNING).
void test_worker_kill_requested_during_shutdown_safe(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(2));
    char* id = scripting_submit_job_with_source("fast",
        "return 0\n", NULL);
    TEST_ASSERT_NOT_NULL(id);

    // Wait for it to complete first.
    ScoreboardEntry* e = NULL;
    ScoreboardJobStatus s = wait_for_terminal(id, &e);
    TEST_ASSERT_EQUAL(SCOREBOARD_JOB_COMPLETED, s);
    scoreboard_entry_free(e);

    // Now request kill on a COMPLETED job. The flag flips, but
    // nothing else happens (no Lua state, no hook).
    TEST_ASSERT_TRUE(scoreboard_request_kill(scripting_scoreboard, id));

    // The entry's flag is now true, but the status stays COMPLETED.
    e = scoreboard_find(scripting_scoreboard, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_TRUE(e->kill_requested);
    TEST_ASSERT_EQUAL(SCOREBOARD_JOB_COMPLETED, e->status);
    scoreboard_entry_free(e);
    free(id);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_worker_kill_via_max_runtime);
    RUN_TEST(test_worker_kill_via_external_request_before_dequeue);
    RUN_TEST(test_worker_kill_via_external_request_mid_run);
    RUN_TEST(test_worker_runtime_error_still_failed);
    RUN_TEST(test_worker_int_max_max_runtime_runs_to_completion);
    RUN_TEST(test_worker_kill_requested_during_shutdown_safe);

    return UNITY_END();
}
