/*
 * Unity Test File: OIDC Launch Readiness Check with Registry
 * Tests the disabled-OIDC path when Registry is registered and launchable.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/launch/launch.h>
#include <src/registry/registry.h>

LaunchReadiness check_oidc_launch_readiness(void);

void test_check_oidc_launch_readiness_disabled_with_registry_mock(void);

void setUp(void) {
    init_registry();
    register_subsystem(SR_REGISTRY, NULL, NULL, NULL, NULL, NULL);
}

void tearDown(void) {
    init_registry();
}

void test_check_oidc_launch_readiness_disabled_with_registry_mock(void) {
    AppConfig* original = app_config;

    AppConfig mock = {0};
    mock.oidc.enabled = false;
    app_config = &mock;

    LaunchReadiness result = check_oidc_launch_readiness();

    app_config = original;

    TEST_ASSERT_TRUE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
    free_readiness_messages(&result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_check_oidc_launch_readiness_disabled_with_registry_mock);

    return UNITY_END();
}
