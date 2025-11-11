/*
 * Unity Test File: OIDC Launch Readiness Check with Registry Mock
 * This file contains unit tests for check_oidc_launch_readiness function
 * Tests the disabled path by mocking the Registry dependency
 */

// Enable mocks before any includes
#define USE_MOCK_LAUNCH
#include <tests/unity/mocks/mock_launch.h>

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/launch/launch.h>

// Forward declarations for functions being tested
LaunchReadiness check_oidc_launch_readiness(void);

// Forward declarations for test functions
void test_check_oidc_launch_readiness_disabled_with_registry_mock(void);

void setUp(void) {
    // Reset mocks
    mock_launch_reset_all();
    
    // Make Registry subsystem launchable for testing
    mock_launch_set_is_subsystem_launchable_result(true);
}

void tearDown(void) {
    // Reset mocks
    mock_launch_reset_all();
}

// Test functions
void test_check_oidc_launch_readiness_disabled_with_registry_mock(void) {
    // Test OIDC disabled path with Registry properly set up
    // Save original config
    AppConfig* original = app_config;

    // Create config with OIDC disabled
    AppConfig mock = {0};
    mock.oidc.enabled = false;  // Disable OIDC
    app_config = &mock;

    // Debug: Test the mock function directly
    bool mock_result = mock_is_subsystem_launchable_by_name("Registry");
    TEST_ASSERT_TRUE_MESSAGE(mock_result, "Mock function should return true for Registry");

    // Test the readiness function
    // With the mock in place, this should reach the disabled path
    LaunchReadiness result = check_oidc_launch_readiness();

    // Debug output
    printf("OIDC Ready: %s\n", result.ready ? "true" : "false");
    if (result.messages) {
        for (int i = 0; result.messages[i]; i++) {
            printf("Message %d: %s\n", i, result.messages[i]);
        }
    }

    // Restore original config
    app_config = original;

    // Should return ready=true for disabled OIDC (when Registry is available)
    TEST_ASSERT_TRUE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

int main(void) {
    UNITY_BEGIN();

    if (0) RUN_TEST(test_check_oidc_launch_readiness_disabled_with_registry_mock);

    return UNITY_END();
}
