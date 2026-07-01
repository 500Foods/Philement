/*
 * Unity Test File: landing_scripting_test_check_scripting_landing_readiness.c
 *
 * Phase 3b of the LUA_PLAN. Tests the landing readiness check for the
 * Scripting subsystem. Phase 3b has no Orchestrator and no worker
 * pool, so the running case must always report ready.
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Module under test
#include <src/landing/landing.h>
#include <src/state/state_types.h>
#include <src/scripting/scripting.h>

// Forward declaration for the function under test
LaunchReadiness check_scripting_landing_readiness(void);

// Forward declarations of test functions (required for -Wmissing-prototypes)
void test_check_scripting_landing_readiness_not_running(void);
void test_check_scripting_landing_readiness_running_idle(void);

// Mock state for the running-by-name function
static bool mock_subsystem_running = false;

__attribute__((weak)) bool is_subsystem_running_by_name(const char* name) {
    (void)name;
    return mock_subsystem_running;
}

void setUp(void) {
    mock_subsystem_running = false;
    scripting_system_shutdown = 0;
    scripting_orchestrator_state = NULL;
    // Force a known-zero thread count
    memset(&scripting_threads, 0, sizeof(scripting_threads));
}

void tearDown(void) {
    // No global state to reset
}

// Not running -> ready=true (landing a non-running subsystem is a no-op)
void test_check_scripting_landing_readiness_not_running(void) {
    mock_subsystem_running = false;
    LaunchReadiness r = check_scripting_landing_readiness();
    TEST_ASSERT_NOT_NULL(r.messages);
    TEST_ASSERT_EQUAL_STRING(SR_SCRIPTING, r.subsystem);
    TEST_ASSERT_TRUE(r.ready);
    free_readiness_messages(&r);
}

// Running with no orchestrator and no workers -> ready=true
void test_check_scripting_landing_readiness_running_idle(void) {
    mock_subsystem_running = true;
    scripting_orchestrator_state = NULL;
    scripting_threads.thread_count = 0;

    LaunchReadiness r = check_scripting_landing_readiness();
    TEST_ASSERT_NOT_NULL(r.messages);
    TEST_ASSERT_TRUE(r.ready);
    free_readiness_messages(&r);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_check_scripting_landing_readiness_not_running);
    RUN_TEST(test_check_scripting_landing_readiness_running_idle);

    return UNITY_END();
}
