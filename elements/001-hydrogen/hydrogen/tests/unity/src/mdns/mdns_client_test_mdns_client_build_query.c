#include <src/hydrogen.h>
#include <unity.h>

#include <src/mdns/mdns_client.h>
#include <src/mdns/mdns_wire.h>

void test_mdns_client_build_query_ptr(void);
void test_mdns_client_build_query_null(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_mdns_client_build_query_ptr(void) {
    uint8_t packet[256];
    size_t len = 0;
    mdns_msg msg;

    TEST_ASSERT_EQUAL_INT(0, mdns_client_build_query(packet, sizeof packet, &len,
                                                     "_http._tcp.local", MDNS_TYPE_PTR));
    TEST_ASSERT_TRUE(len > 12);
    TEST_ASSERT_EQUAL_INT(0, mdns_parse(packet, len, &msg));
    TEST_ASSERT_EQUAL_UINT(1, msg.nquestions);
    TEST_ASSERT_TRUE(mdns_name_equal(msg.questions[0].name, "_http._tcp.local"));
    TEST_ASSERT_EQUAL_UINT16(MDNS_TYPE_PTR, msg.questions[0].type);
    TEST_ASSERT_EQUAL_UINT16(0, msg.flags & DNS_QR_BIT);
}

void test_mdns_client_build_query_null(void) {
    uint8_t packet[64];
    size_t len = 0;

    TEST_ASSERT_EQUAL_INT(-1, mdns_client_build_query(NULL, 64, &len, "a.local", MDNS_TYPE_PTR));
    TEST_ASSERT_EQUAL_INT(-1, mdns_client_build_query(packet, sizeof packet, NULL, "a.local", MDNS_TYPE_PTR));
    TEST_ASSERT_EQUAL_INT(-1, mdns_client_build_query(packet, sizeof packet, &len, NULL, MDNS_TYPE_PTR));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_mdns_client_build_query_ptr);
    RUN_TEST(test_mdns_client_build_query_null);
    return UNITY_END();
}
