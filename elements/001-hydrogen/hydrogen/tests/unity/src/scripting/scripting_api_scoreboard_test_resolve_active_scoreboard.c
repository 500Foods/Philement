/*
 * Unity Test File: scripting_api_scoreboard_test_resolve_active_scoreboard.c
 *
 * Tests resolve_active_scoreboard:
 *   - NULL state returns scripting_scoreboard
 *   - State with job context (scoreboard set) returns ctx->scoreboard
 *   - State with job context (scoreboard NULL) returns scripting_scoreboard
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <src/scripting/lua_context.h>
#include <src/scripting/scoreboard.h>
#include <src/scripting/scripting.h>
#include <src/scripting/scripting_api_internal.h>

void test_resolve_null_state_returns_global_scoreboard(void);
void test_resolve_with_context_scoreboard_returns_ctx_scoreboard(void);
void test_resolve_with_context_null_scoreboard_returns_global(void);

void setUp(void) {
    scripting_init_state();
}

void tearDown(void) {
    scripting_cleanup_state();
}

void test_resolve_null_state_returns_global_scoreboard(void) {
    Scoreboard* sb = resolve_active_scoreboard(NULL);
    TEST_ASSERT_NOT_NULL(sb);
    TEST_ASSERT_EQUAL_PTR(scripting_scoreboard, sb);
}

void test_resolve_with_context_scoreboard_returns_ctx_scoreboard(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    Scoreboard* custom = scoreboard_create();
    TEST_ASSERT_NOT_NULL(custom);

    H_lua_job_context ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.scoreboard = custom;
    H_lua_set_job_context(L, &ctx);

    Scoreboard* sb = resolve_active_scoreboard(L);
    TEST_ASSERT_NOT_NULL(sb);
    TEST_ASSERT_EQUAL_PTR(custom, sb);

    H_lua_set_job_context(L, NULL);
    H_lua_destroy_context(L);
    scoreboard_destroy(custom);
}

void test_resolve_with_context_null_scoreboard_returns_global(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    H_lua_job_context ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.scoreboard = NULL;
    snprintf(ctx.job_id, sizeof(ctx.job_id), "job1");
    H_lua_set_job_context(L, &ctx);

    Scoreboard* sb = resolve_active_scoreboard(L);
    TEST_ASSERT_NOT_NULL(sb);
    TEST_ASSERT_EQUAL_PTR(scripting_scoreboard, sb);

    H_lua_set_job_context(L, NULL);
    H_lua_destroy_context(L);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_resolve_null_state_returns_global_scoreboard);
    RUN_TEST(test_resolve_with_context_scoreboard_returns_ctx_scoreboard);
    RUN_TEST(test_resolve_with_context_null_scoreboard_returns_global);

    return UNITY_END();
}
