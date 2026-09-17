/*
 * Unity tests for firebase_now() with injectable clock.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/firebase/fns_tz.h>

void test_firebase_now_injected_epoch(void);
void test_firebase_now_format_without_override(void);
void test_firebase_now_clear_restores_clock(void);

void setUp(void) {
    firebase_now_test_clear();
}

void tearDown(void) {
    firebase_now_test_clear();
}

void test_firebase_now_injected_epoch(void) {
    firebase_now_test_set_unix(1704110400);
    TEST_ASSERT_EQUAL(1704110400, firebase_now_unix());
    char* now = firebase_now();
    TEST_ASSERT_NOT_NULL(now);
    TEST_ASSERT_EQUAL_STRING("2024-01-01 12:00:00", now);
    free(now);
}

void test_firebase_now_format_without_override(void) {
    char* now = firebase_now();
    TEST_ASSERT_NOT_NULL(now);
    TEST_ASSERT_EQUAL(19, (int)strlen(now));
    TEST_ASSERT_EQUAL('-', now[4]);
    TEST_ASSERT_EQUAL('-', now[7]);
    TEST_ASSERT_EQUAL(' ', now[10]);
    TEST_ASSERT_EQUAL(':', now[13]);
    TEST_ASSERT_EQUAL(':', now[16]);
    free(now);
}

void test_firebase_now_clear_restores_clock(void) {
    firebase_now_test_set_unix(1);
    firebase_now_test_clear();
    TEST_ASSERT_TRUE(firebase_now_unix() > 1);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_firebase_now_injected_epoch);
    RUN_TEST(test_firebase_now_format_without_override);
    RUN_TEST(test_firebase_now_clear_restores_clock);
    return UNITY_END();
}
