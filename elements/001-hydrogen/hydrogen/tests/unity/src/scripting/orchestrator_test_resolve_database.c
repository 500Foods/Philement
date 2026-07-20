/*
 * Unity Test File: orchestrator_test_resolve_database.c
 *
 * Directly exercises orchestrator_resolve_database, the shared
 * database-disambiguation helper, covering the branches that the
 * blackbox tests do not: no app_config at all, the single-configured-
 * database fallback (with its "using the single configured database"
 * log), the ambiguous multi-database case, and the explicit
 * DefaultDatabase path.
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

// Forward declarations (required for -Wmissing-prototypes)
void test_resolve_database_no_app_config(void);
void test_resolve_database_explicit_default(void);
void test_resolve_database_single_database_fallback(void);
void test_resolve_database_multiple_databases_ambiguous(void);

void setUp(void) {
    mock_logging_reset_all();
}

void tearDown(void) {
    mock_logging_reset_all();
}

void test_resolve_database_no_app_config(void) {
    AppConfig* saved = app_config;
    app_config = NULL;

    const char* db = orchestrator_resolve_database();

    app_config = saved;

    TEST_ASSERT_NULL(db);
    TEST_ASSERT_TRUE(mock_logging_message_contains("no DefaultDatabase configured"));
}

void test_resolve_database_explicit_default(void) {
    AppConfig* saved = app_config;
    AppConfig mock = {0};
    mock.scripting.Enabled = true;
    mock.scripting.DefaultDatabase = strdup("ExplicitDB");
    app_config = &mock;

    const char* db = orchestrator_resolve_database();

    TEST_ASSERT_EQUAL_STRING("ExplicitDB", db);
    app_config = saved;
    free(mock.scripting.DefaultDatabase);
}

void test_resolve_database_single_database_fallback(void) {
    AppConfig* saved = app_config;
    AppConfig mock = {0};
    mock.scripting.Enabled = true;
    mock.scripting.DefaultDatabase = NULL; // not configured
    mock.databases.connection_count = 1;
    mock.databases.connections[0].name = strdup("SoloDB");
    app_config = &mock;

    const char* db = orchestrator_resolve_database();

    TEST_ASSERT_EQUAL_STRING("SoloDB", db);
    TEST_ASSERT_TRUE(mock_logging_message_contains("using the single configured database"));
    app_config = saved;
    free(mock.databases.connections[0].name);
}

void test_resolve_database_multiple_databases_ambiguous(void) {
    AppConfig* saved = app_config;
    AppConfig mock = {0};
    mock.scripting.Enabled = true;
    mock.scripting.DefaultDatabase = NULL; // not configured
    mock.databases.connection_count = 2;
    mock.databases.connections[0].name = strdup("FirstDB");
    mock.databases.connections[1].name = strdup("SecondDB");
    app_config = &mock;

    const char* db = orchestrator_resolve_database();

    app_config = saved;
    free(mock.databases.connections[0].name);
    free(mock.databases.connections[1].name);

    TEST_ASSERT_NULL(db);
    TEST_ASSERT_TRUE(mock_logging_message_contains("no DefaultDatabase configured"));
    TEST_ASSERT_FALSE(mock_logging_message_contains("using the single configured database"));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_resolve_database_no_app_config);
    RUN_TEST(test_resolve_database_explicit_default);
    RUN_TEST(test_resolve_database_single_database_fallback);
    RUN_TEST(test_resolve_database_multiple_databases_ambiguous);

    return UNITY_END();
}
