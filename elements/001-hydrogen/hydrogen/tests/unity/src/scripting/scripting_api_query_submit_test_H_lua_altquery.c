/*
 * Unity Test File: scripting_api_query_submit_test_H_lua_altquery
 *
 * Tests uncovered lines in scripting_api_query_submit.c for H_lua_altquery.
 * Covers: missing args, empty database name.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <string.h>
#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <unity/mocks/mock_dbqueue.h>
#include <unity/mocks/mock_generate_query_id.h>

#include <src/scripting/scripting_api.h>
#include <src/scripting/scripting_api_internal.h>
#include <src/scripting/scripting_handle.h>
#include <src/scripting/lua_context.h>

extern DatabaseQueueManager* global_queue_manager;

void test_H_lua_altquery_missing_args_returns_error(void);
void test_H_lua_altquery_empty_db_name_returns_error(void);

static lua_State* L = NULL;

void setUp(void) {
    mock_dbqueue_reset_all();
    mock_generate_query_id_reset();

    L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);
    H_Handle_install_metatable(L);

    lua_newtable(L);
    lua_setglobal(L, "H");
    H_lua_install_query(L);
}

void tearDown(void) {
    if (L) {
        lua_close(L);
        L = NULL;
    }
    app_config = NULL;
    global_queue_manager = NULL;
}

void test_H_lua_altquery_missing_args_returns_error(void) {
    lua_getglobal(L, "H");
    lua_getfield(L, -1, "altquery");
    int rc = lua_pcall(L, 0, 1, 0);
    TEST_ASSERT_EQUAL(LUA_OK, rc);

    H_Handle* h = H_Handle_check(L, -1);
    TEST_ASSERT_NOT_NULL(h);
    TEST_ASSERT_NOT_NULL(h->error);
    TEST_ASSERT_NOT_NULL(strstr(h->error, "missing database_name or sql argument"));
    lua_pop(L, 1);
}

void test_H_lua_altquery_empty_db_name_returns_error(void) {
    lua_getglobal(L, "H");
    lua_getfield(L, -1, "altquery");
    lua_pushstring(L, "");
    lua_pushstring(L, "SELECT 1");
    int rc = lua_pcall(L, 2, 1, 0);
    TEST_ASSERT_EQUAL(LUA_OK, rc);

    H_Handle* h = H_Handle_check(L, -1);
    TEST_ASSERT_NOT_NULL(h);
    TEST_ASSERT_NOT_NULL(h->error);
    TEST_ASSERT_NOT_NULL(strstr(h->error, "database_name is empty"));
    lua_pop(L, 1);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_H_lua_altquery_missing_args_returns_error);
    RUN_TEST(test_H_lua_altquery_empty_db_name_returns_error);

    return UNITY_END();
}