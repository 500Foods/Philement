/*
 * Unity Test File: scripting_api_scoreboard_test_push_scoreboard_entry_as_table.c
 *
 * Tests push_scoreboard_entry_as_table:
 *   - NULL entry pushes nil
 *   - Full entry with all fields populated
 *   - Entry with params_json == NULL
 *   - Entry with started_at == 0 (nil)
 *   - Entry with finished_at == 0 (nil)
 *   - Entry with current_state == NULL (nil)
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <src/scripting/lua_context.h>
#include <src/scripting/scoreboard.h>
#include <src/scripting/scripting_api_internal.h>

void test_push_entry_null_pushes_nil(void);
void test_push_entry_full_populated(void);
void test_push_entry_params_json_null(void);
void test_push_entry_started_at_zero(void);
void test_push_entry_finished_at_zero(void);
void test_push_entry_current_state_null(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_push_entry_null_pushes_nil(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    push_scoreboard_entry_as_table(L, NULL);

    TEST_ASSERT_EQUAL(1, lua_gettop(L));
    TEST_ASSERT_TRUE(lua_isnil(L, -1));

    H_lua_destroy_context(L);
}

void test_push_entry_full_populated(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    ScoreboardEntry e;
    memset(&e, 0, sizeof(e));
    snprintf(e.job_id, sizeof(e.job_id), "%s", "ABCDE");
    e.script_name = strdup("my_script");
    e.params_json = strdup("{\"key\":\"value\"}");
    e.status = SCOREBOARD_JOB_RUNNING;
    e.created_at.tv_sec = 1000;
    e.started_at.tv_sec = 1001;
    e.finished_at.tv_sec = 1002;
    e.instruction_count = 42;
    e.memory_used_kb = 1024;
    e.current_state = strdup("processing");
    e.max_runtime_seconds = 30;
    e.kill_requested = true;

    push_scoreboard_entry_as_table(L, &e);

    TEST_ASSERT_EQUAL(1, lua_gettop(L));
    TEST_ASSERT_TRUE(lua_istable(L, -1));

    lua_getfield(L, -1, "job_id");
    TEST_ASSERT_EQUAL_STRING("ABCDE", lua_tostring(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, -1, "script_name");
    TEST_ASSERT_EQUAL_STRING("my_script", lua_tostring(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, -1, "status");
    TEST_ASSERT_EQUAL_STRING("running", lua_tostring(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, -1, "params_json");
    TEST_ASSERT_EQUAL_STRING("{\"key\":\"value\"}", lua_tostring(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, -1, "created_at");
    TEST_ASSERT_EQUAL(1000, lua_tointeger(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, -1, "started_at");
    TEST_ASSERT_EQUAL(1001, lua_tointeger(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, -1, "finished_at");
    TEST_ASSERT_EQUAL(1002, lua_tointeger(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, -1, "instruction_count");
    TEST_ASSERT_EQUAL(42, lua_tointeger(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, -1, "memory_used_kb");
    TEST_ASSERT_EQUAL(1024, lua_tointeger(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, -1, "current_state");
    TEST_ASSERT_EQUAL_STRING("processing", lua_tostring(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, -1, "max_runtime_seconds");
    TEST_ASSERT_EQUAL(30, lua_tointeger(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, -1, "kill_requested");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);

    lua_pop(L, 1);

    free(e.script_name);
    free(e.params_json);
    free(e.current_state);
    H_lua_destroy_context(L);
}

void test_push_entry_params_json_null(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    ScoreboardEntry e;
    memset(&e, 0, sizeof(e));
    snprintf(e.job_id, sizeof(e.job_id), "%s", "ABCDE");
    e.script_name = strdup("script");
    e.params_json = NULL;
    e.created_at.tv_sec = 100;

    push_scoreboard_entry_as_table(L, &e);

    TEST_ASSERT_EQUAL(1, lua_gettop(L));
    TEST_ASSERT_TRUE(lua_istable(L, -1));

    lua_getfield(L, -1, "params_json");
    TEST_ASSERT_TRUE(lua_isnil(L, -1));
    lua_pop(L, 1);

    lua_pop(L, 1);

    free(e.script_name);
    H_lua_destroy_context(L);
}

void test_push_entry_started_at_zero(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    ScoreboardEntry e;
    memset(&e, 0, sizeof(e));
    snprintf(e.job_id, sizeof(e.job_id), "%s", "ABCDE");
    e.script_name = strdup("script");
    e.created_at.tv_sec = 100;
    e.started_at.tv_sec = 0;

    push_scoreboard_entry_as_table(L, &e);

    lua_getfield(L, -1, "started_at");
    TEST_ASSERT_TRUE(lua_isnil(L, -1));
    lua_pop(L, 1);

    lua_pop(L, 1);

    free(e.script_name);
    H_lua_destroy_context(L);
}

void test_push_entry_finished_at_zero(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    ScoreboardEntry e;
    memset(&e, 0, sizeof(e));
    snprintf(e.job_id, sizeof(e.job_id), "%s", "ABCDE");
    e.script_name = strdup("script");
    e.created_at.tv_sec = 100;
    e.finished_at.tv_sec = 0;

    push_scoreboard_entry_as_table(L, &e);

    lua_getfield(L, -1, "finished_at");
    TEST_ASSERT_TRUE(lua_isnil(L, -1));
    lua_pop(L, 1);

    lua_pop(L, 1);

    free(e.script_name);
    H_lua_destroy_context(L);
}

void test_push_entry_current_state_null(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    ScoreboardEntry e;
    memset(&e, 0, sizeof(e));
    snprintf(e.job_id, sizeof(e.job_id), "%s", "ABCDE");
    e.script_name = strdup("script");
    e.created_at.tv_sec = 100;
    e.current_state = NULL;

    push_scoreboard_entry_as_table(L, &e);

    lua_getfield(L, -1, "current_state");
    TEST_ASSERT_TRUE(lua_isnil(L, -1));
    lua_pop(L, 1);

    lua_pop(L, 1);

    free(e.script_name);
    H_lua_destroy_context(L);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_push_entry_null_pushes_nil);
    RUN_TEST(test_push_entry_full_populated);
    RUN_TEST(test_push_entry_params_json_null);
    RUN_TEST(test_push_entry_started_at_zero);
    RUN_TEST(test_push_entry_finished_at_zero);
    RUN_TEST(test_push_entry_current_state_null);

    return UNITY_END();
}
