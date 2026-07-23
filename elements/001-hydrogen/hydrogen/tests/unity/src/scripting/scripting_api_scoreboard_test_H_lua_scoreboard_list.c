/*
 * Unity Test File: scripting_api_scoreboard_test_H_lua_scoreboard_list.c
 *
 * Tests H_lua_scoreboard_list:
 *   - NULL scoreboard (snapshot failed path)
 *   - Empty scoreboard returns empty table
 *   - Scoreboard with entries returns populated table
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

void test_scoreboard_list_null_scoreboard_returns_empty_table(void);
void test_scoreboard_list_empty_returns_empty_table(void);
void test_scoreboard_list_with_entries(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_scoreboard_list_null_scoreboard_returns_empty_table(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    int nargs = lua_gettop(L);
    int rc = H_lua_scoreboard_list(L);
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT_EQUAL(nargs + 1, lua_gettop(L));
    TEST_ASSERT_TRUE(lua_istable(L, -1));

    lua_Integer count = luaL_len(L, -1);
    TEST_ASSERT_EQUAL(0, count);

    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_scoreboard_list_empty_returns_empty_table(void) {
    Scoreboard* sb = scoreboard_create();
    TEST_ASSERT_NOT_NULL(sb);

    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    H_lua_job_context ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.scoreboard = sb;
    H_lua_set_job_context(L, &ctx);

    int rc = H_lua_scoreboard_list(L);
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT_TRUE(lua_istable(L, -1));

    lua_Integer count = luaL_len(L, -1);
    TEST_ASSERT_EQUAL(0, count);

    lua_pop(L, 1);
    H_lua_set_job_context(L, NULL);
    H_lua_destroy_context(L);
    scoreboard_destroy(sb);
}

void test_scoreboard_list_with_entries(void) {
    Scoreboard* sb = scoreboard_create();
    TEST_ASSERT_NOT_NULL(sb);

    char* id1 = scoreboard_submit(sb, "script_one", "{\"a\":1}");
    TEST_ASSERT_NOT_NULL(id1);
    char* id2 = scoreboard_submit(sb, "script_two", NULL);
    TEST_ASSERT_NOT_NULL(id2);

    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    H_lua_job_context ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.scoreboard = sb;
    H_lua_set_job_context(L, &ctx);

    int rc = H_lua_scoreboard_list(L);
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT_TRUE(lua_istable(L, -1));

    lua_Integer count = luaL_len(L, -1);
    TEST_ASSERT_EQUAL(2, count);

    lua_rawgeti(L, -1, 1);
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_getfield(L, -1, "job_id");
    TEST_ASSERT_EQUAL_STRING(id1, lua_tostring(L, -1));
    lua_pop(L, 2);

    lua_rawgeti(L, -1, 2);
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_getfield(L, -1, "job_id");
    TEST_ASSERT_EQUAL_STRING(id2, lua_tostring(L, -1));
    lua_pop(L, 2);

    lua_pop(L, 1);
    H_lua_set_job_context(L, NULL);
    H_lua_destroy_context(L);
    free(id1);
    free(id2);
    scoreboard_destroy(sb);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_scoreboard_list_null_scoreboard_returns_empty_table);
    RUN_TEST(test_scoreboard_list_empty_returns_empty_table);
    RUN_TEST(test_scoreboard_list_with_entries);

    return UNITY_END();
}
