/*
 * Unity Test File: orchestrator_test_load_configured_blocking.c
 *
 * Exercises orchestrator_load_configured_blocking, the synchronous
 * body of the configured-Orchestrator load (normally run on the
 * loader thread). The blackbox and existing Unity tests only reach
 * this through the async loader, so its early-return guards and the
 * "not loaded" hand-off are uncovered there. This file drives them
 * directly without a live database:
 *
 *   - app_config NULL or scripting disabled -> silent early return
 *   - Orchestrator name missing/empty -> silent early return
 *   - Orchestrator name with no '.' -> silent early return
 *   - group malloc failure (via mock_system) -> silent early return
 *   - valid name but unresolvable database -> start_from_db returns
 *     false and the "not loaded" branch logs (no DB fetch, so no hang)
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Standard includes
#include <stdlib.h>
#include <string.h>

// Modules under test
#include <src/scripting/orchestrator.h>
#include <src/scripting/scripting.h>

// Mock log helpers (USE_MOCK_LOGGING is defined by the CMake build)
#include "mock_logging.h"

// USE_MOCK_SYSTEM is a global Unity define; the explicit #define matches
// the CMake -D so the mock function prototypes below are visible.
#define USE_MOCK_SYSTEM
#include "mock_system.h"

// Forward declarations (required for -Wmissing-prototypes)
void test_load_blocking_null_app_config(void);
void test_load_blocking_no_orchestrator_name(void);
void test_load_blocking_name_without_dot(void);
void test_load_blocking_group_allocation_failure(void);
void test_load_blocking_start_fails_logs_not_loaded(void);

void setUp(void) {
    scripting_orchestrator_state = NULL;
    scripting_system_shutdown = 0;
    mock_logging_reset_all();
    mock_system_reset_all();
}

void tearDown(void) {
    mock_system_reset_all();
    mock_logging_reset_all();
    scripting_orchestrator_state = NULL;
    scripting_system_shutdown = 0;
}

void test_load_blocking_null_app_config(void) {
    AppConfig* saved = app_config;
    app_config = NULL;

    orchestrator_load_configured_blocking();

    app_config = saved;
    TEST_ASSERT_FALSE(scripting_orchestrator_is_running());
}

void test_load_blocking_no_orchestrator_name(void) {
    AppConfig* saved = app_config;
    AppConfig mock = {0};
    mock.scripting.Enabled = true;
    mock.scripting.Orchestrator = NULL; // missing name
    app_config = &mock;

    orchestrator_load_configured_blocking();

    app_config = saved;
    TEST_ASSERT_FALSE(scripting_orchestrator_is_running());
}

void test_load_blocking_name_without_dot(void) {
    AppConfig* saved = app_config;
    AppConfig mock = {0};
    mock.scripting.Enabled = true;
    mock.scripting.Orchestrator = strdup("no-dot-name"); // not group.script
    app_config = &mock;

    orchestrator_load_configured_blocking();

    app_config = saved;
    free(mock.scripting.Orchestrator);
    TEST_ASSERT_FALSE(scripting_orchestrator_is_running());
}

void test_load_blocking_group_allocation_failure(void) {
    AppConfig* saved = app_config;
    AppConfig mock = {0};
    mock.scripting.Enabled = true;
    mock.scripting.Orchestrator = strdup("Group.Script"); // valid dotted name
    app_config = &mock;

    // The only heap allocation before the start attempt is the group
    // copy (malloc(group_len + 1)). Force that first post-reset
    // allocation to fail; the helper must free nothing extra and return.
    // The strdup above happened before the failure is armed, so it does
    // not consume the failure counter.
    mock_system_set_malloc_failure(1);

    orchestrator_load_configured_blocking();

    mock_system_reset_all();
    app_config = saved;
    free(mock.scripting.Orchestrator);
    TEST_ASSERT_FALSE(scripting_orchestrator_is_running());
}

void test_load_blocking_start_fails_logs_not_loaded(void) {
    // Valid dotted name but an unresolvable database: resolve_database
    // returns NULL, start_from_db returns false without touching the
    // DB, and the "not loaded" branch is logged. No query is submitted,
    // so the test cannot hang on a pending result.
    AppConfig* saved = app_config;
    AppConfig mock = {0};
    mock.scripting.Enabled = true;
    mock.scripting.Orchestrator = strdup("Group.Script");
    mock.scripting.DefaultDatabase = NULL; // not configured
    mock.databases.connection_count = 0;   // not the single-DB fallback
    app_config = &mock;

    orchestrator_load_configured_blocking();

    app_config = saved;
    free(mock.scripting.Orchestrator);
    TEST_ASSERT_FALSE(scripting_orchestrator_is_running());
    TEST_ASSERT_TRUE(mock_logging_message_contains("not loaded"));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_load_blocking_null_app_config);
    RUN_TEST(test_load_blocking_no_orchestrator_name);
    RUN_TEST(test_load_blocking_name_without_dot);
    RUN_TEST(test_load_blocking_group_allocation_failure);
    RUN_TEST(test_load_blocking_start_fails_logs_not_loaded);

    return UNITY_END();
}
