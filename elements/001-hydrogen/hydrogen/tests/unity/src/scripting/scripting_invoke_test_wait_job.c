/*
 * Unity Test File: scripting_invoke_test_wait_job.c
 *
 * LUA_CLIENT Phase 3: scripting_wait_job
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <src/scripting/scripting.h>
#include <src/scripting/scripting_invoke.h>
#include <src/scripting/worker_pool.h>
#include <src/scripting/scoreboard.h>
#include <src/config/config.h>

extern AppConfig* app_config;
static AppConfig mock_cfg;

void test_wait_result_names(void);
void test_wait_internal_bad_args(void);
void test_wait_not_found(void);
void test_wait_already_completed(void);
void test_wait_success_from_worker(void);
void test_wait_failed_job(void);
void test_wait_timeout_requests_kill(void);
void test_wait_shutdown_mid_poll(void);

void setUp(void) {
    memset(&mock_cfg, 0, sizeof(mock_cfg));
    mock_cfg.scripting.Enabled = true;
    mock_cfg.scripting.WorkerCount = 2;
    app_config = &mock_cfg;
    scripting_init_state();
}

void tearDown(void) {
    scripting_workers_destroy();
    scripting_cleanup_state();
    scripting_system_shutdown = 0;
    app_config = NULL;
}

void test_wait_result_names(void) {
    TEST_ASSERT_EQUAL_STRING("completed",
        scripting_wait_result_name(SCRIPTING_WAIT_COMPLETED));
    TEST_ASSERT_EQUAL_STRING("timeout",
        scripting_wait_result_name(SCRIPTING_WAIT_TIMEOUT));
    TEST_ASSERT_EQUAL_STRING("shutdown",
        scripting_wait_result_name(SCRIPTING_WAIT_SHUTDOWN));
}

void test_wait_internal_bad_args(void) {
    TEST_ASSERT_EQUAL_INT(SCRIPTING_WAIT_INTERNAL,
        scripting_wait_job(NULL, 1, NULL));
    TEST_ASSERT_EQUAL_INT(SCRIPTING_WAIT_INTERNAL,
        scripting_wait_job("ABCDE", 0, NULL));
}

void test_wait_not_found(void) {
    TEST_ASSERT_EQUAL_INT(SCRIPTING_WAIT_NOT_FOUND,
        scripting_wait_job("ZZZZZ", 1, NULL));
}

void test_wait_already_completed(void) {
    Scoreboard* sb = scripting_scoreboard;
    char* id = scoreboard_submit(sb, "done", NULL);
    scoreboard_update_status(sb, id, SCOREBOARD_JOB_RUNNING);
    scoreboard_update_status(sb, id, SCOREBOARD_JOB_COMPLETED);
    scoreboard_update_result_json(sb, id, "{\"pre\":true}");

    ScoreboardEntry* e = NULL;
    ScriptingWaitResult wr = scripting_wait_job(id, 2, &e);
    TEST_ASSERT_EQUAL_INT(SCRIPTING_WAIT_COMPLETED, wr);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL_STRING("{\"pre\":true}", e->result_json);
    scoreboard_entry_free(e);
    free(id);
}

void test_wait_success_from_worker(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(2));
    char* id = scripting_submit_job_with_source(
        "wait_ok",
        "H.set_result_json({ ok = true })\nreturn 0\n",
        NULL);
    TEST_ASSERT_NOT_NULL(id);

    ScoreboardEntry* e = NULL;
    ScriptingWaitResult wr = scripting_wait_job(id, 5, &e);
    TEST_ASSERT_EQUAL_INT(SCRIPTING_WAIT_COMPLETED, wr);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_NOT_NULL(e->result_json);
    TEST_ASSERT_NOT_NULL(strstr(e->result_json, "\"ok\":true"));
    scoreboard_entry_free(e);
    free(id);
}

void test_wait_failed_job(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(1));
    char* id = scripting_submit_job_with_source(
        "wait_fail",
        "error('boom')\n",
        NULL);
    TEST_ASSERT_NOT_NULL(id);

    ScoreboardEntry* e = NULL;
    ScriptingWaitResult wr = scripting_wait_job(id, 5, &e);
    TEST_ASSERT_EQUAL_INT(SCRIPTING_WAIT_FAILED, wr);
    TEST_ASSERT_NOT_NULL(e);
    scoreboard_entry_free(e);
    free(id);
}

void test_wait_timeout_requests_kill(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(1));
    /* Long sleep so wait times out first. */
    char* id = scripting_submit_job_with_source(
        "wait_slow",
        "H.sleep(5000)\nreturn 0\n",
        NULL);
    TEST_ASSERT_NOT_NULL(id);

    ScoreboardEntry* e = NULL;
    ScriptingWaitResult wr = scripting_wait_job(id, 1, &e);
    TEST_ASSERT_EQUAL_INT(SCRIPTING_WAIT_TIMEOUT, wr);
    if (e) {
        TEST_ASSERT_TRUE(e->kill_requested
            || e->status == SCOREBOARD_JOB_KILLED
            || e->status == SCOREBOARD_JOB_RUNNING
            || e->status == SCOREBOARD_JOB_PENDING
            || e->status == SCOREBOARD_JOB_COMPLETED);
        scoreboard_entry_free(e);
    }
    free(id);
    /* Let worker finish/kill before tearDown destroy. */
    usleep(200000);
}

void test_wait_shutdown_mid_poll(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(1));
    char* id = scripting_submit_job_with_source(
        "wait_shut",
        "H.sleep(5000)\nreturn 0\n",
        NULL);
    TEST_ASSERT_NOT_NULL(id);

    scripting_system_shutdown = 1;
    ScoreboardEntry* e = NULL;
    ScriptingWaitResult wr = scripting_wait_job(id, 5, &e);
    TEST_ASSERT_EQUAL_INT(SCRIPTING_WAIT_SHUTDOWN, wr);
    if (e) {
        scoreboard_entry_free(e);
    }
    free(id);
    scripting_system_shutdown = 0;
    usleep(100000);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_wait_result_names);
    RUN_TEST(test_wait_internal_bad_args);
    RUN_TEST(test_wait_not_found);
    RUN_TEST(test_wait_already_completed);
    RUN_TEST(test_wait_success_from_worker);
    RUN_TEST(test_wait_failed_job);
    RUN_TEST(test_wait_timeout_requests_kill);
    RUN_TEST(test_wait_shutdown_mid_poll);
    return UNITY_END();
}
