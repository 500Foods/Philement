/*
 * Unity Test File: API Utils api_get_client_ip Function Tests
 * This file contains unit tests for the api_get_client_ip function in api_utils.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../src/api/api_utils.h"
#include "../../../../tests/unity/mocks/mock_libmicrohttpd.h"

// Include system headers for sockaddr structures
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

// Mock structures for testing
struct MockMHDConnection {
    int dummy; // Minimal mock
};

// Mock sockaddr structures for testing
static struct sockaddr_in mock_addr_ipv4;
static struct sockaddr_in6 mock_addr_ipv6;

// Function declarations
void test_api_get_client_ip_null_connection(void);
void test_api_get_client_ip_no_connection_info(void);
void test_api_get_client_ip_ipv4(void);
void test_api_get_client_ip_ipv6(void);
void test_api_get_client_ip_unsupported_family(void);

void setUp(void) {
    // Reset mocks before each test
    mock_mhd_reset_all();
}

void tearDown(void) {
    // Clean up after each test
    mock_mhd_reset_all();
}

// Test api_get_client_ip with NULL connection
void test_api_get_client_ip_null_connection(void) {
    char *result = api_get_client_ip(NULL);

    TEST_ASSERT_NULL(result);
}

// Test api_get_client_ip with no connection info (mock returns NULL)
void test_api_get_client_ip_no_connection_info(void) {
    // Mock returns NULL for connection info
    mock_mhd_set_connection_info(NULL);

    struct MockMHDConnection mock_conn;
    char *result = api_get_client_ip((struct MHD_Connection *)&mock_conn);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("unknown", result);

    free(result);
}

// Test api_get_client_ip with IPv4 address
void test_api_get_client_ip_ipv4(void) {
    // Set up mock IPv4 connection info
    static union MHD_ConnectionInfo info;
    memset(&mock_addr_ipv4, 0, sizeof(mock_addr_ipv4));
    mock_addr_ipv4.sin_family = AF_INET;
    inet_pton(AF_INET, "192.168.1.100", &mock_addr_ipv4.sin_addr);
    info.client_addr = (struct sockaddr *)&mock_addr_ipv4;
    mock_mhd_set_connection_info(&info);

    struct MockMHDConnection mock_conn;
    char *result = api_get_client_ip((struct MHD_Connection *)&mock_conn);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("192.168.1.100", result);

    free(result);
}

// Test api_get_client_ip with IPv6 address
void test_api_get_client_ip_ipv6(void) {
    // Set up mock IPv6 connection info
    static union MHD_ConnectionInfo info;
    memset(&mock_addr_ipv6, 0, sizeof(mock_addr_ipv6));
    mock_addr_ipv6.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "::1", &mock_addr_ipv6.sin6_addr);
    info.client_addr = (struct sockaddr *)&mock_addr_ipv6;
    mock_mhd_set_connection_info(&info);

    struct MockMHDConnection mock_conn;
    char *result = api_get_client_ip((struct MHD_Connection *)&mock_conn);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("::1", result);

    free(result);
}

// Test api_get_client_ip with unsupported address family
void test_api_get_client_ip_unsupported_family(void) {
    // Create mock sockaddr with unsupported family
    static union MHD_ConnectionInfo info;
    static struct sockaddr addr;
    memset(&addr, 0, sizeof(addr));
    addr.sa_family = AF_UNIX;  // Unsupported family
    info.client_addr = &addr;
    mock_mhd_set_connection_info(&info);

    struct MockMHDConnection mock_conn;
    char *result = api_get_client_ip((struct MHD_Connection *)&mock_conn);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("unknown", result);

    free(result);
}

int main(void) {
    UNITY_BEGIN();

    if (0) RUN_TEST(test_api_get_client_ip_null_connection);
    RUN_TEST(test_api_get_client_ip_no_connection_info);
    RUN_TEST(test_api_get_client_ip_ipv4);
    RUN_TEST(test_api_get_client_ip_ipv6);
    RUN_TEST(test_api_get_client_ip_unsupported_family);

    return UNITY_END();
}