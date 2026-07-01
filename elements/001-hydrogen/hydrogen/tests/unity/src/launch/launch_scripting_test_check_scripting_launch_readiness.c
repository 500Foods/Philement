/*
 * Unity Test File: launch_scripting_test_check_scripting_launch_readiness.c
 *
 * Phase 3b of the LUA_PLAN. Tests the readiness check that gates the
 * Scripting subsystem's launch.
 *
 * Cases:
 *   - Disabled in config -> ready=false, message says "disabled"
 *   - Enabled with default worker count -> ready=true
 *   - Enabled but WorkerCount out of range -> ready=false
 *   - app_config NULL -> ready=false
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Module under test
#include <src/launch/launch.h>

// Forward declaration for the function under test
LaunchReadiness check_scripting_launch_readiness(void);

// Forward declarations of test functions (required for -Wmissing-prototypes)
void test_check_scripting_launch_readiness_disabled(void);
void test_check_scripting_launch_readiness_enabled(void);
void test_check_scripting_launch_readiness_worker_count_too_high(void);
void test_check_scripting_launch_readiness_worker_count_zero(void);

// Mock state
static AppConfig mock_config = {0};

void setUp(void) {
    memset(&mock_config, 0, sizeof(mock_config));
    // Provide a default WorkerCount inside bounds
    mock_config.scripting.WorkerCount = 2;
    app_config = &mock_config;
}

void tearDown(void) {
    app_config = NULL;
    free_launch_messages(NULL, 0); // no-op when NULL
}

// Disabled in config -> No-Go, not an error
void test_check_scripting_launch_readiness_disabled(void) {
    mock_config.scripting.Enabled = false;
    LaunchReadiness r = check_scripting_launch_readiness();
    TEST_ASSERT_NOT_NULL(r.messages);
    TEST_ASSERT_EQUAL_STRING(SR_SCRIPTING, r.subsystem);
    TEST_ASSERT_FALSE(r.ready);
    // The "disabled" message should be present
    bool found_disabled = false;
    for (int i = 0; r.messages[i] != NULL; i++) {
        if (strstr(r.messages[i], "disabled in configuration") != NULL) {
            found_disabled = true;
            break;
        }
    }
    TEST_ASSERT_TRUE_MESSAGE(found_disabled, "expected 'disabled in configuration' message");
    free_readiness_messages(&r);
}

// Enabled with default worker count -> Go
void test_check_scripting_launch_readiness_enabled(void) {
    mock_config.scripting.Enabled = true;
    mock_config.scripting.WorkerCount = 2;
    LaunchReadiness r = check_scripting_launch_readiness();
    TEST_ASSERT_NOT_NULL(r.messages);
    TEST_ASSERT_EQUAL_STRING(SR_SCRIPTING, r.subsystem);
    TEST_ASSERT_TRUE(r.ready);
    // The "Decide: Go For Launch" message should be present
    bool found_decide = false;
    for (int i = 0; r.messages[i] != NULL; i++) {
        if (strstr(r.messages[i], "Go For Launch of Scripting") != NULL) {
            found_decide = true;
            break;
        }
    }
    TEST_ASSERT_TRUE_MESSAGE(found_decide, "expected 'Go For Launch' decision message");
    free_readiness_messages(&r);
}

// Enabled but worker count too high -> No-Go
void test_check_scripting_launch_readiness_worker_count_too_high(void) {
    mock_config.scripting.Enabled = true;
    mock_config.scripting.WorkerCount = MAX_CONCURRENT_JOBS + 1;
    LaunchReadiness r = check_scripting_launch_readiness();
    TEST_ASSERT_NOT_NULL(r.messages);
    TEST_ASSERT_FALSE(r.ready);
    free_readiness_messages(&r);
}

// Enabled but worker count too low -> No-Go
void test_check_scripting_launch_readiness_worker_count_zero(void) {
    mock_config.scripting.Enabled = true;
    mock_config.scripting.WorkerCount = 0;
    LaunchReadiness r = check_scripting_launch_readiness();
    TEST_ASSERT_NOT_NULL(r.messages);
    TEST_ASSERT_FALSE(r.ready);
    free_readiness_messages(&r);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_check_scripting_launch_readiness_disabled);
    RUN_TEST(test_check_scripting_launch_readiness_enabled);
    RUN_TEST(test_check_scripting_launch_readiness_worker_count_too_high);
    RUN_TEST(test_check_scripting_launch_readiness_worker_count_zero);

    return UNITY_END();
}
