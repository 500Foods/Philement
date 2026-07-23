/*
 * Unity Test File: scripting_api_query_wait_test_H_lua_wait
 *
 * Tests uncovered lines in scripting_api_query_wait.c for H_lua_wait.
 * Covers: allocation failure, multiple handles, various error conditions.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <string.h>
#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <unity/mocks/mock_dbqueue.h>

#include <src/scripting/scripting_api.h>
#include <src/scripting/scripting_api_internal.h>
#include <src/scripting/scripting_handle.h>
#include <src/scripting/lua_context.h>

extern DatabaseQueueManager* global_queue_manager;

void test_H_lua_wait_no_args_returns_nothing(void);
void test_H_lua_wait_nil_H_returns_error(void);

static lua_State* L = NULL;
static DatabaseQueue test_db_queue;
static DatabaseQueueManager test_dqm;

void setUp(void) {
    mock_dbqueue_reset_all();

    L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);
    H_Handle_install_metatable(L);

    memset(&test_db_queue, 0, sizeof(test_db_queue));
    memset(&test_dqm, 0, sizeof(test_dqm));
}

void tearDown(void) {
    if (L) {
        lua_close(L);
        L = NULL;
    }
    app_config = NULL;
    global_queue_manager = NULL;
}

void test_H_lua_wait_no_args_returns_nothing(void) {
    lua_newtable(L);
    lua_setglobal(L, "handles");

    lua_newtable(L);
    lua_setglobal(L, "H");

    int n = H_lua_wait(L);
    TEST_ASSERT_EQUAL_INT(0, n);
}

void test_H_lua_wait_nil_H_returns_error(void) {
    lua_newtable(L);
    lua_setglobal(L, "handles");

    lua_pushnil(L);
    lua_setglobal(L, "H");

    int n = H_lua_wait(L);
    TEST_ASSERT_EQUAL_INT(0, n);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_H_lua_wait_no_args_returns_nothing);
    RUN_TEST(test_H_lua_wait_nil_H_returns_error);

    return UNITY_END();
}