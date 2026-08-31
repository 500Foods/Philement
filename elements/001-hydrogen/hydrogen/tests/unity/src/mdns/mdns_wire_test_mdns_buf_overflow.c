/*
 * Unity Test File: mdns_wire_test_mdns_buf_overflow.c
 * Tests mdns_buf overflow handling
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mdns/mdns_wire.h>

void test_mdns_buf_overflow_put_u8(void);
void test_mdns_buf_overflow_put_u16(void);
void test_mdns_buf_overflow_put_bytes(void);
void test_mdns_buf_overflow_null_storage(void);
void test_mdns_buf_overflow_sticks(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_mdns_buf_overflow_put_u8(void) {
    uint8_t storage[1];
    mdns_buf b;

    mdns_buf_init(&b, storage, sizeof storage);
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u8(&b, 0xaa));
    TEST_ASSERT_EQUAL_INT(-1, mdns_put_u8(&b, 0xbb));
    TEST_ASSERT_EQUAL_INT(1, b.overflow);
    TEST_ASSERT_EQUAL_UINT(1, b.len);
    TEST_ASSERT_EQUAL_UINT8(0xaa, storage[0]);
}

void test_mdns_buf_overflow_put_u16(void) {
    uint8_t storage[2];
    mdns_buf b;

    mdns_buf_init(&b, storage, sizeof storage);
    TEST_ASSERT_EQUAL_INT(0, mdns_put_u16(&b, 0x1234));
    TEST_ASSERT_EQUAL_UINT8(0x12, storage[0]);
    TEST_ASSERT_EQUAL_UINT8(0x34, storage[1]);
    TEST_ASSERT_EQUAL_INT(-1, mdns_put_u32(&b, 1));
    TEST_ASSERT_EQUAL_INT(1, b.overflow);
}

void test_mdns_buf_overflow_put_bytes(void) {
    uint8_t storage[3];
    mdns_buf b;

    mdns_buf_init(&b, storage, sizeof storage);
    TEST_ASSERT_EQUAL_INT(1, mdns_buf_room(&b, 3));
    TEST_ASSERT_EQUAL_INT(-1, mdns_put_bytes(&b, "abcd", 4));
    TEST_ASSERT_EQUAL_INT(1, b.overflow);
    TEST_ASSERT_EQUAL_INT(0, mdns_buf_room(&b, 1));
}

void test_mdns_buf_overflow_null_storage(void) {
    mdns_buf b;

    mdns_buf_init(&b, NULL, 16);
    TEST_ASSERT_EQUAL_INT(1, b.overflow);
    TEST_ASSERT_EQUAL_INT(-1, mdns_put_u8(&b, 1));
}

void test_mdns_buf_overflow_sticks(void) {
    uint8_t storage[8];
    mdns_buf b;

    mdns_buf_init(&b, storage, sizeof storage);
    TEST_ASSERT_EQUAL_INT(-1, mdns_put_bytes(&b, NULL, 2));
    TEST_ASSERT_EQUAL_INT(1, b.overflow);
    TEST_ASSERT_EQUAL_INT(-1, mdns_put_u8(&b, 1));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_mdns_buf_overflow_put_u8);
    RUN_TEST(test_mdns_buf_overflow_put_u16);
    RUN_TEST(test_mdns_buf_overflow_put_bytes);
    RUN_TEST(test_mdns_buf_overflow_null_storage);
    RUN_TEST(test_mdns_buf_overflow_sticks);
    return UNITY_END();
}
