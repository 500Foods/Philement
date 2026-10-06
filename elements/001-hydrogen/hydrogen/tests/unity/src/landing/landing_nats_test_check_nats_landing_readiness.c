/*
 * Unity Test File: check_nats_landing_readiness
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/landing/landing.h>
#include <src/launch/launch.h>

LaunchReadiness check_nats_landing_readiness(void);

void test_check_nats_landing_readiness_not_running(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_check_nats_landing_readiness_not_running(void) {
    LaunchReadiness result = check_nats_landing_readiness();

    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_NATS, result.subsystem);
    cleanup_readiness_messages(&result);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_check_nats_landing_readiness_not_running);
    return UNITY_END();
}
