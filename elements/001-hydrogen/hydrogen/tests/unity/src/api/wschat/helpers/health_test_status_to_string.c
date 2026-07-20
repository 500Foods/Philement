/*
 * Unity Test File: chat_health_status_to_string
 * Tests the ChatHealthStatus -> string mapping in
 * src/api/wschat/helpers/health.c
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/wschat/helpers/health.h>

void test_chat_health_status_to_string_all(void);

void setUp(void) {}
void tearDown(void) {}

void test_chat_health_status_to_string_all(void) {
    TEST_ASSERT_EQUAL_STRING("healthy", chat_health_status_to_string(HEALTH_HEALTHY));
    TEST_ASSERT_EQUAL_STRING("degraded", chat_health_status_to_string(HEALTH_DEGRADED));
    TEST_ASSERT_EQUAL_STRING("unavailable", chat_health_status_to_string(HEALTH_UNAVAILABLE));
    TEST_ASSERT_EQUAL_STRING("unknown", chat_health_status_to_string(HEALTH_UNKNOWN));
    TEST_ASSERT_EQUAL_STRING("unknown", chat_health_status_to_string((ChatHealthStatus)999));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_chat_health_status_to_string_all);
    return UNITY_END();
}
