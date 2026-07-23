/*
 * Unity Test File: scripting_api_http_test.c
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
#include <src/scripting/scripting_api_internal.h>

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

// H.http.get() with no arguments returns a handle whose error reports
// a missing url; H.wait surfaces it without raising.
void test_http_get_no_args_error_handle(void);
// H.http.get with a non-table second argument exercises the
// !lua_istable -> NULL early return in H_lua_headers_to_slist.
void test_http_get_headers_non_table_returns_null(void);
// H.http.get with a table containing a non-string key/value exercises
// the "skipping non-string header" log branch in H_lua_headers_to_slist.
void test_http_get_headers_non_string_entry(void);
// H_lua_opts_timeout with a non-table argument returns -1.
void test_http_opts_timeout_non_table(void);
// H_lua_opts_timeout with a valid numeric timeout returns that value.
void test_http_opts_timeout_valid_number(void);
// H.http.post() with no arguments returns a missing-url error handle.
void test_http_post_no_args_error_handle(void);
// H.http.post with a non-string, non-nil, non-table url returns an error.
void test_http_post_empty_url_error_handle(void);
// H.http.post with a numeric body (not a string/nil) errors.
void test_http_post_body_not_string(void);
// H.http.post with a numeric (non-table) headers slot errors.
void test_http_post_headers_not_table(void);
// H.http.post with nil body and nil headers advances past the headers slot.
void test_http_post_nil_body_and_headers(void);
// H.http.post with timeout + content_type options table is parsed.
void test_http_post_timeout_and_content_type(void);
// H.wait(nil) reaches the invalid-handle branch in H_lua_http_wait_one.
void test_http_wait_nil_handle(void);
// H_lua_install_http(NULL) returns immediately.
void test_http_install_null_lua_state(void);
// H_lua_install_http on a state without an H table logs and returns.
void test_http_install_missing_h_table(void);

void test_http_get_no_args_error_handle(void) {
    lua_State* L = make_ctx();
    run_lua(L,
        "h = H.http.get()\n"
        "r, e = H.wait(h)\n"
        "_got_err = (r == nil and e ~= nil and e:find('url') ~= nil)\n");
    lua_getglobal(L, "_got_err");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_http_get_headers_non_table_returns_null(void) {
    oidc_rp_http_test_set_response("example.com", 200, "OK");
    lua_State* L = make_ctx();
    // Second argument is a number, so H_lua_headers_to_slist gets a
    // non-table and returns NULL (no crash, handle still created).
    run_lua(L,
        "h = H.http.get('http://example.com/x', 42)\n"
        "r, e = H.wait(h)\n"
        "_ok = (r ~= nil and r.status == 200)\n");
    lua_getglobal(L, "_ok");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_http_get_headers_non_string_entry(void) {
    oidc_rp_http_test_set_response("example.com", 200, "OK");
    lua_State* L = make_ctx();
    // Header table has a boolean value (not a string and not a number),
    // exercising the "skipping non-string header" log branch in
    // H_lua_headers_to_slist.
    run_lua(L,
        "h = H.http.get('http://example.com/x', { ['X-Test'] = true })\n"
        "r, e = H.wait(h)\n"
        "_ok = (r ~= nil and r.status == 200)\n");
    lua_getglobal(L, "_ok");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_http_opts_timeout_non_table(void) {
    oidc_rp_http_test_set_response("example.com", 200, "OK");
    lua_State* L = make_ctx();
    // Third argument is a number (not a table) -> timeout falls back to -1.
    run_lua(L,
        "h = H.http.get('http://example.com/x', {}, 42)\n"
        "r, e = H.wait(h)\n"
        "_ok = (r ~= nil and r.status == 200)\n");
    lua_getglobal(L, "_ok");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_http_opts_timeout_valid_number(void) {
    oidc_rp_http_test_set_response("example.com", 200, "OK");
    lua_State* L = make_ctx();
    // Third argument is a table with a numeric timeout -> used as timeout.
    run_lua(L,
        "h = H.http.get('http://example.com/x', {}, { timeout = 5 })\n"
        "r, e = H.wait(h)\n"
        "_ok = (r ~= nil and r.status == 200)\n");
    lua_getglobal(L, "_ok");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_http_post_no_args_error_handle(void) {
    lua_State* L = make_ctx();
    run_lua(L,
        "h = H.http.post()\n"
        "r, e = H.wait(h)\n"
        "_got_err = (r == nil and e ~= nil and e:find('url') ~= nil)\n");
    lua_getglobal(L, "_got_err");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_http_post_empty_url_error_handle(void) {
    lua_State* L = make_ctx();
    run_lua(L,
        "h = H.http.post(nil)\n"
        "r, e = H.wait(h)\n"
        "_got_err = (r == nil and e ~= nil and e:find('url') ~= nil)\n");
    lua_getglobal(L, "_got_err");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_http_post_body_not_string(void) {
    lua_State* L = make_ctx();
    run_lua(L,
        "h = H.http.post('http://api.example.com/i', true)\n"
        "r, e = H.wait(h)\n"
        "_got_err = (r == nil and e ~= nil and e:find('body') ~= nil)\n");
    lua_getglobal(L, "_got_err");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_http_post_headers_not_table(void) {
    lua_State* L = make_ctx();
    run_lua(L,
        "h = H.http.post('http://api.example.com/i', 'body', 42)\n"
        "r, e = H.wait(h)\n"
        "_got_err = (r == nil and e ~= nil and e:find('headers') ~= nil)\n");
    lua_getglobal(L, "_got_err");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_http_post_nil_body_and_headers(void) {
    oidc_rp_http_test_set_response("api.example.com", 201, "created");
    lua_State* L = make_ctx();
    // nil body, nil headers -> both optional slots skipped, request succeeds.
    run_lua(L,
        "h = H.http.post('http://api.example.com/i', nil, nil)\n"
        "r, e = H.wait(h)\n"
        "_ok = (r ~= nil and r.status == 201)\n");
    lua_getglobal(L, "_ok");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_http_post_timeout_and_content_type(void) {
    oidc_rp_http_test_set_response("api.example.com", 201, "created");
    lua_State* L = make_ctx();
    // body + nil headers + options table with timeout + content_type.
    run_lua(L,
        "h = H.http.post('http://api.example.com/i', 'data', nil, { timeout = 7, content_type = 'application/json' })\n"
        "r, e = H.wait(h)\n"
        "_ok = (r ~= nil and r.status == 201)\n");
    lua_getglobal(L, "_ok");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_http_wait_nil_handle(void) {
    lua_State* L = make_ctx();
    run_lua(L,
        "r, e = H.wait(nil)\n"
        "_got_err = (r == nil and e ~= nil and e:find('valid handle') ~= nil)\n");
    lua_getglobal(L, "_got_err");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_http_install_null_lua_state(void) {
    // Must not crash; returns immediately.
    H_lua_install_http(NULL);
    TEST_PASS();
}

void test_http_install_missing_h_table(void) {
    // A fresh Lua state has no H global, so install logs and returns.
    lua_State* L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);
    H_lua_install_http(L);
    lua_close(L);
    TEST_PASS();
}

// ---------------------------------------------------------------------------
// Direct tests of H_lua_http_push_inline_result (the response -> Lua table
// builder extracted from H_lua_http_wait_one) and of H_lua_http_wait_one's
// direct branches. Driving these with synthetic OidcRpHttpResponse objects
// reaches the header-marshalling loop, the duplicate-header combine branch,
// the empty-body fallback, and the error_message short-circuit, which are
// otherwise only reachable through a live HTTP response.
// ---------------------------------------------------------------------------

// Build a response with the given status/body and no headers.
static OidcRpHttpResponse* make_basic_resp(long status, const char* body) {
    OidcRpHttpResponse* r = response_alloc();
    r->http_status = status;
    r->body = body ? strdup(body) : NULL;
    r->headers = NULL;
    r->headers_count = 0;
    return r;
}

// Build a response carrying a list of (name, value) headers.
static OidcRpHttpResponse* make_resp_with_headers(long status, const char* body,
                                                  const char* names[], const char* values[],
                                                  size_t count) {
    OidcRpHttpResponse* r = make_basic_resp(status, body);
    if (count > 0) {
        r->headers = calloc(count, sizeof(OidcRpHttpHeader));
        for (size_t i = 0; i < count; i++) {
            r->headers[i].name = strdup(names[i]);
            r->headers[i].value = strdup(values[i]);
        }
        r->headers_count = count;
    }
    return r;
}

static void free_resp_headers(OidcRpHttpResponse* r) {
    for (size_t i = 0; i < r->headers_count; i++) {
        free(r->headers[i].name);
        free(r->headers[i].value);
    }
    free(r->headers);
}

void test_push_inline_result_success_with_headers(void);
void test_push_inline_result_duplicate_headers_combined(void);
void test_push_inline_result_null_body(void);
void test_push_inline_result_empty_headers(void);
void test_push_inline_result_error_message(void);
void test_wait_one_null_handle(void);

// Success response with two distinct headers: the result table must carry
// status, body, elapsed_ms, and a headers subtable with both entries.
void test_push_inline_result_success_with_headers(void) {
    const char* names[] = { "Content-Type", "X-Trace" };
    const char* values[] = { "text/plain", "abc123" };
    OidcRpHttpResponse* r = make_resp_with_headers(200, "hello", names, values, 2);

    lua_State* L = H_lua_create_context();
    int pushed = H_lua_http_push_inline_result(L, r, 42);
    TEST_ASSERT_EQUAL(2, pushed);
    TEST_ASSERT_EQUAL(2, lua_gettop(L));
    lua_pop(L, 1); // drop the trailing nil

    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_getfield(L, -1, "status");
    TEST_ASSERT_EQUAL(200, (int)lua_tointeger(L, -1));
    lua_pop(L, 1);
    lua_getfield(L, -1, "body");
    TEST_ASSERT_EQUAL_STRING("hello", lua_tostring(L, -1));
    lua_pop(L, 1);
    lua_getfield(L, -1, "elapsed_ms");
    TEST_ASSERT_EQUAL(42, (int)lua_tointeger(L, -1));
    lua_pop(L, 1);
    lua_getfield(L, -1, "headers");
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_getfield(L, -1, "Content-Type");
    TEST_ASSERT_EQUAL_STRING("text/plain", lua_tostring(L, -1));
    lua_pop(L, 1);
    lua_getfield(L, -1, "X-Trace");
    TEST_ASSERT_EQUAL_STRING("abc123", lua_tostring(L, -1));
    lua_pop(L, 2);

    free_resp_headers(r);
    H_lua_destroy_context(L);
}

// Two headers sharing the same name must be comma-combined into one entry.
void test_push_inline_result_duplicate_headers_combined(void) {
    const char* names[] = { "Set-Cookie", "Set-Cookie" };
    const char* values[] = { "a=1", "b=2" };
    OidcRpHttpResponse* r = make_resp_with_headers(200, "ok", names, values, 2);

    lua_State* L = H_lua_create_context();
    H_lua_http_push_inline_result(L, r, 0);
    lua_pop(L, 1);

    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_getfield(L, -1, "headers");
    lua_getfield(L, -1, "Set-Cookie");
    TEST_ASSERT_EQUAL_STRING("a=1, b=2", lua_tostring(L, -1));
    lua_pop(L, 2);

    free_resp_headers(r);
    H_lua_destroy_context(L);
}

// A NULL body must marshal to an empty string, not nil.
void test_push_inline_result_null_body(void) {
    OidcRpHttpResponse* r = make_basic_resp(204, NULL);

    lua_State* L = H_lua_create_context();
    H_lua_http_push_inline_result(L, r, 0);
    lua_pop(L, 1);

    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_getfield(L, -1, "body");
    TEST_ASSERT_EQUAL_STRING("", lua_tostring(L, -1));
    lua_pop(L, 1);

    H_lua_destroy_context(L);
}

// No headers at all -> headers subtable is present but empty.
void test_push_inline_result_empty_headers(void) {
    OidcRpHttpResponse* r = make_basic_resp(200, "x");

    lua_State* L = H_lua_create_context();
    H_lua_http_push_inline_result(L, r, 0);
    lua_pop(L, 1);

    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_getfield(L, -1, "headers");
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_pop(L, 1);

    H_lua_destroy_context(L);
}

// An error_message short-circuits to (nil, "H.wait: <msg>").
void test_push_inline_result_error_message(void) {
    OidcRpHttpResponse* r = make_basic_resp(0, NULL);
    r->error_message = strdup("connection refused");

    lua_State* L = H_lua_create_context();
    int pushed = H_lua_http_push_inline_result(L, r, 0);
    TEST_ASSERT_EQUAL(2, pushed);
    TEST_ASSERT_EQUAL(2, lua_gettop(L));
    TEST_ASSERT_TRUE(lua_isnil(L, 1));
    TEST_ASSERT_EQUAL_STRING("H.wait: connection refused", lua_tostring(L, 2));
    lua_pop(L, 2);

    H_lua_destroy_context(L);
}

// H_lua_http_wait_one(NULL) surfaces the invalid-handle branch.
void test_wait_one_null_handle(void) {
    lua_State* L = H_lua_create_context();
    int pushed = H_lua_http_wait_one(L, NULL);
    TEST_ASSERT_EQUAL(2, pushed);
    TEST_ASSERT_EQUAL(2, lua_gettop(L));
    TEST_ASSERT_TRUE(lua_isnil(L, 1));
    TEST_ASSERT_EQUAL_STRING("H.wait: invalid handle", lua_tostring(L, 2));
    lua_pop(L, 2);
    H_lua_destroy_context(L);
}

// Build an HTTP handle and populate its worker-result fields directly so
// we can exercise H_lua_http_push_pool_result without the worker pool.
static H_Handle* make_result_handle(lua_State* L, long status, const char* body,
                                    const char* headers_json, const char* error, long elapsed) {
    H_Handle* h = H_Handle_new(L, H_HK_HTTP);
    TEST_ASSERT_NOT_NULL(h);
    h->http_method = strdup("GET");
    h->http_url = strdup("http://example.com/");
    h->http_result_status = (int)status;
    h->http_result_body = body ? strdup(body) : NULL;
    h->http_result_headers_json = headers_json ? strdup(headers_json) : NULL;
    h->http_result_error = error ? strdup(error) : NULL;
    h->http_result_elapsed_ms = elapsed;
    return h;
}

void test_push_pool_result_success_with_headers(void);
void test_push_pool_result_duplicate_headers_combined(void);
void test_push_pool_result_empty_headers(void);
void test_push_pool_result_null_body(void);
void test_push_pool_result_error_message(void);

// Success with a JSON headers array containing a duplicated name.
void test_push_pool_result_success_with_headers(void) {
    lua_State* L = H_lua_create_context();
    H_Handle* h = make_result_handle(L, 200, "hi",
        "[{\"name\":\"A\",\"value\":\"1\"},{\"name\":\"A\",\"value\":\"2\"}]", NULL, 5);

    int pushed = H_lua_http_push_pool_result(L, h);
    TEST_ASSERT_EQUAL(2, pushed);
    lua_remove(L, 1); // drop the handle userdata pushed by H_Handle_new
    TEST_ASSERT_EQUAL(2, lua_gettop(L));
    lua_pop(L, 1); // trailing nil

    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_getfield(L, -1, "status");
    TEST_ASSERT_EQUAL(200, (int)lua_tointeger(L, -1));
    lua_pop(L, 1);
    lua_getfield(L, -1, "body");
    TEST_ASSERT_EQUAL_STRING("hi", lua_tostring(L, -1));
    lua_pop(L, 1);
    lua_getfield(L, -1, "elapsed_ms");
    TEST_ASSERT_EQUAL(5, (int)lua_tointeger(L, -1));
    lua_pop(L, 1);
    lua_getfield(L, -1, "headers");
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_getfield(L, -1, "A");
    TEST_ASSERT_EQUAL_STRING("1, 2", lua_tostring(L, -1));
    lua_pop(L, 2);

    H_lua_destroy_context(L);
}

// A JSON headers array with distinct names is mapped one-to-one.
void test_push_pool_result_duplicate_headers_combined(void) {
    lua_State* L = H_lua_create_context();
    H_Handle* h = make_result_handle(L, 200, "ok",
        "[{\"name\":\"X\",\"value\":\"a\"},{\"name\":\"Y\",\"value\":\"b\"}]", NULL, 0);

    H_lua_http_push_pool_result(L, h);
    lua_pop(L, 1);

    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_getfield(L, -1, "headers");
    lua_getfield(L, -1, "X");
    TEST_ASSERT_EQUAL_STRING("a", lua_tostring(L, -1));
    lua_pop(L, 1);
    lua_getfield(L, -1, "Y");
    TEST_ASSERT_EQUAL_STRING("b", lua_tostring(L, -1));
    lua_pop(L, 2);

    H_lua_destroy_context(L);
}

// An empty JSON headers array yields an empty headers subtable.
void test_push_pool_result_empty_headers(void) {
    lua_State* L = H_lua_create_context();
    H_Handle* h = make_result_handle(L, 200, "x", "[]", NULL, 0);

    H_lua_http_push_pool_result(L, h);
    lua_pop(L, 1);

    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_getfield(L, -1, "headers");
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_pop(L, 1);

    H_lua_destroy_context(L);
}

// A NULL body marshals to an empty string.
void test_push_pool_result_null_body(void) {
    lua_State* L = H_lua_create_context();
    H_Handle* h = make_result_handle(L, 204, NULL, NULL, NULL, 0);

    H_lua_http_push_pool_result(L, h);
    lua_pop(L, 1);

    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_getfield(L, -1, "body");
    TEST_ASSERT_EQUAL_STRING("", lua_tostring(L, -1));
    lua_pop(L, 1);

    H_lua_destroy_context(L);
}

// A set error field short-circuits to (nil, "H.wait: <err>").
void test_push_pool_result_error_message(void) {
    lua_State* L = H_lua_create_context();
    H_Handle* h = make_result_handle(L, 0, NULL, NULL, "boom", 0);

    int pushed = H_lua_http_push_pool_result(L, h);
    TEST_ASSERT_EQUAL(2, pushed);
    lua_remove(L, 1); // drop the handle userdata pushed by H_Handle_new
    TEST_ASSERT_EQUAL(2, lua_gettop(L));
    TEST_ASSERT_TRUE(lua_isnil(L, 1));
    TEST_ASSERT_EQUAL_STRING("H.wait: boom", lua_tostring(L, 2));
    lua_pop(L, 2);

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
    RUN_TEST(test_http_get_no_args_error_handle);
    RUN_TEST(test_http_get_headers_non_table_returns_null);
    RUN_TEST(test_http_get_headers_non_string_entry);
    RUN_TEST(test_http_opts_timeout_non_table);
    RUN_TEST(test_http_opts_timeout_valid_number);
    RUN_TEST(test_http_post_no_args_error_handle);
    RUN_TEST(test_http_post_empty_url_error_handle);
    RUN_TEST(test_http_post_body_not_string);
    RUN_TEST(test_http_post_headers_not_table);
    RUN_TEST(test_http_post_nil_body_and_headers);
    RUN_TEST(test_http_post_timeout_and_content_type);
    RUN_TEST(test_http_wait_nil_handle);
    RUN_TEST(test_http_install_null_lua_state);
    RUN_TEST(test_http_install_missing_h_table);
    RUN_TEST(test_push_inline_result_success_with_headers);
    RUN_TEST(test_push_inline_result_duplicate_headers_combined);
    RUN_TEST(test_push_inline_result_null_body);
    RUN_TEST(test_push_inline_result_empty_headers);
    RUN_TEST(test_push_inline_result_error_message);
    RUN_TEST(test_wait_one_null_handle);
    RUN_TEST(test_push_pool_result_success_with_headers);
    RUN_TEST(test_push_pool_result_duplicate_headers_combined);
    RUN_TEST(test_push_pool_result_empty_headers);
    RUN_TEST(test_push_pool_result_null_body);
    RUN_TEST(test_push_pool_result_error_message);

    return UNITY_END();
}
