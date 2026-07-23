/*
 * Unity Test File: scripting_api_scoreboard_test_H_lua_scoreboard_get.c
 *
 * Tests H_lua_scoreboard_get:
 *   - No arguments: error, nil
 *   - Non-string argument: error, nil
 *   - Unknown job_id: nil
 *   - Found job_id: table with entry fields
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

void test_scoreboard_get_no_args_returns_nil(void);
void test_scoreboard_get_non_string_arg_returns_nil(void);
void test_scoreboard_get_unknown_id_returns_nil(void);
void test_scoreboard_get_found_id_returns_table(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_scoreboard_get_no_args_returns_nil(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    int rc = H_lua_scoreboard_get(L);
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT_TRUE(lua_isnil(L, -1));

    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_scoreboard_get_non_string_arg_returns_nil(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    lua_pushinteger(L, 42);

    int rc = H_lua_scoreboard_get(L);
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT_TRUE(lua_isnil(L, -1));

    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_scoreboard_get_unknown_id_returns_nil(void) {
    Scoreboard* sb = scoreboard_create();
    TEST_ASSERT_NOT_NULL(sb);

    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    H_lua_job_context ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.scoreboard = sb;
    H_lua_set_job_context(L, &ctx);

    lua_pushstring(L, "ZZZZZ");

    int rc = H_lua_scoreboard_get(L);
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT_TRUE(lua_isnil(L, -1));

    lua_pop(L, 1);
    H_lua_set_job_context(L, NULL);
    H_lua_destroy_context(L);
    scoreboard_destroy(sb);
}

void test_scoreboard_get_found_id_returns_table(void) {
    Scoreboard* sb = scoreboard_create();
    TEST_ASSERT_NOT_NULL(sb);

    char* id = scoreboard_submit(sb, "lookup_script", "{\"k\":1}");
    TEST_ASSERT_NOT_NULL(id);

    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    H_lua_job_context ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.scoreboard = sb;
    H_lua_set_job_context(L, &ctx);

    lua_pushstring(L, id);

    int rc = H_lua_scoreboard_get(L);
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT_TRUE(lua_istable(L, -1));

    lua_getfield(L, -1, "job_id");
    TEST_ASSERT_EQUAL_STRING(id, lua_tostring(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, -1, "script_name");
    TEST_ASSERT_EQUAL_STRING("lookup_script", lua_tostring(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, -1, "params_json");
    TEST_ASSERT_EQUAL_STRING("{\"k\":1}", lua_tostring(L, -1));
    lua_pop(L, 1);

    lua_pop(L, 1);
    H_lua_set_job_context(L, NULL);
    H_lua_destroy_context(L);
    free(id);
    scoreboard_destroy(sb);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_scoreboard_get_no_args_returns_nil);
    RUN_TEST(test_scoreboard_get_non_string_arg_returns_nil);
    RUN_TEST(test_scoreboard_get_unknown_id_returns_nil);
    RUN_TEST(test_scoreboard_get_found_id_returns_table);

    return UNITY_END();
}
