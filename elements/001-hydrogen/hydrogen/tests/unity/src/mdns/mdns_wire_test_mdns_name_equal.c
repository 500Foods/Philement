/*
 * Unity Test File: mdns_wire_test_mdns_name_equal.c
 * Tests mdns_name_equal case and trailing-dot handling
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mdns/mdns_wire.h>

void test_mdns_name_equal_exact(void);
void test_mdns_name_equal_case(void);
void test_mdns_name_equal_trailing_dot(void);
void test_mdns_name_equal_mismatch(void);
void test_mdns_name_equal_null(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_mdns_name_equal_exact(void) {
    TEST_ASSERT_TRUE(mdns_name_equal("_http._tcp.local", "_http._tcp.local"));
}

void test_mdns_name_equal_case(void) {
    TEST_ASSERT_TRUE(mdns_name_equal("_HTTP._TCP.local", "_http._tcp.local"));
}

void test_mdns_name_equal_trailing_dot(void) {
    TEST_ASSERT_TRUE(mdns_name_equal("host.local.", "host.local"));
    TEST_ASSERT_TRUE(mdns_name_equal("host.local", "host.local."));
}

void test_mdns_name_equal_mismatch(void) {
    TEST_ASSERT_FALSE(mdns_name_equal("a.local", "b.local"));
    TEST_ASSERT_FALSE(mdns_name_equal("host.local", "host.localx"));
}

void test_mdns_name_equal_null(void) {
    TEST_ASSERT_FALSE(mdns_name_equal(NULL, "a"));
    TEST_ASSERT_FALSE(mdns_name_equal("a", NULL));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_mdns_name_equal_exact);
    RUN_TEST(test_mdns_name_equal_case);
    RUN_TEST(test_mdns_name_equal_trailing_dot);
    RUN_TEST(test_mdns_name_equal_mismatch);
    RUN_TEST(test_mdns_name_equal_null);
    return UNITY_END();
}
