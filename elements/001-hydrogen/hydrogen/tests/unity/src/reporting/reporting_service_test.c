/*
 * Unity Test File: reporting_service init/cleanup/name
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/reporting/reporting_service.h>

void test_reporting_service_name(void);
void test_reporting_service_init_cleanup(void);
void test_reporting_service_double_init(void);
void test_reporting_service_cleanup_when_not_init(void);
void test_reporting_service_apply_resource_limits(void);

void setUp(void) {
    /* Ensure clean slate; cleanup is idempotent when not initialized */
    if (reporting_service_is_initialized()) {
        reporting_service_cleanup();
    }
}

void tearDown(void) {
    if (reporting_service_is_initialized()) {
        reporting_service_cleanup();
    }
}

void test_reporting_service_name(void) {
    TEST_ASSERT_EQUAL_STRING("Reporting", reporting_service_name());
}

void test_reporting_service_init_cleanup(void) {
    TEST_ASSERT_FALSE(reporting_service_is_initialized());
    TEST_ASSERT_TRUE(reporting_service_init());
    TEST_ASSERT_TRUE(reporting_service_is_initialized());
    reporting_service_cleanup();
    TEST_ASSERT_FALSE(reporting_service_is_initialized());
}

void test_reporting_service_double_init(void) {
    TEST_ASSERT_TRUE(reporting_service_init());
    TEST_ASSERT_TRUE(reporting_service_init());
    TEST_ASSERT_TRUE(reporting_service_is_initialized());
    reporting_service_cleanup();
}

void test_reporting_service_cleanup_when_not_init(void) {
    TEST_ASSERT_FALSE(reporting_service_is_initialized());
    reporting_service_cleanup();
    TEST_ASSERT_FALSE(reporting_service_is_initialized());
}

void test_reporting_service_apply_resource_limits(void) {
    TEST_ASSERT_TRUE(reporting_service_init());
    reporting_service_apply_resource_limits();
    TEST_ASSERT_TRUE(reporting_service_is_initialized());
    reporting_service_cleanup();
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_reporting_service_name);
    RUN_TEST(test_reporting_service_init_cleanup);
    RUN_TEST(test_reporting_service_double_init);
    RUN_TEST(test_reporting_service_cleanup_when_not_init);
    RUN_TEST(test_reporting_service_apply_resource_limits);
    return UNITY_END();
}
