/*
 * Unity Test: mdns_server_init_test_validate_services.c
 * Tests mdns_server_validate_services function
 */

#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../src/mdns/mdns_server.h"

// Forward declarations for helper functions being tested
int mdns_server_validate_services(const mdns_server_service_t *services, size_t num_services);

// Test function prototypes
void test_mdns_server_validate_services_valid(void);
void test_mdns_server_validate_services_invalid(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test mdns_server_validate_services function with valid inputs
void test_mdns_server_validate_services_valid(void) {
    // Test valid cases
    TEST_ASSERT_EQUAL(0, mdns_server_validate_services(NULL, 0));

    char name[] = "test";
    char type[] = "_http._tcp.local";
    mdns_server_service_t service = {name, type, 8080, NULL, 0};
    TEST_ASSERT_EQUAL(0, mdns_server_validate_services(&service, 1));
}

// Test mdns_server_validate_services function with invalid inputs
void test_mdns_server_validate_services_invalid(void) {
    // Test invalid case - NULL services array but num_services > 0
    TEST_ASSERT_EQUAL(-1, mdns_server_validate_services(NULL, 1));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mdns_server_validate_services_valid);
    RUN_TEST(test_mdns_server_validate_services_invalid);

    return UNITY_END();
}