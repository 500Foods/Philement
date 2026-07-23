/*
 * Unity Test File: scripting_api_scoreboard_test_H_lua_scoreboard_cancel.c
 *
 * Tests H_lua_scoreboard_cancel:
 *   - No arguments: error, false
 *   - Non-string argument: error, false
 *   - Unknown job_id: false
 *   - Found job_id: true
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

void test_scoreboard_cancel_no_args_returns_false(void);
void test_scoreboard_cancel_non_string_arg_returns_false(void);
void test_scoreboard_cancel_unknown_id_returns_false(void);
void test_scoreboard_cancel_found_id_returns_true(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_scoreboard_cancel_no_args_returns_false(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    int rc = H_lua_scoreboard_cancel(L);
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT_FALSE(lua_toboolean(L, -1));

    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_scoreboard_cancel_non_string_arg_returns_false(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    lua_pushinteger(L, 42);

    int rc = H_lua_scoreboard_cancel(L);
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT_FALSE(lua_toboolean(L, -1));

    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_scoreboard_cancel_unknown_id_returns_false(void) {
    Scoreboard* sb = scoreboard_create();
    TEST_ASSERT_NOT_NULL(sb);

    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    H_lua_job_context ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.scoreboard = sb;
    H_lua_set_job_context(L, &ctx);

    lua_pushstring(L, "ZZZZZ");

    int rc = H_lua_scoreboard_cancel(L);
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT_FALSE(lua_toboolean(L, -1));

    lua_pop(L, 1);
    H_lua_set_job_context(L, NULL);
    H_lua_destroy_context(L);
    scoreboard_destroy(sb);
}

void test_scoreboard_cancel_found_id_returns_true(void) {
    Scoreboard* sb = scoreboard_create();
    TEST_ASSERT_NOT_NULL(sb);

    char* id = scoreboard_submit(sb, "cancel_me", NULL);
    TEST_ASSERT_NOT_NULL(id);

    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    H_lua_job_context ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.scoreboard = sb;
    H_lua_set_job_context(L, &ctx);

    lua_pushstring(L, id);

    int rc = H_lua_scoreboard_cancel(L);
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));

    lua_pop(L, 1);
    H_lua_set_job_context(L, NULL);
    H_lua_destroy_context(L);
    free(id);
    scoreboard_destroy(sb);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_scoreboard_cancel_no_args_returns_false);
    RUN_TEST(test_scoreboard_cancel_non_string_arg_returns_false);
    RUN_TEST(test_scoreboard_cancel_unknown_id_returns_false);
    RUN_TEST(test_scoreboard_cancel_found_id_returns_true);

    return UNITY_END();
}
