/*
 * Unity Test File: mdns_server_defend_test_mdns_server_rr_cmp.c
 * Tests RR wire comparison: class, type, then rdata bytes, then length.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mdns/mdns_server.h>
#include <src/mdns/mdns_wire.h>

void test_rr_cmp_equal(void);
void test_rr_cmp_class_differs(void);
void test_rr_cmp_type_differs(void);
void test_rr_cmp_rdata_differs(void);
void test_rr_cmp_length_differs(void);
void test_rr_cmp_null_args(void);

void setUp(void)
{
}

void tearDown(void)
{
}

void test_rr_cmp_equal(void)
{
    uint8_t a[] = {0x01, 0x02, 0x03};
    uint8_t b[] = {0x01, 0x02, 0x03};
    mdns_rr ra, rb;

    memset(&ra, 0, sizeof ra);
    memset(&rb, 0, sizeof rb);
    ra.cls = 1; ra.type = 1; ra.rdlen = 3; ra.rdoff = 0;
    rb.cls = 1; rb.type = 1; rb.rdlen = 3; rb.rdoff = 0;
    TEST_ASSERT_EQUAL_INT(0, mdns_server_rr_cmp(&ra, &rb, a, b, sizeof a, sizeof b));
}

void test_rr_cmp_class_differs(void)
{
    uint8_t a[] = {0x01};
    uint8_t b[] = {0x02};
    mdns_rr ra, rb;

    memset(&ra, 0, sizeof ra);
    memset(&rb, 0, sizeof rb);
    ra.cls = 1; ra.type = 1; ra.rdlen = 1; ra.rdoff = 0;
    rb.cls = 2; rb.type = 1; rb.rdlen = 1; rb.rdoff = 0;
    TEST_ASSERT_TRUE(mdns_server_rr_cmp(&ra, &rb, a, b, sizeof a, sizeof b) < 0);
}

void test_rr_cmp_type_differs(void)
{
    uint8_t a[] = {0x00};
    uint8_t b[] = {0x00};
    mdns_rr ra, rb;

    memset(&ra, 0, sizeof ra);
    memset(&rb, 0, sizeof rb);
    ra.cls = 1; ra.type = 1; ra.rdlen = 1; ra.rdoff = 0;
    rb.cls = 1; rb.type = 2; rb.rdlen = 1; rb.rdoff = 0;
    TEST_ASSERT_TRUE(mdns_server_rr_cmp(&ra, &rb, a, b, sizeof a, sizeof b) < 0);
}

void test_rr_cmp_rdata_differs(void)
{
    uint8_t a[] = {0x01, 0x02};
    uint8_t b[] = {0x01, 0x03};
    mdns_rr ra, rb;

    memset(&ra, 0, sizeof ra);
    memset(&rb, 0, sizeof rb);
    ra.cls = 1; ra.type = 1; ra.rdlen = 2; ra.rdoff = 0;
    rb.cls = 1; rb.type = 1; rb.rdlen = 2; rb.rdoff = 0;
    TEST_ASSERT_TRUE(mdns_server_rr_cmp(&ra, &rb, a, b, sizeof a, sizeof b) < 0);
}

void test_rr_cmp_length_differs(void)
{
    uint8_t a[] = {0x01, 0x02, 0x03};
    uint8_t b[] = {0x01, 0x02};
    mdns_rr ra, rb;

    memset(&ra, 0, sizeof ra);
    memset(&rb, 0, sizeof rb);
    ra.cls = 1; ra.type = 1; ra.rdlen = 3; ra.rdoff = 0;
    rb.cls = 1; rb.type = 1; rb.rdlen = 2; rb.rdoff = 0;
    TEST_ASSERT_TRUE(mdns_server_rr_cmp(&ra, &rb, a, b, sizeof a, sizeof b) > 0);
}

void test_rr_cmp_null_args(void)
{
    mdns_rr ra, rb;

    memset(&ra, 0, sizeof ra);
    memset(&rb, 0, sizeof rb);
    TEST_ASSERT_EQUAL_INT(0, mdns_server_rr_cmp(NULL, &rb, NULL, NULL, 0, 0));
    TEST_ASSERT_EQUAL_INT(0, mdns_server_rr_cmp(&ra, NULL, NULL, NULL, 0, 0));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_rr_cmp_equal);
    RUN_TEST(test_rr_cmp_class_differs);
    RUN_TEST(test_rr_cmp_type_differs);
    RUN_TEST(test_rr_cmp_rdata_differs);
    RUN_TEST(test_rr_cmp_length_differs);
    RUN_TEST(test_rr_cmp_null_args);
    return UNITY_END();
}
