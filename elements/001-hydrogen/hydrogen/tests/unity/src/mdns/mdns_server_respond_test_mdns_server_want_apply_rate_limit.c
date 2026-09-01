/*
 * Unity Test File: mdns_server_respond_test_mdns_server_want_apply_rate_limit.c
 * Tests mdns_server_want_apply_rate_limit (RFC 6762 s6 unique-record rate limit).
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mdns/mdns_server.h>
#include <src/mdns/mdns_wire.h>

void test_rate_limit_no_clock(void);
void test_rate_limit_first_send_passes(void);
void test_rate_limit_service_unique_suppressed(void);
void test_rate_limit_hostname_unique_suppressed(void);
void test_rate_limit_shared_preserved(void);
void test_rate_limit_old_send_passes(void);
void test_rate_limit_null_args(void);
void test_rate_limit_now_zero(void);

static uint64_t g_fake_now_ms = 1000000;

static uint64_t fake_now_ms(void)
{
    return g_fake_now_ms;
}

void setUp(void)
{
    g_fake_now_ms = 1000000;
}

void tearDown(void)
{
}

static void fill_server(mdns_server_t *server, mdns_server_service_t *svc)
{
    memset(server, 0, sizeof(*server));
    memset(svc, 0, sizeof(*svc));
    svc->name = (char *)"Printer";
    svc->type = (char *)"_http._tcp.local";
    svc->port = 80;
    server->hostname = (char *)"host.local";
    server->services = svc;
    server->num_services = 1;
    server->now_ms_fn = fake_now_ms;
}

void test_rate_limit_no_clock(void)
{
    mdns_server_t server;
    mdns_server_service_t svc;
    mdns_server_want_t want;

    fill_server(&server, &svc);
    server.now_ms_fn = NULL;
    mdns_server_want_clear(&want, 1);
    want.svc_answer[0] = MDNS_W_SRV | MDNS_W_PTR;
    mdns_server_want_apply_rate_limit(&want, &server);
    TEST_ASSERT_TRUE((want.svc_answer[0] & MDNS_W_SRV) != 0);
}

void test_rate_limit_first_send_passes(void)
{
    mdns_server_t server;
    mdns_server_service_t svc;
    mdns_server_want_t want;

    fill_server(&server, &svc);
    mdns_server_want_clear(&want, 1);
    want.svc_answer[0] = MDNS_W_SRV | MDNS_W_PTR;
    mdns_server_want_apply_rate_limit(&want, &server);
    TEST_ASSERT_TRUE((want.svc_answer[0] & MDNS_W_SRV) != 0);
    TEST_ASSERT_TRUE((want.svc_answer[0] & MDNS_W_PTR) != 0);
}

void test_rate_limit_service_unique_suppressed(void)
{
    mdns_server_t server;
    mdns_server_service_t svc;
    mdns_server_want_t want;

    fill_server(&server, &svc);
    svc.last_send_ms = g_fake_now_ms - 100;
    mdns_server_want_clear(&want, 1);
    want.svc_answer[0] = MDNS_W_SRV | MDNS_W_TXT | MDNS_W_A | MDNS_W_PTR;
    want.svc_additional[0] = MDNS_W_SRV | MDNS_W_TXT;
    mdns_server_want_apply_rate_limit(&want, &server);
    TEST_ASSERT_TRUE((want.svc_answer[0] & MDNS_W_SRV) == 0);
    TEST_ASSERT_TRUE((want.svc_answer[0] & MDNS_W_TXT) == 0);
    TEST_ASSERT_TRUE((want.svc_answer[0] & MDNS_W_A) == 0);
    TEST_ASSERT_TRUE((want.svc_additional[0] & MDNS_W_SRV) == 0);
    TEST_ASSERT_TRUE((want.svc_answer[0] & MDNS_W_PTR) != 0);
}

void test_rate_limit_hostname_unique_suppressed(void)
{
    mdns_server_t server;
    mdns_server_service_t svc;
    mdns_server_want_t want;

    fill_server(&server, &svc);
    server.hostname_last_send_ms = g_fake_now_ms - 100;
    mdns_server_want_clear(&want, 1);
    want.host_answer = MDNS_W_A | MDNS_W_AAAA | MDNS_W_NSEC;
    want.host_additional = MDNS_W_A | MDNS_W_AAAA;
    mdns_server_want_apply_rate_limit(&want, &server);
    TEST_ASSERT_TRUE((want.host_answer & MDNS_W_A) == 0);
    TEST_ASSERT_TRUE((want.host_answer & MDNS_W_AAAA) == 0);
    TEST_ASSERT_TRUE((want.host_answer & MDNS_W_NSEC) == 0);
    TEST_ASSERT_TRUE((want.host_additional & MDNS_W_A) == 0);
}

void test_rate_limit_shared_preserved(void)
{
    mdns_server_t server;
    mdns_server_service_t svc;
    mdns_server_want_t want;

    fill_server(&server, &svc);
    svc.last_send_ms = g_fake_now_ms - 100;
    mdns_server_want_clear(&want, 1);
    want.svc_answer[0] = MDNS_W_PTR | MDNS_W_SD;
    want.svc_additional[0] = MDNS_W_SRV;
    mdns_server_want_apply_rate_limit(&want, &server);
    TEST_ASSERT_TRUE((want.svc_answer[0] & MDNS_W_PTR) != 0);
    TEST_ASSERT_TRUE((want.svc_answer[0] & MDNS_W_SD) != 0);
    TEST_ASSERT_TRUE((want.svc_additional[0] & MDNS_W_SRV) == 0);
}

void test_rate_limit_old_send_passes(void)
{
    mdns_server_t server;
    mdns_server_service_t svc;
    mdns_server_want_t want;

    fill_server(&server, &svc);
    svc.last_send_ms = g_fake_now_ms - (uint64_t)MDNS_RATE_LIMIT_MS - 1;
    mdns_server_want_clear(&want, 1);
    want.svc_answer[0] = MDNS_W_SRV | MDNS_W_PTR;
    mdns_server_want_apply_rate_limit(&want, &server);
    TEST_ASSERT_TRUE((want.svc_answer[0] & MDNS_W_SRV) != 0);
    TEST_ASSERT_TRUE((want.svc_answer[0] & MDNS_W_PTR) != 0);
}

void test_rate_limit_null_args(void)
{
    mdns_server_t server;
    mdns_server_service_t svc;
    mdns_server_want_t want;

    fill_server(&server, &svc);
    mdns_server_want_clear(&want, 1);
    want.svc_answer[0] = MDNS_W_SRV;
    mdns_server_want_apply_rate_limit(NULL, &server);
    mdns_server_want_apply_rate_limit(&want, NULL);
    TEST_ASSERT_TRUE((want.svc_answer[0] & MDNS_W_SRV) != 0);
}

void test_rate_limit_now_zero(void)
{
    mdns_server_t server;
    mdns_server_service_t svc;
    mdns_server_want_t want;

    fill_server(&server, &svc);
    g_fake_now_ms = 0;
    mdns_server_want_clear(&want, 1);
    want.svc_answer[0] = MDNS_W_SRV | MDNS_W_PTR;
    mdns_server_want_apply_rate_limit(&want, &server);
    TEST_ASSERT_TRUE((want.svc_answer[0] & MDNS_W_SRV) != 0);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_rate_limit_no_clock);
    RUN_TEST(test_rate_limit_first_send_passes);
    RUN_TEST(test_rate_limit_service_unique_suppressed);
    RUN_TEST(test_rate_limit_hostname_unique_suppressed);
    RUN_TEST(test_rate_limit_shared_preserved);
    RUN_TEST(test_rate_limit_old_send_passes);
    RUN_TEST(test_rate_limit_null_args);
    RUN_TEST(test_rate_limit_now_zero);
    return UNITY_END();
}