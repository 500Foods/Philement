/*
 * Unity Test File: mdns_wire_test_mdns_put_name.c
 * Tests mdns_put_name uncompressed label encoding
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mdns/mdns_wire.h>

void test_mdns_put_name_simple(void);
void test_mdns_put_name_trailing_dot(void);
void test_mdns_put_name_empty_labels(void);
void test_mdns_put_name_root(void);
void test_mdns_put_name_label_too_long(void);
void test_mdns_put_name_null(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_mdns_put_name_simple(void) {
    uint8_t storage[32];
    mdns_buf b;

    mdns_buf_init(&b, storage, sizeof storage);
    TEST_ASSERT_EQUAL_INT(0, mdns_put_name(&b, "test.local"));
    TEST_ASSERT_EQUAL_UINT(12, b.len);
    TEST_ASSERT_EQUAL_UINT8(4, storage[0]);
    TEST_ASSERT_EQUAL_MEMORY("test", &storage[1], 4);
    TEST_ASSERT_EQUAL_UINT8(5, storage[5]);
    TEST_ASSERT_EQUAL_MEMORY("local", &storage[6], 5);
    TEST_ASSERT_EQUAL_UINT8(0, storage[11]);
}

void test_mdns_put_name_trailing_dot(void) {
    uint8_t storage[32];
    mdns_buf b;

    mdns_buf_init(&b, storage, sizeof storage);
    TEST_ASSERT_EQUAL_INT(0, mdns_put_name(&b, "test.local."));
    TEST_ASSERT_EQUAL_UINT(12, b.len);
    TEST_ASSERT_EQUAL_UINT8(0, storage[11]);
}

void test_mdns_put_name_empty_labels(void) {
    uint8_t storage[32];
    mdns_buf b;

    mdns_buf_init(&b, storage, sizeof storage);
    TEST_ASSERT_EQUAL_INT(0, mdns_put_name(&b, "a..b"));
    TEST_ASSERT_EQUAL_UINT8(1, storage[0]);
    TEST_ASSERT_EQUAL_UINT8('a', storage[1]);
    TEST_ASSERT_EQUAL_UINT8(1, storage[2]);
    TEST_ASSERT_EQUAL_UINT8('b', storage[3]);
    TEST_ASSERT_EQUAL_UINT8(0, storage[4]);
}

void test_mdns_put_name_root(void) {
    uint8_t storage[8];
    mdns_buf b;

    mdns_buf_init(&b, storage, sizeof storage);
    TEST_ASSERT_EQUAL_INT(0, mdns_put_name(&b, "."));
    TEST_ASSERT_EQUAL_UINT(1, b.len);
    TEST_ASSERT_EQUAL_UINT8(0, storage[0]);
}

void test_mdns_put_name_label_too_long(void) {
    uint8_t storage[128];
    mdns_buf b;
    char name[80];

    memset(name, 'a', 64);
    name[64] = '\0';
    mdns_buf_init(&b, storage, sizeof storage);
    TEST_ASSERT_EQUAL_INT(-1, mdns_put_name(&b, name));
    TEST_ASSERT_EQUAL_INT(1, b.overflow);
}

void test_mdns_put_name_null(void) {
    uint8_t storage[8];
    mdns_buf b;

    mdns_buf_init(&b, storage, sizeof storage);
    TEST_ASSERT_EQUAL_INT(-1, mdns_put_name(&b, NULL));
    TEST_ASSERT_EQUAL_INT(1, b.overflow);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_mdns_put_name_simple);
    RUN_TEST(test_mdns_put_name_trailing_dot);
    RUN_TEST(test_mdns_put_name_empty_labels);
    RUN_TEST(test_mdns_put_name_root);
    RUN_TEST(test_mdns_put_name_label_too_long);
    RUN_TEST(test_mdns_put_name_null);
    return UNITY_END();
}
