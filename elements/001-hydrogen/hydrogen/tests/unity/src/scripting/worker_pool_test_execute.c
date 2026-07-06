/*
 * Unity Test File: worker_pool_test_execute.c
 *
 * Phase 7 of the LUA_PLAN. End-to-end worker execution:
 *   - submit a valid job -> worker picks it up, runs it, status COMPLETED
 *   - submit a job with a runtime error -> status FAILED
 *   - submit a job with a syntax error -> status FAILED
 *   - submit a job whose script_name is not registered -> status FAILED
 *   - worker tracks started_at on PENDING->RUNNING and finished_at on
 *     RUNNING->COMPLETED
 *
 * The polling helper waits for a terminal status with a bounded
 * timeout. Workers may pick up a job in <1ms (poll interval is 1ms).
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Standard includes
#include <string.h>
#include <time.h>

// Modules under test
#include <src/scripting/scripting.h>
#include <src/scripting/scoreboard.h>
#include <src/scripting/worker_pool.h>
#include <src/threads/threads.h>

// Forward declarations (required for -Wmissing-prototypes)
void test_execute_valid_job_completes(void);
void test_execute_runtime_error_marks_failed(void);
void test_execute_syntax_error_marks_failed(void);
void test_execute_unknown_script_marks_failed(void);
void test_execute_timestamps_are_stamped(void);
void test_execute_lua_h_table_is_available(void);

#define POLL_TIMEOUT_MS 5000
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
    scripting_init_state();
}

void tearDown(void) {
    scripting_workers_destroy();
    scripting_cleanup_state();
    scripting_system_shutdown = 0;
    scripting_orchestrator_state = NULL;
}

// A valid job runs to COMPLETED.
void test_execute_valid_job_completes(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(2));
    char* id = scripting_submit_job_with_source("ok",
        "return 1\n", NULL);
    TEST_ASSERT_NOT_NULL(id);

    ScoreboardEntry* e = NULL;
    ScoreboardJobStatus s = wait_for_terminal(id, &e);
    TEST_ASSERT_EQUAL(SCOREBOARD_JOB_COMPLETED, s);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL_STRING("ok", e->script_name);
    scoreboard_entry_free(e);
    free(id);
}

// A runtime error in the script is logged and status is FAILED.
void test_execute_runtime_error_marks_failed(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(2));
    char* id = scripting_submit_job_with_source("boom",
        "error('kaboom')\n", NULL);
    TEST_ASSERT_NOT_NULL(id);

    ScoreboardEntry* e = NULL;
    ScoreboardJobStatus s = wait_for_terminal(id, &e);
    TEST_ASSERT_EQUAL(SCOREBOARD_JOB_FAILED, s);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_NOT_NULL(e->error_message);
    TEST_ASSERT_TRUE(strstr(e->error_message, "kaboom") != NULL);
    TEST_ASSERT_NOT_NULL(e->error_traceback);
    scoreboard_entry_free(e);
    free(id);
}

// A syntax error in the script is caught at compile time and the
// job is marked FAILED.
void test_execute_syntax_error_marks_failed(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(2));
    char* id = scripting_submit_job_with_source("bad_syntax",
        "this is not valid lua (\n", NULL);
    TEST_ASSERT_NOT_NULL(id);

    ScoreboardEntry* e = NULL;
    ScoreboardJobStatus s = wait_for_terminal(id, &e);
    TEST_ASSERT_EQUAL(SCOREBOARD_JOB_FAILED, s);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_NOT_NULL(e->error_message);
    TEST_ASSERT_TRUE(strstr(e->error_message, "syntax error") != NULL ||
                     strstr(e->error_message, "'(' expected") != NULL);
    TEST_ASSERT_NOT_NULL(e->error_traceback);
    scoreboard_entry_free(e);
    free(id);
}

// A submit_job (without source) for a name that was never registered
// is marked FAILED by the worker.
void test_execute_unknown_script_marks_failed(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(2));
    char* id = scripting_submit_job("never_registered", NULL);
    TEST_ASSERT_NOT_NULL(id);

    ScoreboardEntry* e = NULL;
    ScoreboardJobStatus s = wait_for_terminal(id, &e);
    TEST_ASSERT_EQUAL(SCOREBOARD_JOB_FAILED, s);
    // Unknown script path may or may not populate error fields;
    // the key invariant is the FAILED status.
    if (e) {
        scoreboard_entry_free(e);
    }
    free(id);
}

// PENDING->RUNNING stamps started_at; RUNNING->COMPLETED stamps
// finished_at (Phase 5 behaviour). Workers preserve this on the
// happy path.
void test_execute_timestamps_are_stamped(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(2));
    char* id = scripting_submit_job_with_source("ts",
        "return 1\n", NULL);
    TEST_ASSERT_NOT_NULL(id);

    ScoreboardEntry* e = NULL;
    ScoreboardJobStatus s = wait_for_terminal(id, &e);
    TEST_ASSERT_EQUAL(SCOREBOARD_JOB_COMPLETED, s);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_TRUE(e->started_at.tv_sec > 0);
    TEST_ASSERT_TRUE(e->finished_at.tv_sec > 0);
    scoreboard_entry_free(e);
    free(id);
}

// The H table is available in the worker's lua_State (Phase 3
// installation). A script that reads H.log.info and H.system.now
// should not error.
void test_execute_lua_h_table_is_available(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(2));
    char* id = scripting_submit_job_with_source("h_check",
        "assert(type(H) == 'table')\n"
        "assert(type(H.log) == 'table')\n"
        "assert(type(H.log.info) == 'function')\n"
        "assert(type(H.system) == 'table')\n"
        "assert(type(H.system.now) == 'function')\n"
        "return 0\n",
        NULL);
    TEST_ASSERT_NOT_NULL(id);

    ScoreboardJobStatus s = wait_for_terminal(id, NULL);
    TEST_ASSERT_EQUAL(SCOREBOARD_JOB_COMPLETED, s);
    free(id);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_execute_valid_job_completes);
    RUN_TEST(test_execute_runtime_error_marks_failed);
    RUN_TEST(test_execute_syntax_error_marks_failed);
    RUN_TEST(test_execute_unknown_script_marks_failed);
    RUN_TEST(test_execute_timestamps_are_stamped);
    RUN_TEST(test_execute_lua_h_table_is_available);

    return UNITY_END();
}
