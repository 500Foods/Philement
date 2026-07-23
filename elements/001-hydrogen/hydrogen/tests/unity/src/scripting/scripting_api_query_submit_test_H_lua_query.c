/*
 * Unity Test File: scripting_api_query_submit_test_H_lua_query
 *
 * Tests H_lua_query: missing args, params, timeout opts, success, pushed==0.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <string.h>
#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define USE_MOCK_DBQUEUE
#include <unity/mocks/mock_dbqueue.h>
#define USE_MOCK_GENERATE_QUERY_ID
#include <unity/mocks/mock_generate_query_id.h>
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

#include <src/scripting/scripting_api.h>
#include <src/scripting/scripting_api_internal.h>
#include <src/scripting/scripting_handle.h>
#include <src/scripting/lua_context.h>

extern DatabaseQueueManager* global_queue_manager;

void test_H_lua_query_missing_args_returns_error(void);
void test_H_lua_query_with_params_and_timeout(void);
void test_H_lua_query_timeout_non_positive_ignored(void);
void test_H_lua_query_nil_params_ok(void);
void test_H_lua_query_success(void);

static lua_State* L = NULL;
static DatabaseQueue test_db_queue;
static DatabaseQueueManager test_dqm;
static AppConfig test_config;

static void setup_ok(void) {
    memset(&test_config, 0, sizeof(test_config));
    test_config.scripting.DefaultDatabase = strdup("testdb");
    test_config.scripting.DefaultQueryTimeout = 10;
    app_config = &test_config;
    memset(&test_dqm, 0, sizeof(test_dqm));
    global_queue_manager = &test_dqm;
    mock_dbqueue_set_get_database_result(&test_db_queue);
    mock_dbqueue_set_submit_query_result(true);
    mock_generate_query_id_set_result("q-1");
}

void setUp(void) {
    mock_dbqueue_reset_all();
    mock_generate_query_id_reset();
    mock_system_reset_all();

    L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);
    H_Handle_install_metatable(L);

    lua_newtable(L);
    lua_setglobal(L, "H");

    memset(&test_db_queue, 0, sizeof(test_db_queue));
    memset(&test_config, 0, sizeof(test_config));
    app_config = NULL;
    global_queue_manager = NULL;
}

void tearDown(void) {
    if (L) {
        lua_close(L);
        L = NULL;
    }
    free(test_config.scripting.DefaultDatabase);
    test_config.scripting.DefaultDatabase = NULL;
    app_config = NULL;
    global_queue_manager = NULL;
    mock_dbqueue_reset_all();
    mock_generate_query_id_reset();
    mock_system_reset_all();
}

void test_H_lua_query_missing_args_returns_error(void) {
    int n = H_lua_query(L);
    TEST_ASSERT_EQUAL_INT(1, n);
    H_Handle* h = H_Handle_check(L, -1);
    TEST_ASSERT_NOT_NULL(h);
    TEST_ASSERT_NOT_NULL(strstr(h->error, "missing sql argument"));
}

void test_H_lua_query_with_params_and_timeout(void) {
    setup_ok();

    lua_pushstring(L, "SELECT $1");
    lua_newtable(L);
    lua_pushinteger(L, 42);
    lua_setfield(L, -2, "id");
    lua_newtable(L);
    lua_pushinteger(L, 7);
    lua_setfield(L, -2, "timeout");

    int n = H_lua_query(L);
    TEST_ASSERT_EQUAL_INT(1, n);
    H_Handle* h = H_Handle_check(L, -1);
    TEST_ASSERT_NOT_NULL(h);
    TEST_ASSERT_NULL(h->error);
    TEST_ASSERT_NOT_NULL(h->query_id);
}

void test_H_lua_query_timeout_non_positive_ignored(void) {
    setup_ok();

    lua_pushstring(L, "SELECT 1");
    lua_pushnil(L);
    lua_newtable(L);
    lua_pushinteger(L, 0);
    lua_setfield(L, -2, "timeout");

    int n = H_lua_query(L);
    TEST_ASSERT_EQUAL_INT(1, n);
    H_Handle* h = H_Handle_check(L, -1);
    TEST_ASSERT_NOT_NULL(h);
    TEST_ASSERT_NULL(h->error);
}

void test_H_lua_query_nil_params_ok(void) {
    setup_ok();

    lua_pushstring(L, "SELECT 1");
    lua_pushnil(L);

    int n = H_lua_query(L);
    TEST_ASSERT_EQUAL_INT(1, n);
    H_Handle* h = H_Handle_check(L, -1);
    TEST_ASSERT_NOT_NULL(h);
    TEST_ASSERT_NULL(h->error);
}

void test_H_lua_query_success(void) {
    setup_ok();
    lua_pushstring(L, "SELECT 1");
    int n = H_lua_query(L);
    TEST_ASSERT_EQUAL_INT(1, n);
    H_Handle* h = H_Handle_check(L, -1);
    TEST_ASSERT_NOT_NULL(h);
    TEST_ASSERT_EQUAL_STRING("q-1", h->query_id);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_H_lua_query_missing_args_returns_error);
    RUN_TEST(test_H_lua_query_with_params_and_timeout);
    RUN_TEST(test_H_lua_query_timeout_non_positive_ignored);
    RUN_TEST(test_H_lua_query_nil_params_ok);
    RUN_TEST(test_H_lua_query_success);

    return UNITY_END();
}
