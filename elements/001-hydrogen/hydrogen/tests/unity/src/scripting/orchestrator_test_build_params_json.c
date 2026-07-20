/*
 * Unity Test File: orchestrator_test_build_params_json.c
 *
 * Exercises orchestrator_build_params_json, the helper that builds the
 * typed-JSON parameter object for QueryRef #087:
 *   { "STRING": { "GROUP_NAME": "...", "SCRIPT_NAME": "..." } }
 *
 * Focuses on the NULL-argument branches that emit json_null() for the
 * missing parameter (lines 307 and 312), plus the happy path for
 * completeness. The jansson-internal allocation-failure branches
 * (297/301/302) live inside libjansson and cannot be fault-injected
 * from a Unity test, so they remain at the irreducible floor.
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Standard includes
#include <stdlib.h>
#include <string.h>

// Modules under test
#include <src/scripting/orchestrator.h>

// Forward declarations (required for -Wmissing-prototypes)
void test_build_params_json_both_provided(void);
void test_build_params_json_null_group_name(void);
void test_build_params_json_null_script_name(void);

void setUp(void) {
    // No global state to manage for this pure JSON builder.
}

void tearDown(void) {
    // Nothing to clean up between tests.
}

void test_build_params_json_both_provided(void) {
    char* out = orchestrator_build_params_json("Orchestrators", "Orchestrator.lua");
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_NOT_NULL(strstr(out, "\"GROUP_NAME\":\"Orchestrators\""));
    TEST_ASSERT_NOT_NULL(strstr(out, "\"SCRIPT_NAME\":\"Orchestrator.lua\""));
    TEST_ASSERT_NOT_NULL(strstr(out, "\"STRING\""));
    free(out);
}

void test_build_params_json_null_group_name(void) {
    // NULL group_name must serialize GROUP_NAME as JSON null.
    char* out = orchestrator_build_params_json(NULL, "Orchestrator.lua");
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_NOT_NULL(strstr(out, "\"GROUP_NAME\":null"));
    TEST_ASSERT_NOT_NULL(strstr(out, "\"SCRIPT_NAME\":\"Orchestrator.lua\""));
    free(out);
}

void test_build_params_json_null_script_name(void) {
    // NULL script_name must serialize SCRIPT_NAME as JSON null.
    char* out = orchestrator_build_params_json("Orchestrators", NULL);
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_NOT_NULL(strstr(out, "\"GROUP_NAME\":\"Orchestrators\""));
    TEST_ASSERT_NOT_NULL(strstr(out, "\"SCRIPT_NAME\":null"));
    free(out);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_build_params_json_both_provided);
    RUN_TEST(test_build_params_json_null_group_name);
    RUN_TEST(test_build_params_json_null_script_name);

    return UNITY_END();
}
