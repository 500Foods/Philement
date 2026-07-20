/*
 * Unity Test File: orchestrator_test_thread_main.c
 *
 * Targets orchestrator_thread_main (the Orchestrator pthread entry
 * point) and its error branches that the existing success-path tests
 * never reach:
 *
 *   - Early-out when the module state (lua_State / source / chunk
 *     name) was torn down before the thread ran (start() raced with
 *     destroy()). This is reached by calling orchestrator_thread_main
 *     directly while the module-level statics are still NULL.
 *   - Compile-failure branch: a Lua source that does not parse. The
 *     thread logs "failed to compile" and tears the state down.
 *   - Runtime-failure branch: a Lua source that parses but errors on
 *     pcall. The thread logs "returned error" and tears down.
 *
 * The success path (compile + run) is exercised by the blackbox
 * scripting tests and orchestrator_test_load_run.c, so it is not
 * repeated here.
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Standard includes
#include <pthread.h>
#include <string.h>
#include <unistd.h>

// Modules under test
#include <src/scripting/scripting.h>
#include <src/scripting/orchestrator.h>

// Mock log helpers (USE_MOCK_LOGGING is defined by the CMake build)
#include "mock_logging.h"

// Forward declarations (required for -Wmissing-prototypes)
void test_thread_main_null_state_exits_cleanly(void);
void test_thread_main_compile_error_logs_and_exits(void);
void test_thread_main_runtime_error_logs_and_exits(void);

void setUp(void) {
    scripting_system_shutdown = 0;
    scripting_orchestrator_state = NULL;
    memset(&scripting_threads, 0, sizeof(scripting_threads));
    mock_logging_reset_all();
}

void tearDown(void) {
    if (scripting_orchestrator_is_running()) {
        scripting_orchestrator_destroy();
    }
    scripting_orchestrator_state = NULL;
    scripting_system_shutdown = 0;
    memset(&scripting_threads, 0, sizeof(scripting_threads));
}

/*
 * When start() races with destroy(), the thread runs against a NULL
 * lua_State / source / chunk name and must free the (NULL) buffers
 * and return. The module-level statics are NULL at process start and
 * after every destroy(), so calling orchestrator_thread_main directly
 * with no orchestrator ever started exercises the 116-121 guard.
 */
void test_thread_main_null_state_exits_cleanly(void) {
    // No orchestrator has been started in this binary, so the
    // module-level statics (orchestrator_source / state / chunk name)
    // are NULL. orchestrator_thread_main must free(NULL) twice and
    // return NULL without touching the lua_State.
    void* rc = orchestrator_thread_main(NULL);
    TEST_ASSERT_NULL(rc);
    TEST_ASSERT_FALSE(scripting_orchestrator_is_running());
    TEST_ASSERT_NULL(scripting_orchestrator_state);
}

/*
 * A Lua source that fails to compile. start_with_source still succeeds
 * (it only creates the state and spawns the thread); the compile error
 * is reported from the thread, which then destroys the state. We join
 * via destroy() so the error branch is guaranteed to have executed.
 */
void test_thread_main_compile_error_logs_and_exits(void) {
    static const char* BAD = "this is not valid lua (((@@@\n";

    int rc = scripting_orchestrator_start_with_source(
        BAD, "orchestrator_compile_error");
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT_TRUE(scripting_orchestrator_is_running());

    // Give the thread a moment to attempt the compile.
    for (int i = 0; i < 50; i++) {
        if (mock_logging_message_contains("failed to compile")) {
            break;
        }
        usleep(5000);
    }
    TEST_ASSERT_TRUE_MESSAGE(
        mock_logging_message_contains("failed to compile"),
        "compile-failure branch was not exercised");

    scripting_orchestrator_destroy();
    TEST_ASSERT_FALSE(scripting_orchestrator_is_running());
    TEST_ASSERT_NULL(scripting_orchestrator_state);
}

/*
 * A Lua source that compiles but raises at runtime. The pcall branch
 * logs "returned error" and tears the state down.
 */
void test_thread_main_runtime_error_logs_and_exits(void) {
    static const char* BAD = "error('boom')\n";

    int rc = scripting_orchestrator_start_with_source(
        BAD, "orchestrator_runtime_error");
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT_TRUE(scripting_orchestrator_is_running());

    for (int i = 0; i < 50; i++) {
        if (mock_logging_message_contains("returned error")) {
            break;
        }
        usleep(5000);
    }
    TEST_ASSERT_TRUE_MESSAGE(
        mock_logging_message_contains("returned error"),
        "runtime-error (pcall) branch was not exercised");

    scripting_orchestrator_destroy();
    TEST_ASSERT_FALSE(scripting_orchestrator_is_running());
    TEST_ASSERT_NULL(scripting_orchestrator_state);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_thread_main_null_state_exits_cleanly);
    RUN_TEST(test_thread_main_compile_error_logs_and_exits);
    RUN_TEST(test_thread_main_runtime_error_logs_and_exits);

    return UNITY_END();
}
