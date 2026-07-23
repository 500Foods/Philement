/*
 * Unity Test File: scripting_api_scoreboard_test_H_lua_scoreboard_submit.c
 *
 * Tests H_lua_scoreboard_submit:
 *   - Non-table argument: error, nil
 *   - Table without script_name: error, nil
 *   - Table with non-string script_name: error, nil
 *   - Table with valid script_name (no worker pool): nil
 *   - Table with valid script_name + source (no worker pool): nil
 *   - Table with valid script_name + params_json (no worker pool): nil
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

void test_scoreboard_submit_non_table_returns_nil(void);
void test_scoreboard_submit_missing_script_name_returns_nil(void);
void test_scoreboard_submit_non_string_script_name_returns_nil(void);
void test_scoreboard_submit_valid_no_source_returns_nil(void);
void test_scoreboard_submit_valid_with_source_returns_nil(void);
void test_scoreboard_submit_valid_with_params_returns_nil(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_scoreboard_submit_non_table_returns_nil(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    lua_pushinteger(L, 42);

    int rc = H_lua_scoreboard_submit(L);
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT_TRUE(lua_isnil(L, -1));

    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_scoreboard_submit_missing_script_name_returns_nil(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    lua_newtable(L);

    int rc = H_lua_scoreboard_submit(L);
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT_TRUE(lua_isnil(L, -1));

    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_scoreboard_submit_non_string_script_name_returns_nil(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    lua_newtable(L);
    lua_pushinteger(L, 99);
    lua_setfield(L, -2, "script_name");

    int rc = H_lua_scoreboard_submit(L);
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT_TRUE(lua_isnil(L, -1));

    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_scoreboard_submit_valid_no_source_returns_nil(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    lua_newtable(L);
    lua_pushstring(L, "my_script");
    lua_setfield(L, -2, "script_name");

    int rc = H_lua_scoreboard_submit(L);
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT_TRUE(lua_isnil(L, -1));

    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_scoreboard_submit_valid_with_source_returns_nil(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    lua_newtable(L);
    lua_pushstring(L, "my_script");
    lua_setfield(L, -2, "script_name");
    lua_pushstring(L, "return 42");
    lua_setfield(L, -2, "source");

    int rc = H_lua_scoreboard_submit(L);
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT_TRUE(lua_isnil(L, -1));

    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_scoreboard_submit_valid_with_params_returns_nil(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    lua_newtable(L);
    lua_pushstring(L, "my_script");
    lua_setfield(L, -2, "script_name");
    lua_pushstring(L, "{\"param\":\"value\"}");
    lua_setfield(L, -2, "params_json");

    int rc = H_lua_scoreboard_submit(L);
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT_TRUE(lua_isnil(L, -1));

    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_scoreboard_submit_non_table_returns_nil);
    RUN_TEST(test_scoreboard_submit_missing_script_name_returns_nil);
    RUN_TEST(test_scoreboard_submit_non_string_script_name_returns_nil);
    RUN_TEST(test_scoreboard_submit_valid_no_source_returns_nil);
    RUN_TEST(test_scoreboard_submit_valid_with_source_returns_nil);
    RUN_TEST(test_scoreboard_submit_valid_with_params_returns_nil);

    return UNITY_END();
}
