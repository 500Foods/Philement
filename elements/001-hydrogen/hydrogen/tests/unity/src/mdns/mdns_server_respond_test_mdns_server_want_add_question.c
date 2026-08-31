/*
 * Unity Test File: mdns_server_respond_test_mdns_server_want_add_question.c
 * Tests mdns_server_want_add_question (dns-sd, case-insensitive type, PTR extras)
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mdns/mdns_server.h>
#include <src/mdns/mdns_wire.h>

void test_want_add_ptr_service_type(void);
void test_want_add_case_insensitive_type(void);
void test_want_add_dns_sd(void);
void test_want_add_srv_instance(void);
void test_want_add_hostname_a(void);
void test_want_add_qu_bit(void);
void test_want_add_nulls(void);

void setUp(void)
{
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
}

void test_want_add_ptr_service_type(void)
{
    mdns_server_t server;
    mdns_server_service_t svc;
    mdns_server_want_t want;
    mdns_rr q;

    fill_server(&server, &svc);
    mdns_server_want_clear(&want, 1);
    memset(&q, 0, sizeof q);
    snprintf(q.name, sizeof q.name, "%s", "_http._tcp.local");
    q.type = MDNS_TYPE_PTR;
    q.cls = DNS_CLASS_IN;
    mdns_server_want_add_question(&want, &server, &q);
    TEST_ASSERT_TRUE((want.svc_answer[0] & MDNS_W_PTR) != 0);
    TEST_ASSERT_TRUE((want.svc_additional[0] & MDNS_W_SRV) != 0);
    TEST_ASSERT_TRUE((want.svc_additional[0] & MDNS_W_TXT) != 0);
    TEST_ASSERT_TRUE((want.host_additional & MDNS_W_A) != 0);
    TEST_ASSERT_FALSE(mdns_server_want_empty(&want));
}

void test_want_add_case_insensitive_type(void)
{
    mdns_server_t server;
    mdns_server_service_t svc;
    mdns_server_want_t want;
    mdns_rr q;

    fill_server(&server, &svc);
    mdns_server_want_clear(&want, 1);
    memset(&q, 0, sizeof q);
    snprintf(q.name, sizeof q.name, "%s", "_HTTP._TCP.local");
    q.type = MDNS_TYPE_PTR;
    q.cls = DNS_CLASS_IN;
    mdns_server_want_add_question(&want, &server, &q);
    TEST_ASSERT_TRUE((want.svc_answer[0] & MDNS_W_PTR) != 0);
}

void test_want_add_dns_sd(void)
{
    mdns_server_t server;
    mdns_server_service_t svc;
    mdns_server_want_t want;
    mdns_rr q;

    fill_server(&server, &svc);
    mdns_server_want_clear(&want, 1);
    memset(&q, 0, sizeof q);
    snprintf(q.name, sizeof q.name, "%s", MDNS_DNS_SD_NAME);
    q.type = MDNS_TYPE_PTR;
    q.cls = DNS_CLASS_IN;
    mdns_server_want_add_question(&want, &server, &q);
    TEST_ASSERT_TRUE((want.svc_answer[0] & MDNS_W_SD) != 0);
}

void test_want_add_srv_instance(void)
{
    mdns_server_t server;
    mdns_server_service_t svc;
    mdns_server_want_t want;
    mdns_rr q;

    fill_server(&server, &svc);
    mdns_server_want_clear(&want, 1);
    memset(&q, 0, sizeof q);
    snprintf(q.name, sizeof q.name, "%s", "Printer._http._tcp.local");
    q.type = MDNS_TYPE_SRV;
    q.cls = DNS_CLASS_IN;
    mdns_server_want_add_question(&want, &server, &q);
    TEST_ASSERT_TRUE((want.svc_answer[0] & MDNS_W_SRV) != 0);
    TEST_ASSERT_TRUE((want.svc_answer[0] & MDNS_W_PTR) == 0);
}

void test_want_add_hostname_a(void)
{
    mdns_server_t server;
    mdns_server_service_t svc;
    mdns_server_want_t want;
    mdns_rr q;

    fill_server(&server, &svc);
    mdns_server_want_clear(&want, 1);
    memset(&q, 0, sizeof q);
    snprintf(q.name, sizeof q.name, "%s", "host.local");
    q.type = MDNS_TYPE_A;
    q.cls = DNS_CLASS_IN;
    mdns_server_want_add_question(&want, &server, &q);
    TEST_ASSERT_TRUE((want.host_answer & MDNS_W_A) != 0);
    TEST_ASSERT_TRUE((want.host_answer & MDNS_W_AAAA) == 0);
    TEST_ASSERT_TRUE((want.host_answer & MDNS_W_NSEC) != 0);
}

void test_want_add_qu_bit(void)
{
    mdns_server_t server;
    mdns_server_service_t svc;
    mdns_server_want_t want;
    mdns_rr q;

    fill_server(&server, &svc);
    mdns_server_want_clear(&want, 1);
    memset(&q, 0, sizeof q);
    snprintf(q.name, sizeof q.name, "%s", "_http._tcp.local");
    q.type = MDNS_TYPE_PTR;
    q.cls = (uint16_t)(DNS_CLASS_IN | DNS_QU_BIT);
    mdns_server_want_add_question(&want, &server, &q);
    TEST_ASSERT_EQUAL_INT(1, want.qu);
}

void test_want_add_nulls(void)
{
    mdns_server_want_t want;

    mdns_server_want_clear(&want, 0);
    mdns_server_want_add_question(NULL, NULL, NULL);
    TEST_ASSERT_TRUE(mdns_server_want_empty(&want));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_want_add_ptr_service_type);
    RUN_TEST(test_want_add_case_insensitive_type);
    RUN_TEST(test_want_add_dns_sd);
    RUN_TEST(test_want_add_srv_instance);
    RUN_TEST(test_want_add_hostname_a);
    RUN_TEST(test_want_add_qu_bit);
    RUN_TEST(test_want_add_nulls);
    return UNITY_END();
}
