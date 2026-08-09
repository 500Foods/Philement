/*
 * Unity Test File: scripting_api_system_test_set_result_json.c
 *
 * LUA_CLIENT Phase 1: H.set_result_json
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <string.h>
#include <unistd.h>
#include <time.h>

#include <src/scripting/lua_context.h>
#include <src/scripting/scoreboard.h>
#include <src/scripting/scripting.h>
#include <src/scripting/scripting_api.h>
#include <src/scripting/worker_pool.h>
#include <src/config/config.h>

#include <lua.h>
#include <lauxlib.h>

extern AppConfig* app_config;
static AppConfig mock_app_config_storage;

void test_set_result_json_is_function(void);
void test_set_result_json_encodes_table(void);
void test_set_result_json_empty_object(void);
void test_set_result_json_array(void);
void test_set_result_json_no_context_noop(void);
void test_set_result_json_non_table_logs(void);
void test_set_result_json_worker_pool_e2e(void);
void test_inject_job_params_global(void);
void test_set_result_json_echo_params_e2e(void);

#define POLL_TIMEOUT_MS 5000
#define POLL_SLEEP_USEC 10000

static void wait_terminal(Scoreboard* sb, const char* id) {
    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (;;) {
        ScoreboardEntry* e = scoreboard_find(sb, id);
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
        long elapsed_ms = (now.tv_sec - start.tv_sec) * 1000
                        + (now.tv_nsec - start.tv_nsec) / 1000000;
        if (elapsed_ms >= POLL_TIMEOUT_MS) {
            TEST_FAIL_MESSAGE("Timed out waiting for job terminal status");
        }
        usleep(POLL_SLEEP_USEC);
    }
}

void setUp(void) {
    memset(&mock_app_config_storage, 0, sizeof(mock_app_config_storage));
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

void test_set_result_json_is_function(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);
    lua_getglobal(L, "H");
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_getfield(L, -1, "set_result_json");
    TEST_ASSERT_TRUE(lua_isfunction(L, -1));
    lua_pop(L, 2);
    H_lua_destroy_context(L);
}

void test_set_result_json_encodes_table(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "direct", NULL);
    lua_State* L = H_lua_create_context();
    H_lua_job_context ctx = {0};
    snprintf(ctx.job_id, sizeof(ctx.job_id), "%s", id);
    ctx.scoreboard = sb;
    H_lua_set_job_context(L, &ctx);

    int rc = H_lua_run_string(L,
        "H.set_result_json({ ok = true, code = 'X', n = 3 })",
        "[set_result_json:direct]");
    TEST_ASSERT_EQUAL_INT(LUA_OK, rc);

    H_lua_set_job_context(L, NULL);
    H_lua_destroy_context(L);

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e->result_json);
    TEST_ASSERT_NOT_NULL(strstr(e->result_json, "\"ok\":true"));
    TEST_ASSERT_NOT_NULL(strstr(e->result_json, "\"code\":\"X\""));
    TEST_ASSERT_NOT_NULL(strstr(e->result_json, "\"n\":3"));
    TEST_ASSERT_EQUAL_STRING("json", e->result_type);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

void test_set_result_json_empty_object(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "empty", NULL);
    lua_State* L = H_lua_create_context();
    H_lua_job_context ctx = {0};
    snprintf(ctx.job_id, sizeof(ctx.job_id), "%s", id);
    ctx.scoreboard = sb;
    H_lua_set_job_context(L, &ctx);

    TEST_ASSERT_EQUAL_INT(LUA_OK, H_lua_run_string(L,
        "H.set_result_json({})", "[empty]"));

    H_lua_set_job_context(L, NULL);
    H_lua_destroy_context(L);

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_EQUAL_STRING("{}", e->result_json);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

void test_set_result_json_array(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "arr", NULL);
    lua_State* L = H_lua_create_context();
    H_lua_job_context ctx = {0};
    snprintf(ctx.job_id, sizeof(ctx.job_id), "%s", id);
    ctx.scoreboard = sb;
    H_lua_set_job_context(L, &ctx);

    TEST_ASSERT_EQUAL_INT(LUA_OK, H_lua_run_string(L,
        "H.set_result_json({10, 20, 30})", "[arr]"));

    H_lua_set_job_context(L, NULL);
    H_lua_destroy_context(L);

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_EQUAL_STRING("[10,20,30]", e->result_json);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

void test_set_result_json_no_context_noop(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_EQUAL_INT(LUA_OK, H_lua_run_string(L,
        "H.set_result_json({ a = 1 })", "[noop]"));
    H_lua_destroy_context(L);
}

void test_set_result_json_non_table_logs(void) {
    lua_State* L = H_lua_create_context();
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "bad", NULL);
    H_lua_job_context ctx = {0};
    snprintf(ctx.job_id, sizeof(ctx.job_id), "%s", id);
    ctx.scoreboard = sb;
    H_lua_set_job_context(L, &ctx);

    TEST_ASSERT_EQUAL_INT(LUA_OK, H_lua_run_string(L,
        "H.set_result_json('not a table')", "[bad]"));

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NULL(e->result_json);
    scoreboard_entry_free(e);
    H_lua_set_job_context(L, NULL);
    H_lua_destroy_context(L);
    free(id);
    scoreboard_destroy(sb);
}

void test_set_result_json_worker_pool_e2e(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(2));

    const char* src =
        "H.set_result_json({ ok = true, via = 'worker' })\n"
        "return 0\n";
    char* job_id = scripting_submit_job_with_source("echo_json", src, NULL);
    TEST_ASSERT_NOT_NULL(job_id);

    wait_terminal(scripting_scoreboard, job_id);

    ScoreboardEntry* e = scoreboard_find(scripting_scoreboard, job_id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL_INT(SCOREBOARD_JOB_COMPLETED, e->status);
    TEST_ASSERT_NOT_NULL(e->result_json);
    TEST_ASSERT_NOT_NULL(strstr(e->result_json, "\"ok\":true"));
    TEST_ASSERT_NOT_NULL(strstr(e->result_json, "\"via\":\"worker\""));
    scoreboard_entry_free(e);
    free(job_id);
}

void test_inject_job_params_global(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);
    H_lua_inject_job_params(L, "{\"n\":3,\"tag\":\"x\"}");
    lua_getglobal(L, "params");
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_getfield(L, -1, "n");
    TEST_ASSERT_EQUAL_INT(3, (int)lua_tointeger(L, -1));
    lua_pop(L, 1);
    lua_getfield(L, -1, "tag");
    TEST_ASSERT_EQUAL_STRING("x", lua_tostring(L, -1));
    lua_pop(L, 2);
    H_lua_inject_job_params(L, NULL);
    lua_getglobal(L, "params");
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    H_lua_destroy_context(L);
}

void test_set_result_json_echo_params_e2e(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(2));

    const char* src =
        "H.set_result_json(params)\n"
        "return 0\n";
    const char* pj = "{\"ping\":1,\"_hydrogen\":{\"sub\":\"u1\"}}";
    char* job_id = scripting_submit_job_with_source("api_echo", src, pj);
    TEST_ASSERT_NOT_NULL(job_id);

    wait_terminal(scripting_scoreboard, job_id);

    ScoreboardEntry* e = scoreboard_find(scripting_scoreboard, job_id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL_INT(SCOREBOARD_JOB_COMPLETED, e->status);
    TEST_ASSERT_NOT_NULL(e->result_json);
    TEST_ASSERT_NOT_NULL(strstr(e->result_json, "\"ping\":1"));
    TEST_ASSERT_NOT_NULL(strstr(e->result_json, "\"sub\":\"u1\""));
    scoreboard_entry_free(e);
    free(job_id);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_set_result_json_is_function);
    RUN_TEST(test_set_result_json_encodes_table);
    RUN_TEST(test_set_result_json_empty_object);
    RUN_TEST(test_set_result_json_array);
    RUN_TEST(test_set_result_json_no_context_noop);
    RUN_TEST(test_set_result_json_non_table_logs);
    RUN_TEST(test_set_result_json_worker_pool_e2e);
    RUN_TEST(test_inject_job_params_global);
    RUN_TEST(test_set_result_json_echo_params_e2e);
    return UNITY_END();
}
