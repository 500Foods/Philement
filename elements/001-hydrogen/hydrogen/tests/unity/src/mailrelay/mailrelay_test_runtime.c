/*
 * Unit tests for Mail Relay runtime lifecycle and thread tracking.
 *
 * Verifies that mailrelay_init()/mailrelay_shutdown() manage the runtime
 * instance, mutex/condition, counters, and service thread tracking correctly.
 */

#include <src/hydrogen.h>

#include <src/mailrelay/mailrelay.h>
#include <src/mailrelay/mailrelay_internal.h>

#include <src/threads/threads.h>

#include <unity.h>
#include <string.h>

// Forward declarations for test functions
void test_runtime_init_succeeds(void);
void test_runtime_init_is_idempotent(void);
void test_runtime_counters_start_zero(void);
void test_runtime_shutdown_frees_instance(void);
void test_runtime_shutdown_without_init_is_safe(void);
void test_runtime_shutdown_is_idempotent(void);
void test_runtime_service_threads_initialized(void);
void test_runtime_shutdown_resets_service_threads(void);

void setUp(void) {
    // Ensure a clean starting state for every test.
    mailrelay_shutdown();
}

void tearDown(void) {
    // Clean up any runtime left behind by the test.
    mailrelay_shutdown();
}

void test_runtime_init_succeeds(void) {
    TEST_ASSERT_TRUE(mailrelay_init());
    TEST_ASSERT_NOT_NULL(mailrelay_runtime);
    TEST_ASSERT_TRUE(mailrelay_runtime_is_initialized());
}

void test_runtime_init_is_idempotent(void) {
    TEST_ASSERT_TRUE(mailrelay_init());
    MailRelayRuntime* first = mailrelay_runtime;
    TEST_ASSERT_TRUE(mailrelay_init());
    TEST_ASSERT_EQUAL_PTR(first, mailrelay_runtime);
    TEST_ASSERT_TRUE(mailrelay_runtime_is_initialized());
}

void test_runtime_counters_start_zero(void) {
    TEST_ASSERT_TRUE(mailrelay_init());
    TEST_ASSERT_EQUAL_INT(0, mailrelay_runtime->worker_count);
    TEST_ASSERT_EQUAL_size_t(0, mailrelay_runtime->queued_count);
    TEST_ASSERT_EQUAL_size_t(0, mailrelay_runtime->sent_count);
    TEST_ASSERT_EQUAL_size_t(0, mailrelay_runtime->failed_count);
    TEST_ASSERT_EQUAL_size_t(0, mailrelay_runtime->retry_count);
}

void test_runtime_shutdown_frees_instance(void) {
    TEST_ASSERT_TRUE(mailrelay_init());
    mailrelay_shutdown();
    TEST_ASSERT_NULL(mailrelay_runtime);
    TEST_ASSERT_FALSE(mailrelay_runtime_is_initialized());
}

void test_runtime_shutdown_without_init_is_safe(void) {
    // Must not crash or leak when there is no initialized runtime.
    mailrelay_shutdown();
    TEST_ASSERT_NULL(mailrelay_runtime);
    TEST_ASSERT_FALSE(mailrelay_runtime_is_initialized());
}

void test_runtime_shutdown_is_idempotent(void) {
    TEST_ASSERT_TRUE(mailrelay_init());
    mailrelay_shutdown();
    mailrelay_shutdown();
    TEST_ASSERT_NULL(mailrelay_runtime);
    TEST_ASSERT_FALSE(mailrelay_runtime_is_initialized());
}

void test_runtime_service_threads_initialized(void) {
    TEST_ASSERT_TRUE(mailrelay_init());
    TEST_ASSERT_EQUAL_STRING(SR_MAIL_RELAY, mailrelay_threads.subsystem);
    TEST_ASSERT_EQUAL_INT(0, mailrelay_threads.thread_count);
}

void test_runtime_shutdown_resets_service_threads(void) {
    TEST_ASSERT_TRUE(mailrelay_init());
    mailrelay_shutdown();
    TEST_ASSERT_EQUAL_STRING(SR_MAIL_RELAY, mailrelay_threads.subsystem);
    TEST_ASSERT_EQUAL_INT(0, mailrelay_threads.thread_count);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_runtime_init_succeeds);
    RUN_TEST(test_runtime_init_is_idempotent);
    RUN_TEST(test_runtime_counters_start_zero);
    RUN_TEST(test_runtime_shutdown_frees_instance);
    RUN_TEST(test_runtime_shutdown_without_init_is_safe);
    RUN_TEST(test_runtime_shutdown_is_idempotent);
    RUN_TEST(test_runtime_service_threads_initialized);
    RUN_TEST(test_runtime_shutdown_resets_service_threads);
    return UNITY_END();
}
