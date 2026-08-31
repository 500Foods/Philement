/*
 * Unity Test File: mdns_wire_test_mdns_rdata_srv.c
 * Tests mdns_rdata_srv
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mdns/mdns_wire.h>

void test_mdns_rdata_srv_basic(void);
void test_mdns_rdata_srv_too_short(void);
void test_mdns_rdata_srv_null(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_mdns_rdata_srv_basic(void) {
    uint8_t storage[128];
    mdns_buf b;
    mdns_msg msg;
    size_t pos;
    uint16_t prio = 99;
    uint16_t weight = 99;
    uint16_t port = 0;
    char target[MDNS_NAME_MAX];

    mdns_buf_init(&b, storage, sizeof storage);
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, 0));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, DNS_FLAG_RESPONSE));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, 0));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, 1));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, 0));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, 0));
    TEST_ASSERT_EQUAL_INT(0, mdns_rr_head(&b, "svc.local", MDNS_TYPE_SRV, 120, 1, &pos));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, 1));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, 2));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, 8080));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_name(&b, "host.local"));
    TEST_ASSERT_EQUAL_INT(0, mdns_rr_tail(&b, pos));

    TEST_ASSERT_EQUAL_INT(0, mdns_parse(storage, b.len, &msg));
    TEST_ASSERT_EQUAL_INT(0, mdns_rdata_srv(storage, b.len, &msg.rr[0], &prio, &weight, &port,
                                            target, sizeof target));
    TEST_ASSERT_EQUAL_UINT16(1, prio);
    TEST_ASSERT_EQUAL_UINT16(2, weight);
    TEST_ASSERT_EQUAL_UINT16(8080, port);
    TEST_ASSERT_TRUE(mdns_name_equal(target, "host.local"));
}

void test_mdns_rdata_srv_too_short(void) {
    uint8_t storage[4] = {0, 1, 2, 3};
    mdns_rr rr;
    char target[8];

    memset(&rr, 0, sizeof rr);
    rr.rdoff = 0;
    rr.rdlen = 4;
    TEST_ASSERT_EQUAL_INT(-1, mdns_rdata_srv(storage, sizeof storage, &rr, NULL, NULL, NULL,
                                             target, sizeof target));
}

void test_mdns_rdata_srv_null(void) {
    mdns_rr rr;
    char target[8];

    memset(&rr, 0, sizeof rr);
    TEST_ASSERT_EQUAL_INT(-1, mdns_rdata_srv(NULL, 10, &rr, NULL, NULL, NULL, target, sizeof target));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_mdns_rdata_srv_basic);
    RUN_TEST(test_mdns_rdata_srv_too_short);
    RUN_TEST(test_mdns_rdata_srv_null);
    return UNITY_END();
}
