/*
 * Unity Test File: scripting_api_http_test_async.c
 *
 * Phase 17 of the LUA_PLAN. End-to-end tests of the async H.http
 * path:
 *   - H.http.get returns a handle immediately (the calling Lua
 *     thread does not block on the HTTP call)
 *   - H.wait on the handle blocks on the per-handle condvar and
 *     returns the result once the worker pool finishes
 *   - The result table has the documented shape
 *     { status, headers, body, elapsed_ms }
 *   - Multiple concurrent H.http.get calls run in parallel and
 *     all complete via H.wait
 *   - A second H.wait on the same handle returns "consumed"
 *   - The new http_client.c test seam is used; the OIDC helper is
 *     not exercised
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Standard includes
#include <string.h>
#include <stdlib.h>

// Third-party includes
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

// Module under test
#include <src/scripting/lua_context.h>
#include <src/scripting/scripting_handle.h>
#include <src/scripting/scripting_api.h>
#include <src/scripting/http_pool.h>
#include <src/scripting/http_client.h>

// Forward declarations
void test_http_async_get_returns_handle_immediately(void);
void test_http_async_wait_returns_result_table(void);
void test_http_async_concurrent_calls(void);
void test_http_async_second_wait_consumed(void);
void test_http_async_error_handle_no_pool(void);

static AppConfig mock_app_config_storage = {0};

void setUp(void) {
    memset(&mock_app_config_storage, 0, sizeof(mock_app_config_storage));
    mock_app_config_storage.scripting.DefaultHTTPTimeout = 30;
    app_config = &mock_app_config_storage;
    scripting_http_test_clear_responses();
    // The pool is not auto-initialized by H_lua_create_context; tests
    // that need the pool start it in the test body. Tests that want
    // the "pool not running" fallback leave it NULL.
}

void tearDown(void) {
    scripting_http_test_clear_responses();
    scripting_http_pool_destroy();
    app_config = NULL;
}

static void run_lua(lua_State* L, const char* code) {
    int rc = luaL_loadbuffer(L, code, strlen(code), "test");
    TEST_ASSERT_EQUAL_MESSAGE(LUA_OK, rc, "luaL_loadbuffer failed");
    rc = lua_pcall(L, 0, LUA_MULTRET, 0);
    TEST_ASSERT_EQUAL_MESSAGE(LUA_OK, rc, lua_tostring(L, -1));
}

static lua_State* make_ctx(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);
    return L;
}

// H.http.get returns a handle immediately; the calling Lua thread
// does not block on the HTTP call.
void test_http_async_get_returns_handle_immediately(void) {
    scripting_http_test_set_response("immediate.test", 200, "ok");
    TEST_ASSERT_TRUE(scripting_http_pool_init(2));

    lua_State* L = make_ctx();
    run_lua(L, "h = H.http.get('http://immediate.test/')");
    // The handle is on the stack (or stored in global h). The Lua
    // thread is NOT blocked; the HTTP call happens on a worker.
    lua_getglobal(L, "h");
    TEST_ASSERT_TRUE(lua_isuserdata(L, -1));
    H_Handle* h = H_Handle_check(L, -1);
    TEST_ASSERT_NOT_NULL(h);
    TEST_ASSERT_EQUAL(H_HK_HTTP, h->kind);
    TEST_ASSERT_FALSE(h->http_ready);  // not ready yet (worker hasn't run)
    lua_pop(L, 1);

    // Now wait; the worker should have finished by now.
    run_lua(L, "r, e = H.wait(h)");
    lua_getglobal(L, "e");
    TEST_ASSERT_TRUE(lua_isnil(L, -1));
    lua_pop(L, 1);
    lua_getglobal(L, "r");
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_getfield(L, -1, "status");
    TEST_ASSERT_EQUAL(200, (int)lua_tointeger(L, -1));
    lua_pop(L, 1);
    lua_getfield(L, -1, "body");
    TEST_ASSERT_EQUAL_STRING("ok", lua_tostring(L, -1));
    lua_pop(L, 2);

    H_lua_destroy_context(L);
}

// H.wait blocks on the condvar; the result table has the
// documented shape.
void test_http_async_wait_returns_result_table(void) {
    scripting_http_test_set_response("shape.test", 404, "missing");
    TEST_ASSERT_TRUE(scripting_http_pool_init(2));

    lua_State* L = make_ctx();
    run_lua(L,
        "h = H.http.get('http://shape.test/x')\n"
        "r, e = H.wait(h)\n"
        "_has_status  = (r.status == 404)\n"
        "_has_headers = (type(r.headers) == 'table')\n"
        "_has_body    = (r.body == 'missing')\n"
        "_has_elapsed = (type(r.elapsed_ms) == 'number')\n"
        "_no_err      = (e == nil)\n");
    lua_getglobal(L, "_has_status");  TEST_ASSERT_TRUE(lua_toboolean(L, -1)); lua_pop(L, 1);
    lua_getglobal(L, "_has_headers"); TEST_ASSERT_TRUE(lua_toboolean(L, -1)); lua_pop(L, 1);
    lua_getglobal(L, "_has_body");    TEST_ASSERT_TRUE(lua_toboolean(L, -1)); lua_pop(L, 1);
    lua_getglobal(L, "_has_elapsed"); TEST_ASSERT_TRUE(lua_toboolean(L, -1)); lua_pop(L, 1);
    lua_getglobal(L, "_no_err");      TEST_ASSERT_TRUE(lua_toboolean(L, -1)); lua_pop(L, 1);
    H_lua_destroy_context(L);
}

// Multiple concurrent H.http.get calls run in parallel. We submit
// 4 handles, then call H.wait on each. The 4 fixtures are distinct
// and the test asserts the correct status/body on each.
void test_http_async_concurrent_calls(void) {
    scripting_http_test_set_response("a.test", 200, "A");
    scripting_http_test_set_response("b.test", 201, "B");
    scripting_http_test_set_response("c.test", 202, "C");
    scripting_http_test_set_response("d.test", 203, "D");
    TEST_ASSERT_TRUE(scripting_http_pool_init(4));

    lua_State* L = make_ctx();
    run_lua(L,
        "h1 = H.http.get('http://a.test/')\n"
        "h2 = H.http.get('http://b.test/')\n"
        "h3 = H.http.get('http://c.test/')\n"
        "h4 = H.http.get('http://d.test/')\n"
        "r1, e1 = H.wait(h1)\n"
        "r2, e2 = H.wait(h2)\n"
        "r3, e3 = H.wait(h3)\n"
        "r4, e4 = H.wait(h4)\n"
        "_ok1 = (r1.status == 200 and r1.body == 'A' and e1 == nil)\n"
        "_ok2 = (r2.status == 201 and r2.body == 'B' and e2 == nil)\n"
        "_ok3 = (r3.status == 202 and r3.body == 'C' and e3 == nil)\n"
        "_ok4 = (r4.status == 203 and r4.body == 'D' and e4 == nil)\n");
    lua_getglobal(L, "_ok1"); TEST_ASSERT_TRUE(lua_toboolean(L, -1)); lua_pop(L, 1);
    lua_getglobal(L, "_ok2"); TEST_ASSERT_TRUE(lua_toboolean(L, -1)); lua_pop(L, 1);
    lua_getglobal(L, "_ok3"); TEST_ASSERT_TRUE(lua_toboolean(L, -1)); lua_pop(L, 1);
    lua_getglobal(L, "_ok4"); TEST_ASSERT_TRUE(lua_toboolean(L, -1)); lua_pop(L, 1);
    H_lua_destroy_context(L);
    TEST_ASSERT_EQUAL(4, scripting_http_test_get_consumed_count());
}

// A second H.wait on the same handle returns nil + "consumed".
void test_http_async_second_wait_consumed(void) {
    scripting_http_test_set_response("twice.test", 200, "x");
    TEST_ASSERT_TRUE(scripting_http_pool_init(2));

    lua_State* L = make_ctx();
    run_lua(L,
        "h = H.http.get('http://twice.test/')\n"
        "r1, e1 = H.wait(h)\n"
        "r2, e2 = H.wait(h)\n"
        "_consumed = (r2 == nil and e2:find('consumed') ~= nil)\n");
    lua_getglobal(L, "_consumed");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

// Pool not running: H.http.get_sync falls back to the inline
// (Phase 16) path. The test seam still intercepts the call.
void test_http_async_error_handle_no_pool(void) {
    scripting_http_test_set_response("fallback.test", 200, "fallback");
    TEST_ASSERT_NULL(scripting_http_pool);

    lua_State* L = make_ctx();
    run_lua(L,
        "r, e = H.http.get_sync('http://fallback.test/')\n"
        "_ok_status = (r and r.status == 200)\n"
        "_ok_body   = (r and r.body == 'fallback')\n"
        "_no_err    = (e == nil)\n");
    lua_getglobal(L, "_ok_status"); TEST_ASSERT_TRUE(lua_toboolean(L, -1)); lua_pop(L, 1);
    lua_getglobal(L, "_ok_body");   TEST_ASSERT_TRUE(lua_toboolean(L, -1)); lua_pop(L, 1);
    lua_getglobal(L, "_no_err");    TEST_ASSERT_TRUE(lua_toboolean(L, -1)); lua_pop(L, 1);
    H_lua_destroy_context(L);
    TEST_ASSERT_EQUAL(1, scripting_http_test_get_consumed_count());
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_http_async_get_returns_handle_immediately);
    RUN_TEST(test_http_async_wait_returns_result_table);
    RUN_TEST(test_http_async_concurrent_calls);
    RUN_TEST(test_http_async_second_wait_consumed);
    RUN_TEST(test_http_async_error_handle_no_pool);

    return UNITY_END();
}
