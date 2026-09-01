/*
 * Unity Test: mdns_server_respond_test_helpers.c
 * Tests helper functions from mdns_server_respond.c:
 * sockaddr_port, iface_for_sock, response_ttl, iface_has_af,
 * want_apply_missing_family, put_host_addrs, put_host_nsec, put_service_bits
 *
 * CHANGELOG
 * 1.0.0 - 2026-09-01 - Initial creation
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mdns/mdns_server.h>
#include <src/mdns/mdns_wire.h>

/* Function prototypes */
uint16_t mdns_server_sockaddr_port(const void *src_addr, uint32_t src_len);
const mdns_server_interface_t *mdns_server_iface_for_sock(const mdns_server_t *server, int sockfd);
uint32_t mdns_server_response_ttl(uint32_t base, int legacy);
int mdns_server_iface_has_af(const mdns_server_interface_t *iface, int family);
void mdns_server_want_apply_missing_family(mdns_server_want_t *w, const mdns_server_interface_t *iface);
int mdns_server_put_host_addrs(mdns_buf *b, const char *hostname, const mdns_server_interface_t *iface,
                               uint32_t ttl, int flush, uint32_t bits, uint16_t *count);
int mdns_server_put_host_nsec(mdns_buf *b, const char *hostname, const mdns_server_interface_t *iface,
                              uint32_t ttl, int flush, uint16_t *count);
int mdns_server_put_service_bits(mdns_buf *b, const mdns_server_t *server, size_t si, const char *hostname,
                                 uint32_t bits, uint32_t shared_ttl, uint32_t host_ttl, int flush, uint16_t *count);

/* Test prototypes */
void test_sockaddr_port_null(void);
void test_sockaddr_port_ipv4(void);
void test_sockaddr_port_ipv6(void);
void test_sockaddr_port_small_len(void);
void test_iface_for_sock_null_server(void);
void test_iface_for_sock_match_v4(void);
void test_iface_for_sock_no_match(void);
void test_response_ttl_legacy_cap(void);
void test_response_ttl_modern(void);
void test_iface_has_af_null(void);
void test_iface_has_af_v4(void);
void test_iface_has_af_v6(void);
void test_want_apply_missing_family_null(void);
void test_want_apply_missing_family_no_v4(void);
void test_want_apply_missing_family_no_v6(void);
void test_put_host_addrs_null(void);
void test_put_host_addrs_ipv4(void);
void test_put_host_addrs_null_ip_addresses(void);
void test_put_host_nsec_null(void);
void test_put_host_nsec_basic(void);
void test_put_service_bits_null(void);
void test_put_service_bits_sd(void);
void test_put_service_bits_srv(void);

void setUp(void) {}
void tearDown(void) {}

/* sockaddr_port tests */
void test_sockaddr_port_null(void) {
    TEST_ASSERT_EQUAL_UINT16(MDNS_PORT, mdns_server_sockaddr_port(NULL, 0));
}

void test_sockaddr_port_ipv4(void) {
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(5353);
    TEST_ASSERT_EQUAL_UINT16(5353, mdns_server_sockaddr_port(&sa, sizeof(sa)));
}

void test_sockaddr_port_ipv6(void) {
    struct sockaddr_in6 sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin6_family = AF_INET6;
    sa.sin6_port = htons(12345);
    TEST_ASSERT_EQUAL_UINT16(12345, mdns_server_sockaddr_port(&sa, sizeof(sa)));
}

void test_sockaddr_port_small_len(void) {
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    TEST_ASSERT_EQUAL_UINT16(MDNS_PORT, mdns_server_sockaddr_port(&sa, 1));
}

/* iface_for_sock tests */
void test_iface_for_sock_null_server(void) {
    TEST_ASSERT_NULL(mdns_server_iface_for_sock(NULL, 0));
}

void test_iface_for_sock_match_v4(void) {
    mdns_server_interface_t ifaces[1];
    mdns_server_t server;
    memset(ifaces, 0, sizeof(ifaces));
    memset(&server, 0, sizeof(server));
    ifaces[0].sockfd_v4 = 42;
    server.interfaces = ifaces;
    server.num_interfaces = 1;

    const mdns_server_interface_t *result = mdns_server_iface_for_sock(&server, 42);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(42, result->sockfd_v4);
}

void test_iface_for_sock_no_match(void) {
    mdns_server_interface_t ifaces[1];
    mdns_server_t server;
    memset(ifaces, 0, sizeof(ifaces));
    memset(&server, 0, sizeof(server));
    ifaces[0].sockfd_v4 = 42;
    server.interfaces = ifaces;
    server.num_interfaces = 1;

    TEST_ASSERT_NULL(mdns_server_iface_for_sock(&server, 99));
}

/* response_ttl tests */
void test_response_ttl_legacy_cap(void) {
    TEST_ASSERT_EQUAL_UINT32(MDNS_LEGACY_TTL_CAP, mdns_server_response_ttl(MDNS_TTL_HOST, 1));
}

void test_response_ttl_modern(void) {
    TEST_ASSERT_EQUAL_UINT32(MDNS_TTL_HOST, mdns_server_response_ttl(MDNS_TTL_HOST, 0));
}

/* iface_has_af tests */
void test_iface_has_af_null(void) {
    TEST_ASSERT_EQUAL_INT(0, mdns_server_iface_has_af(NULL, AF_INET));
}

void test_iface_has_af_v4(void) {
    mdns_server_interface_t iface;
    char *ips[] = {(char *)"192.168.1.100"};
    memset(&iface, 0, sizeof(iface));
    iface.ip_addresses = ips;
    iface.num_addresses = 1;

    TEST_ASSERT_EQUAL_INT(1, mdns_server_iface_has_af(&iface, AF_INET));
    TEST_ASSERT_EQUAL_INT(0, mdns_server_iface_has_af(&iface, AF_INET6));
}

void test_iface_has_af_v6(void) {
    mdns_server_interface_t iface;
    char *ips[] = {(char *)"2001:db8::1"};
    memset(&iface, 0, sizeof(iface));
    iface.ip_addresses = ips;
    iface.num_addresses = 1;

    TEST_ASSERT_EQUAL_INT(0, mdns_server_iface_has_af(&iface, AF_INET));
    TEST_ASSERT_EQUAL_INT(1, mdns_server_iface_has_af(&iface, AF_INET6));
}

/* want_apply_missing_family tests */
void test_want_apply_missing_family_null(void) {
    mdns_server_want_t want;
    mdns_server_interface_t iface;
    memset(&want, 0, sizeof(want));
    memset(&iface, 0, sizeof(iface));
    want.host_answer = MDNS_W_A;
    mdns_server_want_apply_missing_family(NULL, &iface);
    mdns_server_want_apply_missing_family(&want, NULL);
    TEST_ASSERT_EQUAL_UINT32(MDNS_W_A, want.host_answer);
}

void test_want_apply_missing_family_no_v4(void) {
    mdns_server_want_t want;
    mdns_server_interface_t iface;
    char *ips[] = {(char *)"2001:db8::1"};
    memset(&want, 0, sizeof(want));
    memset(&iface, 0, sizeof(iface));
    want.host_answer = MDNS_W_A | MDNS_W_AAAA;
    want.host_additional = MDNS_W_A;
    iface.ip_addresses = ips;
    iface.num_addresses = 1;

    mdns_server_want_apply_missing_family(&want, &iface);
    TEST_ASSERT_EQUAL_INT(0, (want.host_answer & MDNS_W_A) != 0);
    TEST_ASSERT_EQUAL_INT(1, (want.host_answer & MDNS_W_AAAA) != 0);
    TEST_ASSERT_EQUAL_INT(1, (want.host_answer & MDNS_W_NSEC) != 0);
    TEST_ASSERT_EQUAL_INT(0, (want.host_additional & MDNS_W_A) != 0);
    TEST_ASSERT_EQUAL_INT(1, (want.host_additional & MDNS_W_NSEC) != 0);
}

void test_want_apply_missing_family_no_v6(void) {
    mdns_server_want_t want;
    mdns_server_interface_t iface;
    char *ips[] = {(char *)"192.168.1.100"};
    memset(&want, 0, sizeof(want));
    memset(&iface, 0, sizeof(iface));
    want.host_answer = MDNS_W_A | MDNS_W_AAAA;
    want.host_additional = MDNS_W_AAAA;
    iface.ip_addresses = ips;
    iface.num_addresses = 1;

    mdns_server_want_apply_missing_family(&want, &iface);
    TEST_ASSERT_EQUAL_INT(1, (want.host_answer & MDNS_W_A) != 0);
    TEST_ASSERT_EQUAL_INT(0, (want.host_answer & MDNS_W_AAAA) != 0);
    TEST_ASSERT_EQUAL_INT(1, (want.host_answer & MDNS_W_NSEC) != 0);
}

/* put_host_addrs tests */
void test_put_host_addrs_null(void) {
    uint16_t count = 0;
    TEST_ASSERT_EQUAL_INT(0, mdns_server_put_host_addrs(NULL, NULL, NULL, 0, 0, 0, &count));
    TEST_ASSERT_EQUAL_UINT16(0, count);
}

void test_put_host_addrs_ipv4(void) {
    uint8_t packet[2048];
    mdns_buf b;
    mdns_server_interface_t iface;
    char *ips[] = {(char *)"192.168.1.100"};
    uint16_t count = 0;

    memset(&iface, 0, sizeof(iface));
    iface.ip_addresses = ips;
    iface.num_addresses = 1;
    mdns_buf_init(&b, packet, sizeof(packet));

    TEST_ASSERT_EQUAL_INT(0, mdns_server_put_host_addrs(&b, "host.local", &iface, 120, 1, MDNS_W_A, &count));
    TEST_ASSERT_EQUAL_UINT16(1, count);
}

void test_put_host_addrs_null_ip_addresses(void) {
    uint8_t packet[2048];
    mdns_buf b;
    mdns_server_interface_t iface;
    uint16_t count = 0;

    memset(&iface, 0, sizeof(iface));
    iface.ip_addresses = NULL;
    iface.num_addresses = 1;
    mdns_buf_init(&b, packet, sizeof(packet));

    TEST_ASSERT_EQUAL_INT(0, mdns_server_put_host_addrs(&b, "host.local", &iface, 120, 1, MDNS_W_A, &count));
    TEST_ASSERT_EQUAL_UINT16(0, count);
}

/* put_host_nsec tests */
void test_put_host_nsec_null(void) {
    uint16_t count = 0;
    TEST_ASSERT_EQUAL_INT(0, mdns_server_put_host_nsec(NULL, NULL, NULL, 0, 0, &count));
    TEST_ASSERT_EQUAL_UINT16(0, count);
}

void test_put_host_nsec_basic(void) {
    uint8_t packet[2048];
    mdns_buf b;
    mdns_server_interface_t iface;
    char *ips[] = {(char *)"192.168.1.100"};
    uint16_t count = 0;

    memset(&iface, 0, sizeof(iface));
    iface.ip_addresses = ips;
    iface.num_addresses = 1;
    mdns_buf_init(&b, packet, sizeof(packet));

    TEST_ASSERT_EQUAL_INT(0, mdns_server_put_host_nsec(&b, "host.local", &iface, 120, 1, &count));
    TEST_ASSERT_GREATER_THAN_UINT16(0, count);
}

/* put_service_bits tests */
void test_put_service_bits_null(void) {
    uint16_t count = 0;
    TEST_ASSERT_EQUAL_INT(0, mdns_server_put_service_bits(NULL, NULL, 0, NULL, 0, 0, 0, 0, &count));
    TEST_ASSERT_EQUAL_UINT16(0, count);
}

void test_put_service_bits_sd(void) {
    uint8_t packet[2048];
    mdns_buf b;
    mdns_server_t server;
    mdns_server_service_t svc;
    char name[] = "Test";
    char type[] = "_http._tcp.local";
    uint16_t count = 0;

    memset(&server, 0, sizeof(server));
    memset(&svc, 0, sizeof(svc));
    svc.name = name;
    svc.type = type;
    svc.port = 80;
    server.services = &svc;
    server.num_services = 1;
    mdns_buf_init(&b, packet, sizeof(packet));

    TEST_ASSERT_EQUAL_INT(0, mdns_server_put_service_bits(&b, &server, 0, "host.local", MDNS_W_SD, 4500, 120, 1, &count));
    TEST_ASSERT_GREATER_THAN_UINT16(0, count);
}

void test_put_service_bits_srv(void) {
    uint8_t packet[2048];
    mdns_buf b;
    mdns_server_t server;
    mdns_server_service_t svc;
    char name[] = "Test";
    char type[] = "_http._tcp.local";
    uint16_t count = 0;

    memset(&server, 0, sizeof(server));
    memset(&svc, 0, sizeof(svc));
    svc.name = name;
    svc.type = type;
    svc.port = 8080;
    server.services = &svc;
    server.num_services = 1;
    mdns_buf_init(&b, packet, sizeof(packet));

    TEST_ASSERT_EQUAL_INT(0, mdns_server_put_service_bits(&b, &server, 0, "host.local", MDNS_W_SRV | MDNS_W_TXT, 4500, 120, 1, &count));
    TEST_ASSERT_GREATER_THAN_UINT16(1, count);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_sockaddr_port_null);
    RUN_TEST(test_sockaddr_port_ipv4);
    RUN_TEST(test_sockaddr_port_ipv6);
    RUN_TEST(test_sockaddr_port_small_len);
    RUN_TEST(test_iface_for_sock_null_server);
    RUN_TEST(test_iface_for_sock_match_v4);
    RUN_TEST(test_iface_for_sock_no_match);
    RUN_TEST(test_response_ttl_legacy_cap);
    RUN_TEST(test_response_ttl_modern);
    RUN_TEST(test_iface_has_af_null);
    RUN_TEST(test_iface_has_af_v4);
    RUN_TEST(test_iface_has_af_v6);
    RUN_TEST(test_want_apply_missing_family_null);
    RUN_TEST(test_want_apply_missing_family_no_v4);
    RUN_TEST(test_want_apply_missing_family_no_v6);
    RUN_TEST(test_put_host_addrs_null);
    RUN_TEST(test_put_host_addrs_ipv4);
    RUN_TEST(test_put_host_addrs_null_ip_addresses);
    RUN_TEST(test_put_host_nsec_null);
    RUN_TEST(test_put_host_nsec_basic);
    RUN_TEST(test_put_service_bits_null);
    RUN_TEST(test_put_service_bits_sd);
    RUN_TEST(test_put_service_bits_srv);

    return UNITY_END();
}
