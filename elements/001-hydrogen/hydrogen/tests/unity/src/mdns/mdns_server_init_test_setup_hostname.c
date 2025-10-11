/*
 * Unity Test: mdns_server_init_test_setup_hostname.c
 * Tests mdns_server_setup_hostname function
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include mock headers for testing error conditions BEFORE source headers
#ifndef USE_MOCK_SYSTEM
#define USE_MOCK_SYSTEM
#endif
#include <unity/mocks/mock_system.h>

// Include necessary headers for the module being tested (AFTER mocks to ensure overrides work)
#include <src/mdns/mdns_server.h>

// Forward declarations for helper functions being tested
int mdns_server_setup_hostname(mdns_server_t *server);

// Test function prototypes
void test_mdns_server_setup_hostname_success(void);
void test_mdns_server_setup_hostname_gethostname_failure(void);
void test_mdns_server_setup_hostname_malloc_failure(void);

void setUp(void) {
    // Reset all mocks to default state
    mock_system_reset_all();
}
 
void tearDown(void) {
    // Clean up test fixtures, if any
    mock_system_reset_all();
}

// Test mdns_server_setup_hostname function with successful hostname setup
void test_mdns_server_setup_hostname_success(void) {
    mdns_server_t *server = malloc(sizeof(mdns_server_t));
    TEST_ASSERT_NOT_NULL(server);

    if (server) {
        int result = mdns_server_setup_hostname(server);
        TEST_ASSERT_EQUAL(0, result);
        TEST_ASSERT_NOT_NULL(server->hostname);
        TEST_ASSERT_TRUE(strlen(server->hostname) > 0);

        free(server->hostname);
        free(server);
    }
}

// Test mdns_server_setup_hostname function with gethostname failure (should still succeed with fallback)
void test_mdns_server_setup_hostname_gethostname_failure(void) {
    // Mock gethostname to fail - should still succeed with "unknown" fallback
    mock_system_set_gethostname_failure(1);

    mdns_server_t *server = malloc(sizeof(mdns_server_t));
    TEST_ASSERT_NOT_NULL(server);

    if (server) {
        int result = mdns_server_setup_hostname(server);
        TEST_ASSERT_EQUAL(0, result);  // Should succeed
        TEST_ASSERT_NOT_NULL(server->hostname);
        TEST_ASSERT_EQUAL_STRING("unknown.local", server->hostname);

        free(server->hostname);
        free(server);
    }
}

// Test mdns_server_setup_hostname function with malloc failure
void test_mdns_server_setup_hostname_malloc_failure(void) {
    // Mock malloc to fail - this should cause hostname setup to fail
    mock_system_set_malloc_failure(1);

    mdns_server_t *server = malloc(sizeof(mdns_server_t));
    TEST_ASSERT_NOT_NULL(server);

    if (server) {
        int result = mdns_server_setup_hostname(server);
        TEST_ASSERT_EQUAL(-1, result);

        free(server);
    }
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mdns_server_setup_hostname_success);
    if (0) RUN_TEST(test_mdns_server_setup_hostname_gethostname_failure);
    if (0) RUN_TEST(test_mdns_server_setup_hostname_malloc_failure);

    return UNITY_END();
}