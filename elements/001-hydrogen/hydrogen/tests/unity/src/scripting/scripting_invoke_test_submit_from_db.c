/*
 * Unity Test File: scripting_invoke_test_submit_from_db.c
 *
 * LUA_CLIENT Phase 2: parse name, load-source hook, submit_from_db.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <src/scripting/scripting.h>
#include <src/scripting/scripting_invoke.h>
#include <src/scripting/worker_pool.h>
#include <src/scripting/scoreboard.h>
#include <src/config/config.h>

extern AppConfig* app_config;
static AppConfig mock_cfg;

void test_error_name_known(void);
void test_parse_valid_dot(void);
void test_parse_rejects_slash(void);
void test_parse_rejects_bad(void);
void test_submit_null_args(void);
void test_submit_disabled(void);
void test_submit_invalid_name(void);
void test_submit_not_found_hook(void);
void test_submit_no_database_hook(void);
void test_submit_success_hook(void);
void test_submit_with_limits_hook(void);
void test_allowlist_allow_api_echo(void);
void test_allowlist_deny_orchestrator(void);
void test_allowlist_deny_unknown(void);

#define POLL_TIMEOUT_MS 5000
#define POLL_SLEEP_USEC 10000

/* Simulates QueryRef #149: only Api.Echo is invokable. */
static char* hook_allowlist(const char* group, const char* script,
                            int timeout, ScriptingInvokeError* err) {
    (void)timeout;
    if (group && script
        && strcmp(group, "Api") == 0
        && strcmp(script, "Echo") == 0) {
        if (err) {
            *err = SCRIPTING_INVOKE_OK;
        }
        return strdup("H.set_result_json({ ok = true })\nreturn 0\n");
    }
    if (err) {
        *err = SCRIPTING_INVOKE_ERR_NOT_FOUND;
    }
    return NULL;
}

static char* hook_ok_source(const char* group, const char* script,
                            int timeout, ScriptingInvokeError* err) {
    (void)timeout;
    TEST_ASSERT_EQUAL_STRING("Api", group);
    TEST_ASSERT_EQUAL_STRING("Echo", script);
    if (err) {
        *err = SCRIPTING_INVOKE_OK;
    }
    return strdup("H.set_result_json({ ok = true })\nreturn 0\n");
}

static char* hook_not_found(const char* group, const char* script,
                            int timeout, ScriptingInvokeError* err) {
    (void)group;
    (void)script;
    (void)timeout;
    if (err) {
        *err = SCRIPTING_INVOKE_ERR_NOT_FOUND;
    }
    return NULL;
}

static char* hook_no_db(const char* group, const char* script,
                        int timeout, ScriptingInvokeError* err) {
    (void)group;
    (void)script;
    (void)timeout;
    if (err) {
        *err = SCRIPTING_INVOKE_ERR_NO_DATABASE;
    }
    return NULL;
}

static void wait_terminal(const char* id) {
    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (;;) {
        ScoreboardEntry* e = scoreboard_find(scripting_scoreboard, id);
        if (e) {
            ScoreboardJobStatus st = e->status;
            if (st == SCOREBOARD_JOB_COMPLETED || st == SCOREBOARD_JOB_FAILED
                || st == SCOREBOARD_JOB_KILLED) {
                scoreboard_entry_free(e);
                return;
            }
            scoreboard_entry_free(e);
        }
        clock_gettime(CLOCK_MONOTONIC, &now);
        long ms = (now.tv_sec - start.tv_sec) * 1000
                + (now.tv_nsec - start.tv_nsec) / 1000000;
        if (ms >= POLL_TIMEOUT_MS) {
            TEST_FAIL_MESSAGE("timeout waiting for job");
        }
        usleep(POLL_SLEEP_USEC);
    }
}

void setUp(void) {
    memset(&mock_cfg, 0, sizeof(mock_cfg));
    mock_cfg.scripting.Enabled = true;
    mock_cfg.scripting.WorkerCount = 2;
    app_config = &mock_cfg;
    scripting_invoke_set_load_source_hook(NULL);
    scripting_init_state();
}

void tearDown(void) {
    scripting_invoke_set_load_source_hook(NULL);
    scripting_workers_destroy();
    scripting_cleanup_state();
    scripting_system_shutdown = 0;
    app_config = NULL;
}

void test_error_name_known(void) {
    TEST_ASSERT_EQUAL_STRING("ok", scripting_invoke_error_name(SCRIPTING_INVOKE_OK));
    TEST_ASSERT_EQUAL_STRING("invalid_script_name",
        scripting_invoke_error_name(SCRIPTING_INVOKE_ERR_INVALID_NAME));
    TEST_ASSERT_EQUAL_STRING("script_not_found",
        scripting_invoke_error_name(SCRIPTING_INVOKE_ERR_NOT_FOUND));
}

void test_parse_valid_dot(void) {
    char* g = NULL;
    char* s = NULL;
    TEST_ASSERT_TRUE(scripting_invoke_parse_script_name("Api.Echo", &g, &s));
    TEST_ASSERT_EQUAL_STRING("Api", g);
    TEST_ASSERT_EQUAL_STRING("Echo", s);
    free(g);
    free(s);

    TEST_ASSERT_TRUE(scripting_invoke_parse_script_name("G.Sub.Name", &g, &s));
    TEST_ASSERT_EQUAL_STRING("G", g);
    TEST_ASSERT_EQUAL_STRING("Sub.Name", s);
    free(g);
    free(s);
}

void test_parse_rejects_slash(void) {
    char* g = NULL;
    char* s = NULL;
    TEST_ASSERT_FALSE(scripting_invoke_parse_script_name("Api/Echo", &g, &s));
    TEST_ASSERT_NULL(g);
    TEST_ASSERT_NULL(s);
}

void test_parse_rejects_bad(void) {
    char* g = NULL;
    char* s = NULL;
    TEST_ASSERT_FALSE(scripting_invoke_parse_script_name("", &g, &s));
    TEST_ASSERT_FALSE(scripting_invoke_parse_script_name("NoDot", &g, &s));
    TEST_ASSERT_FALSE(scripting_invoke_parse_script_name(".Echo", &g, &s));
    TEST_ASSERT_FALSE(scripting_invoke_parse_script_name("Api.", &g, &s));
    TEST_ASSERT_FALSE(scripting_invoke_parse_script_name(NULL, &g, &s));
}

void test_submit_null_args(void) {
    char* id = NULL;
    TEST_ASSERT_EQUAL_INT(SCRIPTING_INVOKE_ERR_INTERNAL,
        scripting_submit_job_from_db(NULL, NULL, NULL, 5, &id));
    TEST_ASSERT_EQUAL_INT(SCRIPTING_INVOKE_ERR_INTERNAL,
        scripting_submit_job_from_db("Api.Echo", NULL, NULL, 5, NULL));
}

void test_submit_disabled(void) {
    mock_cfg.scripting.Enabled = false;
    char* id = NULL;
    TEST_ASSERT_EQUAL_INT(SCRIPTING_INVOKE_ERR_DISABLED,
        scripting_submit_job_from_db("Api.Echo", "{}", NULL, 5, &id));
    TEST_ASSERT_NULL(id);

    mock_cfg.scripting.Enabled = true;
    /* workers not started → disabled */
    TEST_ASSERT_EQUAL_INT(SCRIPTING_INVOKE_ERR_DISABLED,
        scripting_submit_job_from_db("Api.Echo", NULL, NULL, 5, &id));
}

void test_submit_invalid_name(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(1));
    char* id = NULL;
    TEST_ASSERT_EQUAL_INT(SCRIPTING_INVOKE_ERR_INVALID_NAME,
        scripting_submit_job_from_db("bad", NULL, NULL, 5, &id));
    TEST_ASSERT_EQUAL_INT(SCRIPTING_INVOKE_ERR_INVALID_NAME,
        scripting_submit_job_from_db("a/b", NULL, NULL, 5, &id));
}

void test_submit_not_found_hook(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(1));
    scripting_invoke_set_load_source_hook(hook_not_found);
    char* id = NULL;
    TEST_ASSERT_EQUAL_INT(SCRIPTING_INVOKE_ERR_NOT_FOUND,
        scripting_submit_job_from_db("Api.Echo", NULL, NULL, 5, &id));
    TEST_ASSERT_NULL(id);
}

void test_submit_no_database_hook(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(1));
    scripting_invoke_set_load_source_hook(hook_no_db);
    char* id = NULL;
    TEST_ASSERT_EQUAL_INT(SCRIPTING_INVOKE_ERR_NO_DATABASE,
        scripting_submit_job_from_db("Api.Echo", NULL, NULL, 5, &id));
}

void test_submit_success_hook(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(2));
    scripting_invoke_set_load_source_hook(hook_ok_source);
    char* id = NULL;
    ScriptingInvokeError err = scripting_submit_job_from_db(
        "Api.Echo", "{\"n\":1}", NULL, 5, &id);
    TEST_ASSERT_EQUAL_INT(SCRIPTING_INVOKE_OK, err);
    TEST_ASSERT_NOT_NULL(id);

    wait_terminal(id);
    ScoreboardEntry* e = scoreboard_find(scripting_scoreboard, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL_INT(SCOREBOARD_JOB_COMPLETED, e->status);
    TEST_ASSERT_NOT_NULL(e->result_json);
    TEST_ASSERT_NOT_NULL(strstr(e->result_json, "\"ok\":true"));
    scoreboard_entry_free(e);
    free(id);
}

void test_submit_with_limits_hook(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(1));
    scripting_invoke_set_load_source_hook(hook_ok_source);
    ScoreboardJobLimits lim = {0};
    lim.max_runtime_seconds = 30;
    lim.enforce_limits = true;
    char* id = NULL;
    TEST_ASSERT_EQUAL_INT(SCRIPTING_INVOKE_OK,
        scripting_submit_job_from_db("Api.Echo", NULL, &lim, 5, &id));
    TEST_ASSERT_NOT_NULL(id);
    wait_terminal(id);
    free(id);
}

/* Phase 7: allow / deny matrix (404 existence-hiding, not 403). */
void test_allowlist_allow_api_echo(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(1));
    scripting_invoke_set_load_source_hook(hook_allowlist);
    char* id = NULL;
    TEST_ASSERT_EQUAL_INT(SCRIPTING_INVOKE_OK,
        scripting_submit_job_from_db("Api.Echo", "{}", NULL, 5, &id));
    TEST_ASSERT_NOT_NULL(id);
    wait_terminal(id);
    free(id);
}

void test_allowlist_deny_orchestrator(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(1));
    scripting_invoke_set_load_source_hook(hook_allowlist);
    char* id = NULL;
    TEST_ASSERT_EQUAL_INT(SCRIPTING_INVOKE_ERR_NOT_FOUND,
        scripting_submit_job_from_db("Orchestrators.Orchestrator",
                                     NULL, NULL, 5, &id));
    TEST_ASSERT_NULL(id);
}

void test_allowlist_deny_unknown(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(1));
    scripting_invoke_set_load_source_hook(hook_allowlist);
    char* id = NULL;
    TEST_ASSERT_EQUAL_INT(SCRIPTING_INVOKE_ERR_NOT_FOUND,
        scripting_submit_job_from_db("Secret.Rce", NULL, NULL, 5, &id));
    TEST_ASSERT_NULL(id);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_error_name_known);
    RUN_TEST(test_parse_valid_dot);
    RUN_TEST(test_parse_rejects_slash);
    RUN_TEST(test_parse_rejects_bad);
    RUN_TEST(test_submit_null_args);
    RUN_TEST(test_submit_disabled);
    RUN_TEST(test_submit_invalid_name);
    RUN_TEST(test_submit_not_found_hook);
    RUN_TEST(test_submit_no_database_hook);
    RUN_TEST(test_submit_success_hook);
    RUN_TEST(test_submit_with_limits_hook);
    RUN_TEST(test_allowlist_allow_api_echo);
    RUN_TEST(test_allowlist_deny_orchestrator);
    RUN_TEST(test_allowlist_deny_unknown);
    return UNITY_END();
}
