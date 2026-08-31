/*
 * Unity Test File: mdns_server_defend_test_mdns_server_want_is_shared_only.c
 * Tests shared-record detection: only shared records have no unique claims.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mdns/mdns_server.h>

void test_want_empty_is_shared(void);
void test_want_host_unique_is_not_shared(void);
void test_want_host_nsec_not_shared(void);
void test_want_service_srv_not_shared(void);
void test_want_service_a_not_shared(void);
void test_want_only_shared_is_shared(void);

void setUp(void)
{
}

void tearDown(void)
{
}

void test_want_empty_is_shared(void)
{
    mdns_server_want_t w;

    mdns_server_want_clear(&w, 1);
    TEST_ASSERT_EQUAL_INT(1, mdns_server_want_is_shared_only(&w));
}

void test_want_host_unique_is_not_shared(void)
{
    mdns_server_want_t w;

    mdns_server_want_clear(&w, 1);
    w.host_answer |= MDNS_W_A;
    TEST_ASSERT_EQUAL_INT(0, mdns_server_want_is_shared_only(&w));
}

void test_want_host_nsec_not_shared(void)
{
    mdns_server_want_t w;

    mdns_server_want_clear(&w, 1);
    w.host_answer |= MDNS_W_NSEC;
    TEST_ASSERT_EQUAL_INT(0, mdns_server_want_is_shared_only(&w));
}

void test_want_service_srv_not_shared(void)
{
    mdns_server_want_t w;

    mdns_server_want_clear(&w, 1);
    w.svc_answer[0] |= MDNS_W_SRV;
    TEST_ASSERT_EQUAL_INT(0, mdns_server_want_is_shared_only(&w));
}

void test_want_service_a_not_shared(void)
{
    mdns_server_want_t w;

    mdns_server_want_clear(&w, 1);
    w.svc_answer[0] |= MDNS_W_A;
    TEST_ASSERT_EQUAL_INT(0, mdns_server_want_is_shared_only(&w));
}

void test_want_only_shared_is_shared(void)
{
    mdns_server_want_t w;

    mdns_server_want_clear(&w, 1);
    w.svc_answer[0] |= MDNS_W_TXT;
    TEST_ASSERT_EQUAL_INT(1, mdns_server_want_is_shared_only(&w));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_want_empty_is_shared);
    RUN_TEST(test_want_host_unique_is_not_shared);
    RUN_TEST(test_want_host_nsec_not_shared);
    RUN_TEST(test_want_service_srv_not_shared);
    RUN_TEST(test_want_service_a_not_shared);
    RUN_TEST(test_want_only_shared_is_shared);
    return UNITY_END();
}
