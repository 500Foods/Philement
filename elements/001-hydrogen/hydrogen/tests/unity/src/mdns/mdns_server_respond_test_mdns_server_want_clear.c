/*
 * Unity Test: mdns_server_respond_test_mdns_server_want_clear.c
 * Tests mdns_server_want_clear from mdns_server_respond.c
 *
 * CHANGELOG
 * 1.0.0 - 2026-09-01 - Initial creation
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mdns/mdns_keys.h>
#include <src/mdns/mdns_server.h>

void test_mdns_server_want_clear_null(void);
void test_mdns_server_want_clear_normal(void);
void test_mdns_server_want_clear_overflow(void);

void setUp(void) {}
void tearDown(void) {}

void test_mdns_server_want_clear_null(void) {
    mdns_server_want_clear(NULL, 4);
}

void test_mdns_server_want_clear_normal(void) {
    mdns_server_want_t w;
    memset(&w, 0xFF, sizeof(w));

    mdns_server_want_clear(&w, 4);

    TEST_ASSERT_EQUAL_UINT(0, w.host_answer);
    TEST_ASSERT_EQUAL_UINT(0, w.host_additional);
    TEST_ASSERT_EQUAL_UINT(0, w.qu);
    TEST_ASSERT_EQUAL_UINT(4, w.nsvc);
    for (size_t i = 0; i < 4; i++) {
        TEST_ASSERT_EQUAL_UINT(0, w.svc_answer[i]);
        TEST_ASSERT_EQUAL_UINT(0, w.svc_additional[i]);
    }
}

void test_mdns_server_want_clear_overflow(void) {
    mdns_server_want_t w;
    memset(&w, 0xFF, sizeof(w));

    mdns_server_want_clear(&w, 100);

    TEST_ASSERT_EQUAL_UINT(MDNS_SERVER_WANT_MAX_SVC, w.nsvc);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mdns_server_want_clear_null);
    RUN_TEST(test_mdns_server_want_clear_normal);
    RUN_TEST(test_mdns_server_want_clear_overflow);

    return UNITY_END();
}
