/*
 * Unity Test File: mdns_wire_test_mdns_put_rr_nsec.c
 * Tests mdns_put_rr_nsec window-0 type bitmap
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mdns/mdns_wire.h>

void test_mdns_put_rr_nsec_bitmap(void);
void test_mdns_put_rr_nsec_empty_types(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_mdns_put_rr_nsec_bitmap(void) {
    uint8_t storage[128];
    mdns_buf b;
    mdns_msg msg;
    uint16_t types[3];
    size_t off;
    uint8_t window;
    uint8_t blen;

    types[0] = MDNS_TYPE_A;
    types[1] = MDNS_TYPE_AAAA;
    types[2] = (uint16_t)RR_NSEC;

    mdns_buf_init(&b, storage, sizeof storage);
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, 0));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, DNS_FLAG_RESPONSE));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, 0));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, 1));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, 0));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, 0));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_rr_nsec(&b, "host.local", types, 3, 120, 1));

    TEST_ASSERT_EQUAL_INT(0, mdns_parse(storage, b.len, &msg));
    TEST_ASSERT_EQUAL_UINT(1, msg.nrr);
    TEST_ASSERT_EQUAL_UINT16(RR_NSEC, msg.rr[0].type);
    TEST_ASSERT_EQUAL_UINT16(DNS_CLASS_IN | DNS_CACHE_FLUSH, msg.rr[0].cls);

    off = msg.rr[0].rdoff;
    {
        char nsec_name[MDNS_NAME_MAX];
        size_t next = 0;

        TEST_ASSERT_EQUAL_INT(0, mdns_name_decode(storage, b.len, off, nsec_name,
                                                  sizeof nsec_name, &next));
        TEST_ASSERT_TRUE(mdns_name_equal(nsec_name, "host.local"));
        off = next;
    }
    window = storage[off];
    blen = storage[off + 1];
    TEST_ASSERT_EQUAL_UINT8(0, window);
    TEST_ASSERT_TRUE(blen >= 6);
    TEST_ASSERT_TRUE((storage[off + 2 + (MDNS_TYPE_A / 8)] & (uint8_t)(0x80u >> (MDNS_TYPE_A % 8))) != 0);
    TEST_ASSERT_TRUE((storage[off + 2 + (MDNS_TYPE_AAAA / 8)] & (uint8_t)(0x80u >> (MDNS_TYPE_AAAA % 8))) != 0);
    TEST_ASSERT_TRUE((storage[off + 2 + (RR_NSEC / 8)] & (uint8_t)(0x80u >> (RR_NSEC % 8))) != 0);
}

void test_mdns_put_rr_nsec_empty_types(void) {
    uint8_t storage[64];
    mdns_buf b;

    mdns_buf_init(&b, storage, sizeof storage);
    TEST_ASSERT_EQUAL_INT(0, mdns_put_rr_nsec(&b, "host.local", NULL, 0, 120, 0));
    TEST_ASSERT_EQUAL_INT(0, b.overflow);
    TEST_ASSERT_TRUE(b.len > 0);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_mdns_put_rr_nsec_bitmap);
    RUN_TEST(test_mdns_put_rr_nsec_empty_types);
    return UNITY_END();
}
