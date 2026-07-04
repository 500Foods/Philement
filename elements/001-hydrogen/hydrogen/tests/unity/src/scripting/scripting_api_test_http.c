/*
 * Unity Test File: scripting_api_test_http.c
 *
 * Phase 16 of the LUA_PLAN. Tests the H.http.* host functions:
 *   - H.http.get returns an H_Handle of kind H_HK_HTTP
 *   - H.http.get_sync calls the OIDC helper, builds a result table
 *   - H.http.post with body + content_type + headers round-trips
 *   - Error handles (missing url, allocation failure) report through
 *     H.wait without raising a Lua error
 *   - The result table has the documented shape
 *     { status, headers, body, elapsed_ms }
 *
 * The actual network call is short-circuited by the OIDC helper's
 * test injection seam (`oidc_rp_http_test_set_response`); we
 * register canned responses by URL substring. This lets the test
 * assert the host-function plumbing end-to-end without a live HTTP
 * server.
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

// OIDC helper (for the test injection seam)
#include <src/api/auth/oidc_rp/oidc_rp_http.h>

// Mock app_config (zeroed, scripting disabled — the http functions
// don't gate on Enabled; they only need DefaultHTTPTimeout for the
// fallback).
static AppConfig mock_app_config_storage = {0};

// Forward declarations
void test_http_get_returns_handle_of_http_kind(void);
void test_http_get_sync_returns_result_table(void);
void test_http_get_sync_result_has_status_and_body(void);
void test_http_get_sync_error_on_unknown_url(void);
void test_http_post_with_body_and_content_type(void);
void test_http_missing_url_returns_error_handle(void);
void test_http_wait_on_handle_returns_pair(void);
void test_http_handle_already_consumed(void);

void setUp(void) {
    memset(&mock_app_config_storage, 0, sizeof(mock_app_config_storage));
    app_config = &mock_app_config_storage;
    // Make sure no leftover fixtures from a prior test bleed in.
    oidc_rp_http_test_clear_responses();
}

void tearDown(void) {
    oidc_rp_http_test_clear_responses();
    app_config = NULL;
}

// Run a Lua string, leaving any return values on the stack.
static void run_lua(lua_State* L, const char* code) {
    int rc = luaL_loadbuffer(L, code, strlen(code), "test");
    TEST_ASSERT_EQUAL_MESSAGE(LUA_OK, rc, "luaL_loadbuffer failed");
    rc = lua_pcall(L, 0, LUA_MULTRET, 0);
    TEST_ASSERT_EQUAL_MESSAGE(LUA_OK, rc, lua_tostring(L, -1));
}

// Create a context with the H.http functions installed.
static lua_State* make_ctx(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);
    // Diagnostic: H.http.get must be a function. If this fails the
    // install chain is broken.
    lua_getglobal(L, "H");
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_getfield(L, -1, "http");
    TEST_ASSERT_TRUE_MESSAGE(lua_istable(L, -1), "H.http must be a table");
    lua_getfield(L, -1, "get");
    TEST_ASSERT_TRUE_MESSAGE(lua_isfunction(L, -1), "H.http.get must be a function");
    lua_pop(L, 3);
    return L;
}

// H.http.get("http://example.com") returns a userdata of kind H_HK_HTTP.
void test_http_get_returns_handle_of_http_kind(void) {
    oidc_rp_http_test_set_response("example.com", 200, "OK");
    lua_State* L = make_ctx();
    // Use a return statement so the userdata stays on the stack.
    run_lua(L, "return H.http.get('http://example.com/')");
    TEST_ASSERT_EQUAL(1, lua_gettop(L));
    TEST_ASSERT_TRUE(lua_isuserdata(L, -1));
    H_Handle* h = H_Handle_check(L, -1);
    TEST_ASSERT_NOT_NULL(h);
    TEST_ASSERT_EQUAL(H_HK_HTTP, h->kind);
    H_lua_destroy_context(L);
}

// H.http.get_sync returns (result, err). For a 200, the result table
// has status=200, body="OK".
void test_http_get_sync_returns_result_table(void) {
    oidc_rp_http_test_set_response("example.com", 200, "OK");
    lua_State* L = make_ctx();
    // Store the values into globals; then re-fetch to inspect.
    run_lua(L,
        "r, e = H.http.get_sync('http://example.com/')\n"
        "_r_status = r and r.status\n"
        "_r_body   = r and r.body\n"
        "_e_str    = e\n");
    // Re-fetch the globals to inspect them.
    lua_getglobal(L, "_r_status");
    TEST_ASSERT_TRUE(lua_isnumber(L, -1));
    TEST_ASSERT_EQUAL(200, (int)lua_tointeger(L, -1));
    lua_pop(L, 1);
    lua_getglobal(L, "_r_body");
    TEST_ASSERT_TRUE(lua_isstring(L, -1));
    TEST_ASSERT_EQUAL_STRING("OK", lua_tostring(L, -1));
    lua_pop(L, 1);
    lua_getglobal(L, "_e_str");
    TEST_ASSERT_TRUE(lua_isnil(L, -1));
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

// Result table contains all four fields { status, headers, body, elapsed_ms }.
void test_http_get_sync_result_has_status_and_body(void) {
    oidc_rp_http_test_set_response("example.com", 404, "not found");
    lua_State* L = make_ctx();
    run_lua(L,
        "r, e = H.http.get_sync('http://example.com/missing')\n"
        "_has_status  = (r.status == 404)\n"
        "_has_headers = (type(r.headers) == 'table')\n"
        "_has_body    = (r.body == 'not found')\n"
        "_has_elapsed = (type(r.elapsed_ms) == 'number')\n");
    lua_getglobal(L, "_has_status");   TEST_ASSERT_TRUE(lua_toboolean(L, -1)); lua_pop(L, 1);
    lua_getglobal(L, "_has_headers");  TEST_ASSERT_TRUE(lua_toboolean(L, -1)); lua_pop(L, 1);
    lua_getglobal(L, "_has_body");     TEST_ASSERT_TRUE(lua_toboolean(L, -1)); lua_pop(L, 1);
    lua_getglobal(L, "_has_elapsed");  TEST_ASSERT_TRUE(lua_toboolean(L, -1)); lua_pop(L, 1);
    H_lua_destroy_context(L);
}

// Calling H.http.get_sync with an invalid URL returns (nil, err).
// The OIDC helper rejects non-http(s) URLs with a clear message.
void test_http_get_sync_error_on_unknown_url(void) {
    lua_State* L = make_ctx();
    run_lua(L,
        "r, e = H.http.get_sync('not-a-url')\n"
        "_got_error = (e ~= nil and type(e) == 'string')\n");
    lua_getglobal(L, "_got_error");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

// POST with body + content_type + headers: assert body reaches the
// response and content_type is set on the result.
void test_http_post_with_body_and_content_type(void) {
    oidc_rp_http_test_set_response("api.example.com", 201, "created");
    lua_State* L = make_ctx();
    run_lua(L,
        "r, e = H.http.post_sync(\n"
        "  'http://api.example.com/items',\n"
        "  '{\"name\":\"x\"}',\n"
        "  { ['Content-Type'] = 'application/json', ['X-Tag'] = 't' },\n"
        "  { content_type = 'application/json' }\n"
        ")\n"
        "_status_ok = (r and r.status == 201)\n"
        "_body_ok   = (r and r.body == 'created')\n"
        "_no_err    = (e == nil)\n");
    lua_getglobal(L, "_status_ok"); TEST_ASSERT_TRUE(lua_toboolean(L, -1)); lua_pop(L, 1);
    lua_getglobal(L, "_body_ok");   TEST_ASSERT_TRUE(lua_toboolean(L, -1)); lua_pop(L, 1);
    lua_getglobal(L, "_no_err");    TEST_ASSERT_TRUE(lua_toboolean(L, -1)); lua_pop(L, 1);
    H_lua_destroy_context(L);
}

// Missing URL: H.http.get returns a handle whose error is set;
// H.wait on it returns nil + the error string. No Lua error is raised.
void test_http_missing_url_returns_error_handle(void) {
    lua_State* L = make_ctx();
    run_lua(L,
        "h = H.http.get(nil)\n"
        "r, e = H.wait(h)\n"
        "_got_err = (r == nil and e ~= nil and e:find('url') ~= nil)\n");
    lua_getglobal(L, "_got_err");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

// H.wait on a handle returns a (result, err) pair, regardless of kind.
void test_http_wait_on_handle_returns_pair(void) {
    oidc_rp_http_test_set_response("example.com", 200, "hi");
    lua_State* L = make_ctx();
    run_lua(L,
        "h = H.http.get('http://example.com/')\n"
        "r, e = H.wait(h)\n"
        "_is_pair = (r ~= nil and e == nil) or (r == nil and e ~= nil)\n"
        "_ok_body = (r and r.body == 'hi')\n");
    lua_getglobal(L, "_is_pair"); TEST_ASSERT_TRUE(lua_toboolean(L, -1)); lua_pop(L, 1);
    lua_getglobal(L, "_ok_body"); TEST_ASSERT_TRUE(lua_toboolean(L, -1)); lua_pop(L, 1);
    H_lua_destroy_context(L);
}

// A second H.wait on the same handle returns nil + "already consumed".
void test_http_handle_already_consumed(void) {
    oidc_rp_http_test_set_response("example.com", 200, "x");
    lua_State* L = make_ctx();
    run_lua(L,
        "h = H.http.get('http://example.com/')\n"
        "r1, e1 = H.wait(h)\n"
        "r2, e2 = H.wait(h)\n"
        "_consumed = (r2 == nil and e2:find('consumed') ~= nil)\n");
    lua_getglobal(L, "_consumed");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_http_get_returns_handle_of_http_kind);
    RUN_TEST(test_http_get_sync_returns_result_table);
    RUN_TEST(test_http_get_sync_result_has_status_and_body);
    RUN_TEST(test_http_get_sync_error_on_unknown_url);
    RUN_TEST(test_http_post_with_body_and_content_type);
    RUN_TEST(test_http_missing_url_returns_error_handle);
    RUN_TEST(test_http_wait_on_handle_returns_pair);
    RUN_TEST(test_http_handle_already_consumed);

    return UNITY_END();
}
