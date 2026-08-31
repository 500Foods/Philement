#include <src/hydrogen.h>
#include <unity.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <src/mdns/mdns_client.h>

void test_mdns_client_tcp_check_listen(void);
void test_mdns_client_tcp_check_refused(void);
void test_mdns_client_tcp_check_null(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_mdns_client_tcp_check_listen(void) {
#ifdef USE_MOCK_SYSTEM
    TEST_IGNORE_MESSAGE("socket/connect mocked; live listen covered by Test 25");
#else
    int listen_fd;
    struct sockaddr_in addr;
    socklen_t alen = sizeof addr;
    mdns_client_endpoint_t ep;
    uint16_t port;

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    TEST_ASSERT_TRUE(listen_fd >= 0);
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    TEST_ASSERT_EQUAL_INT(0, bind(listen_fd, (struct sockaddr *)&addr, sizeof addr));
    TEST_ASSERT_EQUAL_INT(0, listen(listen_fd, 1));
    TEST_ASSERT_EQUAL_INT(0, getsockname(listen_fd, (struct sockaddr *)&addr, &alen));
    port = ntohs(addr.sin_port);

    memset(&ep, 0, sizeof ep);
    ep.family = AF_INET;
    ep.addrlen = 4;
    ep.addr[0] = 127;
    ep.addr[3] = 1;
    TEST_ASSERT_EQUAL_INT(0, mdns_client_tcp_check(&ep, port, 500));
    close(listen_fd);
#endif
}

void test_mdns_client_tcp_check_null(void) {
    TEST_ASSERT_EQUAL_INT(-1, mdns_client_tcp_check(NULL, 80, 100));
}

void test_mdns_client_tcp_check_refused(void) {
    mdns_client_endpoint_t ep;

    memset(&ep, 0, sizeof ep);
    ep.family = AF_INET;
    ep.addrlen = 4;
    ep.addr[0] = 127;
    ep.addr[3] = 1;
    TEST_ASSERT_EQUAL_INT(-1, mdns_client_tcp_check(&ep, 1, 200));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_mdns_client_tcp_check_listen);
    RUN_TEST(test_mdns_client_tcp_check_null);
    RUN_TEST(test_mdns_client_tcp_check_refused);
    return UNITY_END();
}
