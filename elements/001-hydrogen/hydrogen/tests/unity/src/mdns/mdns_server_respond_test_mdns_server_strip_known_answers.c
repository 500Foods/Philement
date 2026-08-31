/*
 * Unity Test File: mdns_server_respond_test_mdns_server_strip_known_answers.c
 * Tests mdns_server_strip_known_answers (ANSWER only, TTL > ours/2, our instance)
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mdns/mdns_server.h>
#include <src/mdns/mdns_wire.h>

void test_strip_ptr_high_ttl(void);
void test_strip_ptr_low_ttl_kept(void);
void test_strip_ignores_authority(void);
void test_strip_other_instance_kept(void);

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

static size_t build_ptr_query_with_known(uint8_t *buf, size_t cap, uint32_t ttl, int authority,
                                         const char *rdata_name)
{
    mdns_buf b;
    size_t pos;

    mdns_buf_init(&b, buf, cap);
    (void)mdns_put_u16(&b, 0);
    (void)mdns_put_u16(&b, DNS_FLAG_QUERY);
    (void)mdns_put_u16(&b, 1);
    (void)mdns_put_u16(&b, (uint16_t)(authority ? 0 : 1));
    (void)mdns_put_u16(&b, (uint16_t)(authority ? 1 : 0));
    (void)mdns_put_u16(&b, 0);
    (void)mdns_put_name(&b, "_http._tcp.local");
    (void)mdns_put_u16(&b, MDNS_TYPE_PTR);
    (void)mdns_put_u16(&b, DNS_CLASS_IN);
    (void)mdns_rr_head(&b, "_http._tcp.local", MDNS_TYPE_PTR, ttl, 0, &pos);
    (void)mdns_put_name(&b, rdata_name);
    (void)mdns_rr_tail(&b, pos);
    return b.len;
}

void test_strip_ptr_high_ttl(void)
{
    mdns_server_t server;
    mdns_server_service_t svc;
    mdns_server_want_t want;
    mdns_rr q;
    mdns_msg msg;
    uint8_t buf[512];
    size_t len;

    fill_server(&server, &svc);
    mdns_server_want_clear(&want, 1);
    memset(&q, 0, sizeof q);
    snprintf(q.name, sizeof q.name, "%s", "_http._tcp.local");
    q.type = MDNS_TYPE_PTR;
    q.cls = DNS_CLASS_IN;
    mdns_server_want_add_question(&want, &server, &q);
    TEST_ASSERT_TRUE((want.svc_answer[0] & MDNS_W_PTR) != 0);

    len = build_ptr_query_with_known(buf, sizeof buf, 3000, 0, "Printer._http._tcp.local");
    TEST_ASSERT_EQUAL_INT(0, mdns_parse(buf, len, &msg));
    mdns_server_strip_known_answers(&want, &server, buf, len, &msg);
    TEST_ASSERT_TRUE((want.svc_answer[0] & MDNS_W_PTR) == 0);
}

void test_strip_ptr_low_ttl_kept(void)
{
    mdns_server_t server;
    mdns_server_service_t svc;
    mdns_server_want_t want;
    mdns_rr q;
    mdns_msg msg;
    uint8_t buf[512];
    size_t len;

    fill_server(&server, &svc);
    mdns_server_want_clear(&want, 1);
    memset(&q, 0, sizeof q);
    snprintf(q.name, sizeof q.name, "%s", "_http._tcp.local");
    q.type = MDNS_TYPE_PTR;
    q.cls = DNS_CLASS_IN;
    mdns_server_want_add_question(&want, &server, &q);

    len = build_ptr_query_with_known(buf, sizeof buf, 100, 0, "Printer._http._tcp.local");
    TEST_ASSERT_EQUAL_INT(0, mdns_parse(buf, len, &msg));
    mdns_server_strip_known_answers(&want, &server, buf, len, &msg);
    TEST_ASSERT_TRUE((want.svc_answer[0] & MDNS_W_PTR) != 0);
}

void test_strip_ignores_authority(void)
{
    mdns_server_t server;
    mdns_server_service_t svc;
    mdns_server_want_t want;
    mdns_rr q;
    mdns_msg msg;
    uint8_t buf[512];
    size_t len;

    fill_server(&server, &svc);
    mdns_server_want_clear(&want, 1);
    memset(&q, 0, sizeof q);
    snprintf(q.name, sizeof q.name, "%s", "_http._tcp.local");
    q.type = MDNS_TYPE_PTR;
    q.cls = DNS_CLASS_IN;
    mdns_server_want_add_question(&want, &server, &q);

    len = build_ptr_query_with_known(buf, sizeof buf, 3000, 1, "Printer._http._tcp.local");
    TEST_ASSERT_EQUAL_INT(0, mdns_parse(buf, len, &msg));
    TEST_ASSERT_EQUAL_INT(MDNS_SEC_AUTHORITY, msg.rr[0].section);
    mdns_server_strip_known_answers(&want, &server, buf, len, &msg);
    TEST_ASSERT_TRUE((want.svc_answer[0] & MDNS_W_PTR) != 0);
}

void test_strip_other_instance_kept(void)
{
    mdns_server_t server;
    mdns_server_service_t svc;
    mdns_server_want_t want;
    mdns_rr q;
    mdns_msg msg;
    uint8_t buf[512];
    size_t len;

    fill_server(&server, &svc);
    mdns_server_want_clear(&want, 1);
    memset(&q, 0, sizeof q);
    snprintf(q.name, sizeof q.name, "%s", "_http._tcp.local");
    q.type = MDNS_TYPE_PTR;
    q.cls = DNS_CLASS_IN;
    mdns_server_want_add_question(&want, &server, &q);

    len = build_ptr_query_with_known(buf, sizeof buf, 3000, 0, "Other._http._tcp.local");
    TEST_ASSERT_EQUAL_INT(0, mdns_parse(buf, len, &msg));
    mdns_server_strip_known_answers(&want, &server, buf, len, &msg);
    TEST_ASSERT_TRUE((want.svc_answer[0] & MDNS_W_PTR) != 0);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_strip_ptr_high_ttl);
    RUN_TEST(test_strip_ptr_low_ttl_kept);
    RUN_TEST(test_strip_ignores_authority);
    RUN_TEST(test_strip_other_instance_kept);
    return UNITY_END();
}
