/*
 * Unity Test File: dest_enabled Function Tests
 * Tests src/logging/log_fanout.c :: dest_enabled()
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/logging/log_fanout.h>
#include <src/config/config_logging.h>

static void test_dest_enabled_null_dest(void);
static void test_dest_enabled_disabled(void);
static void test_dest_enabled_below_level(void);
static void test_dest_enabled_at_level(void);
static void test_dest_enabled_above_level(void);

void setUp(void) {}
void tearDown(void) {}

void test_dest_enabled_null_dest(void) {
    TEST_ASSERT_FALSE(dest_enabled(NULL, LOG_LEVEL_ALERT));
}

void test_dest_enabled_disabled(void) {
    LoggingDestConfig dest = {0};
    dest.enabled = false;
    dest.default_level = LOG_LEVEL_ALERT;
    TEST_ASSERT_FALSE(dest_enabled(&dest, LOG_LEVEL_FATAL));
}

void test_dest_enabled_below_level(void) {
    LoggingDestConfig dest = {0};
    dest.enabled = true;
    dest.default_level = LOG_LEVEL_ERROR;  // messages must be >= ERROR
    TEST_ASSERT_FALSE(dest_enabled(&dest, LOG_LEVEL_ALERT));
    TEST_ASSERT_FALSE(dest_enabled(&dest, LOG_LEVEL_DEBUG));
}

void test_dest_enabled_at_level(void) {
    LoggingDestConfig dest = {0};
    dest.enabled = true;
    dest.default_level = LOG_LEVEL_ALERT;
    TEST_ASSERT_TRUE(dest_enabled(&dest, LOG_LEVEL_ALERT));
}

void test_dest_enabled_above_level(void) {
    LoggingDestConfig dest = {0};
    dest.enabled = true;
    dest.default_level = LOG_LEVEL_ALERT;
    TEST_ASSERT_TRUE(dest_enabled(&dest, LOG_LEVEL_ERROR));
    TEST_ASSERT_TRUE(dest_enabled(&dest, LOG_LEVEL_FATAL));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_dest_enabled_null_dest);
    RUN_TEST(test_dest_enabled_disabled);
    RUN_TEST(test_dest_enabled_below_level);
    RUN_TEST(test_dest_enabled_at_level);
    RUN_TEST(test_dest_enabled_above_level);
    return UNITY_END();
}
