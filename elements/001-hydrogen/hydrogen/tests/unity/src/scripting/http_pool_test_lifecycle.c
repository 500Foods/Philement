/*
 * Unity Test File: http_pool_test_lifecycle.c
 *
 * Phase 17 of the LUA_PLAN. Tests the HTTP worker pool lifecycle:
 *   - scripting_http_pool_init creates a pool with the requested
 *     number of workers
 *   - scripting_http_pool_destroy tears it down cleanly
 *   - Both are idempotent and NULL-safe
 *   - A second init after a destroy works (the subsystem can be
 *     relaunched within the same process, e.g. between Unity tests)
 *
 * The pool's worker threads do not perform real HTTP calls in this
 * test (no handles are submitted). The test seam
 * (scripting_http_test_set_response) is unused here.
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Standard includes
#include <string.h>
#include <stdlib.h>

// Module under test
#include <src/scripting/http_pool.h>
#include <src/scripting/scripting.h>

// Forward declarations (required for -Wmissing-prototypes)
void test_http_pool_init_destroy(void);
void test_http_pool_init_invalid_count(void);
void test_http_pool_destroy_null_safe(void);
void test_http_pool_init_idempotent(void);

void setUp(void) {
}

void tearDown(void) {
    // Ensure the pool is torn down between tests so a failure in
    // one test does not leak workers into the next.
    scripting_http_pool_destroy();
}

// Init creates a pool; destroy tears it down. The pool is NULL
// before init and NULL after destroy.
void test_http_pool_init_destroy(void) {
    TEST_ASSERT_NULL(scripting_http_pool);
    TEST_ASSERT_TRUE(scripting_http_pool_init(2));
    TEST_ASSERT_NOT_NULL(scripting_http_pool);
    TEST_ASSERT_EQUAL(2, scripting_http_pool->worker_count);
    scripting_http_pool_destroy();
    TEST_ASSERT_NULL(scripting_http_pool);
}

// worker_count < 1 is rejected; pool stays NULL.
void test_http_pool_init_invalid_count(void) {
    TEST_ASSERT_FALSE(scripting_http_pool_init(0));
    TEST_ASSERT_FALSE(scripting_http_pool_init(-1));
    TEST_ASSERT_NULL(scripting_http_pool);
}

// Destroy on a NULL pool is a no-op (does not crash).
void test_http_pool_destroy_null_safe(void) {
    TEST_ASSERT_NULL(scripting_http_pool);
    scripting_http_pool_destroy();
    TEST_ASSERT_NULL(scripting_http_pool);
}

// Init twice in a row is idempotent: the second call is a no-op
// and returns true.
void test_http_pool_init_idempotent(void) {
    TEST_ASSERT_TRUE(scripting_http_pool_init(2));
    ScriptingHttpPool* first = scripting_http_pool;
    TEST_ASSERT_TRUE(scripting_http_pool_init(2));
    TEST_ASSERT_EQUAL_PTR(first, scripting_http_pool);
    scripting_http_pool_destroy();
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_http_pool_init_destroy);
    RUN_TEST(test_http_pool_init_invalid_count);
    RUN_TEST(test_http_pool_destroy_null_safe);
    RUN_TEST(test_http_pool_init_idempotent);

    return UNITY_END();
}
