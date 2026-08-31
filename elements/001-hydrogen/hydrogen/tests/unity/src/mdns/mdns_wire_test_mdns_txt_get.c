/*
 * Unity Test File: mdns_wire_test_mdns_txt_get.c
 * Tests mdns_txt_get key lookup
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mdns/mdns_wire.h>

void test_mdns_txt_get_found(void);
void test_mdns_txt_get_case(void);
void test_mdns_txt_get_bare_key(void);
void test_mdns_txt_get_missing(void);
void test_mdns_txt_get_null(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_mdns_txt_get_found(void) {
    const uint8_t rdata[] = {
        10, 'p', 'a', 't', 'h', '=', '/', 'h', 't', 'm', 'l',
        7, 't', 'x', 't', 'v', 'e', 'r', 's'
    };
    char out[32];

    TEST_ASSERT_EQUAL_INT(0, mdns_txt_get(rdata, sizeof rdata, "path", out, sizeof out));
    TEST_ASSERT_EQUAL_STRING("/html", out);
}

void test_mdns_txt_get_case(void) {
    const uint8_t rdata[] = { 10, 'P', 'a', 't', 'h', '=', '/', 'h', 't', 'm', 'l' };
    char out[32];

    TEST_ASSERT_EQUAL_INT(0, mdns_txt_get(rdata, sizeof rdata, "path", out, sizeof out));
    TEST_ASSERT_EQUAL_STRING("/html", out);
}

void test_mdns_txt_get_bare_key(void) {
    const uint8_t rdata[] = { 5, 'e', 'm', 'p', 't', 'y' };
    char out[32];

    TEST_ASSERT_EQUAL_INT(0, mdns_txt_get(rdata, sizeof rdata, "empty", out, sizeof out));
    TEST_ASSERT_EQUAL_STRING("", out);
}

void test_mdns_txt_get_missing(void) {
    const uint8_t rdata[] = { 3, 'f', 'o', 'o' };
    char out[32];

    TEST_ASSERT_EQUAL_INT(-1, mdns_txt_get(rdata, sizeof rdata, "path", out, sizeof out));
}

void test_mdns_txt_get_null(void) {
    char out[8];

    TEST_ASSERT_EQUAL_INT(-1, mdns_txt_get(NULL, 1, "a", out, sizeof out));
    TEST_ASSERT_EQUAL_INT(-1, mdns_txt_get((const uint8_t *)"x", 1, NULL, out, sizeof out));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_mdns_txt_get_found);
    RUN_TEST(test_mdns_txt_get_case);
    RUN_TEST(test_mdns_txt_get_bare_key);
    RUN_TEST(test_mdns_txt_get_missing);
    RUN_TEST(test_mdns_txt_get_null);
    return UNITY_END();
}
