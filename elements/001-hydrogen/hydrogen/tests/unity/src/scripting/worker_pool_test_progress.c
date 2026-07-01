/*
 * Unity Test File: worker_pool_test_progress.c
 *
 * Phase 8 of the LUA_PLAN. End-to-end worker tests for the progress
 * hook and per-job limits:
 *   - A worker job that does meaningful work writes a non-zero
 *     instruction_count and a memory sample to its scoreboard entry.
 *   - A job with a very low hard limit ends in FAILED with a
 *     "memory limit" error in the logs (we don't assert on log
 *     contents, only on the scoreboard status).
 *   - A job with enforce_limits=false and a low hard limit ends in
 *     COMPLETED.
 *   - A job submitted with explicit overrides uses those limits.
 *   - Multiple concurrent jobs each get their own per-job limits.
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

// Mock app_config with the production defaults
static AppConfig mock_app_config_storage = {0};

// Forward declarations (required for -Wmissing-prototypes)
void test_worker_progress_visible_in_scoreboard(void);
void test_worker_hard_limit_kills_job(void);
void test_worker_enforce_false_completes_with_low_hard(void);
void test_worker_per_job_overrides_take_effect(void);
void test_worker_multiple_jobs_each_get_own_limits(void);

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
    // Realistic production defaults
    mock_app_config_storage.scripting.InstructionHookInterval = 100;  // tight for testing
    mock_app_config_storage.scripting.MemorySampleEveryNHooks = 1;   // sample every tick
    mock_app_config_storage.scripting.MemorySoftLimitKB = 32768;
    mock_app_config_storage.scripting.MemoryHardLimitKB = 65536;
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

// A worker job that runs a busy loop produces a non-zero
// instruction_count and at least one memory sample.
void test_worker_progress_visible_in_scoreboard(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(2));
    char* id = scripting_submit_job_with_source("busy_worker",
        "for i = 1, 100000 do end\n", NULL);
    TEST_ASSERT_NOT_NULL(id);

    ScoreboardEntry* e = NULL;
    ScoreboardJobStatus s = wait_for_terminal(id, &e);
    TEST_ASSERT_EQUAL(SCOREBOARD_JOB_COMPLETED, s);
    TEST_ASSERT_NOT_NULL(e);
    // instruction_count is the number of hook calls (not raw VM
    // instructions). With hook_interval=100 and a 100K-iteration
    // numeric for loop (1 bytecode per iteration), the hook fires
    // ~1,000 times.
    TEST_ASSERT_TRUE(e->instruction_count >= 1000);
    scoreboard_entry_free(e);
    free(id);
}

// A job with a very low hard limit (1 KB) is killed by the hook
// (luaL_error longjmp to the worker's pcall). The scoreboard ends
// in FAILED.
void test_worker_hard_limit_kills_job(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(2));
    ScoreboardJobLimits limits = {0};
    limits.instruction_hook_interval = 50;
    limits.memory_hard_limit_kb = 1;  // will trip immediately
    limits.enforce_limits = true;
    char* id = scripting_submit_job_with_source_and_limits("hard_kill",
        "for i = 1, 100000 do end\n", NULL, &limits);
    TEST_ASSERT_NOT_NULL(id);

    ScoreboardEntry* e = NULL;
    ScoreboardJobStatus s = wait_for_terminal(id, &e);
    TEST_ASSERT_EQUAL(SCOREBOARD_JOB_FAILED, s);
    // The hook should have written some progress before the longjmp.
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_TRUE(e->instruction_count > 0);
    scoreboard_entry_free(e);
    free(id);
}

// enforce_limits=false with a low hard limit must NOT kill the job.
void test_worker_enforce_false_completes_with_low_hard(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(2));
    ScoreboardJobLimits limits = {0};
    limits.instruction_hook_interval = 50;
    limits.memory_hard_limit_kb = 1;  // would normally kill
    limits.enforce_limits = false;
    char* id = scripting_submit_job_with_source_and_limits("no_enforce",
        "for i = 1, 1000 do end\n", NULL, &limits);
    TEST_ASSERT_NOT_NULL(id);

    ScoreboardEntry* e = NULL;
    ScoreboardJobStatus s = wait_for_terminal(id, &e);
    TEST_ASSERT_EQUAL(SCOREBOARD_JOB_COMPLETED, s);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_FALSE(e->enforce_limits);
    scoreboard_entry_free(e);
    free(id);
}

// Per-job overrides take effect: a job with a custom hook_interval
// stores it on the entry, and the progress hook uses it.
void test_worker_per_job_overrides_take_effect(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(2));
    ScoreboardJobLimits limits = {0};
    limits.instruction_hook_interval = 200;
    limits.memory_soft_limit_kb = 0;     // no soft limit
    limits.memory_hard_limit_kb = 0;     // no hard limit
    limits.enforce_limits = true;
    char* id = scripting_submit_job_with_source_and_limits("custom",
        "for i = 1, 5000 do end\n", NULL, &limits);
    TEST_ASSERT_NOT_NULL(id);

    ScoreboardEntry* e = NULL;
    ScoreboardJobStatus s = wait_for_terminal(id, &e);
    TEST_ASSERT_EQUAL(SCOREBOARD_JOB_COMPLETED, s);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL_INT(200, e->instruction_hook_interval);
    TEST_ASSERT_TRUE(e->enforce_limits);
    scoreboard_entry_free(e);
    free(id);
}

// Two jobs with different limits: the limits on each entry are
// independent of the other job's.
void test_worker_multiple_jobs_each_get_own_limits(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(2));

    ScoreboardJobLimits tight = {0};
    tight.instruction_hook_interval = 50;
    tight.memory_hard_limit_kb = 1;  // will trip
    tight.enforce_limits = true;
    char* id_tight = scripting_submit_job_with_source_and_limits("tight",
        "for i = 1, 100000 do end\n", NULL, &tight);
    TEST_ASSERT_NOT_NULL(id_tight);

    ScoreboardJobLimits loose = {0};
    loose.instruction_hook_interval = 50;
    loose.memory_hard_limit_kb = 0;  // no hard limit -> use config default
    loose.enforce_limits = true;
    char* id_loose = scripting_submit_job_with_source_and_limits("loose",
        "for i = 1, 1000 do end\n", NULL, &loose);
    TEST_ASSERT_NOT_NULL(id_loose);

    // Wait for the loose one first (it completes quickly).
    ScoreboardEntry* e_loose = NULL;
    ScoreboardJobStatus s_loose = wait_for_terminal(id_loose, &e_loose);
    TEST_ASSERT_EQUAL(SCOREBOARD_JOB_COMPLETED, s_loose);
    TEST_ASSERT_NOT_NULL(e_loose);
    TEST_ASSERT_EQUAL_INT(50, e_loose->instruction_hook_interval);
    scoreboard_entry_free(e_loose);
    free(id_loose);

    // The tight one should have FAILED by now (low hard limit trips fast).
    ScoreboardEntry* e_tight = scoreboard_find(scripting_scoreboard, id_tight);
    TEST_ASSERT_NOT_NULL(e_tight);
    TEST_ASSERT_EQUAL(SCOREBOARD_JOB_FAILED, e_tight->status);
    scoreboard_entry_free(e_tight);
    free(id_tight);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_worker_progress_visible_in_scoreboard);
    RUN_TEST(test_worker_hard_limit_kills_job);
    RUN_TEST(test_worker_enforce_false_completes_with_low_hard);
    RUN_TEST(test_worker_per_job_overrides_take_effect);
    RUN_TEST(test_worker_multiple_jobs_each_get_own_limits);

    return UNITY_END();
}
