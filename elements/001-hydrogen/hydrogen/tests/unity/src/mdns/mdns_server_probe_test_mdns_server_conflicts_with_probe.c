/*
 * Unity Test File: mdns_server_probe_test_mdns_server_conflicts_with_probe.c
 * Tests conflict detector: QR=1 TTL!=0 on unclaimed names; ignore goodbye and questions
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mdns/mdns_server.h>
#include <src/mdns/mdns_wire.h>

void test_conflict_live_instance(void);
void test_conflict_ttl_zero_ignored(void);
void test_conflict_query_ignored(void);
void test_conflict_claimed_ignored(void);
void test_conflict_hostname(void);

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
    svc->name_base = (char *)"Printer";
    svc->type = (char *)"_http._tcp.local";
    svc->port = 80;
    svc->name_attempts = 1;
    server->hostname = (char *)"host.local";
    server->hostname_base = (char *)"host";
    server->hostname_attempts = 1;
    server->services = svc;
    server->num_services = 1;
}

void test_conflict_live_instance(void)
{
    mdns_server_t server;
    mdns_server_service_t svc;
    mdns_rr rr;

    fill_server(&server, &svc);
    memset(&rr, 0, sizeof rr);
    snprintf(rr.name, sizeof rr.name, "%s", "Printer._http._tcp.local");
    rr.type = MDNS_TYPE_SRV;
    rr.ttl = 120;
    TEST_ASSERT_EQUAL_INT(1, mdns_server_rr_conflicts_probe(&server, &rr));
}

void test_conflict_ttl_zero_ignored(void)
{
    mdns_server_t server;
    mdns_server_service_t svc;
    mdns_rr rr;

    fill_server(&server, &svc);
    memset(&rr, 0, sizeof rr);
    snprintf(rr.name, sizeof rr.name, "%s", "Printer._http._tcp.local");
    rr.type = MDNS_TYPE_SRV;
    rr.ttl = 0;
    TEST_ASSERT_EQUAL_INT(0, mdns_server_rr_conflicts_probe(&server, &rr));
}

void test_conflict_query_ignored(void)
{
    mdns_server_t server;
    mdns_server_service_t svc;
    mdns_msg msg;

    fill_server(&server, &svc);
    memset(&msg, 0, sizeof msg);
    msg.flags = DNS_FLAG_QUERY;
    msg.nrr = 1;
    snprintf(msg.rr[0].name, sizeof msg.rr[0].name, "%s", "Printer._http._tcp.local");
    msg.rr[0].ttl = 120;
    mdns_server_note_probe_conflicts(&server, &msg);
    TEST_ASSERT_EQUAL_INT(0, server.services[0].probe_conflict);
}

void test_conflict_claimed_ignored(void)
{
    mdns_server_t server;
    mdns_server_service_t svc;
    mdns_rr rr;

    fill_server(&server, &svc);
    svc.claimed = 1;
    memset(&rr, 0, sizeof rr);
    snprintf(rr.name, sizeof rr.name, "%s", "Printer._http._tcp.local");
    rr.ttl = 120;
    TEST_ASSERT_EQUAL_INT(0, mdns_server_rr_conflicts_probe(&server, &rr));
}

void test_conflict_hostname(void)
{
    mdns_server_t server;
    mdns_server_service_t svc;
    mdns_msg msg;

    fill_server(&server, &svc);
    memset(&msg, 0, sizeof msg);
    msg.flags = DNS_FLAG_RESPONSE;
    msg.nrr = 1;
    snprintf(msg.rr[0].name, sizeof msg.rr[0].name, "%s", "host.local");
    msg.rr[0].type = MDNS_TYPE_A;
    msg.rr[0].ttl = 120;
    mdns_server_note_probe_conflicts(&server, &msg);
    TEST_ASSERT_EQUAL_INT(1, server.hostname_conflict);
    TEST_ASSERT_EQUAL_INT(1, mdns_server_any_probe_conflict(&server));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_conflict_live_instance);
    RUN_TEST(test_conflict_ttl_zero_ignored);
    RUN_TEST(test_conflict_query_ignored);
    RUN_TEST(test_conflict_claimed_ignored);
    RUN_TEST(test_conflict_hostname);
    return UNITY_END();
}
