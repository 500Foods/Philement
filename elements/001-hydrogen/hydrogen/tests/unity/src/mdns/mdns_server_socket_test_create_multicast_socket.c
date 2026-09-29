/*
 * Unity Test: mdns_server_socket_test_create_multicast_socket.c
 * Tests create_multicast_socket function for additional coverage
 * This function handles socket creation and multicast setup
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mdns/mdns_keys.h>
#include <src/mdns/mdns_server.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <unity/mocks/mock_system.h>

// Test function prototypes
void test_create_multicast_socket_ipv4_success(void);
void test_create_multicast_socket_ipv6_success(void);
void test_create_multicast_socket_ipv6_v6only_failure(void);
void test_create_multicast_socket_invalid_interface(void);
void test_create_multicast_socket_null_interface(void);
void test_create_multicast_socket_socket_failure(void);
void test_create_multicast_socket_setsockopt_reuseaddr_failure(void);
void test_create_multicast_socket_setsockopt_reuseport_failure(void);
void test_create_multicast_socket_bind_failure(void);
void test_create_multicast_socket_setsockopt_ttl_failure(void);

void setUp(void) {
    mock_system_reset_all();
}

void tearDown(void) {
    mock_system_reset_all();
}

// Test successful IPv4 multicast socket creation
void test_create_multicast_socket_ipv4_success(void) {
    mock_system_set_if_nametoindex_result(1);
    mock_system_set_inet_addr_result(INADDR_ANY);

    int sockfd = create_multicast_socket(AF_INET, MDNS_GROUP_V4, "lo");

    TEST_ASSERT_GREATER_OR_EQUAL_INT(0, sockfd);
    if (sockfd >= 0) {
        close(sockfd);
    }
}

// Test successful IPv6 multicast socket creation
void test_create_multicast_socket_ipv6_success(void) {
    mock_system_set_if_nametoindex_result(1);

    int sockfd = create_multicast_socket(AF_INET6, MDNS_GROUP_V6, "lo");

    TEST_ASSERT_GREATER_OR_EQUAL_INT(0, sockfd);
    if (sockfd >= 0) {
        close(sockfd);
    }
}

// Test socket creation with invalid interface
void test_create_multicast_socket_invalid_interface(void) {
    mock_system_set_if_nametoindex_result(0);

    int sockfd = create_multicast_socket(AF_INET, MDNS_GROUP_V4, "nonexistent_interface_12345");

    TEST_ASSERT_TRUE(sockfd < 0);
}

// Test socket creation with NULL interface
void test_create_multicast_socket_null_interface(void) {
    int sockfd = create_multicast_socket(AF_INET, MDNS_GROUP_V4, NULL);

    TEST_ASSERT_TRUE(sockfd < 0);
}

// Test socket() failure
void test_create_multicast_socket_socket_failure(void) {
    mock_system_set_socket_failure(1);

    int sockfd = create_multicast_socket(AF_INET, MDNS_GROUP_V4, "lo");
    TEST_ASSERT_EQUAL_INT(-1, sockfd);

    mock_system_reset_all();
}

// Test setsockopt(SO_BINDTODEVICE → SO_REUSEADDR) failure
// setsockopt call sequence:
// Call 1: SO_BINDTODEVICE (line 30)
// Call 2: SO_REUSEADDR (line 44)
void test_create_multicast_socket_setsockopt_reuseaddr_failure(void) {
    mock_system_set_setsockopt_fail_at(2);

    int sockfd = create_multicast_socket(AF_INET, MDNS_GROUP_V4, "lo");
    TEST_ASSERT_EQUAL_INT(-1, sockfd);

    mock_system_reset_all();
}

// Test setsockopt(SO_REUSEPORT) failure
// Call 3: SO_REUSEPORT (line 50)
void test_create_multicast_socket_setsockopt_reuseport_failure(void) {
    mock_system_set_setsockopt_fail_at(3);

    int sockfd = create_multicast_socket(AF_INET, MDNS_GROUP_V4, "lo");
    TEST_ASSERT_EQUAL_INT(-1, sockfd);

    mock_system_reset_all();
}

// Test bind() failure
void test_create_multicast_socket_bind_failure(void) {
    mock_system_set_bind_failure(1);

    int sockfd = create_multicast_socket(AF_INET, MDNS_GROUP_V4, "lo");
    TEST_ASSERT_EQUAL_INT(-1, sockfd);

    mock_system_reset_all();
}

// Test setsockopt(IPV6_V6ONLY) failure (IPv6 only)
// For IPv6, setsockopt call sequence:
// Call 1: SO_BINDTODEVICE (line 30)
// Call 2: SO_REUSEADDR (line 44)
// Call 3: SO_REUSEPORT (line 50)
// Call 4: IPV6_V6ONLY (line 57)
void test_create_multicast_socket_ipv6_v6only_failure(void) {
    mock_system_set_setsockopt_fail_at(4);

    int sockfd = create_multicast_socket(AF_INET6, MDNS_GROUP_V6, "lo");
    TEST_ASSERT_EQUAL_INT(-1, sockfd);

    mock_system_reset_all();
}

// Test setsockopt(IP_MULTICAST_TTL) failure
// Call 5: IP_MULTICAST_TTL (line 85)
void test_create_multicast_socket_setsockopt_ttl_failure(void) {
    mock_system_set_setsockopt_fail_at(5);

    int sockfd = create_multicast_socket(AF_INET, MDNS_GROUP_V4, "lo");
    TEST_ASSERT_EQUAL_INT(-1, sockfd);

    mock_system_reset_all();
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_create_multicast_socket_ipv4_success);
    RUN_TEST(test_create_multicast_socket_ipv6_success);
    RUN_TEST(test_create_multicast_socket_ipv6_v6only_failure);
    RUN_TEST(test_create_multicast_socket_invalid_interface);
    RUN_TEST(test_create_multicast_socket_null_interface);
    RUN_TEST(test_create_multicast_socket_socket_failure);
    RUN_TEST(test_create_multicast_socket_setsockopt_reuseaddr_failure);
    RUN_TEST(test_create_multicast_socket_setsockopt_reuseport_failure);
    RUN_TEST(test_create_multicast_socket_bind_failure);
    RUN_TEST(test_create_multicast_socket_setsockopt_ttl_failure);

    return UNITY_END();
}
