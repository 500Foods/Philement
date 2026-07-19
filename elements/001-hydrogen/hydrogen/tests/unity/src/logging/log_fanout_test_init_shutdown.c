/*
 * Unity Test File: init_log_fanout / shutdown_log_fanout Lifecycle Tests
 * Tests src/logging/log_fanout.c lifecycle functions.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/logging/log_fanout.h>

#include <pthread.h>

static void test_shutdown_before_init_is_safe(void);
static void test_init_then_shutdown(void);

void setUp(void) {}
void tearDown(void) {}

void test_shutdown_before_init_is_safe(void) {
    // Must not crash or hang when never initialized.
    TEST_ASSERT_TRUE(shutdown_log_fanout());
}

void test_init_then_shutdown(void) {
    bool ok = init_log_fanout();
    TEST_ASSERT_TRUE(ok);

    // A consumer thread should now be joinable via shutdown.
    TEST_ASSERT_TRUE(shutdown_log_fanout());

    // Second shutdown must be safe (idempotent guard).
    TEST_ASSERT_TRUE(shutdown_log_fanout());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_shutdown_before_init_is_safe);
    RUN_TEST(test_init_then_shutdown);
    return UNITY_END();
}
