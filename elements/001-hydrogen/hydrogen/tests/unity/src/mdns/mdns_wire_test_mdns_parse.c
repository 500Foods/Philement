/*
 * Unity Test File: mdns_wire_test_mdns_parse.c
 * Tests mdns_parse sections and PTR/SRV/TXT/A/AAAA round-trip
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mdns/mdns_wire.h>

void test_mdns_parse_questions(void);
void test_mdns_parse_sections(void);
void test_mdns_parse_round_trip(void);
void test_mdns_parse_too_short(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_mdns_parse_questions(void) {
    uint8_t storage[128];
    mdns_buf b;
    mdns_msg msg;

    mdns_buf_init(&b, storage, sizeof storage);
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, 0x1234));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, DNS_FLAG_QUERY));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, 1));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, 0));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, 0));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, 0));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_name(&b, "_http._tcp.local"));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, MDNS_TYPE_PTR));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, DNS_CLASS_IN | DNS_QU_BIT));

    TEST_ASSERT_EQUAL_INT(0, mdns_parse(storage, b.len, &msg));
    TEST_ASSERT_EQUAL_UINT16(0x1234, msg.id);
    TEST_ASSERT_EQUAL_UINT(1, msg.nquestions);
    TEST_ASSERT_TRUE(mdns_name_equal(msg.questions[0].name, "_http._tcp.local"));
    TEST_ASSERT_EQUAL_UINT16(MDNS_TYPE_PTR, msg.questions[0].type);
    TEST_ASSERT_EQUAL_UINT16(DNS_CLASS_IN | DNS_QU_BIT, msg.questions[0].cls);
}

void test_mdns_parse_sections(void) {
    uint8_t storage[256];
    mdns_buf b;
    mdns_msg msg;
    size_t pos;

    mdns_buf_init(&b, storage, sizeof storage);
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, 0));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, DNS_FLAG_RESPONSE));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, 0));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, 1));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, 1));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, 1));

    TEST_ASSERT_EQUAL_INT(0, mdns_rr_head(&b, "ans.local", MDNS_TYPE_A, 120, 0, &pos));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u32(&b, 0x7f000001u));
    TEST_ASSERT_EQUAL_INT(0, mdns_rr_tail(&b, pos));

    TEST_ASSERT_EQUAL_INT(0, mdns_rr_head(&b, "auth.local", MDNS_TYPE_A, 120, 0, &pos));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u32(&b, 0x7f000002u));
    TEST_ASSERT_EQUAL_INT(0, mdns_rr_tail(&b, pos));

    TEST_ASSERT_EQUAL_INT(0, mdns_rr_head(&b, "add.local", MDNS_TYPE_A, 120, 0, &pos));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u32(&b, 0x7f000003u));
    TEST_ASSERT_EQUAL_INT(0, mdns_rr_tail(&b, pos));

    TEST_ASSERT_EQUAL_INT(0, mdns_parse(storage, b.len, &msg));
    TEST_ASSERT_EQUAL_UINT(3, msg.nrr);
    TEST_ASSERT_EQUAL_INT(MDNS_SEC_ANSWER, msg.rr[0].section);
    TEST_ASSERT_EQUAL_INT(MDNS_SEC_AUTHORITY, msg.rr[1].section);
    TEST_ASSERT_EQUAL_INT(MDNS_SEC_ADDITIONAL, msg.rr[2].section);
    TEST_ASSERT_TRUE(mdns_name_equal(msg.rr[0].name, "ans.local"));
    TEST_ASSERT_TRUE(mdns_name_equal(msg.rr[1].name, "auth.local"));
    TEST_ASSERT_TRUE(mdns_name_equal(msg.rr[2].name, "add.local"));
}

void test_mdns_parse_round_trip(void) {
    uint8_t storage[512];
    uint8_t addr6[16];
    mdns_buf b;
    mdns_msg msg;
    size_t pos;
    char ptr_name[MDNS_NAME_MAX];
    char srv_target[MDNS_NAME_MAX];
    char txt[32];
    uint16_t prio = 0;
    uint16_t weight = 0;
    uint16_t port = 0;

    memset(addr6, 0, sizeof addr6);
    addr6[15] = 1;

    mdns_buf_init(&b, storage, sizeof storage);
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, 0));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, DNS_FLAG_RESPONSE));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, 0));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, 5));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, 0));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, 0));

    TEST_ASSERT_EQUAL_INT(0, mdns_rr_head(&b, "_http._tcp.local", MDNS_TYPE_PTR, 4500, 0, &pos));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_name(&b, "Printer._http._tcp.local"));
    TEST_ASSERT_EQUAL_INT(0, mdns_rr_tail(&b, pos));

    TEST_ASSERT_EQUAL_INT(0, mdns_rr_head(&b, "Printer._http._tcp.local", MDNS_TYPE_SRV, 120, 1, &pos));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, 0));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, 0));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, 8080));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_name(&b, "host.local"));
    TEST_ASSERT_EQUAL_INT(0, mdns_rr_tail(&b, pos));

    TEST_ASSERT_EQUAL_INT(0, mdns_rr_head(&b, "Printer._http._tcp.local", MDNS_TYPE_TXT, 4500, 1, &pos));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u8(&b, 10));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_bytes(&b, "path=/html", 10));
    TEST_ASSERT_EQUAL_INT(0, mdns_rr_tail(&b, pos));

    TEST_ASSERT_EQUAL_INT(0, mdns_rr_head(&b, "host.local", MDNS_TYPE_A, 120, 1, &pos));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u8(&b, 192));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u8(&b, 168));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u8(&b, 1));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u8(&b, 10));
    TEST_ASSERT_EQUAL_INT(0, mdns_rr_tail(&b, pos));

    TEST_ASSERT_EQUAL_INT(0, mdns_rr_head(&b, "host.local", MDNS_TYPE_AAAA, 120, 1, &pos));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_bytes(&b, addr6, 16));
    TEST_ASSERT_EQUAL_INT(0, mdns_rr_tail(&b, pos));

    TEST_ASSERT_EQUAL_INT(0, mdns_parse(storage, b.len, &msg));
    TEST_ASSERT_EQUAL_UINT(5, msg.nrr);
    TEST_ASSERT_EQUAL_UINT16(MDNS_TYPE_PTR, msg.rr[0].type);
    TEST_ASSERT_EQUAL_UINT16(MDNS_TYPE_SRV, msg.rr[1].type);
    TEST_ASSERT_EQUAL_UINT16(MDNS_TYPE_TXT, msg.rr[2].type);
    TEST_ASSERT_EQUAL_UINT16(MDNS_TYPE_A, msg.rr[3].type);
    TEST_ASSERT_EQUAL_UINT16(MDNS_TYPE_AAAA, msg.rr[4].type);
    TEST_ASSERT_EQUAL_UINT16(DNS_CLASS_IN, msg.rr[0].cls);
    TEST_ASSERT_EQUAL_UINT16(DNS_CLASS_IN | DNS_CACHE_FLUSH, msg.rr[1].cls);

    TEST_ASSERT_EQUAL_INT(0, mdns_rdata_name(storage, b.len, &msg.rr[0], ptr_name, sizeof ptr_name));
    TEST_ASSERT_TRUE(mdns_name_equal(ptr_name, "Printer._http._tcp.local"));

    TEST_ASSERT_EQUAL_INT(0, mdns_rdata_srv(storage, b.len, &msg.rr[1], &prio, &weight, &port,
                                            srv_target, sizeof srv_target));
    TEST_ASSERT_EQUAL_UINT16(8080, port);
    TEST_ASSERT_TRUE(mdns_name_equal(srv_target, "host.local"));

    TEST_ASSERT_EQUAL_INT(0, mdns_txt_get(storage + msg.rr[2].rdoff, msg.rr[2].rdlen, "path",
                                          txt, sizeof txt));
    TEST_ASSERT_EQUAL_STRING("/html", txt);

    TEST_ASSERT_EQUAL_UINT16(4, msg.rr[3].rdlen);
    TEST_ASSERT_EQUAL_UINT8(192, storage[msg.rr[3].rdoff]);
    TEST_ASSERT_EQUAL_UINT16(16, msg.rr[4].rdlen);
}

void test_mdns_parse_too_short(void) {
    uint8_t buf[8];
    mdns_msg msg;

    memset(buf, 0, sizeof buf);
    TEST_ASSERT_EQUAL_INT(-1, mdns_parse(buf, sizeof buf, &msg));
    TEST_ASSERT_EQUAL_INT(-1, mdns_parse(NULL, 12, &msg));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_mdns_parse_questions);
    RUN_TEST(test_mdns_parse_sections);
    RUN_TEST(test_mdns_parse_round_trip);
    RUN_TEST(test_mdns_parse_too_short);
    return UNITY_END();
}
