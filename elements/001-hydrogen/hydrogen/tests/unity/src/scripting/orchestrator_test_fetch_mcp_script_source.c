/*
 * Unity Test File: orchestrator_test_fetch_mcp_script_source.c
 *
 * Argument-validation for scripting_fetch_mcp_script_source (QueryRef #153).
 * Same guard as scripting_fetch_script_source; no live database.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <stdlib.h>
#include <string.h>

#include <src/scripting/orchestrator.h>
#include <src/scripting/scripting.h>

#include "mock_logging.h"

void test_fetch_mcp_script_source_null_group(void);
void test_fetch_mcp_script_source_null_script(void);
void test_fetch_mcp_script_source_null_database(void);
void test_fetch_mcp_script_source_zero_timeout(void);

void setUp(void) {
    mock_logging_reset_all();
}

void tearDown(void) {
    mock_logging_reset_all();
}

void test_fetch_mcp_script_source_null_group(void) {
    char *code = scripting_fetch_mcp_script_source(NULL, "script", "TestDB", 5);
    TEST_ASSERT_NULL(code);
    TEST_ASSERT_TRUE(mock_logging_message_contains("invalid arguments"));
}

void test_fetch_mcp_script_source_null_script(void) {
    char *code = scripting_fetch_mcp_script_source("group", NULL, "TestDB", 5);
    TEST_ASSERT_NULL(code);
    TEST_ASSERT_TRUE(mock_logging_message_contains("invalid arguments"));
}

void test_fetch_mcp_script_source_null_database(void) {
    char *code = scripting_fetch_mcp_script_source("group", "script", NULL, 5);
    TEST_ASSERT_NULL(code);
    TEST_ASSERT_TRUE(mock_logging_message_contains("invalid arguments"));
}

void test_fetch_mcp_script_source_zero_timeout(void) {
    char *code = scripting_fetch_mcp_script_source("group", "script", "TestDB", 0);
    TEST_ASSERT_NULL(code);
    TEST_ASSERT_TRUE(mock_logging_message_contains("invalid arguments"));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_fetch_mcp_script_source_null_group);
    RUN_TEST(test_fetch_mcp_script_source_null_script);
    RUN_TEST(test_fetch_mcp_script_source_null_database);
    RUN_TEST(test_fetch_mcp_script_source_zero_timeout);

    return UNITY_END();
}
