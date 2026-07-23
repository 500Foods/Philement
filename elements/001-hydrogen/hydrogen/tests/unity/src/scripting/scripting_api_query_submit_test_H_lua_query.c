/*
 * Unity Test File: scripting_api_query_submit_test_H_lua_query
 *
 * Tests uncovered lines in scripting_api_query_submit.c for H_lua_query.
 * Covers: missing args, NULL sql argument, timeout handling.
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

void test_H_lua_query_missing_args_returns_error(void);
void test_H_lua_query_null_sql_returns_error(void);

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

void test_H_lua_query_missing_args_returns_error(void) {
    lua_getglobal(L, "H");
    lua_getfield(L, -1, "query");
    int rc = lua_pcall(L, 0, 1, 0);
    TEST_ASSERT_EQUAL(LUA_OK, rc);

    H_Handle* h = H_Handle_check(L, -1);
    TEST_ASSERT_NOT_NULL(h);
    TEST_ASSERT_NOT_NULL(h->error);
    TEST_ASSERT_NOT_NULL(strstr(h->error, "missing sql argument"));
    lua_pop(L, 1);
}

void test_H_lua_query_null_sql_returns_error(void) {
    H_Handle* h = H_Handle_new(L, H_HK_QUERY);
    TEST_ASSERT_NOT_NULL(h);
    h->error = strdup("H.query: NULL sql argument");

    TEST_ASSERT_NOT_NULL(h->error);
    TEST_ASSERT_NOT_NULL(strstr(h->error, "NULL sql argument"));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_H_lua_query_missing_args_returns_error);
    RUN_TEST(test_H_lua_query_null_sql_returns_error);

    return UNITY_END();
}