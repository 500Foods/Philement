/*
 * Unity Test File: mdns_wire_test_mdns_name_decode.c
 * Tests mdns_name_decode including compression and pointer loops
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mdns/mdns_wire.h>

void test_mdns_name_decode_plain(void);
void test_mdns_name_decode_compression(void);
void test_mdns_name_decode_pointer_loop(void);
void test_mdns_name_decode_truncated(void);
void test_mdns_name_decode_null(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_mdns_name_decode_plain(void) {
    uint8_t msg[16];
    char out[MDNS_NAME_MAX];
    size_t next = 0;
    mdns_buf b;

    mdns_buf_init(&b, msg, sizeof msg);
    TEST_ASSERT_EQUAL_INT(0, mdns_put_name(&b, "foo.local"));
    TEST_ASSERT_EQUAL_INT(0, mdns_name_decode(msg, b.len, 0, out, sizeof out, &next));
    TEST_ASSERT_EQUAL_STRING("foo.local", out);
    TEST_ASSERT_EQUAL_UINT(b.len, next);
}

void test_mdns_name_decode_compression(void) {
    uint8_t msg[32];
    char out[MDNS_NAME_MAX];
    size_t next = 0;

    memset(msg, 0, sizeof msg);
    msg[0] = 3;
    memcpy(&msg[1], "foo", 3);
    msg[4] = 5;
    memcpy(&msg[5], "local", 5);
    msg[10] = 0;
    msg[11] = 3;
    memcpy(&msg[12], "bar", 3);
    msg[15] = 0xc0;
    msg[16] = 0x00;

    TEST_ASSERT_EQUAL_INT(0, mdns_name_decode(msg, 17, 11, out, sizeof out, &next));
    TEST_ASSERT_EQUAL_STRING("bar.foo.local", out);
    TEST_ASSERT_EQUAL_UINT(17, next);
}

void test_mdns_name_decode_pointer_loop(void) {
    uint8_t msg[2];
    char out[MDNS_NAME_MAX];
    size_t next = 0;

    msg[0] = 0xc0;
    msg[1] = 0x00;
    TEST_ASSERT_EQUAL_INT(-1, mdns_name_decode(msg, sizeof msg, 0, out, sizeof out, &next));
}

void test_mdns_name_decode_truncated(void) {
    uint8_t msg[3];
    char out[MDNS_NAME_MAX];
    size_t next = 0;

    msg[0] = 5;
    msg[1] = 'a';
    msg[2] = 'b';
    TEST_ASSERT_EQUAL_INT(-1, mdns_name_decode(msg, sizeof msg, 0, out, sizeof out, &next));
}

void test_mdns_name_decode_null(void) {
    char out[8];
    size_t next = 0;

    TEST_ASSERT_EQUAL_INT(-1, mdns_name_decode(NULL, 4, 0, out, sizeof out, &next));
    TEST_ASSERT_EQUAL_INT(-1, mdns_name_decode((const uint8_t *)"\x00", 1, 0, NULL, 8, &next));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_mdns_name_decode_plain);
    RUN_TEST(test_mdns_name_decode_compression);
    RUN_TEST(test_mdns_name_decode_pointer_loop);
    RUN_TEST(test_mdns_name_decode_truncated);
    RUN_TEST(test_mdns_name_decode_null);
    return UNITY_END();
}
