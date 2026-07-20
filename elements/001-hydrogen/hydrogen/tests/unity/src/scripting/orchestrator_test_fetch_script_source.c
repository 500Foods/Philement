/*
 * Unity Test File: orchestrator_test_fetch_script_source.c
 *
 * Exercises the argument-validation guard at the top of
 * scripting_fetch_script_source. These branches (lines 384-388) bail
 * out before any database infrastructure is touched, so they are
 * reachable from a Unity test without a live database. The deeper
 * DB-queue / pending-result failure paths require a live backend and
 * are covered by the blackbox scripting tests.
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
void test_fetch_script_source_null_group(void);
void test_fetch_script_source_null_script(void);
void test_fetch_script_source_null_database(void);
void test_fetch_script_source_zero_timeout(void);

void setUp(void) {
    mock_logging_reset_all();
}

void tearDown(void) {
    mock_logging_reset_all();
}

void test_fetch_script_source_null_group(void) {
    char* code = scripting_fetch_script_source(NULL, "script", "TestDB", 5);
    TEST_ASSERT_NULL(code);
    TEST_ASSERT_TRUE(mock_logging_message_contains("invalid arguments"));
}

void test_fetch_script_source_null_script(void) {
    char* code = scripting_fetch_script_source("group", NULL, "TestDB", 5);
    TEST_ASSERT_NULL(code);
    TEST_ASSERT_TRUE(mock_logging_message_contains("invalid arguments"));
}

void test_fetch_script_source_null_database(void) {
    char* code = scripting_fetch_script_source("group", "script", NULL, 5);
    TEST_ASSERT_NULL(code);
    TEST_ASSERT_TRUE(mock_logging_message_contains("invalid arguments"));
}

void test_fetch_script_source_zero_timeout(void) {
    char* code = scripting_fetch_script_source("group", "script", "TestDB", 0);
    TEST_ASSERT_NULL(code);
    TEST_ASSERT_TRUE(mock_logging_message_contains("invalid arguments"));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_fetch_script_source_null_group);
    RUN_TEST(test_fetch_script_source_null_script);
    RUN_TEST(test_fetch_script_source_null_database);
    RUN_TEST(test_fetch_script_source_zero_timeout);

    return UNITY_END();
}
