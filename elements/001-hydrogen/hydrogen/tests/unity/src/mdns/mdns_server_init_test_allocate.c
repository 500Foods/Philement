/*
 * Unity Test: mdns_server_init_test_allocate.c
 * Tests mdns_server_allocate function
 */

// USE_MOCK_SYSTEM, USE_MOCK_NETWORK, USE_MOCK_THREADS defined by CMake
#include <unity/mocks/mock_system.h>

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/mdns/mdns_server.h>

// Forward declarations for helper functions being tested
mdns_server_t *mdns_server_allocate(void);

// Test function prototypes
void test_mdns_server_allocate(void);
void test_mdns_server_allocate_malloc_failure(void);

void setUp(void) {
    // Reset all mocks to default state
    mock_system_reset_all();
}

void tearDown(void) {
    // Clean up test fixtures, if any
    mock_system_reset_all();
}

// Test mdns_server_allocate function
void test_mdns_server_allocate(void) {
    mdns_server_t *server = mdns_server_allocate();
    TEST_ASSERT_NOT_NULL(server);
    if (server) {
        // The server structure is not zero-initialized by malloc, so we can't test for zero values
        // Instead, we just verify it's not NULL and can be freed
        // This matches the actual behavior of the mdns_server_allocate function

        free(server);
    }
}

// Test mdns_server_allocate function with malloc failure
void test_mdns_server_allocate_malloc_failure(void) {
    // Mock malloc to fail
    mock_system_set_malloc_failure(1);

    mdns_server_t *server = mdns_server_allocate();
    TEST_ASSERT_NULL(server);  // Should return NULL due to malloc failure
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mdns_server_allocate);
    RUN_TEST(test_mdns_server_allocate_malloc_failure);

    return UNITY_END();
}