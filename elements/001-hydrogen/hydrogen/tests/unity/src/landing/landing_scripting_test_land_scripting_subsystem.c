/*
 * Unity Test File: landing_scripting_test_land_scripting_subsystem.c
 *
 * Phase 3b of the LUA_PLAN. Tests the land function for the Scripting
 * subsystem. Phase 3b has no Orchestrator and no workers, so the
 * primary assertion is that the shutdown flag is set and the function
 * returns success.
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Module under test
#include <src/landing/landing.h>
#include <src/scripting/scripting.h>
#include <src/scripting/lua_context.h>

// Forward declaration for the function under test
int land_scripting_subsystem(void);

// Forward declarations of test functions (required for -Wmissing-prototypes)
void test_land_scripting_subsystem_basic(void);
void test_land_scripting_subsystem_idempotent(void);
void test_land_scripting_subsystem_cleans_up_orchestrator(void);

void setUp(void) {
    scripting_system_shutdown = 0;
    scripting_orchestrator_state = NULL;
    memset(&scripting_threads, 0, sizeof(scripting_threads));
}

void tearDown(void) {
    // Reset to a clean baseline
    scripting_system_shutdown = 0;
    scripting_orchestrator_state = NULL;
}

// Basic land: sets shutdown flag, returns success
void test_land_scripting_subsystem_basic(void) {
    int result = land_scripting_subsystem();
    TEST_ASSERT_EQUAL(1, result);
    TEST_ASSERT_EQUAL(1, scripting_system_shutdown);
}

// Land is safe to call repeatedly (idempotent)
void test_land_scripting_subsystem_idempotent(void) {
    int r1 = land_scripting_subsystem();
    int r2 = land_scripting_subsystem();
    TEST_ASSERT_EQUAL(1, r1);
    TEST_ASSERT_EQUAL(1, r2);
    TEST_ASSERT_EQUAL(1, scripting_system_shutdown);
}

// Land cleans up the Orchestrator state if one exists.
// This is the "Phase 3b shutdown is proven" point: create a real Lua
// state, assign it as the orchestrator placeholder, then land and
// verify it has been released.
void test_land_scripting_subsystem_cleans_up_orchestrator(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);
    scripting_orchestrator_state = L;

    int result = land_scripting_subsystem();
    TEST_ASSERT_EQUAL(1, result);
    TEST_ASSERT_NULL(scripting_orchestrator_state);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_land_scripting_subsystem_basic);
    RUN_TEST(test_land_scripting_subsystem_idempotent);
    RUN_TEST(test_land_scripting_subsystem_cleans_up_orchestrator);

    return UNITY_END();
}
