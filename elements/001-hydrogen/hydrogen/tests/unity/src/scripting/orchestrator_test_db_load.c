/*
 * Unity Test File: orchestrator_test_db_load.c
 *
 * Phase 11f of the LUA_PLAN. Validates the Orchestrator's
 * production load path without requiring a live database connection:
 *
 *   - scripting_orchestrator_start_from_db rejects NULL inputs.
 *   - scripting_orchestrator_start_from_db is idempotent when an
 *     orchestrator is already running.
 *   - scripting_orchestrator_start_from_db returns false and logs a
 *     clear message when config->scripting.DefaultDatabase is empty.
 *   - scripting_orchestrator_load_configured respects the Enabled flag,
 *     handles missing/invalid Orchestrator names, and passes the
 *     dot-form "group.script" name to _from_db.
 *
 * The full end-to-end DB fetch is exercised by the blackbox test
 * test_43_scripting.sh (Phase 11h); this Unity file covers the
 * decision paths and error handling that are awkward to reach from
 * a shell script.
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Standard includes
#include <string.h>

// Modules under test
#include <src/scripting/scripting.h>
#include <src/scripting/orchestrator.h>

// Mock log helpers (USE_MOCK_LOGGING is defined by the CMake build)
#include "mock_logging.h"

// Forward declarations (required for -Wmissing-prototypes)
void test_start_from_db_null_inputs_fail(void);
void test_start_from_db_idempotent_when_running(void);
void test_start_from_db_requires_default_database(void);
void test_load_configured_disabled_does_nothing(void);
void test_load_configured_no_orchestrator_logs_idle(void);
void test_load_configured_invalid_name_logs_error(void);
void test_load_configured_missing_default_database_logs_clearly(void);
void test_start_from_db_falls_back_to_single_database(void);
void test_start_from_db_multiple_databases_still_requires_default(void);

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

void test_start_from_db_null_inputs_fail(void) {
    bool rc1 = scripting_orchestrator_start_from_db(NULL, "script");
    bool rc2 = scripting_orchestrator_start_from_db("group", NULL);
    TEST_ASSERT_FALSE(rc1);
    TEST_ASSERT_FALSE(rc2);
    TEST_ASSERT_FALSE(scripting_orchestrator_is_running());
}

void test_start_from_db_idempotent_when_running(void) {
    static const char* SCRIPT = "while not H.shutdown_requested() do H.sleep(10) end\n";
    bool rc1 = scripting_orchestrator_start_with_source(SCRIPT, "already_running");
    TEST_ASSERT_TRUE(rc1);
    TEST_ASSERT_TRUE(scripting_orchestrator_is_running());

    bool rc2 = scripting_orchestrator_start_from_db("Orchestrators", "Orchestrator");
    TEST_ASSERT_TRUE(rc2); // idempotent: already running
}

void test_start_from_db_requires_default_database(void) {
    AppConfig* saved = app_config;
    AppConfig mock = {0};
    mock.scripting.Enabled = true;
    mock.scripting.DefaultDatabase = NULL; // empty / not configured
    app_config = &mock;

    bool rc = scripting_orchestrator_start_from_db("Orchestrators", "Orchestrator");

    app_config = saved;

    TEST_ASSERT_FALSE(rc);
    TEST_ASSERT_FALSE(scripting_orchestrator_is_running());
    TEST_ASSERT_GREATER_THAN(0, mock_logging_get_call_count());
    TEST_ASSERT_TRUE(mock_logging_message_contains("no DefaultDatabase configured"));
}

void test_load_configured_disabled_does_nothing(void) {
    AppConfig* saved = app_config;
    AppConfig mock = {0};
    mock.scripting.Enabled = false;
    app_config = &mock;

    scripting_orchestrator_load_configured();

    app_config = saved;

    TEST_ASSERT_FALSE(scripting_orchestrator_is_running());
    TEST_ASSERT_EQUAL(0, mock_logging_get_call_count());
}

void test_load_configured_no_orchestrator_logs_idle(void) {
    AppConfig* saved = app_config;
    AppConfig mock = {0};
    mock.scripting.Enabled = true;
    mock.scripting.DefaultDatabase = strdup("TestDB");
    // Orchestrator intentionally left NULL
    app_config = &mock;

    scripting_orchestrator_load_configured();

    app_config = saved;
    free(mock.scripting.DefaultDatabase);

    TEST_ASSERT_FALSE(scripting_orchestrator_is_running());
    TEST_ASSERT_GREATER_THAN(0, mock_logging_get_call_count());
    TEST_ASSERT_TRUE(mock_logging_message_contains("No Orchestrator configured"));
}

void test_load_configured_invalid_name_logs_error(void) {
    AppConfig* saved = app_config;
    AppConfig mock = {0};
    mock.scripting.Enabled = true;
    mock.scripting.Orchestrator = strdup("no-dot-here");
    mock.scripting.DefaultDatabase = strdup("TestDB");
    app_config = &mock;

    scripting_orchestrator_load_configured();

    app_config = saved;
    free(mock.scripting.Orchestrator);
    free(mock.scripting.DefaultDatabase);

    TEST_ASSERT_FALSE(scripting_orchestrator_is_running());
    TEST_ASSERT_GREATER_THAN(0, mock_logging_get_call_count());
    TEST_ASSERT_TRUE(mock_logging_message_contains("Invalid Orchestrator name"));
}

void test_load_configured_missing_default_database_logs_clearly(void) {
    AppConfig* saved = app_config;
    AppConfig mock = {0};
    mock.scripting.Enabled = true;
    mock.scripting.Orchestrator = strdup("Orchestrators.Orchestrator");
    // DefaultDatabase intentionally left NULL
    app_config = &mock;

    scripting_orchestrator_load_configured();

    app_config = saved;
    free(mock.scripting.Orchestrator);

    TEST_ASSERT_FALSE(scripting_orchestrator_is_running());
    TEST_ASSERT_GREATER_THAN(0, mock_logging_get_call_count());
    TEST_ASSERT_TRUE(mock_logging_message_contains("no DefaultDatabase configured"));
}

void test_start_from_db_falls_back_to_single_database(void) {
    // With no DefaultDatabase but exactly one configured database, the
    // Orchestrator resolves the single database automatically. The DB
    // fetch then fails (no live DB in a Unity test), so the function
    // still returns false, but the "using the single configured
    // database" fallback message proves the disambiguation happened
    // instead of the "will not start" early-out.
    AppConfig* saved = app_config;
    AppConfig mock = {0};
    mock.scripting.Enabled = true;
    mock.scripting.DefaultDatabase = NULL; // not configured
    mock.databases.connection_count = 1;
    mock.databases.connections[0].name = strdup("SoloDB");
    app_config = &mock;

    bool rc = scripting_orchestrator_start_from_db("Orchestrators", "Orchestrator");

    app_config = saved;
    free(mock.databases.connections[0].name);

    TEST_ASSERT_FALSE(rc); // no live DB, so the fetch fails
    TEST_ASSERT_FALSE(scripting_orchestrator_is_running());
    TEST_ASSERT_TRUE(mock_logging_message_contains("using the single configured database"));
    TEST_ASSERT_FALSE(mock_logging_message_contains("Orchestrator will not start"));
}

void test_start_from_db_multiple_databases_still_requires_default(void) {
    // With two or more configured databases and no DefaultDatabase, the
    // choice is ambiguous, so the Orchestrator refuses to start.
    AppConfig* saved = app_config;
    AppConfig mock = {0};
    mock.scripting.Enabled = true;
    mock.scripting.DefaultDatabase = NULL;
    mock.databases.connection_count = 2;
    mock.databases.connections[0].name = strdup("FirstDB");
    mock.databases.connections[1].name = strdup("SecondDB");
    app_config = &mock;

    bool rc = scripting_orchestrator_start_from_db("Orchestrators", "Orchestrator");

    app_config = saved;
    free(mock.databases.connections[0].name);
    free(mock.databases.connections[1].name);

    TEST_ASSERT_FALSE(rc);
    TEST_ASSERT_FALSE(scripting_orchestrator_is_running());
    TEST_ASSERT_TRUE(mock_logging_message_contains("no DefaultDatabase configured"));
    TEST_ASSERT_FALSE(mock_logging_message_contains("using the single configured database"));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_start_from_db_null_inputs_fail);
    RUN_TEST(test_start_from_db_idempotent_when_running);
    RUN_TEST(test_start_from_db_requires_default_database);
    RUN_TEST(test_load_configured_disabled_does_nothing);
    RUN_TEST(test_load_configured_no_orchestrator_logs_idle);
    RUN_TEST(test_load_configured_invalid_name_logs_error);
    RUN_TEST(test_load_configured_missing_default_database_logs_clearly);
    RUN_TEST(test_start_from_db_falls_back_to_single_database);
    RUN_TEST(test_start_from_db_multiple_databases_still_requires_default);

    return UNITY_END();
}
