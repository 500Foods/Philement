/*
 * Unity Test File: check_mcp_landing_readiness
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/landing/landing.h>

LaunchReadiness check_mcp_landing_readiness(void);

void test_check_mcp_landing_readiness_not_running(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_check_mcp_landing_readiness_not_running(void) {
    LaunchReadiness result = check_mcp_landing_readiness();

    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_MCP, result.subsystem);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_check_mcp_landing_readiness_not_running);

    return UNITY_END();
}
