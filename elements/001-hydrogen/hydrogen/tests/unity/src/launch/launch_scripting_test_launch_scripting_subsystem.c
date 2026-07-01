/*
 * Unity Test File: launch_scripting_test_launch_scripting_subsystem.c
 *
 * Phase 3b of the LUA_PLAN. Tests the actual launch function for the
 * Scripting subsystem. Phase 3b has no side effects beyond initializing
 * subsystem state, so a successful return is the main assertion.
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Module under test
#include <src/launch/launch.h>
#include <src/scripting/scripting.h>

// Forward declaration for the function under test
int launch_scripting_subsystem(void);

// Forward declarations of test functions (required for -Wmissing-prototypes)
void test_launch_scripting_subsystem_with_zeroed_config(void);
void test_launch_scripting_subsystem_idempotent(void);

void setUp(void) {
    // Reset the shutdown flag in case a prior test set it
    scripting_system_shutdown = 1;
}

void tearDown(void) {
    scripting_system_shutdown = 0;
    scripting_orchestrator_state = NULL;
}

// Zeroed config should still succeed (Phase 3b has no per-field checks
// beyond what readiness does; launch is a no-op on the static state)
void test_launch_scripting_subsystem_with_zeroed_config(void) {
    AppConfig* saved = app_config;
    AppConfig mock = {0};
    app_config = &mock;

    int result = launch_scripting_subsystem();

    app_config = saved;

    TEST_ASSERT_EQUAL(1, result);
    TEST_ASSERT_EQUAL(0, scripting_system_shutdown);
    TEST_ASSERT_NULL(scripting_orchestrator_state);
}

// Calling launch twice should be safe (idempotent init)
void test_launch_scripting_subsystem_idempotent(void) {
    AppConfig* saved = app_config;
    AppConfig mock = {0};
    app_config = &mock;

    int r1 = launch_scripting_subsystem();
    int r2 = launch_scripting_subsystem();

    app_config = saved;

    TEST_ASSERT_EQUAL(1, r1);
    TEST_ASSERT_EQUAL(1, r2);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_launch_scripting_subsystem_with_zeroed_config);
    RUN_TEST(test_launch_scripting_subsystem_idempotent);

    return UNITY_END();
}
