/*
 * Unity Test: network_linux_test_interface_config.c
 * Tests interface configuration functions
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/network/network.h>

// Test function prototypes
void test_is_interface_configured_null_interface(void);
void test_is_interface_configured_null_is_available(void);
void test_is_interface_configured_no_config(void);
void test_is_interface_configured_interface_not_found(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test NULL interface parameter
void test_is_interface_configured_null_interface(void) {
    bool is_available = false;
    bool result = is_interface_configured(NULL, &is_available);

    // Should return false and set is_available to true (default)
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_TRUE(is_available);
}

// Test NULL is_available parameter
void test_is_interface_configured_null_is_available(void) {
    bool result = is_interface_configured("eth0", NULL);

    // Should return false when is_available is NULL
    TEST_ASSERT_FALSE(result);
}

// Test with no app_config
void test_is_interface_configured_no_config(void) {
    // Temporarily clear app_config if it exists
    struct AppConfig *saved_config = app_config;
    app_config = NULL;

    bool is_available = false;
    bool result = is_interface_configured("eth0", &is_available);

    // Should return false and set is_available to true (default)
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_TRUE(is_available);

    // Restore config
    app_config = saved_config;
}

// Test interface not found in configuration
void test_is_interface_configured_interface_not_found(void) {
    bool is_available = false;
    bool result = is_interface_configured("nonexistent_interface", &is_available);

    // Should return false and set is_available to true (default)
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_TRUE(is_available);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_is_interface_configured_null_interface);
    RUN_TEST(test_is_interface_configured_null_is_available);
    RUN_TEST(test_is_interface_configured_no_config);
    RUN_TEST(test_is_interface_configured_interface_not_found);

    return UNITY_END();
}
