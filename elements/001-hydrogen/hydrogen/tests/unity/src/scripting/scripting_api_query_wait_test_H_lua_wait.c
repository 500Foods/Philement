/*
 * Unity Test File: scripting_api_query_wait_test_H_lua_wait
 *
 * Tests H_lua_wait multi-handle paths: no args, invalid/consumed/error
 * handles, query await success/error/timeout, HTTP/LLM/MAIL kinds,
 * and allocation failure.
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
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

#include <src/scripting/scripting_api.h>
#include <src/scripting/scripting_api_internal.h>
#include <src/scripting/scripting_handle.h>
#include <src/scripting/lua_context.h>

extern DatabaseQueueManager* global_queue_manager;

void test_H_lua_wait_no_args_returns_nothing(void);
void test_H_lua_wait_invalid_arg_returns_error(void);
void test_H_lua_wait_consumed_handle(void);
void test_H_lua_wait_handle_with_error(void);
void test_H_lua_wait_no_pending_query(void);
void test_H_lua_wait_timeout(void);
void test_H_lua_wait_query_error_message(void);
void test_H_lua_wait_query_success(void);
void test_H_lua_wait_http_kind(void);
void test_H_lua_wait_llm_kind(void);
void test_H_lua_wait_mail_kind(void);
void test_H_lua_wait_notify_kind(void);
void test_H_lua_wait_allocation_failure(void);
void test_H_lua_wait_two_handles_mixed(void);

static lua_State* L = NULL;
static DatabaseQueue test_db_queue;

static H_Handle* push_query_handle(void) {
    H_Handle* h = H_Handle_new(L, H_HK_QUERY);
    TEST_ASSERT_NOT_NULL(h);
    return h;
}

void setUp(void) {
    mock_dbqueue_reset_all();
    mock_system_reset_all();

    L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);
    H_Handle_install_metatable(L);

    memset(&test_db_queue, 0, sizeof(test_db_queue));
    app_config = NULL;
    global_queue_manager = NULL;
}

void tearDown(void) {
    if (L) {
        lua_close(L);
        L = NULL;
    }
    app_config = NULL;
    global_queue_manager = NULL;
    mock_dbqueue_reset_all();
    mock_system_reset_all();
}

void test_H_lua_wait_no_args_returns_nothing(void) {
    int n = H_lua_wait(L);
    TEST_ASSERT_EQUAL_INT(0, n);
}

void test_H_lua_wait_invalid_arg_returns_error(void) {
    lua_pushnil(L);
    int n = H_lua_wait(L);
    TEST_ASSERT_EQUAL_INT(2, n);
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -2));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, -1));
    const char* err = lua_tostring(L, -1);
    TEST_ASSERT_NOT_NULL(strstr(err, "not a valid handle"));
    lua_pop(L, 2);
}

void test_H_lua_wait_consumed_handle(void) {
    H_Handle* h = push_query_handle();
    h->consumed = true;
    int n = H_lua_wait(L);
    TEST_ASSERT_EQUAL_INT(2, n);
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -2));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, -1));
    TEST_ASSERT_NOT_NULL(strstr(lua_tostring(L, -1), "already consumed"));
    lua_pop(L, 2);
}

void test_H_lua_wait_handle_with_error(void) {
    H_Handle* h = push_query_handle();
    h->error = strdup("pre-error");
    int n = H_lua_wait(L);
    TEST_ASSERT_EQUAL_INT(2, n);
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -2));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, -1));
    TEST_ASSERT_EQUAL_STRING("pre-error", lua_tostring(L, -1));
    TEST_ASSERT_TRUE(h->consumed);
    lua_pop(L, 2);
}

void test_H_lua_wait_no_pending_query(void) {
    H_Handle* h = push_query_handle();
    int n = H_lua_wait(L);
    TEST_ASSERT_EQUAL_INT(2, n);
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -2));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, -1));
    TEST_ASSERT_NOT_NULL(strstr(lua_tostring(L, -1), "no pending query"));
    TEST_ASSERT_TRUE(h->consumed);
    lua_pop(L, 2);
}

void test_H_lua_wait_timeout(void) {
    AppConfig config = {0};
    config.scripting.DefaultQueryTimeout = 2;
    app_config = &config;

    H_Handle* h = push_query_handle();
    h->query_id = strdup("to-id");
    h->db_queue = &test_db_queue;

    int n = H_lua_wait(L);
    TEST_ASSERT_EQUAL_INT(2, n);
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -2));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, -1));
    TEST_ASSERT_NOT_NULL(strstr(lua_tostring(L, -1), "timeout"));
    TEST_ASSERT_TRUE(h->consumed);
    lua_pop(L, 2);
}

void test_H_lua_wait_query_error_message(void) {
    AppConfig config = {0};
    config.scripting.DefaultQueryTimeout = 2;
    app_config = &config;

    DatabaseQuery q = {0};
    q.query_id = (char*)"e-id";
    q.error_message = (char*)"sql failed";
    mock_dbqueue_set_await_result(&q);

    H_Handle* h = push_query_handle();
    h->query_id = strdup("e-id");
    h->db_queue = &test_db_queue;

    int n = H_lua_wait(L);
    TEST_ASSERT_EQUAL_INT(2, n);
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -2));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, -1));
    TEST_ASSERT_EQUAL_STRING("sql failed", lua_tostring(L, -1));
    TEST_ASSERT_TRUE(h->consumed);
    lua_pop(L, 2);
}

void test_H_lua_wait_query_success(void) {
    AppConfig config = {0};
    config.scripting.DefaultQueryTimeout = 2;
    app_config = &config;

    DatabaseQuery q = {0};
    q.query_id = (char*)"ok";
    q.query_template = (char*)"[]";
    q.affected_rows = 0;
    mock_dbqueue_set_await_result(&q);

    H_Handle* h = push_query_handle();
    h->query_id = strdup("ok");
    h->db_queue = &test_db_queue;

    int n = H_lua_wait(L);
    TEST_ASSERT_EQUAL_INT(2, n);
    TEST_ASSERT_EQUAL(LUA_TTABLE, lua_type(L, -2));
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -1));
    TEST_ASSERT_TRUE(h->consumed);
    lua_pop(L, 2);
}

void test_H_lua_wait_http_kind(void) {
    H_Handle* h = H_Handle_new(L, H_HK_HTTP);
    TEST_ASSERT_NOT_NULL(h);
    int n = H_lua_wait(L);
    TEST_ASSERT_EQUAL_INT(2, n);
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -2));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, -1));
    TEST_ASSERT_TRUE(h->consumed);
    lua_pop(L, 2);
}

void test_H_lua_wait_llm_kind(void) {
    H_Handle* h = H_Handle_new(L, H_HK_LLM);
    TEST_ASSERT_NOT_NULL(h);
    int n = H_lua_wait(L);
    TEST_ASSERT_EQUAL_INT(2, n);
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -2));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, -1));
    TEST_ASSERT_TRUE(h->consumed);
    lua_pop(L, 2);
}

void test_H_lua_wait_mail_kind(void) {
    H_Handle* h = H_Handle_new(L, H_HK_MAIL);
    TEST_ASSERT_NOT_NULL(h);
    h->mail_message_id = strdup("mid-1");
    h->mail_status = strdup("queued");
    int n = H_lua_wait(L);
    TEST_ASSERT_EQUAL_INT(2, n);
    TEST_ASSERT_EQUAL(LUA_TTABLE, lua_type(L, -2));
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -1));
    TEST_ASSERT_TRUE(h->consumed);
    lua_pop(L, 2);
}

void test_H_lua_wait_notify_kind(void) {
    H_Handle* h = H_Handle_new(L, H_HK_NOTIFY);
    TEST_ASSERT_NOT_NULL(h);
    int n = H_lua_wait(L);
    TEST_ASSERT_EQUAL_INT(2, n);
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -2));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, -1));
    TEST_ASSERT_TRUE(h->consumed);
    lua_pop(L, 2);
}

void test_H_lua_wait_allocation_failure(void) {
    H_Handle* h = push_query_handle();
    (void)h;
    /* Fail first calloc (handles array) inside H_lua_wait */
    mock_system_set_malloc_failure(1);
    int n = H_lua_wait(L);
    mock_system_reset_all();
    TEST_ASSERT_EQUAL_INT(2, n);
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -2));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, -1));
    TEST_ASSERT_NOT_NULL(strstr(lua_tostring(L, -1), "allocation failure"));
    lua_pop(L, 2);
}

void test_H_lua_wait_two_handles_mixed(void) {
    AppConfig config = {0};
    config.scripting.DefaultQueryTimeout = 2;
    app_config = &config;

    DatabaseQuery q = {0};
    q.query_id = (char*)"m";
    q.query_template = (char*)"[]";
    mock_dbqueue_set_await_result(&q);

    H_Handle* h1 = push_query_handle();
    h1->query_id = strdup("m");
    h1->db_queue = &test_db_queue;

    lua_pushstring(L, "not-a-handle");

    int n = H_lua_wait(L);
    TEST_ASSERT_EQUAL_INT(4, n);
    /* results: result1, nil_for_invalid, err1_or_nil, err2 */
    TEST_ASSERT_EQUAL(LUA_TTABLE, lua_type(L, -4));
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -3));
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -2));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, -1));
    lua_pop(L, 4);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_H_lua_wait_no_args_returns_nothing);
    RUN_TEST(test_H_lua_wait_invalid_arg_returns_error);
    RUN_TEST(test_H_lua_wait_consumed_handle);
    RUN_TEST(test_H_lua_wait_handle_with_error);
    RUN_TEST(test_H_lua_wait_no_pending_query);
    RUN_TEST(test_H_lua_wait_timeout);
    RUN_TEST(test_H_lua_wait_query_error_message);
    RUN_TEST(test_H_lua_wait_query_success);
    RUN_TEST(test_H_lua_wait_http_kind);
    RUN_TEST(test_H_lua_wait_llm_kind);
    RUN_TEST(test_H_lua_wait_mail_kind);
    RUN_TEST(test_H_lua_wait_notify_kind);
    RUN_TEST(test_H_lua_wait_allocation_failure);
    RUN_TEST(test_H_lua_wait_two_handles_mixed);

    return UNITY_END();
}
