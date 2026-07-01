/*
 * Unity Test File: launch_scripting_test_launch_scripting_subsystem.c
 *
 * Phase 3b of the LUA_PLAN. Tests the actual launch function for the
 * Scripting subsystem. Phase 3b had no side effects beyond initializing
 * subsystem state; Phase 7 added the worker pool, so the tests now
 * also set a valid WorkerCount on the mock config (the launch
 * readiness check normally enforces 1..MAX_CONCURRENT_JOBS, but the
 * test bypasses readiness to test launch directly).
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Module under test
#include <src/launch/launch.h>
#include <src/scripting/scripting.h>
#include <src/scripting/scoreboard.h>
#include <src/scripting/worker_pool.h>

// Forward declaration for the function under test
int launch_scripting_subsystem(void);

// Forward declarations of test functions (required for -Wmissing-prototypes)
void test_launch_scripting_subsystem_with_valid_config(void);
void test_launch_scripting_subsystem_idempotent(void);

void setUp(void) {
    // Reset the shutdown flag in case a prior test set it
    scripting_system_shutdown = 1;
}

void tearDown(void) {
    // Phase 5: scripting_init_state allocates a scoreboard; destroy it
    // here so the scoreboard doesn't leak between tests in this
    // executable.
    scripting_workers_destroy();
    scripting_cleanup_state();
    scripting_system_shutdown = 0;
    scripting_orchestrator_state = NULL;
}

// A config with the default WorkerCount (2) and Scripting enabled
// should succeed. The launch readiness check normally enforces the
// 1..MAX_CONCURRENT_JOBS range, but the test calls launch directly,
// so we set a known-good value here.
void test_launch_scripting_subsystem_with_valid_config(void) {
    AppConfig* saved = app_config;
    AppConfig mock = {0};
    mock.scripting.Enabled = true;
    mock.scripting.WorkerCount = 2;
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
    mock.scripting.Enabled = true;
    mock.scripting.WorkerCount = 2;
    app_config = &mock;

    int r1 = launch_scripting_subsystem();
    int r2 = launch_scripting_subsystem();

    app_config = saved;

    TEST_ASSERT_EQUAL(1, r1);
    TEST_ASSERT_EQUAL(1, r2);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_launch_scripting_subsystem_with_valid_config);
    RUN_TEST(test_launch_scripting_subsystem_idempotent);

    return UNITY_END();
}
