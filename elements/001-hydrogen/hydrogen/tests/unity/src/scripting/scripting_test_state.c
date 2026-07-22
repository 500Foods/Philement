/*
 * Unity Test File: scripting_test_state.c
 *
 * Records unit-test coverage specifically against
 * src/scripting/scripting.c. Exercises the two public helpers that own
 * the scripting subsystem's static state:
 *   - scripting_init_state()
 *   - scripting_cleanup_state()
 *
 * The scoreboard and source-cache pointers are owned here, so this test
 * also verifies the create/destroy lifecycle of those owned resources and
 * the orchestrator-state teardown branch (scripting.c lines 62-64) by
 * installing a live lua_State before cleanup.
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Third-party includes
#include <lua.h>

// Module under test (state helpers live here)
#include <src/scripting/scripting.h>
#include <src/scripting/lua_context.h>

// Forward declarations
void test_init_state_allocates_owned_resources(void);
void test_init_state_is_idempotent_on_shutdown_flag(void);
void test_cleanup_state_frees_owned_resources(void);
void test_cleanup_state_destroys_orchestrator_context(void);
void test_init_then_cleanup_roundtrip(void);

void setUp(void) {
    // Start each test from a fully torn-down state so global pointers
    // do not leak between cases. scripting_cleanup_state tolerates NULL
    // pointers, so this is safe even on the first run.
    scripting_orchestrator_state = NULL;
    scripting_scoreboard = NULL;
    scripting_source_cache = NULL;
    scripting_system_shutdown = 1;
}

void tearDown(void) {
    // Guarantee a clean global slate after every case.
    scripting_orchestrator_state = NULL;
    scripting_scoreboard = NULL;
    scripting_source_cache = NULL;
    scripting_system_shutdown = 1;
}

// scripting_init_state must allocate the scoreboard and source cache the
// first time it runs and must clear the subsystem shutdown flag.
void test_init_state_allocates_owned_resources(void) {
    scripting_init_state();

    TEST_ASSERT_EQUAL(0, scripting_system_shutdown);
    TEST_ASSERT_NOT_NULL(scripting_scoreboard);
    TEST_ASSERT_NOT_NULL(scripting_source_cache);
    TEST_ASSERT_NULL(scripting_orchestrator_state);

    scripting_cleanup_state();
}

// The shutdown flag must be cleared to 0 even if it started as 1, which
// confirms init_state sets it rather than leaving the prior value.
void test_init_state_is_idempotent_on_shutdown_flag(void) {
    scripting_system_shutdown = 1;

    scripting_init_state();

    TEST_ASSERT_EQUAL(0, scripting_system_shutdown);

    scripting_cleanup_state();
}

// scripting_cleanup_state must release the scoreboard and source cache and
// raise the shutdown flag. This covers the scoreboard/source-cache teardown
// branches of scripting.c.
void test_cleanup_state_frees_owned_resources(void) {
    scripting_init_state();
    TEST_ASSERT_NOT_NULL(scripting_scoreboard);
    TEST_ASSERT_NOT_NULL(scripting_source_cache);

    scripting_cleanup_state();

    TEST_ASSERT_NULL(scripting_scoreboard);
    TEST_ASSERT_NULL(scripting_source_cache);
    TEST_ASSERT_EQUAL(1, scripting_system_shutdown);
}

// Cover scripting.c lines 62-64: when a live orchestrator context is
// present, cleanup must destroy it via H_lua_destroy_context and reset the
// pointer. The orchestrator branch is otherwise never hit in Phase 3b.
void test_cleanup_state_destroys_orchestrator_context(void) {
    scripting_init_state();

    scripting_orchestrator_state = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(scripting_orchestrator_state);

    scripting_cleanup_state();

    TEST_ASSERT_NULL(scripting_orchestrator_state);
}

// Full lifecycle: init, install an orchestrator context, destroy it through
// cleanup, then re-init to confirm the owned resources come back and the
// shutdown flag is cleared again.
void test_init_then_cleanup_roundtrip(void) {
    scripting_init_state();
    TEST_ASSERT_NOT_NULL(scripting_scoreboard);
    TEST_ASSERT_NOT_NULL(scripting_source_cache);

    scripting_orchestrator_state = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(scripting_orchestrator_state);

    scripting_cleanup_state();

    TEST_ASSERT_NULL(scripting_orchestrator_state);
    TEST_ASSERT_NULL(scripting_scoreboard);
    TEST_ASSERT_NULL(scripting_source_cache);
    TEST_ASSERT_EQUAL(1, scripting_system_shutdown);

    // Re-initialize: owned resources are recreated and the flag cleared.
    scripting_init_state();
    TEST_ASSERT_NOT_NULL(scripting_scoreboard);
    TEST_ASSERT_NOT_NULL(scripting_source_cache);
    TEST_ASSERT_EQUAL(0, scripting_system_shutdown);

    scripting_cleanup_state();
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_init_state_allocates_owned_resources);
    RUN_TEST(test_init_state_is_idempotent_on_shutdown_flag);
    RUN_TEST(test_cleanup_state_frees_owned_resources);
    RUN_TEST(test_cleanup_state_destroys_orchestrator_context);
    RUN_TEST(test_init_then_cleanup_roundtrip);

    return UNITY_END();
}
