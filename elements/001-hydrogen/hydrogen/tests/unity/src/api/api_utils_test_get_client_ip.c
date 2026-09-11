/*
 * Unity Test File: API Utils api_get_client_ip Function Tests
 * This file contains unit tests for the api_get_client_ip function in api_utils.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/api/api_utils.h>
#include <src/config/config_network.h>
#include <unity/mocks/mock_libmicrohttpd.h>

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

// Test config with trusted proxies
static AppConfig g_test_config;
static AppConfig *g_saved_app_config = NULL;

// Function declarations
void test_api_get_client_ip_null_connection(void);
void test_api_get_client_ip_no_connection_info(void);
void test_api_get_client_ip_ipv4(void);
void test_api_get_client_ip_ipv6(void);
void test_api_get_client_ip_unsupported_family(void);
void test_api_get_client_ip_xforwarded_for_single(void);
void test_api_get_client_ip_xforwarded_for_first_in_chain(void);
void test_api_get_client_ip_xforwarded_for_skips_internal(void);
void test_api_get_client_ip_xforwarded_for_all_internal(void);
void test_api_get_client_ip_xforwarded_for_internal_then_external(void);
void test_api_get_client_ip_untrusted_peer_ignores_xff(void);
void test_api_get_client_ip_trusted_peer_rightmost_external(void);
void test_api_get_client_ip_trusted_peer_all_trusted_falls_back(void);
void test_api_get_client_ip_trusted_peer_single_trusted(void);
void test_api_get_client_ip_null_app_config_untrusted(void);
void test_is_trusted_proxy_trusted(void);
void test_is_trusted_proxy_untrusted(void);
void test_is_trusted_proxy_null_app_config(void);
void test_is_ip_in_cidr_ipv4(void);
void test_is_ip_in_cidr_ipv6(void);
void test_is_ip_in_cidr_no_prefix(void);
void test_is_ip_in_cidr_invalid(void);
void test_is_ip_internal_private_ranges(void);
void test_is_ip_internal_public_ips(void);
void test_is_ip_internal_invalid(void);

void setUp(void) {
    // Reset mocks before each test
    mock_mhd_reset_all();

    // Save and set up test config
    g_saved_app_config = app_config;
    memset(&g_test_config, 0, sizeof(g_test_config));
    app_config = &g_test_config;
}

void tearDown(void) {
    // Restore original app_config
    app_config = g_saved_app_config;

    // Clean up mock
    mock_mhd_reset_all();
}

// Helper to set up IPv4 connection info with a given address
static void setup_mock_ipv4(const char *ip_str) {
    static union MHD_ConnectionInfo info;
    memset(&mock_addr_ipv4, 0, sizeof(mock_addr_ipv4));
    mock_addr_ipv4.sin_family = AF_INET;
    inet_pton(AF_INET, ip_str, &mock_addr_ipv4.sin_addr);
    info.client_addr = (struct sockaddr *)&mock_addr_ipv4;
    mock_mhd_set_connection_info(&info);
}

// Helper to set up IPv6 connection info with a given address
static void setup_mock_ipv6(const char *ip_str) {
    static union MHD_ConnectionInfo info;
    memset(&mock_addr_ipv6, 0, sizeof(mock_addr_ipv6));
    mock_addr_ipv6.sin6_family = AF_INET6;
    inet_pton(AF_INET6, ip_str, &mock_addr_ipv6.sin6_addr);
    info.client_addr = (struct sockaddr *)&mock_addr_ipv6;
    mock_mhd_set_connection_info(&info);
}

// Helper to configure a trusted proxy CIDR
static void set_trusted_proxy(const char *cidr) {
    NetworkConfig *net = &app_config->network;
    if (net->trusted_proxies_count < NETWORK_MAX_TRUSTED_PROXIES) {
        net->trusted_proxies[net->trusted_proxies_count] = strdup(cidr);
        net->trusted_proxies_count++;
    }
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

// Test api_get_client_ip with IPv4 address (no trusted proxy set)
void test_api_get_client_ip_ipv4(void) {
    // Set up mock IPv4 connection info
    setup_mock_ipv4("192.168.1.100");

    struct MockMHDConnection mock_conn;
    char *result = api_get_client_ip((struct MHD_Connection *)&mock_conn);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("192.168.1.100", result);

    free(result);
}

// Test api_get_client_ip with IPv6 address (no trusted proxy set)
void test_api_get_client_ip_ipv6(void) {
    // Set up mock IPv6 connection info
    setup_mock_ipv6("::1");

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

// Test api_get_client_ip with X-Forwarded-For header (single IP)
// No trusted proxy set, so XFF must be ignored
void test_api_get_client_ip_xforwarded_for_single(void) {
    setup_mock_ipv4("10.0.0.1");
    mock_mhd_add_lookup("X-Forwarded-For", "203.0.113.5");

    struct MockMHDConnection mock_conn;
    char *result = api_get_client_ip((struct MHD_Connection *)&mock_conn);

    // Without trusted proxy, XFF is ignored
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("10.0.0.1", result);

    free(result);
}

// Test api_get_client_ip with X-Forwarded-For header (first in chain)
// No trusted proxy set, so XFF must be ignored
void test_api_get_client_ip_xforwarded_for_first_in_chain(void) {
    setup_mock_ipv4("10.0.0.1");
    mock_mhd_add_lookup("X-Forwarded-For", "203.0.113.5, 10.0.0.1, 198.51.100.2");

    struct MockMHDConnection mock_conn;
    char *result = api_get_client_ip((struct MHD_Connection *)&mock_conn);

    // Without trusted proxy, XFF is ignored
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("10.0.0.1", result);

    free(result);
}

// Test api_get_client_ip with X-Forwarded-For where first is internal
// No trusted proxy set, so XFF must be ignored
void test_api_get_client_ip_xforwarded_for_skips_internal(void) {
    setup_mock_ipv4("10.0.0.1");
    mock_mhd_add_lookup("X-Forwarded-For", "10.118.0.19, 24.86.168.176");

    struct MockMHDConnection mock_conn;
    char *result = api_get_client_ip((struct MHD_Connection *)&mock_conn);

    // Without trusted proxy, XFF is ignored
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("10.0.0.1", result);

    free(result);
}

// Test api_get_client_ip with all internal IPs
// No trusted proxy set, so XFF must be ignored
void test_api_get_client_ip_xforwarded_for_all_internal(void) {
    setup_mock_ipv4("10.0.0.1");
    mock_mhd_add_lookup("X-Forwarded-For", "10.0.0.1, 192.168.1.1");

    struct MockMHDConnection mock_conn;
    char *result = api_get_client_ip((struct MHD_Connection *)&mock_conn);

    // Without trusted proxy, XFF is ignored
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("10.0.0.1", result);

    free(result);
}

// Test api_get_client_ip with internal IPs then an external one
// No trusted proxy set, so XFF must be ignored
void test_api_get_client_ip_xforwarded_for_internal_then_external(void) {
    setup_mock_ipv4("10.0.0.1");
    mock_mhd_add_lookup("X-Forwarded-For", "10.0.0.1, 192.168.1.1, 8.8.8.8");

    struct MockMHDConnection mock_conn;
    char *result = api_get_client_ip((struct MHD_Connection *)&mock_conn);

    // Without trusted proxy, XFF is ignored
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("10.0.0.1", result);

    free(result);
}

// Test: untrusted peer ignores X-Forwarded-For
void test_api_get_client_ip_untrusted_peer_ignores_xff(void) {
    setup_mock_ipv4("8.8.8.8");
    set_trusted_proxy("10.0.0.0/8");
    mock_mhd_add_lookup("X-Forwarded-For", "203.0.113.5");

    struct MockMHDConnection mock_conn;
    char *result = api_get_client_ip((struct MHD_Connection *)&mock_conn);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("8.8.8.8", result);

    free(result);
}

// Test: trusted peer with XFF, rightmost non-trusted address is returned
void test_api_get_client_ip_trusted_peer_rightmost_external(void) {
    setup_mock_ipv4("10.0.0.1");
    set_trusted_proxy("10.0.0.0/8");
    mock_mhd_add_lookup("X-Forwarded-For", "203.0.113.5, 10.0.0.10");

    struct MockMHDConnection mock_conn;
    char *result = api_get_client_ip((struct MHD_Connection *)&mock_conn);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("203.0.113.5", result);

    free(result);
}

// Test: trusted peer, all XFF addresses are trusted, falls back to peer
void test_api_get_client_ip_trusted_peer_all_trusted_falls_back(void) {
    setup_mock_ipv4("10.0.0.1");
    set_trusted_proxy("10.0.0.0/8");
    mock_mhd_add_lookup("X-Forwarded-For", "10.0.0.5, 10.0.0.6");

    struct MockMHDConnection mock_conn;
    char *result = api_get_client_ip((struct MHD_Connection *)&mock_conn);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("10.0.0.1", result);

    free(result);
}

// Test: trusted peer, single XFF address from untrusted IP
void test_api_get_client_ip_trusted_peer_single_trusted(void) {
    setup_mock_ipv4("10.0.0.1");
    set_trusted_proxy("10.0.0.0/8");
    mock_mhd_add_lookup("X-Forwarded-For", "8.8.8.8");

    struct MockMHDConnection mock_conn;
    char *result = api_get_client_ip((struct MHD_Connection *)&mock_conn);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("8.8.8.8", result);

    free(result);
}

// Test: NULL app_config, untrusted peer, uses TCP peer
void test_api_get_client_ip_null_app_config_untrusted(void) {
    app_config = NULL;
    setup_mock_ipv4("8.8.8.8");
    mock_mhd_add_lookup("X-Forwarded-For", "203.0.113.5");

    struct MockMHDConnection mock_conn;
    char *result = api_get_client_ip((struct MHD_Connection *)&mock_conn);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("8.8.8.8", result);

    free(result);
}

// Test: is_trusted_proxy with trusted IPs
void test_is_trusted_proxy_trusted(void) {
    set_trusted_proxy("10.0.0.0/8");

    TEST_ASSERT_TRUE(is_trusted_proxy("10.0.0.1"));
    TEST_ASSERT_TRUE(is_trusted_proxy("10.255.255.255"));
    TEST_ASSERT_TRUE(is_trusted_proxy("10.10.10.10"));
}

// Test: is_trusted_proxy with untrusted IPs
void test_is_trusted_proxy_untrusted(void) {
    set_trusted_proxy("10.0.0.0/8");

    TEST_ASSERT_FALSE(is_trusted_proxy("8.8.8.8"));
    TEST_ASSERT_FALSE(is_trusted_proxy("203.0.113.5"));
    TEST_ASSERT_FALSE(is_trusted_proxy("192.168.1.1"));
}

// Test: is_trusted_proxy with NULL app_config
void test_is_trusted_proxy_null_app_config(void) {
    app_config = NULL;

    TEST_ASSERT_FALSE(is_trusted_proxy("10.0.0.1"));
    TEST_ASSERT_FALSE(is_trusted_proxy(NULL));
}

// Test: is_ip_in_cidr with IPv4 CIDR matching
void test_is_ip_in_cidr_ipv4(void) {
    TEST_ASSERT_TRUE(is_ip_in_cidr("10.0.0.1", "10.0.0.0/8"));
    TEST_ASSERT_TRUE(is_ip_in_cidr("10.255.255.255", "10.0.0.0/8"));
    TEST_ASSERT_FALSE(is_ip_in_cidr("11.0.0.1", "10.0.0.0/8"));
    TEST_ASSERT_TRUE(is_ip_in_cidr("192.168.1.1", "192.168.0.0/16"));
    TEST_ASSERT_FALSE(is_ip_in_cidr("192.169.1.1", "192.168.0.0/16"));
}

// Test: is_ip_in_cidr with IPv6 CIDR matching
void test_is_ip_in_cidr_ipv6(void) {
    TEST_ASSERT_TRUE(is_ip_in_cidr("::1", "::/0"));
    TEST_ASSERT_TRUE(is_ip_in_cidr("2001:db8::1", "2001:db8::/32"));
    TEST_ASSERT_FALSE(is_ip_in_cidr("2002:db8::1", "2001:db8::/32"));
}

// Test: is_ip_in_cidr with no prefix (should return false)
void test_is_ip_in_cidr_no_prefix(void) {
    TEST_ASSERT_FALSE(is_ip_in_cidr("10.0.0.1", "10.0.0.0"));
    TEST_ASSERT_FALSE(is_ip_in_cidr("10.0.0.1", NULL));
    TEST_ASSERT_FALSE(is_ip_in_cidr(NULL, "10.0.0.0/8"));
}

// Test: is_ip_in_cidr with invalid addresses
void test_is_ip_in_cidr_invalid(void) {
    TEST_ASSERT_FALSE(is_ip_in_cidr("not.an.ip", "10.0.0.0/8"));
    TEST_ASSERT_FALSE(is_ip_in_cidr("10.0.0.1", "not-a-cidr"));
}

// Test is_ip_internal with private ranges
void test_is_ip_internal_private_ranges(void) {
    TEST_ASSERT_TRUE(is_ip_internal("10.0.0.1"));
    TEST_ASSERT_TRUE(is_ip_internal("10.255.255.255"));
    TEST_ASSERT_TRUE(is_ip_internal("172.16.0.1"));
    TEST_ASSERT_TRUE(is_ip_internal("172.31.255.255"));
    TEST_ASSERT_TRUE(is_ip_internal("192.168.1.1"));
    TEST_ASSERT_TRUE(is_ip_internal("192.168.0.0"));
    TEST_ASSERT_TRUE(is_ip_internal("127.0.0.1"));
    TEST_ASSERT_TRUE(is_ip_internal("169.254.0.1"));
    TEST_ASSERT_TRUE(is_ip_internal("0.0.0.0"));
    TEST_ASSERT_TRUE(is_ip_internal("100.64.0.1"));
}

// Test is_ip_internal with public IPs
void test_is_ip_internal_public_ips(void) {
    TEST_ASSERT_FALSE(is_ip_internal("8.8.8.8"));
    TEST_ASSERT_FALSE(is_ip_internal("203.0.113.5"));
    TEST_ASSERT_FALSE(is_ip_internal("24.86.168.176"));
    TEST_ASSERT_FALSE(is_ip_internal("1.1.1.1"));
    TEST_ASSERT_FALSE(is_ip_internal("172.15.255.255"));
    TEST_ASSERT_FALSE(is_ip_internal("172.32.0.1"));
}

// Test is_ip_internal with invalid input
void test_is_ip_internal_invalid(void) {
    TEST_ASSERT_TRUE(is_ip_internal(NULL));
    TEST_ASSERT_TRUE(is_ip_internal("not.an.ip.address"));
    TEST_ASSERT_TRUE(is_ip_internal(""));
    TEST_ASSERT_TRUE(is_ip_internal("999.999.999.999"));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_api_get_client_ip_null_connection);
    RUN_TEST(test_api_get_client_ip_no_connection_info);
    RUN_TEST(test_api_get_client_ip_ipv4);
    RUN_TEST(test_api_get_client_ip_ipv6);
    RUN_TEST(test_api_get_client_ip_unsupported_family);
    RUN_TEST(test_api_get_client_ip_xforwarded_for_single);
    RUN_TEST(test_api_get_client_ip_xforwarded_for_first_in_chain);
    RUN_TEST(test_api_get_client_ip_xforwarded_for_skips_internal);
    RUN_TEST(test_api_get_client_ip_xforwarded_for_all_internal);
    RUN_TEST(test_api_get_client_ip_xforwarded_for_internal_then_external);
    RUN_TEST(test_api_get_client_ip_untrusted_peer_ignores_xff);
    RUN_TEST(test_api_get_client_ip_trusted_peer_rightmost_external);
    RUN_TEST(test_api_get_client_ip_trusted_peer_all_trusted_falls_back);
    RUN_TEST(test_api_get_client_ip_trusted_peer_single_trusted);
    RUN_TEST(test_api_get_client_ip_null_app_config_untrusted);
    RUN_TEST(test_is_trusted_proxy_trusted);
    RUN_TEST(test_is_trusted_proxy_untrusted);
    RUN_TEST(test_is_trusted_proxy_null_app_config);
    RUN_TEST(test_is_ip_in_cidr_ipv4);
    RUN_TEST(test_is_ip_in_cidr_ipv6);
    RUN_TEST(test_is_ip_in_cidr_no_prefix);
    RUN_TEST(test_is_ip_in_cidr_invalid);
    RUN_TEST(test_is_ip_internal_private_ranges);
    RUN_TEST(test_is_ip_internal_public_ips);
    RUN_TEST(test_is_ip_internal_invalid);

    return UNITY_END();
}