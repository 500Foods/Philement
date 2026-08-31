/*
 * Unity Test File: mdns_server_respond_test_mdns_server_put_host_nsec.c
 * Tests mdns_server_put_host_nsec bitmap of present families + NSEC
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mdns/mdns_server.h>
#include <src/mdns/mdns_wire.h>

void test_put_host_nsec_ipv4_only(void);
void test_put_host_nsec_nulls(void);

void setUp(void)
{
}

void tearDown(void)
{
}

void test_put_host_nsec_ipv4_only(void)
{
    mdns_server_interface_t iface;
    char *ips[1];
    uint8_t storage[256];
    mdns_buf b;
    mdns_msg msg;
    uint16_t count = 0;
    size_t off;
    uint8_t window;
    uint8_t blen;

    memset(&iface, 0, sizeof iface);
    ips[0] = (char *)"192.0.2.10";
    iface.ip_addresses = ips;
    iface.num_addresses = 1;

    mdns_buf_init(&b, storage, sizeof storage);
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, 0));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, DNS_FLAG_RESPONSE));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, 0));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, 1));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, 0));
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, 0));
    TEST_ASSERT_EQUAL_INT(0, mdns_server_put_host_nsec(&b, "host.local", &iface, MDNS_TTL_HOST, 1, &count));
    TEST_ASSERT_EQUAL_UINT16(1, count);
    TEST_ASSERT_EQUAL_INT(0, mdns_parse(storage, b.len, &msg));
    TEST_ASSERT_EQUAL_UINT16(RR_NSEC, msg.rr[0].type);
    TEST_ASSERT_TRUE(mdns_name_equal(msg.rr[0].name, "host.local"));

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
    TEST_ASSERT_TRUE(blen >= 1);
    TEST_ASSERT_TRUE((storage[off + 2 + (MDNS_TYPE_A / 8)] & (uint8_t)(0x80u >> (MDNS_TYPE_A % 8))) != 0);
    TEST_ASSERT_TRUE((storage[off + 2 + (RR_NSEC / 8)] & (uint8_t)(0x80u >> (RR_NSEC % 8))) != 0);
    if (blen > (MDNS_TYPE_AAAA / 8)) {
        TEST_ASSERT_TRUE((storage[off + 2 + (MDNS_TYPE_AAAA / 8)] & (uint8_t)(0x80u >> (MDNS_TYPE_AAAA % 8))) == 0);
    }
}

void test_put_host_nsec_nulls(void)
{
    uint16_t count = 0;

    TEST_ASSERT_EQUAL_INT(0, mdns_server_put_host_nsec(NULL, NULL, NULL, 0, 0, &count));
    TEST_ASSERT_EQUAL_UINT16(0, count);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_put_host_nsec_ipv4_only);
    RUN_TEST(test_put_host_nsec_nulls);
    return UNITY_END();
}
