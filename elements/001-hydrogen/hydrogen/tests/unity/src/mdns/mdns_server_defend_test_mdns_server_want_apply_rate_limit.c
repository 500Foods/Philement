/*
 * Unity Test File: mdns_server_defend_test_mdns_server_want_apply_rate_limit.c
 * Tests rate-limit masking: recent unique sends suppress re-announcement
 * within the 1-second RFC 6762 s6 window.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mdns/mdns_server.h>

void test_rate_limit_never_sent_not_masked(void);
void test_rate_limit_fresh_not_masked(void);
void test_rate_limit_recent_hostname_masked(void);
void test_rate_limit_recent_service_masked(void);
void test_rate_limit_null_args(void);

static uint64_t mock_now = 5000;

static uint64_t mock_now_fn(void)
{
    return mock_now;
}

void setUp(void)
{
}

void tearDown(void)
{
}

void test_rate_limit_never_sent_not_masked(void)
{
    mdns_server_t server;
    mdns_server_want_t w;

    memset(&server, 0, sizeof server);
    server.now_ms_fn = mock_now_fn;
    mdns_server_want_clear(&w, 1);
    w.host_answer |= MDNS_W_A;

    mdns_server_want_apply_rate_limit(&w, &server);
    TEST_ASSERT_TRUE(w.host_answer & MDNS_W_A);
}

void test_rate_limit_fresh_not_masked(void)
{
    mdns_server_t server;
    mdns_server_want_t w;

    memset(&server, 0, sizeof server);
    server.now_ms_fn = mock_now_fn;
    server.hostname_last_send_ms = 1000; /* 4 seconds ago, fresh */
    mdns_server_want_clear(&w, 1);
    w.host_answer |= MDNS_W_A;

    mdns_server_want_apply_rate_limit(&w, &server);
    TEST_ASSERT_TRUE(w.host_answer & MDNS_W_A);
}

void test_rate_limit_recent_hostname_masked(void)
{
    mdns_server_t server;
    mdns_server_want_t w;

    memset(&server, 0, sizeof server);
    server.now_ms_fn = mock_now_fn;
    server.hostname_last_send_ms = 4999; /* 1ms ago, too recent */
    mdns_server_want_clear(&w, 1);
    w.host_answer |= MDNS_W_A;
    w.host_answer |= MDNS_W_AAAA;

    mdns_server_want_apply_rate_limit(&w, &server);
    TEST_ASSERT_FALSE(w.host_answer & MDNS_W_A);
    TEST_ASSERT_FALSE(w.host_answer & MDNS_W_AAAA);
    /* NSEC is a unique record, should also be masked */
    w.host_answer |= MDNS_W_NSEC;
    mdns_server_want_apply_rate_limit(&w, &server);
    TEST_ASSERT_FALSE(w.host_answer & MDNS_W_NSEC);
}

void test_rate_limit_recent_service_masked(void)
{
    mdns_server_t server;
    mdns_server_want_t w;

    memset(&server, 0, sizeof server);
    server.now_ms_fn = mock_now_fn;
    server.services = NULL; /* no services */
    server.num_services = 0;
    mdns_server_want_clear(&w, 1);
    w.svc_answer[0] |= MDNS_W_SRV;

    mdns_server_want_apply_rate_limit(&w, &server);
    TEST_ASSERT_TRUE(w.svc_answer[0] & MDNS_W_SRV);
}

void test_rate_limit_null_args(void)
{
    mdns_server_want_apply_rate_limit(NULL, NULL);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_rate_limit_never_sent_not_masked);
    RUN_TEST(test_rate_limit_fresh_not_masked);
    RUN_TEST(test_rate_limit_recent_hostname_masked);
    RUN_TEST(test_rate_limit_recent_service_masked);
    RUN_TEST(test_rate_limit_null_args);
    return UNITY_END();
}
