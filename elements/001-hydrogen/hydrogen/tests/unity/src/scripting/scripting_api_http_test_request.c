/*
 * Unity Test File: scripting_api_http_test_request.c
 *
 * Argent Phase 13. H.http.request and H.http.request_sync:
 *   - PROPFIND is allowlisted and returns { status, headers, body, elapsed_ms }
 *   - CONNECT and TRACE set the handle error and do not call the network
 *   - 207 and 412 come back as that result table
 *   - H.http.get and H.http.post consume the same scripting test seam
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <string.h>
#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <src/scripting/lua_context.h>
#include <src/scripting/scripting_handle.h>
#include <src/scripting/scripting_api.h>
#include <src/scripting/http_pool.h>
#include <src/scripting/http_client.h>

void test_http_request_propfind_sync_result_table(void);
void test_http_request_report_412_is_data(void);
void test_http_request_connect_is_error(void);
void test_http_request_trace_is_error(void);
void test_http_request_lowercase_is_error(void);
void test_http_get_uses_request_seam(void);
void test_http_post_uses_request_seam(void);
void test_http_request_propfind_on_pool(void);

static AppConfig mock_app_config_storage = {0};

void setUp(void) {
    memset(&mock_app_config_storage, 0, sizeof(mock_app_config_storage));
    mock_app_config_storage.scripting.DefaultHTTPTimeout = 30;
    app_config = &mock_app_config_storage;
    scripting_http_test_clear_responses();
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

static void assert_global_true(lua_State* L, const char* name) {
    lua_getglobal(L, name);
    TEST_ASSERT_TRUE_MESSAGE(lua_toboolean(L, -1), name);
    lua_pop(L, 1);
}

void test_http_request_propfind_sync_result_table(void) {
    scripting_http_test_set_response("cal.example", 207, "<multistatus/>");
    TEST_ASSERT_NULL(scripting_http_pool);

    lua_State* L = make_ctx();
    run_lua(L,
        "r, e = H.http.request_sync('PROPFIND', 'http://cal.example/dav',\n"
        "  '<propfind/>', { Depth = '1' },\n"
        "  { content_type = 'application/xml', timeout = 5 })\n"
        "_status  = (r and r.status == 207)\n"
        "_headers = (r and type(r.headers) == 'table')\n"
        "_body    = (r and r.body == '<multistatus/>')\n"
        "_elapsed = (r and type(r.elapsed_ms) == 'number')\n"
        "_no_err  = (e == nil)\n");
    assert_global_true(L, "_status");
    assert_global_true(L, "_headers");
    assert_global_true(L, "_body");
    assert_global_true(L, "_elapsed");
    assert_global_true(L, "_no_err");
    H_lua_destroy_context(L);
    TEST_ASSERT_EQUAL(1, scripting_http_test_get_consumed_count());
}

void test_http_request_report_412_is_data(void) {
    scripting_http_test_set_response("cal.example", 412, "precondition");

    lua_State* L = make_ctx();
    run_lua(L,
        "r, e = H.http.request_sync('REPORT', 'http://cal.example/dav',\n"
        "  '<calendar-query/>', nil, nil)\n"
        "_status = (r and r.status == 412)\n"
        "_body   = (r and r.body == 'precondition')\n"
        "_elapsed = (r and type(r.elapsed_ms) == 'number')\n"
        "_headers = (r and type(r.headers) == 'table')\n"
        "_no_err = (e == nil)\n");
    assert_global_true(L, "_status");
    assert_global_true(L, "_body");
    assert_global_true(L, "_elapsed");
    assert_global_true(L, "_headers");
    assert_global_true(L, "_no_err");
    H_lua_destroy_context(L);
}

void test_http_request_connect_is_error(void) {
    scripting_http_test_set_response("cal.example", 200, "should-not-run");

    lua_State* L = make_ctx();
    run_lua(L,
        "h = H.http.request('CONNECT', 'http://cal.example/dav', nil, nil, nil)\n"
        "r, e = H.wait(h)\n"
        "_rejected = (r == nil and type(e) == 'string' and e:find('not allowed') ~= nil)\n");
    assert_global_true(L, "_rejected");
    H_lua_destroy_context(L);
    TEST_ASSERT_EQUAL(0, scripting_http_test_get_consumed_count());
}

void test_http_request_trace_is_error(void) {
    lua_State* L = make_ctx();
    run_lua(L,
        "r, e = H.http.request_sync('TRACE', 'http://cal.example/dav', nil, nil, nil)\n"
        "_rejected = (r == nil and type(e) == 'string' and e:find('not allowed') ~= nil)\n");
    assert_global_true(L, "_rejected");
    H_lua_destroy_context(L);
}

void test_http_request_lowercase_is_error(void) {
    lua_State* L = make_ctx();
    run_lua(L,
        "r, e = H.http.request_sync('propfind', 'http://cal.example/dav', nil, nil, nil)\n"
        "_rejected = (r == nil and type(e) == 'string' and e:find('not allowed') ~= nil)\n");
    assert_global_true(L, "_rejected");
    H_lua_destroy_context(L);
}

void test_http_get_uses_request_seam(void) {
    scripting_http_test_set_response("wrap.example", 200, "via-get");

    lua_State* L = make_ctx();
    run_lua(L,
        "r, e = H.http.get_sync('http://wrap.example/health')\n"
        "_ok = (r and r.status == 200 and r.body == 'via-get' and e == nil)\n");
    assert_global_true(L, "_ok");
    H_lua_destroy_context(L);
    TEST_ASSERT_EQUAL(1, scripting_http_test_get_consumed_count());
}

void test_http_post_uses_request_seam(void) {
    scripting_http_test_set_response("wrap.example", 201, "via-post");

    lua_State* L = make_ctx();
    run_lua(L,
        "r, e = H.http.post_sync('http://wrap.example/items', 'body', nil, nil)\n"
        "_ok = (r and r.status == 201 and r.body == 'via-post' and e == nil)\n");
    assert_global_true(L, "_ok");
    H_lua_destroy_context(L);
    TEST_ASSERT_EQUAL(1, scripting_http_test_get_consumed_count());
}

void test_http_request_propfind_on_pool(void) {
    scripting_http_test_set_response("pool.example", 207, "pooled");
    TEST_ASSERT_TRUE(scripting_http_pool_init(1));

    lua_State* L = make_ctx();
    run_lua(L,
        "r, e = H.http.request_sync('MKCALENDAR', 'http://pool.example/cal/',\n"
        "  nil, nil, nil)\n"
        "_ok = (r and r.status == 207 and r.body == 'pooled' and e == nil\n"
        "       and type(r.headers) == 'table' and type(r.elapsed_ms) == 'number')\n");
    assert_global_true(L, "_ok");
    H_lua_destroy_context(L);
    TEST_ASSERT_EQUAL(1, scripting_http_test_get_consumed_count());
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_http_request_propfind_sync_result_table);
    RUN_TEST(test_http_request_report_412_is_data);
    RUN_TEST(test_http_request_connect_is_error);
    RUN_TEST(test_http_request_trace_is_error);
    RUN_TEST(test_http_request_lowercase_is_error);
    RUN_TEST(test_http_get_uses_request_seam);
    RUN_TEST(test_http_post_uses_request_seam);
    RUN_TEST(test_http_request_propfind_on_pool);

    return UNITY_END();
}
