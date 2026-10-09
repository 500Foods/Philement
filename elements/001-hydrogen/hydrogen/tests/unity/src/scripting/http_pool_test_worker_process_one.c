/*
 * Unity Test File: http_pool_test_worker_process_one.c
 *
 * Drives scripting_http_worker_process_one() directly (no worker
 * thread) to cover the branches the pool-path submit tests never hit:
 *   - NULL handle (early return, line 115)
 *   - rejected HTTP method (error stored on handle, condvar broadcast,
 *     ref released). CONNECT is outside the Phase 13 allowlist.
 *   - POST method success (scripting_http_request via the test seam
 *     stores status + body on the handle)
 *   - PROPFIND 207 and REPORT 412 are stored as status and body
 *
 * The GET success path is already covered by the pool/submit tests
 * (a worker thread runs process_one for every submitted GET handle).
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
#include <src/scripting/http_pool.h>
#include <src/scripting/scripting_handle.h>
#include <src/scripting/http_client.h>

// Forward declarations
void test_process_one_null_handle(void);
void test_process_one_unknown_method(void);
void test_process_one_post_success(void);
void test_process_one_propfind_207(void);
void test_process_one_report_412(void);

static AppConfig mock_app_config_storage = {0};

void setUp(void) {
    memset(&mock_app_config_storage, 0, sizeof(mock_app_config_storage));
    app_config = &mock_app_config_storage;
    scripting_http_test_clear_responses();
}

void tearDown(void) {
    scripting_http_test_clear_responses();
    scripting_http_pool_destroy();
    app_config = NULL;
}

// NULL handle: process_one returns immediately without touching the
// handle or any global state.
void test_process_one_null_handle(void) {
    scripting_http_worker_process_one(NULL, NULL);
}

// Unknown method: the worker must store an error on the handle, signal
// the condvar, and release its ref. We hold an extra ref so the handle
// survives for assertions, then let the Lua __gc drop the last ref.
void test_process_one_unknown_method(void) {
    lua_State* L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);
    H_Handle_install_metatable(L);
    H_Handle* h = H_Handle_new(L, H_HK_HTTP);
    TEST_ASSERT_NOT_NULL(h);
    h->http_url = strdup("http://example.test/");
    h->http_method = strdup("CONNECT");

    H_Handle_acquire(h);                 // refcount 2
    scripting_http_worker_process_one(NULL, h);
    // The worker released its ref: 2 -> 1, handle still alive.
    TEST_ASSERT_EQUAL(1, H_Handle_get_refcount(h));
    TEST_ASSERT_TRUE(h->http_ready);
    TEST_ASSERT_EQUAL_STRING("H.wait: unknown HTTP method on handle",
                             h->http_result_error);
    TEST_ASSERT_EQUAL(0, h->http_result_status);

    lua_close(L);                        // drops the last ref via __gc
}

// POST success: the test seam returns a canned response; process_one
// stores status + body (serialize_headers sees no headers -> NULL).
void test_process_one_post_success(void) {
    scripting_http_test_set_response("post.test", 201, "created");

    lua_State* L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);
    H_Handle_install_metatable(L);
    H_Handle* h = H_Handle_new(L, H_HK_HTTP);
    TEST_ASSERT_NOT_NULL(h);
    h->http_url = strdup("http://post.test/");
    h->http_method = strdup("POST");
    h->http_body = strdup("payload");
    h->http_content_type = strdup("application/json");
    h->http_timeout = 30;

    H_Handle_acquire(h);                 // refcount 2
    scripting_http_worker_process_one(NULL, h);
    // The worker released its ref: 2 -> 1, handle still alive.
    TEST_ASSERT_EQUAL(1, H_Handle_get_refcount(h));
    TEST_ASSERT_TRUE(h->http_ready);
    TEST_ASSERT_NULL(h->http_result_error);
    TEST_ASSERT_EQUAL(201, h->http_result_status);
    TEST_ASSERT_NOT_NULL(h->http_result_body);
    TEST_ASSERT_EQUAL_STRING("created", h->http_result_body);

    lua_close(L);                        // drops the last ref via __gc
}

void test_process_one_propfind_207(void) {
    scripting_http_test_set_response("cal.test", 207, "multi-status");

    lua_State* L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);
    H_Handle_install_metatable(L);
    H_Handle* h = H_Handle_new(L, H_HK_HTTP);
    TEST_ASSERT_NOT_NULL(h);
    h->http_url = strdup("http://cal.test/dav");
    h->http_method = strdup("PROPFIND");
    h->http_body = strdup("<propfind/>");
    h->http_content_type = strdup("application/xml");

    H_Handle_acquire(h);
    scripting_http_worker_process_one(NULL, h);
    TEST_ASSERT_EQUAL(1, H_Handle_get_refcount(h));
    TEST_ASSERT_TRUE(h->http_ready);
    TEST_ASSERT_NULL(h->http_result_error);
    TEST_ASSERT_EQUAL(207, h->http_result_status);
    TEST_ASSERT_EQUAL_STRING("multi-status", h->http_result_body);

    lua_close(L);
}

void test_process_one_report_412(void) {
    scripting_http_test_set_response("cal.test", 412, "precondition");

    lua_State* L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);
    H_Handle_install_metatable(L);
    H_Handle* h = H_Handle_new(L, H_HK_HTTP);
    TEST_ASSERT_NOT_NULL(h);
    h->http_url = strdup("http://cal.test/dav");
    h->http_method = strdup("REPORT");
    h->http_body = strdup("<calendar-query/>");

    H_Handle_acquire(h);
    scripting_http_worker_process_one(NULL, h);
    TEST_ASSERT_NULL(h->http_result_error);
    TEST_ASSERT_EQUAL(412, h->http_result_status);
    TEST_ASSERT_EQUAL_STRING("precondition", h->http_result_body);

    lua_close(L);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_process_one_null_handle);
    RUN_TEST(test_process_one_unknown_method);
    RUN_TEST(test_process_one_post_success);
    RUN_TEST(test_process_one_propfind_207);
    RUN_TEST(test_process_one_report_412);
    return UNITY_END();
}
