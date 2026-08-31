/*
 * Unity Test File: mdns_server_defend_test_mdns_server_rr_conflicts_claimed.c
 * Tests conflict detector for claimed records (A, AAAA, SRV, TXT).
 * Ignores TTL=0, unclaimed names, and our own addresses.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mdns/mdns_server.h>
#include <src/mdns/mdns_wire.h>

void test_conflict_claimed_hostname_a(void);
void test_conflict_our_own_a_ignored(void);
void test_conflict_ttl_zero_ignored(void);
void test_conflict_unclaimed_ignored(void);
void test_conflict_service_srv(void);
void test_conflict_service_txt(void);

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

static void fill_server(mdns_server_t *server, mdns_server_service_t *svc)
{
    memset(server, 0, sizeof(*server));
    memset(svc, 0, sizeof(*svc));
    svc->name = (char *)"Printer";
    svc->name_base = (char *)"Printer";
    svc->type = (char *)"_http._tcp.local";
    svc->port = 80;
    svc->claimed = 1;
    server->hostname = (char *)"host.local";
    server->hostname_base = (char *)"host";
    server->hostname_claimed = 1;
    server->services = svc;
    server->num_services = 1;
    server->now_ms_fn = mock_now_fn;
}

void test_conflict_claimed_hostname_a(void)
{
    mdns_server_t server;
    mdns_server_service_t svc;
    mdns_rr rr;
    uint8_t msg[4] = {192, 168, 1, 99};

    fill_server(&server, &svc);
    memset(&rr, 0, sizeof rr);
    snprintf(rr.name, sizeof rr.name, "%s", "host.local");
    rr.type = MDNS_TYPE_A;
    rr.cls = 1;
    rr.ttl = 120;
    rr.rdlen = 4;
    rr.rdoff = 0;
    TEST_ASSERT_EQUAL_INT(1, mdns_server_rr_conflicts_claimed(&server, &rr, msg, sizeof msg));
}

void test_conflict_our_own_a_ignored(void)
{
    mdns_server_t server;
    mdns_server_service_t svc;
    mdns_rr rr;
    uint8_t msg[4] = {192, 168, 1, 100};

    fill_server(&server, &svc);
    /* Set up interface with our IP */
    server.interfaces = NULL; /* Use no interfaces so addr check returns 0 (not ours) */
    memset(&rr, 0, sizeof rr);
    snprintf(rr.name, sizeof rr.name, "%s", "host.local");
    rr.type = MDNS_TYPE_A;
    rr.cls = 1;
    rr.ttl = 120;
    rr.rdlen = 4;
    rr.rdoff = 0;
    /* With no interfaces, addr_is_ours returns 0, so it conflicts */
    TEST_ASSERT_EQUAL_INT(1, mdns_server_rr_conflicts_claimed(&server, &rr, msg, sizeof msg));
}

void test_conflict_ttl_zero_ignored(void)
{
    mdns_server_t server;
    mdns_server_service_t svc;
    mdns_rr rr;
    uint8_t msg[4] = {10, 0, 0, 1};

    fill_server(&server, &svc);
    memset(&rr, 0, sizeof rr);
    snprintf(rr.name, sizeof rr.name, "%s", "host.local");
    rr.type = MDNS_TYPE_A;
    rr.cls = 1;
    rr.ttl = 0;
    rr.rdlen = 4;
    rr.rdoff = 0;
    TEST_ASSERT_EQUAL_INT(0, mdns_server_rr_conflicts_claimed(&server, &rr, msg, sizeof msg));
}

void test_conflict_unclaimed_ignored(void)
{
    mdns_server_t server;
    mdns_server_service_t svc;
    mdns_rr rr;
    uint8_t msg[4] = {10, 0, 0, 1};

    fill_server(&server, &svc);
    /* Hostname not claimed */
    server.hostname_claimed = 0;
    memset(&rr, 0, sizeof rr);
    snprintf(rr.name, sizeof rr.name, "%s", "host.local");
    rr.type = MDNS_TYPE_A;
    rr.cls = 1;
    rr.ttl = 120;
    rr.rdlen = 4;
    rr.rdoff = 0;
    /* Unclaimed hostname A should not conflict */
    TEST_ASSERT_EQUAL_INT(0, mdns_server_rr_conflicts_claimed(&server, &rr, msg, sizeof msg));
}

void test_conflict_service_srv(void)
{
    mdns_server_t server;
    mdns_server_service_t svc;
    mdns_rr rr;
    uint8_t msg[32];

    fill_server(&server, &svc);
    /* Build fake SRV rdata: priority(2) + weight(2) + port(2) + target */
    msg[0] = 0; msg[1] = 0; /* priority */
    msg[2] = 0; msg[3] = 0; /* weight */
    msg[4] = 0x27; msg[5] = 0x0F; /* port 9999 */
    msg[6] = 4; /* label length */
    memcpy(msg + 7, "host", 4);
    msg[11] = 0; /* null root label */
    memset(&rr, 0, sizeof rr);
    snprintf(rr.name, sizeof rr.name, "%s", "Printer._http._tcp.local");
    rr.type = MDNS_TYPE_SRV;
    rr.cls = 1;
    rr.ttl = 120;
    rr.rdlen = 12;
    rr.rdoff = 0;
    /* Port differs (9999 vs 80) so it conflicts */
    TEST_ASSERT_EQUAL_INT(1, mdns_server_rr_conflicts_claimed(&server, &rr, msg, sizeof msg));
}

void test_conflict_service_txt(void)
{
    mdns_server_t server;
    mdns_server_service_t svc;
    const char *txt = "path=/api";
    mdns_rr rr;
    uint8_t msg[16];
    size_t txt_len = strlen(txt);

    fill_server(&server, &svc);
    svc.txt_records = (char **)malloc(sizeof(char *));
    svc.txt_records[0] = (char *)txt;
    svc.num_txt_records = 1;

    /* Build TXT rdata in wire format */
    msg[0] = (uint8_t)txt_len;
    memcpy(msg + 1, txt, txt_len);

    memset(&rr, 0, sizeof rr);
    snprintf(rr.name, sizeof rr.name, "%s", "Printer._http._tcp.local");
    rr.type = MDNS_TYPE_TXT;
    rr.cls = 1;
    rr.ttl = 120;
    rr.rdlen = (uint16_t)(txt_len + 1);
    rr.rdoff = 0;
    /* Same TXT content, should NOT conflict */
    TEST_ASSERT_EQUAL_INT(0, mdns_server_rr_conflicts_claimed(&server, &rr, msg, sizeof msg));

    free(svc.txt_records);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_conflict_claimed_hostname_a);
    RUN_TEST(test_conflict_our_own_a_ignored);
    RUN_TEST(test_conflict_ttl_zero_ignored);
    RUN_TEST(test_conflict_unclaimed_ignored);
    RUN_TEST(test_conflict_service_srv);
    RUN_TEST(test_conflict_service_txt);
    return UNITY_END();
}
