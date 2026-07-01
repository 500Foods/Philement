/*
 * Unity Test File: scoreboard_test_create_destroy.c
 *
 * Phase 5 of the LUA_PLAN. Tests the lifecycle of Scoreboard itself:
 *   - create returns a non-NULL pointer
 *   - destroy is safe with NULL
 *   - destroy is idempotent (calling twice on the same pointer is OK
 *     the second time only after NULLing the caller's copy - this
 *     is the standard free-pattern contract)
 *   - count on a fresh scoreboard is 0
 *   - count on NULL scoreboard is 0
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Module under test
#include <src/scripting/scoreboard.h>

// Forward declarations (required for -Wmissing-prototypes)
void test_create_returns_non_null(void);
void test_destroy_null_is_safe(void);
void test_count_on_fresh_is_zero(void);
void test_count_on_null_is_zero(void);
void test_create_destroy_idempotent_caller_managed(void);

void setUp(void) {
}

void tearDown(void) {
}

// create() returns a non-NULL scoreboard
void test_create_returns_non_null(void) {
    Scoreboard* sb = scoreboard_create();
    TEST_ASSERT_NOT_NULL(sb);
    scoreboard_destroy(sb);
}

// destroy(NULL) must be safe
void test_destroy_null_is_safe(void) {
    scoreboard_destroy(NULL);
    // No assertion needed; reaching here is the test
    TEST_ASSERT_TRUE(1);
}

// A fresh scoreboard has 0 entries
void test_count_on_fresh_is_zero(void) {
    Scoreboard* sb = scoreboard_create();
    TEST_ASSERT_NOT_NULL(sb);
    TEST_ASSERT_EQUAL_size_t(0, scoreboard_count(sb));
    scoreboard_destroy(sb);
}

// count(NULL) is 0, not a crash
void test_count_on_null_is_zero(void) {
    TEST_ASSERT_EQUAL_size_t(0, scoreboard_count(NULL));
}

// Calling destroy twice on the same pointer would be a UAF; this test
// documents the *intended* pattern: the caller is responsible for
// NULLing the pointer after destroy. Verifies the helper tolerates
// the post-NULL second call.
void test_create_destroy_idempotent_caller_managed(void) {
    Scoreboard* sb = scoreboard_create();
    TEST_ASSERT_NOT_NULL(sb);
    scoreboard_destroy(sb);
    sb = NULL;
    scoreboard_destroy(sb);   // must be a no-op
    TEST_ASSERT_NULL(sb);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_create_returns_non_null);
    RUN_TEST(test_destroy_null_is_safe);
    RUN_TEST(test_count_on_fresh_is_zero);
    RUN_TEST(test_count_on_null_is_zero);
    RUN_TEST(test_create_destroy_idempotent_caller_managed);

    return UNITY_END();
}
