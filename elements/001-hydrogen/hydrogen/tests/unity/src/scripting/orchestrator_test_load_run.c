/*
 * Unity Test File: orchestrator_test_load_run.c
 *
 * Phase 11d of the LUA_PLAN. Validates the Orchestrator's
 * scripting_orchestrator_start_with_source primitive end-to-end:
 *
 *   - A trivial in-memory Lua source is compiled and run on the
 *     orchestrator's dedicated pthread.
 *   - is_running() returns true while the script is alive.
 *   - The mock log subsystem records a "started" line emitted by
 *     the script (proves the lua_State is the one the orchestrator
 *     created, with the H.* host API installed).
 *   - scripting_orchestrator_destroy() wakes the script, joins the
 *     pthread, and tears the state down. is_running() returns
 *     false within a bounded time.
 *   - Both start and destroy are idempotent.
 *
 * Phase 11d deliberately ships the in-memory primitive as the test
 * path; the production path (`_from_db`) is exercised by the
 * database integration tests, not by Unity.
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Standard includes
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// Modules under test
#include <src/scripting/scripting.h>
#include <src/scripting/orchestrator.h>

// Mock log helpers (USE_MOCK_LOGGING is defined by the CMake build)
#include "mock_logging.h"

// Forward declarations (required for -Wmissing-prototypes)
void test_start_with_source_runs_script(void);
void test_start_with_source_is_idempotent(void);
void test_start_with_null_args_fails(void);
void test_destroy_when_not_running_is_safe(void);

void setUp(void) {
    // Reset scripting subsystem state. orchestrator.c uses the
    // Phase 3b globals (scripting_orchestrator_state,
    // scripting_system_shutdown) plus the ServiceThreads record
    // for thread registration, so we touch all three.
    scripting_system_shutdown = 0;
    scripting_orchestrator_state = NULL;
    memset(&scripting_threads, 0, sizeof(scripting_threads));
    mock_logging_reset_all();
}

void tearDown(void) {
    // Defensive: if a test left an orchestrator running, shut it
    // down so the next setUp has a clean slate.
    if (scripting_orchestrator_is_running()) {
        scripting_orchestrator_destroy();
    }
    scripting_orchestrator_state = NULL;
    scripting_system_shutdown = 0;
    memset(&scripting_threads, 0, sizeof(scripting_threads));
}

// A trivial in-memory orchestrator source. The "hello" log line
// is the marker the tests use to confirm the script is running on
// the orchestrator's lua_State (the mock log captures it).
static const char* ORCH_SCRIPT = "H.log.info('orchestrator-test: started')\n"
                                  "while not H.shutdown_requested() do\n"
                                  "  H.sleep(10)\n"
                                  "end\n"
                                  "H.log.info('orchestrator-test: stopped')\n";

void test_start_with_source_runs_script(void) {
    int rc = scripting_orchestrator_start_with_source(
        ORCH_SCRIPT, "orchestrator_test_load_run");
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT_TRUE(scripting_orchestrator_is_running());
    TEST_ASSERT_NOT_NULL(scripting_orchestrator_state);

    // Give the script a moment to compile, run, and log "started".
    // The mock log captures the call even if the lua_State has not
    // been destroyed yet; the test does not race the destroy path.
    for (int i = 0; i < 50; i++) {  // up to ~250 ms
        if (mock_logging_get_call_count() > 0) {
            break;
        }
        usleep(5000);
    }
    TEST_ASSERT_GREATER_THAN(0, mock_logging_get_call_count());
    const char* last_msg = mock_logging_get_last_message();
    TEST_ASSERT_NOT_NULL(last_msg);
    TEST_ASSERT_NOT_NULL(strstr(last_msg, "started"));
}

void test_start_with_source_is_idempotent(void) {
    int rc1 = scripting_orchestrator_start_with_source(
        ORCH_SCRIPT, "orchestrator_test_idempotent_first");
    int rc2 = scripting_orchestrator_start_with_source(
        ORCH_SCRIPT, "orchestrator_test_idempotent_second");
    TEST_ASSERT_EQUAL(1, rc1);
    TEST_ASSERT_EQUAL(1, rc2); // second call is a no-op
    TEST_ASSERT_TRUE(scripting_orchestrator_is_running());

    scripting_orchestrator_destroy();
    TEST_ASSERT_FALSE(scripting_orchestrator_is_running());
}

void test_start_with_null_args_fails(void) {
    int rc1 = scripting_orchestrator_start_with_source(NULL, "x");
    int rc2 = scripting_orchestrator_start_with_source("return 1\n", NULL);
    TEST_ASSERT_EQUAL(0, rc1);
    TEST_ASSERT_EQUAL(0, rc2);
    TEST_ASSERT_FALSE(scripting_orchestrator_is_running());
}

void test_destroy_when_not_running_is_safe(void) {
    // No-op path: destroy without a running orchestrator must
    // not crash and must not flip the global state.
    scripting_orchestrator_destroy();
    scripting_orchestrator_destroy();
    TEST_ASSERT_FALSE(scripting_orchestrator_is_running());
    TEST_ASSERT_NULL(scripting_orchestrator_state);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_start_with_source_runs_script);
    RUN_TEST(test_start_with_source_is_idempotent);
    RUN_TEST(test_start_with_null_args_fails);
    RUN_TEST(test_destroy_when_not_running_is_safe);

    return UNITY_END();
}
