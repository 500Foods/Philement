/*
 * Unity Tests for Conduit Service
 *
 * Tests the conduit service initialization and lifecycle functions.
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/conduit/conduit_service.h>

// Test function prototypes
void test_conduit_service_test_init_success(void);
void test_conduit_service_test_cleanup_success(void);
void test_conduit_service_test_name_returns_conduit(void);

// Test fixtures
void setUp(void) {
    // No setup needed for these simple service tests
}

void tearDown(void) {
    // No cleanup needed for these simple service tests
}

// Test service initialization
void test_conduit_service_test_init_success(void) {
    bool result = conduit_service_init();
    TEST_ASSERT_TRUE(result);
}

// Test service cleanup
void test_conduit_service_test_cleanup_success(void) {
    // Cleanup should not crash
    conduit_service_cleanup();
    TEST_PASS();
}

// Test service name
void test_conduit_service_test_name_returns_conduit(void) {
    const char* name = conduit_service_name();
    TEST_ASSERT_NOT_NULL(name);
    TEST_ASSERT_EQUAL_STRING("Conduit", name);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_conduit_service_test_init_success);
    RUN_TEST(test_conduit_service_test_cleanup_success);
    RUN_TEST(test_conduit_service_test_name_returns_conduit);

    return UNITY_END();
}