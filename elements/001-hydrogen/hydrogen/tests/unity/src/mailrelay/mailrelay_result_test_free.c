/*
 * Unity Test File: mailrelay_result_test_free.c
 *
 * Tests for mailrelay_result_free() in src/mailrelay/mailrelay_result.c.
 * The result uses fixed buffers so mailrelay_result_free is a no-op provided
 * for API symmetry; this test confirms it safely handles NULL and initialized
 * results while leaving fixed-buffer state intact.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mailrelay/mailrelay_result.h>

#include <string.h>

void test_mailrelay_result_free_null_is_safe(void);
void test_mailrelay_result_free_noop_leaves_init_state(void);
void test_mailrelay_result_free_noop_leaves_populated_state(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_mailrelay_result_free_null_is_safe(void) {
    // Should be a safe no-op, not a crash.
    mailrelay_result_free(NULL);
    TEST_ASSERT_TRUE(true);
}

void test_mailrelay_result_free_noop_leaves_init_state(void) {
    MailRelayResult r;
    mailrelay_result_init(&r);

    mailrelay_result_free(&r);

    // No-op free must not disturb the initialized state.
    TEST_ASSERT_FALSE(r.success);
    TEST_ASSERT_EQUAL_INT(0, r.smtp_code);
    TEST_ASSERT_EQUAL_STRING("", r.smtp_text);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, r.duration_ms);
    TEST_ASSERT_EQUAL_STRING("", r.error);
    TEST_ASSERT_FALSE(r.retryable);
}

void test_mailrelay_result_free_noop_leaves_populated_state(void) {
    MailRelayResult r;
    mailrelay_result_init(&r);
    r.success = true;
    r.smtp_code = 250;
    snprintf(r.smtp_text, sizeof(r.smtp_text), "OK");
    r.duration_ms = 12.5;
    snprintf(r.error, sizeof(r.error), "none");
    r.retryable = true;

    mailrelay_result_free(&r);

    // Fixed-buffer result is not freed, so the populated fields remain.
    TEST_ASSERT_TRUE(r.success);
    TEST_ASSERT_EQUAL_INT(250, r.smtp_code);
    TEST_ASSERT_EQUAL_STRING("OK", r.smtp_text);
    TEST_ASSERT_EQUAL_DOUBLE(12.5, r.duration_ms);
    TEST_ASSERT_EQUAL_STRING("none", r.error);
    TEST_ASSERT_TRUE(r.retryable);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mailrelay_result_free_null_is_safe);
    RUN_TEST(test_mailrelay_result_free_noop_leaves_init_state);
    RUN_TEST(test_mailrelay_result_free_noop_leaves_populated_state);

    return UNITY_END();
}
