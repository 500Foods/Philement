/*
 * Unity Test File: nats_status_state
 *
 * The four lock rows, plus a disabled link that is already up.
 * state is down only when NATS is disabled. No socket.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/nats/nats_internal.h>
#include <src/nats/nats_stats.h>

void test_nats_status_state_disabled_down(void);
void test_nats_status_state_enabled_down(void);
void test_nats_status_state_enabled_degraded(void);
void test_nats_status_state_enabled_up(void);
void test_nats_status_state_disabled_up(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_nats_status_state_disabled_down(void) {
    TEST_ASSERT_EQUAL_STRING("down",
        nats_status_state(false, NATS_LINK_DOWN));
}

void test_nats_status_state_enabled_down(void) {
    TEST_ASSERT_EQUAL_STRING("degraded",
        nats_status_state(true, NATS_LINK_DOWN));
}

void test_nats_status_state_enabled_degraded(void) {
    TEST_ASSERT_EQUAL_STRING("degraded",
        nats_status_state(true, NATS_LINK_DEGRADED));
}

void test_nats_status_state_enabled_up(void) {
    TEST_ASSERT_EQUAL_STRING("up",
        nats_status_state(true, NATS_LINK_UP));
}

void test_nats_status_state_disabled_up(void) {
    TEST_ASSERT_EQUAL_STRING("down",
        nats_status_state(false, NATS_LINK_UP));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_nats_status_state_disabled_down);
    RUN_TEST(test_nats_status_state_enabled_down);
    RUN_TEST(test_nats_status_state_enabled_degraded);
    RUN_TEST(test_nats_status_state_enabled_up);
    RUN_TEST(test_nats_status_state_disabled_up);
    return UNITY_END();
}
