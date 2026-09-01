/*
 * Unity Test: mdns_server_respond_test_mdns_server_want_empty.c
 * Tests mdns_server_want_empty from mdns_server_respond.c
 *
 * CHANGELOG
 * 1.0.0 - 2026-09-01 - Initial creation
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mdns/mdns_keys.h>
#include <src/mdns/mdns_server.h>

void test_mdns_server_want_empty_null(void);
void test_mdns_server_want_empty_truly_empty(void);
void test_mdns_server_want_empty_with_host_answer(void);
void test_mdns_server_want_empty_with_host_additional(void);
void test_mdns_server_want_empty_with_svc_answer(void);
void test_mdns_server_want_empty_with_svc_additional(void);

void setUp(void) {}
void tearDown(void) {}

void test_mdns_server_want_empty_null(void) {
    TEST_ASSERT_EQUAL_INT(1, mdns_server_want_empty(NULL));
}

void test_mdns_server_want_empty_truly_empty(void) {
    mdns_server_want_t w;
    memset(&w, 0, sizeof(w));
    w.nsvc = 0;
    TEST_ASSERT_EQUAL_INT(1, mdns_server_want_empty(&w));
}

void test_mdns_server_want_empty_with_host_answer(void) {
    mdns_server_want_t w;
    memset(&w, 0, sizeof(w));
    w.host_answer = 1;
    TEST_ASSERT_EQUAL_INT(0, mdns_server_want_empty(&w));
}

void test_mdns_server_want_empty_with_host_additional(void) {
    mdns_server_want_t w;
    memset(&w, 0, sizeof(w));
    w.host_additional = 1;
    TEST_ASSERT_EQUAL_INT(0, mdns_server_want_empty(&w));
}

void test_mdns_server_want_empty_with_svc_answer(void) {
    mdns_server_want_t w;
    memset(&w, 0, sizeof(w));
    w.nsvc = 1;
    w.svc_answer[0] = 1;
    TEST_ASSERT_EQUAL_INT(0, mdns_server_want_empty(&w));
}

void test_mdns_server_want_empty_with_svc_additional(void) {
    mdns_server_want_t w;
    memset(&w, 0, sizeof(w));
    w.nsvc = 1;
    w.svc_additional[0] = 1;
    TEST_ASSERT_EQUAL_INT(0, mdns_server_want_empty(&w));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mdns_server_want_empty_null);
    RUN_TEST(test_mdns_server_want_empty_truly_empty);
    RUN_TEST(test_mdns_server_want_empty_with_host_answer);
    RUN_TEST(test_mdns_server_want_empty_with_host_additional);
    RUN_TEST(test_mdns_server_want_empty_with_svc_answer);
    RUN_TEST(test_mdns_server_want_empty_with_svc_additional);

    return UNITY_END();
}
