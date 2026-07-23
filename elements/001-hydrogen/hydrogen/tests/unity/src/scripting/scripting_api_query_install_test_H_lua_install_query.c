/*
 * Unity Test File: scripting_api_query_install_test_H_lua_install_query
 *
 * Tests uncovered lines in scripting_api_query_install.c for H_lua_install_query.
 * Covers: missing H table, NULL L, successful installation.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <string.h>
#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <src/scripting/scripting_api.h>
#include <src/scripting/scripting_api_internal.h>
#include <src/scripting/scripting_handle.h>
#include <src/scripting/lua_context.h>

void test_H_lua_install_query_missing_h_table(void);
void test_H_lua_install_query_null_L(void);
void test_H_lua_install_query_installs_all_functions(void);

static lua_State* L = NULL;

void setUp(void) {
    L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);
    H_Handle_install_metatable(L);

    lua_newtable(L);
    lua_setglobal(L, "H");
}

void tearDown(void) {
    if (L) {
        lua_close(L);
        L = NULL;
    }
}

void test_H_lua_install_query_missing_h_table(void) {
    lua_pushnil(L);
    lua_setglobal(L, "H");

    H_lua_install_query(L);

    TEST_ASSERT_EQUAL_INT(0, lua_gettop(L));
}

void test_H_lua_install_query_null_L(void) {
    H_lua_install_query(NULL);
}

void test_H_lua_install_query_installs_all_functions(void) {
    H_lua_install_query(L);

    lua_getglobal(L, "H");

    lua_getfield(L, -1, "query");
    TEST_ASSERT_EQUAL(LUA_TFUNCTION, lua_type(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, -1, "altquery");
    TEST_ASSERT_EQUAL(LUA_TFUNCTION, lua_type(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, -1, "authquery");
    TEST_ASSERT_EQUAL(LUA_TFUNCTION, lua_type(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, -1, "wait");
    TEST_ASSERT_EQUAL(LUA_TFUNCTION, lua_type(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, -1, "query_sync");
    TEST_ASSERT_EQUAL(LUA_TFUNCTION, lua_type(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, -1, "altquery_sync");
    TEST_ASSERT_EQUAL(LUA_TFUNCTION, lua_type(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, -1, "authquery_sync");
    TEST_ASSERT_EQUAL(LUA_TFUNCTION, lua_type(L, -1));
    lua_pop(L, 1);

    lua_pop(L, 1);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_H_lua_install_query_missing_h_table);
    RUN_TEST(test_H_lua_install_query_null_L);
    RUN_TEST(test_H_lua_install_query_installs_all_functions);

    return UNITY_END();
}