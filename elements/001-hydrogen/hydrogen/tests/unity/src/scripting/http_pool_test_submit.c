/*
 * Unity Test File: http_pool_test_submit.c
 *
 * Phase 17 of the LUA_PLAN. Tests the HTTP worker pool submit path:
 *   - scripting_http_pool_submit enqueues a handle and a worker
 *     processes it
 *   - The result is stored on the handle and the condvar fires
 *   - Multiple concurrent submits run in parallel
 *   - The test injection seam (scripting_http_test_set_response)
 *     short-circuits the libcurl call
 *   - The handle's refcount is correctly managed (worker releases
 *     its ref after storing the result; the test still holds one
 *     ref via the local pointer)
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Standard includes
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

// Third-party includes
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

// Module under test
#include <src/scripting/http_pool.h>
#include <src/scripting/scripting_handle.h>
#include <src/scripting/http_client.h>
#include <src/scripting/lua_context.h>

// Forward declarations
void test_http_pool_submit_stores_result(void);
void test_http_pool_submit_null_handle(void);
void test_http_pool_submit_pool_not_running(void);
void test_http_pool_concurrent_submits(void);
void test_http_pool_refcount_during_call(void);

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

// Wait for the handle's condvar with a bounded timeout. Returns
// true if the result was ready within the timeout.
static bool wait_for_ready(H_Handle* h, int timeout_ms) {
    struct timespec deadline;
    clock_gettime(CLOCK_REALTIME, &deadline);
    deadline.tv_sec += timeout_ms / 1000;
    deadline.tv_nsec += (timeout_ms % 1000) * 1000000L;
    if (deadline.tv_nsec >= 1000000000L) {
        deadline.tv_sec++;
        deadline.tv_nsec -= 1000000000L;
    }
    pthread_mutex_lock(&h->http_mutex);
    while (!h->http_ready) {
        int rc = pthread_cond_timedwait(&h->http_cond, &h->http_mutex, &deadline);
        if (rc != 0) {
            pthread_mutex_unlock(&h->http_mutex);
            return false;
        }
    }
    pthread_mutex_unlock(&h->http_mutex);
    return true;
}

// Poll the thread-safe refcount until it reaches the expected value
// or the bounded timeout expires. Returns true on success.
static bool wait_for_refcount(H_Handle* h, int expected, int timeout_ms) {
    struct timespec deadline;
    clock_gettime(CLOCK_REALTIME, &deadline);
    deadline.tv_sec += timeout_ms / 1000;
    deadline.tv_nsec += (timeout_ms % 1000) * 1000000L;
    if (deadline.tv_nsec >= 1000000000L) {
        deadline.tv_sec++;
        deadline.tv_nsec -= 1000000000L;
    }

    struct timespec short_sleep = {0, 1000000L}; // 1 ms
    while (H_Handle_get_refcount(h) != expected) {
        struct timespec now;
        clock_gettime(CLOCK_REALTIME, &now);
        if (now.tv_sec > deadline.tv_sec ||
            (now.tv_sec == deadline.tv_sec && now.tv_nsec >= deadline.tv_nsec)) {
            return false;
        }
        nanosleep(&short_sleep, NULL);
    }
    return true;
}

// Submit one handle; the worker picks it up, calls the (injected)
// HTTP, and stores the result. H.wait sees the result.
void test_http_pool_submit_stores_result(void) {
    TEST_ASSERT_TRUE(scripting_http_pool_init(2));
    scripting_http_test_set_response("example.com", 200, "hello");

    lua_State* L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);
    H_Handle_install_metatable(L);
    H_Handle* h = H_Handle_new(L, H_HK_HTTP);
    TEST_ASSERT_NOT_NULL(h);
    h->http_url = strdup("http://example.com/test");
    h->http_method = strdup("GET");
    h->http_timeout = 30;

    TEST_ASSERT_TRUE(scripting_http_pool_submit(h));
    TEST_ASSERT_TRUE(wait_for_ready(h, 5000));

    // After the worker is done, the refcount should be back to 1
    // (the worker released its ref). Poll briefly because the release
    // happens after the condvar broadcast.
    TEST_ASSERT_TRUE(wait_for_refcount(h, 1, 1000));
    TEST_ASSERT_TRUE(h->http_ready);
    TEST_ASSERT_NULL(h->http_result_error);
    TEST_ASSERT_EQUAL(200, h->http_result_status);
    TEST_ASSERT_NOT_NULL(h->http_result_body);
    TEST_ASSERT_EQUAL_STRING("hello", h->http_result_body);

    // The Lua userdata still holds the remaining ref; let lua_close run
    // the __gc metamethod to free the handle.
    lua_close(L);
    TEST_ASSERT_EQUAL(1, scripting_http_test_get_consumed_count());
}

// NULL handle: submit returns false.
void test_http_pool_submit_null_handle(void) {
    TEST_ASSERT_TRUE(scripting_http_pool_init(2));
    TEST_ASSERT_FALSE(scripting_http_pool_submit(NULL));
}

// Pool not running: submit returns false (the H_lua_http_wait_one
// caller falls back to the inline path in this case).
void test_http_pool_submit_pool_not_running(void) {
    TEST_ASSERT_NULL(scripting_http_pool);
    lua_State* L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);
    H_Handle_install_metatable(L);
    H_Handle* h = H_Handle_new(L, H_HK_HTTP);
    TEST_ASSERT_NOT_NULL(h);
    h->http_url = strdup("http://example.com/");
    h->http_method = strdup("GET");
    TEST_ASSERT_FALSE(scripting_http_pool_submit(h));
    // Lua userdata holds the only ref; let __gc free the handle.
    lua_close(L);
}

// Multiple concurrent submits: 4 workers process 4 handles in
// parallel. The test exercises the pool's parallelism and proves
// the result routing is correct.
void test_http_pool_concurrent_submits(void) {
    TEST_ASSERT_TRUE(scripting_http_pool_init(4));
    for (int i = 0; i < 4; i++) {
        char url[64];
        snprintf(url, sizeof(url), "http://host%d.test/", i);
        scripting_http_test_set_response(url, 200 + i, url);
    }

    lua_State* L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);
    H_Handle_install_metatable(L);
    H_Handle* handles[4];
    for (int i = 0; i < 4; i++) {
        handles[i] = H_Handle_new(L, H_HK_HTTP);
        TEST_ASSERT_NOT_NULL(handles[i]);
        char url[64];
        snprintf(url, sizeof(url), "http://host%d.test/", i);
        handles[i]->http_url = strdup(url);
        handles[i]->http_method = strdup("GET");
        handles[i]->http_timeout = 30;
        TEST_ASSERT_TRUE(scripting_http_pool_submit(handles[i]));
    }
    for (int i = 0; i < 4; i++) {
        bool ready = wait_for_ready(handles[i], 10000);
        TEST_ASSERT_TRUE_MESSAGE(ready, "handle not ready");
        TEST_ASSERT_TRUE(wait_for_refcount(handles[i], 1, 1000));
        TEST_ASSERT_EQUAL(200 + i, handles[i]->http_result_status);
        TEST_ASSERT_NOT_NULL(handles[i]->http_result_body);
    }
    TEST_ASSERT_EQUAL(4, scripting_http_test_get_consumed_count());
    // Lua userdata still holds the remaining refs; let __gc free them.
    lua_close(L);
}

// During the call, the refcount is 2 (Lua/test + worker). After
// the worker finishes, it drops to 1.
void test_http_pool_refcount_during_call(void) {
    TEST_ASSERT_TRUE(scripting_http_pool_init(1));
    scripting_http_test_set_response("refcount.test", 200, "ok");

    lua_State* L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);
    H_Handle_install_metatable(L);
    H_Handle* h = H_Handle_new(L, H_HK_HTTP);
    TEST_ASSERT_NOT_NULL(h);
    TEST_ASSERT_EQUAL(1, H_Handle_get_refcount(h));
    h->http_url = strdup("http://refcount.test/");
    h->http_method = strdup("GET");
    h->http_timeout = 30;

    TEST_ASSERT_TRUE(scripting_http_pool_submit(h));
    // Submit acquired a ref. With the test injection seam the worker
    // may finish almost immediately, so we only assert the handle is
    // still alive here and verify the final refcount after wait.
    TEST_ASSERT_GREATER_OR_EQUAL(1, H_Handle_get_refcount(h));

    TEST_ASSERT_TRUE(wait_for_ready(h, 5000));
    // Worker released its ref: 2 -> 1 (poll briefly for the release).
    TEST_ASSERT_TRUE(wait_for_refcount(h, 1, 1000));

    // Lua userdata holds the remaining ref; let __gc free the handle.
    lua_close(L);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_http_pool_submit_stores_result);
    RUN_TEST(test_http_pool_submit_null_handle);
    RUN_TEST(test_http_pool_submit_pool_not_running);
    RUN_TEST(test_http_pool_concurrent_submits);
    RUN_TEST(test_http_pool_refcount_during_call);

    return UNITY_END();
}
