/*
 * Unity Test File: http_pool_test_init_failure.c
 *
 * Drives the allocation-failure branches of scripting_http_pool_init()
 * that the happy-path lifecycle tests never reach:
 *   - pool struct calloc failure (line 199-200)
 *   - worker_tids / queue allocation failure (line 209-215)
 *
 * Both failure modes return before any worker thread is started, so the
 * mocked malloc counter is fully deterministic (no background thread
 * can allocate and shift the failure index).
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Standard includes
#include <stdlib.h>

// Mock for allocation-failure injection.
// USE_MOCK_SYSTEM is a global Unity define; the explicit #define matches
// it (value-less) so the mock's control API prototypes are visible.
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

// Module under test
#include <src/scripting/http_pool.h>

// Forward declarations
void test_init_pool_calloc_failure(void);
void test_init_worker_tids_calloc_failure(void);

void setUp(void) {
    scripting_system_shutdown = 0;
    mock_system_reset_all();
}

void tearDown(void) {
    scripting_system_shutdown = 0;
    scripting_http_pool_destroy();
    mock_system_reset_all();
}

// First allocation (the pool struct) fails: init returns false.
void test_init_pool_calloc_failure(void) {
    mock_system_set_malloc_failure(1);
    TEST_ASSERT_FALSE(scripting_http_pool_init(2));
    TEST_ASSERT_NULL(scripting_http_pool);
}

// Second allocation (the worker_tids array) fails: the partial pool is
// torn down and init returns false.
void test_init_worker_tids_calloc_failure(void) {
    mock_system_set_malloc_failure(2);
    TEST_ASSERT_FALSE(scripting_http_pool_init(2));
    TEST_ASSERT_NULL(scripting_http_pool);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_init_pool_calloc_failure);
    RUN_TEST(test_init_worker_tids_calloc_failure);
    return UNITY_END();
}
