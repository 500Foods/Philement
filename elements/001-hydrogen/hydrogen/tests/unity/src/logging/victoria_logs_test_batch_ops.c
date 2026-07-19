/*
 * Unity Test File: victoria_logs_batch_ops Tests
 * This file contains unit tests for the batch and flush internal functions
 * from src/logging/victoria_logs.c
 *   - victoria_logs_add_to_batch
 *   - victoria_logs_clear_batch_impl
 *   - victoria_logs_reset_long_timer
 *   - victoria_logs_flush_batch_internal
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers
#include <src/logging/victoria_logs.h>
#include <src/logging/logging.h>

// External declarations for global state
extern VictoriaLogsConfig victoria_logs_config;
extern VLThreadState victoria_logs_thread;

// Function prototypes for test functions
void test_victoria_logs_add_to_batch_basic(void);
void test_victoria_logs_add_to_batch_separator(void);
void test_victoria_logs_add_to_batch_buffer_full(void);
void test_victoria_logs_add_to_batch_first_message_time(void);
void test_victoria_logs_clear_batch_impl(void);
void test_victoria_logs_reset_long_timer(void);
void test_victoria_logs_flush_batch_internal_empty(void);
void test_victoria_logs_flush_batch_internal_parse_failure(void);

// Test fixtures
void setUp(void) {
    memset(&victoria_logs_config, 0, sizeof(victoria_logs_config));
    memset(&victoria_logs_thread, 0, sizeof(victoria_logs_thread));
    victoria_logs_thread.batch_buffer = malloc(VICTORIA_LOGS_MAX_BATCH_BUFFER);
    victoria_logs_thread.batch_buffer[0] = '\0';
}

void tearDown(void) {
    free(victoria_logs_thread.batch_buffer);
    victoria_logs_thread.batch_buffer = NULL;
    free(victoria_logs_config.url);
    victoria_logs_config.url = NULL;
}

// Test adding a single message to an empty batch
void test_victoria_logs_add_to_batch_basic(void) {
    bool result = victoria_logs_add_to_batch("{\"msg\":\"hello\"}");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, victoria_logs_thread.batch_count);
    TEST_ASSERT_EQUAL_STRING("{\"msg\":\"hello\"}", victoria_logs_thread.batch_buffer);
}

// Test newline separator is added between messages
void test_victoria_logs_add_to_batch_separator(void) {
    TEST_ASSERT_TRUE(victoria_logs_add_to_batch("first"));
    TEST_ASSERT_TRUE(victoria_logs_add_to_batch("second"));
    TEST_ASSERT_EQUAL(2, victoria_logs_thread.batch_count);
    TEST_ASSERT_EQUAL_STRING("first\nsecond", victoria_logs_thread.batch_buffer);
}

// Test adding a message that would overflow the buffer returns false
void test_victoria_logs_add_to_batch_buffer_full(void) {
    // Fill the buffer to near capacity
    size_t cap = VICTORIA_LOGS_MAX_BATCH_BUFFER - 1;
    char* big = malloc(cap + 1);
    TEST_ASSERT_NOT_NULL(big);
    if (!big) {
        return;
    }
    memset(big, 'x', cap);
    big[cap] = '\0';

    // First message fits
    bool result = victoria_logs_add_to_batch(big);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, victoria_logs_thread.batch_count);

    // Second message would require a separator that overflows -> rejected
    bool result2 = victoria_logs_add_to_batch("yy");
    TEST_ASSERT_FALSE(result2);
    TEST_ASSERT_EQUAL(1, victoria_logs_thread.batch_count);

    free(big);
}

// Test first_message_time is set when first message added
void test_victoria_logs_add_to_batch_first_message_time(void) {
    victoria_logs_thread.first_message_time = 0;
    victoria_logs_add_to_batch("first");
    TEST_ASSERT_NOT_EQUAL(0, victoria_logs_thread.first_message_time);
}

// Test clear_batch_impl zeroes everything
void test_victoria_logs_clear_batch_impl(void) {
    victoria_logs_add_to_batch("first");
    TEST_ASSERT_EQUAL(1, victoria_logs_thread.batch_count);

    victoria_logs_clear_batch_impl();

    TEST_ASSERT_EQUAL(0, victoria_logs_thread.batch_count);
    TEST_ASSERT_EQUAL(0, victoria_logs_thread.batch_buffer_size);
    TEST_ASSERT_EQUAL_STRING("", victoria_logs_thread.batch_buffer);
    TEST_ASSERT_EQUAL(0, victoria_logs_thread.first_message_time);
}

// Test reset_long_timer sets a future time
void test_victoria_logs_reset_long_timer(void) {
    victoria_logs_thread.long_timer.tv_sec = 0;
    victoria_logs_thread.long_timer.tv_nsec = 0;

    victoria_logs_reset_long_timer();

    TEST_ASSERT_TRUE(victoria_logs_thread.long_timer.tv_sec >= VICTORIA_LOGS_LONG_TIMER_SEC);
}

// Test flush with empty batch returns true (no-op)
void test_victoria_logs_flush_batch_internal_empty(void) {
    victoria_logs_thread.batch_count = 0;
    victoria_logs_thread.batch_buffer_size = 0;

    bool result = victoria_logs_flush_batch_internal();
    TEST_ASSERT_TRUE(result);
}

// Test flush with invalid URL clears batch and returns false
void test_victoria_logs_flush_batch_internal_parse_failure(void) {
    // Point at an unparseable URL (host too long)
    char url[300];
    strcpy(url, "http://");
    for (int i = 0; i < 260; i++) {
        url[7 + i] = 'a';
    }
    url[267] = '\0';
    victoria_logs_config.url = strdup(url);

    victoria_logs_add_to_batch("first");
    TEST_ASSERT_EQUAL(1, victoria_logs_thread.batch_count);

    bool result = victoria_logs_flush_batch_internal();
    TEST_ASSERT_FALSE(result);
    // Batch dropped on permanent parse failure
    TEST_ASSERT_EQUAL(0, victoria_logs_thread.batch_count);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_victoria_logs_add_to_batch_basic);
    RUN_TEST(test_victoria_logs_add_to_batch_separator);
    RUN_TEST(test_victoria_logs_add_to_batch_buffer_full);
    RUN_TEST(test_victoria_logs_add_to_batch_first_message_time);
    RUN_TEST(test_victoria_logs_clear_batch_impl);
    RUN_TEST(test_victoria_logs_reset_long_timer);
    RUN_TEST(test_victoria_logs_flush_batch_internal_empty);
    RUN_TEST(test_victoria_logs_flush_batch_internal_parse_failure);

    return UNITY_END();
}
