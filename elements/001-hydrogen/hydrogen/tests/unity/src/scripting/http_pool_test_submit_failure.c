/*
 * Unity Test File: http_pool_test_submit_failure.c
 *
 * Covers the error branches of scripting_http_pool_submit() that the
 * happy-path submit tests never reach:
 *   - wrong handle kind (line 311-312)
 *   - HttpPoolItem allocation failure via mocked malloc (line 314-316)
 *
 * The enqueue-failure branch (line 324-328) requires a full queue and
 * is left to the blackbox suite / irreducible floor.
 *
 * For the allocation-failure case the pool is hand-built with no worker
 * threads (and the global pointer is set directly) so no background
 * thread can allocate and corrupt the mocked malloc counter.
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
#include <src/queue/queue.h>

// Mock for allocation-failure injection.
// USE_MOCK_SYSTEM is a global Unity define; the explicit #define matches
// it (value-less) so the mock's control API prototypes are visible.
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

// Forward declarations
void test_submit_wrong_kind(void);
void test_submit_calloc_failure(void);

static AppConfig mock_app_config_storage = {0};

// Build a minimal pool (no worker threads) and publish it as the global
// so scripting_http_pool_submit can reach its allocation logic.
static void publish_idle_pool(void) {
    ScriptingHttpPool* pool = calloc(1, sizeof(ScriptingHttpPool));
    TEST_ASSERT_NOT_NULL(pool);
    pool->worker_count = 0;
    pool->worker_tids = NULL;
    QueueAttributes attrs = {0};
    pool->http_queue = queue_create_with_label(SCRIPTING_HTTP_QUEUE_NAME,
                                               &attrs, SR_SCRIPTING);
    TEST_ASSERT_NOT_NULL(pool->http_queue);
    scripting_http_pool = pool;
}

void setUp(void) {
    memset(&mock_app_config_storage, 0, sizeof(mock_app_config_storage));
    app_config = &mock_app_config_storage;
    scripting_http_test_clear_responses();
    mock_system_reset_all();
}

void tearDown(void) {
    scripting_http_test_clear_responses();
    scripting_http_pool_destroy();
    mock_system_reset_all();
    app_config = NULL;
}

// A non-HTTP handle must be rejected before the pool is touched.
void test_submit_wrong_kind(void) {
    TEST_ASSERT_TRUE(scripting_http_pool_init(1));
    lua_State* L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);
    H_Handle_install_metatable(L);
    H_Handle* h = H_Handle_new(L, H_HK_QUERY);   // not H_HK_HTTP
    TEST_ASSERT_NOT_NULL(h);
    TEST_ASSERT_FALSE(scripting_http_pool_submit(h));
    lua_close(L);
}

// Force the HttpPoolItem calloc to fail; submit must return false
// without acquiring a ref (the Lua userdata still owns the handle).
void test_submit_calloc_failure(void) {
    publish_idle_pool();
    lua_State* L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);
    H_Handle_install_metatable(L);
    H_Handle* h = H_Handle_new(L, H_HK_HTTP);
    TEST_ASSERT_NOT_NULL(h);
    h->http_url = strdup("http://fail.test/");
    h->http_method = strdup("GET");
    h->http_timeout = 30;

    mock_system_set_malloc_failure(1);   // next malloc = item calloc fails
    TEST_ASSERT_FALSE(scripting_http_pool_submit(h));
    // submit must not have acquired a ref on the calloc-failure path.
    TEST_ASSERT_EQUAL(1, H_Handle_get_refcount(h));
    lua_close(L);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_submit_wrong_kind);
    RUN_TEST(test_submit_calloc_failure);
    return UNITY_END();
}
