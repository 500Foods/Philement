/*
 * Unity Test File: scripting_api_query_wait_test_H_lua_wait_one
 *
 * Tests uncovered lines in scripting_api_query_wait.c for H_lua_wait_one.
 * Covers: null handle, consumed handle, handle with error, no query_id/db_queue,
 * timeout, result with error_message.
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

void test_H_lua_wait_one_null_handle_returns_error(void);
void test_H_lua_wait_one_consumed_handle_returns_error(void);
void test_H_lua_wait_one_handle_with_error_returns_error(void);
void test_H_lua_wait_one_handle_no_query_returns_error(void);
void test_H_lua_wait_one_timeout_returns_error(void);
void test_H_lua_wait_one_with_error_message_returns_error(void);

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

void test_H_lua_wait_one_null_handle_returns_error(void) {
    int n = H_lua_wait_one(L, NULL);
    TEST_ASSERT_EQUAL_INT(2, n);

    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -2));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, -1));
    const char* err = lua_tostring(L, -1);
    TEST_ASSERT_NOT_NULL(strstr(err, "invalid handle"));
    lua_pop(L, 2);
}

void test_H_lua_wait_one_consumed_handle_returns_error(void) {
    H_Handle* h = H_Handle_new(L, H_HK_QUERY);
    TEST_ASSERT_NOT_NULL(h);
    h->consumed = true;
    lua_pushlightuserdata(L, h);
    lua_pushstring(L, "H.Handle");
    lua_setmetatable(L, -2);

    int n = H_lua_wait_one(L, h);
    TEST_ASSERT_EQUAL_INT(2, n);

    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -2));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, -1));
    const char* err = lua_tostring(L, -1);
    TEST_ASSERT_NOT_NULL(strstr(err, "already consumed"));
    lua_pop(L, 2);
}

void test_H_lua_wait_one_handle_with_error_returns_error(void) {
    H_Handle* h = H_Handle_new(L, H_HK_QUERY);
    TEST_ASSERT_NOT_NULL(h);
    h->error = strdup("Test error message");
    lua_pushlightuserdata(L, h);
    lua_pushstring(L, "H.Handle");
    lua_setmetatable(L, -2);

    int n = H_lua_wait_one(L, h);
    TEST_ASSERT_EQUAL_INT(2, n);

    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -2));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, -1));
    const char* err = lua_tostring(L, -1);
    TEST_ASSERT_NOT_NULL(err);
    TEST_ASSERT_NOT_NULL(strstr(err, "Test error message"));
    lua_pop(L, 2);
}

void test_H_lua_wait_one_handle_no_query_returns_error(void) {
    H_Handle* h = H_Handle_new(L, H_HK_QUERY);
    TEST_ASSERT_NOT_NULL(h);
    h->query_id = NULL;
    h->db_queue = NULL;
    lua_pushlightuserdata(L, h);
    lua_pushstring(L, "H.Handle");
    lua_setmetatable(L, -2);

    int n = H_lua_wait_one(L, h);
    TEST_ASSERT_EQUAL_INT(2, n);

    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -2));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, -1));
    const char* err = lua_tostring(L, -1);
    TEST_ASSERT_NOT_NULL(strstr(err, "no pending query"));
    lua_pop(L, 2);
}

void test_H_lua_wait_one_timeout_returns_error(void) {
    AppConfig config = {0};
    config.scripting.DefaultQueryTimeout = 1;

    app_config = &config;
    global_queue_manager = &test_dqm;

    mock_dbqueue_set_get_database_result(&test_db_queue);

    DatabaseQuery* q = calloc(1, sizeof(DatabaseQuery));
    TEST_ASSERT_NOT_NULL(q);
    q->query_id = strdup("test-query-id");
    TEST_ASSERT_NOT_NULL(q->query_id);
    mock_dbqueue_set_await_result(q);

    H_Handle* h = H_Handle_new(L, H_HK_QUERY);
    TEST_ASSERT_NOT_NULL(h);
    h->query_id = strdup("test-query-id");
    h->db_queue = &test_db_queue;

    int n = H_lua_wait_one(L, h);
    TEST_ASSERT_EQUAL_INT(2, n);

    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -2));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, -1));
    lua_pop(L, 2);

    free(q->query_id);
    free(q);
}

void test_H_lua_wait_one_with_error_message_returns_error(void) {
    AppConfig config = {0};
    config.scripting.DefaultQueryTimeout = 1;

    app_config = &config;
    global_queue_manager = &test_dqm;

    mock_dbqueue_set_get_database_result(&test_db_queue);

    DatabaseQuery* q = calloc(1, sizeof(DatabaseQuery));
    TEST_ASSERT_NOT_NULL(q);
    q->query_id = strdup("test-query-id");
    q->query_template = strdup("SELECT 1");
    q->error_message = strdup("Database error occurred");
    mock_dbqueue_set_await_result(q);

    H_Handle* h = H_Handle_new(L, H_HK_QUERY);
    TEST_ASSERT_NOT_NULL(h);
    h->query_id = strdup("test-query-id");
    h->db_queue = &test_db_queue;

    int n = H_lua_wait_one(L, h);
    TEST_ASSERT_EQUAL_INT(2, n);

    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -2));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, -1));
    const char* err = lua_tostring(L, -1);
    TEST_ASSERT_NOT_NULL(err);
    lua_pop(L, 2);

    free(q->query_id);
    free(q->query_template);
    free(q->error_message);
    free(q);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_H_lua_wait_one_null_handle_returns_error);
    RUN_TEST(test_H_lua_wait_one_consumed_handle_returns_error);
    RUN_TEST(test_H_lua_wait_one_handle_with_error_returns_error);
    RUN_TEST(test_H_lua_wait_one_handle_no_query_returns_error);
    RUN_TEST(test_H_lua_wait_one_timeout_returns_error);
    RUN_TEST(test_H_lua_wait_one_with_error_message_returns_error);

    return UNITY_END();
}