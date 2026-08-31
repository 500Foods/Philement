/*
 * Unity Test File: mdns_server_defend_test_mdns_server_rate_fresh.c
 * Tests per-name rate-limit freshness: fresh if >1s elapsed or never sent.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mdns/mdns_server.h>

void test_rate_fresh_never_sent(void);
void test_rate_fresh_within_window(void);
void test_rate_fresh_after_window(void);
void test_rate_fresh_null_server(void);

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

void test_rate_fresh_never_sent(void)
{
    mdns_server_t server;

    memset(&server, 0, sizeof server);
    server.now_ms_fn = mock_now_fn;
    TEST_ASSERT_EQUAL_INT(1, mdns_server_rate_fresh(&server, "host.local", 0));
}

void test_rate_fresh_within_window(void)
{
    mdns_server_t server;

    memset(&server, 0, sizeof server);
    server.now_ms_fn = mock_now_fn;
    TEST_ASSERT_EQUAL_INT(0, mdns_server_rate_fresh(&server, "host.local", 4999));
}

void test_rate_fresh_after_window(void)
{
    mdns_server_t server;

    memset(&server, 0, sizeof server);
    server.now_ms_fn = mock_now_fn;
    TEST_ASSERT_EQUAL_INT(1, mdns_server_rate_fresh(&server, "host.local", 3999));
}

void test_rate_fresh_null_server(void)
{
    TEST_ASSERT_EQUAL_INT(1, mdns_server_rate_fresh(NULL, "host.local", 100));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_rate_fresh_never_sent);
    RUN_TEST(test_rate_fresh_within_window);
    RUN_TEST(test_rate_fresh_after_window);
    RUN_TEST(test_rate_fresh_null_server);
    return UNITY_END();
}
