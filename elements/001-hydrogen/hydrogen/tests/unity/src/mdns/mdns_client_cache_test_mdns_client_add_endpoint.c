#include <src/hydrogen.h>
#include <unity.h>

#include <src/mdns/mdns_client.h>

void test_mdns_client_add_endpoint_dedup(void);
void test_mdns_client_add_endpoint_order(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_mdns_client_add_endpoint_dedup(void) {
    mdns_client_service_t svc;
    const uint8_t a4[4] = {192, 0, 2, 1};

    memset(&svc, 0, sizeof svc);
    strncpy(svc.host, "host.local", sizeof svc.host - 1);
    TEST_ASSERT_EQUAL_INT(1, mdns_client_add_endpoint(&svc, AF_INET, a4, 4, 1));
    TEST_ASSERT_EQUAL_INT(0, mdns_client_add_endpoint(&svc, AF_INET, a4, 4, 1));
    TEST_ASSERT_EQUAL_UINT(1, svc.nendpoints);
}

void test_mdns_client_add_endpoint_order(void) {
    mdns_client_service_t svc;
    const uint8_t v4[4] = {192, 0, 2, 9};
    const uint8_t ll[16] = {0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
    const uint8_t g6[16] = {0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};

    memset(&svc, 0, sizeof svc);
    TEST_ASSERT_EQUAL_INT(1, mdns_client_add_endpoint(&svc, AF_INET6, ll, 16, 2));
    TEST_ASSERT_EQUAL_INT(1, mdns_client_add_endpoint(&svc, AF_INET6, g6, 16, 2));
    TEST_ASSERT_EQUAL_INT(1, mdns_client_add_endpoint(&svc, AF_INET, v4, 4, 1));
    TEST_ASSERT_EQUAL_INT(AF_INET, svc.endpoints[0].family);
    TEST_ASSERT_EQUAL_INT(AF_INET6, svc.endpoints[1].family);
    TEST_ASSERT_EQUAL_INT(1, mdns_client_endpoint_rank(AF_INET6, g6, 16));
    TEST_ASSERT_EQUAL_INT(2, mdns_client_endpoint_rank(AF_INET6, ll, 16));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_mdns_client_add_endpoint_dedup);
    RUN_TEST(test_mdns_client_add_endpoint_order);
    return UNITY_END();
}
