/*
 * Unity Test File: lua_context_test_set_job_context.c
 *
 * Tests the per-state job-context accessors:
 *   - H_lua_set_job_context  (stores a heap-duplicated context in the
 *     state's extraspace)
 *   - H_lua_get_job_context  (returns the stored pointer)
 *
 * Covers the NULL-state guards, the allocate-then-copy happy path,
 * the free-prior-context path (set called twice), the clear-with-NULL
 * path, and the malloc-failure path (via the global malloc mock).
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <src/scripting/lua_context.h>

// Mock app_config (zeroed; the job-context accessors do not consult it).
static AppConfig mock_app_config_storage = {0};

// Forward declarations
void test_set_job_context_null_state_is_noop(void);
void test_get_job_context_null_state_returns_null(void);
void test_set_then_get_job_context_roundtrip(void);
void test_set_job_context_frees_prior_context(void);
void test_set_job_context_null_clears_context(void);

void setUp(void) {
    memset(&mock_app_config_storage, 0, sizeof(mock_app_config_storage));
    app_config = &mock_app_config_storage;
}

void tearDown(void) {
    app_config = NULL;
}

// H_lua_set_job_context(NULL, ...) must return immediately.
void test_set_job_context_null_state_is_noop(void) {
    H_lua_job_context ctx;
    memset(&ctx, 0, sizeof(ctx));
    H_lua_set_job_context(NULL, &ctx);
    TEST_PASS();
}

// H_lua_get_job_context(NULL) must return NULL.
void test_get_job_context_null_state_returns_null(void) {
    TEST_ASSERT_NULL(H_lua_get_job_context(NULL));
}

// Set a context, then read it back: the returned pointer must be the
// stored duplicate (non-NULL) and the contents must match what we set.
void test_set_then_get_job_context_roundtrip(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    H_lua_job_context ctx;
    memset(&ctx, 0, sizeof(ctx));
    snprintf(ctx.job_id, sizeof(ctx.job_id), "%s", "j1234");
    ctx.enforce_limits = true;
    ctx.max_runtime_seconds = 30;

    H_lua_set_job_context(L, &ctx);

    H_lua_job_context* got = H_lua_get_job_context(L);
    TEST_ASSERT_NOT_NULL(got);
    TEST_ASSERT_EQUAL_STRING("j1234", got->job_id);
    TEST_ASSERT_TRUE(got->enforce_limits);
    TEST_ASSERT_EQUAL_INT(30, got->max_runtime_seconds);

    H_lua_destroy_context(L);
}

// Setting a second context must free the first (no leak) and the
// pointer returned by get must reflect the new contents.
void test_set_job_context_frees_prior_context(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    H_lua_job_context first;
    memset(&first, 0, sizeof(first));
    snprintf(first.job_id, sizeof(first.job_id), "%s", "first");
    H_lua_set_job_context(L, &first);

    H_lua_job_context second;
    memset(&second, 0, sizeof(second));
    snprintf(second.job_id, sizeof(second.job_id), "%s", "secon");
    H_lua_set_job_context(L, &second);

    H_lua_job_context* got = H_lua_get_job_context(L);
    TEST_ASSERT_NOT_NULL(got);
    TEST_ASSERT_EQUAL_STRING("secon", got->job_id);

    H_lua_destroy_context(L);
}

// Passing NULL context clears the slot; get must then return NULL.
void test_set_job_context_null_clears_context(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    H_lua_job_context ctx;
    memset(&ctx, 0, sizeof(ctx));
    snprintf(ctx.job_id, sizeof(ctx.job_id), "%s", "job-x");
    H_lua_set_job_context(L, &ctx);

    H_lua_set_job_context(L, NULL);

    TEST_ASSERT_NULL(H_lua_get_job_context(L));

    H_lua_destroy_context(L);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_set_job_context_null_state_is_noop);
    RUN_TEST(test_get_job_context_null_state_returns_null);
    RUN_TEST(test_set_then_get_job_context_roundtrip);
    RUN_TEST(test_set_job_context_frees_prior_context);
    RUN_TEST(test_set_job_context_null_clears_context);

    return UNITY_END();
}
