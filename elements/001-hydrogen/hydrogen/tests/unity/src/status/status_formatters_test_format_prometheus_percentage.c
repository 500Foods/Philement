/*
 * Unity Test File: status_formatters_test_format_prometheus_percentage.c
 *
 * Tests for format_prometheus_percentage function from status_formatters.c
 * Following the "One Test File Per Function Rule" from UNITY.md
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

#include <string.h>

// Include status_formatters header for the function declaration
#include <src/status/status_formatters.h>

// Function prototypes for test functions
void test_format_prometheus_percentage_basic_functionality(void);
void test_format_prometheus_percentage_zero_value(void);
void test_format_prometheus_percentage_hundred_value(void);
void test_format_prometheus_percentage_decimal_value(void);
void test_format_prometheus_percentage_with_labels(void);
void test_format_prometheus_percentage_null_labels(void);
void test_format_prometheus_percentage_buffer_size_limit(void);

void setUp(void) {
    // No special setup needed for percentage formatting tests
}

void tearDown(void) {
    // No special cleanup needed for percentage formatting tests
}

// Tests for format_prometheus_percentage function
void test_format_prometheus_percentage_basic_functionality(void) {
    char buffer[256];

    format_prometheus_percentage(buffer, sizeof(buffer),
                               "cpu_usage", "", "50.0");

    // Verify buffer contains expected Prometheus format
    TEST_ASSERT_TRUE(strlen(buffer) > 0);
    TEST_ASSERT_TRUE(strstr(buffer, "# HELP cpu_usage Percentage value") != NULL);
    TEST_ASSERT_TRUE(strstr(buffer, "# TYPE cpu_usage gauge") != NULL);
    TEST_ASSERT_TRUE(strstr(buffer, "cpu_usage{} 0.5") != NULL);

    TEST_PASS();
}

void test_format_prometheus_percentage_zero_value(void) {
    char buffer[256];

    format_prometheus_percentage(buffer, sizeof(buffer),
                               "memory_usage", "", "0.0");

    // Verify zero value is converted to 0.0 ratio
    TEST_ASSERT_TRUE(strstr(buffer, "memory_usage{} 0.0") != NULL);

    TEST_PASS();
}

void test_format_prometheus_percentage_hundred_value(void) {
    char buffer[256];

    format_prometheus_percentage(buffer, sizeof(buffer),
                               "disk_usage", "", "100.0");

    // Verify 100% is converted to 1.0 ratio
    TEST_ASSERT_TRUE(strstr(buffer, "disk_usage{} 1.0") != NULL);

    TEST_PASS();
}

void test_format_prometheus_percentage_decimal_value(void) {
    char buffer[256];

    format_prometheus_percentage(buffer, sizeof(buffer),
                               "load_average", "", "75.5");

    // Verify decimal value is properly converted
    TEST_ASSERT_TRUE(strstr(buffer, "load_average{} 0.755") != NULL);

    TEST_PASS();
}

void test_format_prometheus_percentage_with_labels(void) {
    char buffer[256];

    format_prometheus_percentage(buffer, sizeof(buffer),
                               "cpu_core_usage", "core=\"0\"", "85.2");

    // Verify labels are included in output
    TEST_ASSERT_TRUE(strstr(buffer, "cpu_core_usage{core=\"0\"} 0.852") != NULL);

    TEST_PASS();
}

void test_format_prometheus_percentage_null_labels(void) {
    char buffer[256];

    format_prometheus_percentage(buffer, sizeof(buffer),
                               "network_usage", NULL, "25.0");

    // Verify null labels result in empty braces
    TEST_ASSERT_TRUE(strstr(buffer, "network_usage{} 0.25") != NULL);

    TEST_PASS();
}

void test_format_prometheus_percentage_buffer_size_limit(void) {
    char buffer[64]; // Small buffer

    format_prometheus_percentage(buffer, sizeof(buffer),
                               "very_long_metric_name_that_exceeds_buffer", "", "50.0");

    // Function should handle buffer size limits gracefully
    // The output should be truncated but not cause buffer overflow
    TEST_ASSERT_TRUE(strlen(buffer) < sizeof(buffer));

    TEST_PASS();
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_format_prometheus_percentage_basic_functionality);
    RUN_TEST(test_format_prometheus_percentage_zero_value);
    RUN_TEST(test_format_prometheus_percentage_hundred_value);
    RUN_TEST(test_format_prometheus_percentage_decimal_value);
    RUN_TEST(test_format_prometheus_percentage_with_labels);
    RUN_TEST(test_format_prometheus_percentage_null_labels);
    RUN_TEST(test_format_prometheus_percentage_buffer_size_limit);

    return UNITY_END();
}
