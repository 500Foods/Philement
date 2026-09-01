/*
 * Unity Test File: mdns_server_respond_test_mdns_server_build_query_response.c
 * Tests selective PTR+additional vs legacy unicast shape and overflow split
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mdns/mdns_server.h>
#include <src/mdns/mdns_wire.h>

void test_build_ptr_additional_and_flush(void);
void test_build_legacy_echo_ttl_no_flush(void);
void test_build_dns_sd_ptr(void);
void test_build_ipv4_only_aaaa_yields_nsec(void);
void test_build_overflow_sends_partial(void);

void setUp(void)
{
}

void tearDown(void)
{
}

static void fill_server(mdns_server_t *server, mdns_server_service_t *svc,
                        mdns_server_interface_t *iface, char **ips)
{
    memset(server, 0, sizeof(*server));
    memset(svc, 0, sizeof(*svc));
    memset(iface, 0, sizeof(*iface));
    svc->name = (char *)"Printer";
    svc->type = (char *)"_http._tcp.local";
    svc->port = 8080;
    server->hostname = (char *)"host.local";
    server->services = svc;
    server->num_services = 1;
    ips[0] = (char *)"192.0.2.10";
    iface->if_name = (char *)"eth0";
    iface->ip_addresses = ips;
    iface->num_addresses = 1;
    iface->sockfd_v4 = -1;
    iface->sockfd_v6 = -1;
}

void test_build_ptr_additional_and_flush(void)
{
    mdns_server_t server;
    mdns_server_service_t svc;
    mdns_server_interface_t iface;
    char *ips[1];
    mdns_server_want_t want;
    mdns_rr q;
    mdns_msg query;
    mdns_msg parsed;
    uint8_t packet[MDNS_MAX_PACKET_SIZE];
    size_t packet_len = 0;
    size_t i;
    int saw_ptr = 0;
    int saw_srv = 0;
    int saw_a = 0;
    int srv_flush = 0;
    int ptr_flush = 0;

    fill_server(&server, &svc, &iface, ips);
    mdns_server_want_clear(&want, 1);
    memset(&q, 0, sizeof q);
    snprintf(q.name, sizeof q.name, "%s", "_http._tcp.local");
    q.type = MDNS_TYPE_PTR;
    q.cls = DNS_CLASS_IN;
    mdns_server_want_add_question(&want, &server, &q);
    memset(&query, 0, sizeof query);
    query.id = 0x1234;
    query.questions[0] = q;
    query.nquestions = 1;
    query.qdcount = 1;

    mdns_server_build_query_response(packet, &packet_len, &server, &iface, &query, &want, 0);
    TEST_ASSERT_TRUE(packet_len > 12);
    TEST_ASSERT_EQUAL_INT(0, mdns_parse(packet, packet_len, &parsed));
    TEST_ASSERT_EQUAL_UINT16(0, parsed.id);
    TEST_ASSERT_EQUAL_UINT16(0, parsed.qdcount);
    TEST_ASSERT_TRUE(parsed.ancount >= 1);
    TEST_ASSERT_TRUE(parsed.arcount >= 1);

    for (i = 0; i < parsed.nrr; i++) {
        if (parsed.rr[i].type == MDNS_TYPE_PTR && parsed.rr[i].section == MDNS_SEC_ANSWER) {
            saw_ptr = 1;
            ptr_flush = (parsed.rr[i].cls & DNS_CACHE_FLUSH) ? 1 : 0;
        }
        if (parsed.rr[i].type == MDNS_TYPE_SRV) {
            saw_srv = 1;
            srv_flush = (parsed.rr[i].cls & DNS_CACHE_FLUSH) ? 1 : 0;
            TEST_ASSERT_EQUAL_INT(MDNS_SEC_ADDITIONAL, parsed.rr[i].section);
        }
        if (parsed.rr[i].type == MDNS_TYPE_A) {
            saw_a = 1;
            TEST_ASSERT_EQUAL_INT(MDNS_SEC_ADDITIONAL, parsed.rr[i].section);
        }
    }
    TEST_ASSERT_TRUE(saw_ptr);
    TEST_ASSERT_TRUE(saw_srv);
    TEST_ASSERT_TRUE(saw_a);
    TEST_ASSERT_EQUAL_INT(0, ptr_flush);
    TEST_ASSERT_EQUAL_INT(1, srv_flush);
}

void test_build_legacy_echo_ttl_no_flush(void)
{
    mdns_server_t server;
    mdns_server_service_t svc;
    mdns_server_interface_t iface;
    char *ips[1];
    mdns_server_want_t want;
    mdns_rr q;
    mdns_msg query;
    mdns_msg parsed;
    uint8_t packet[MDNS_MAX_PACKET_SIZE];
    size_t packet_len = 0;
    size_t i;

    fill_server(&server, &svc, &iface, ips);
    mdns_server_want_clear(&want, 1);
    memset(&q, 0, sizeof q);
    snprintf(q.name, sizeof q.name, "%s", "_http._tcp.local");
    q.type = MDNS_TYPE_PTR;
    q.cls = (uint16_t)(DNS_CLASS_IN | DNS_QU_BIT);
    mdns_server_want_add_question(&want, &server, &q);
    memset(&query, 0, sizeof query);
    query.id = 0x4321;
    query.questions[0] = q;
    query.nquestions = 1;
    query.qdcount = 1;

    mdns_server_build_query_response(packet, &packet_len, &server, &iface, &query, &want, 1);
    TEST_ASSERT_TRUE(packet_len > 12);
    TEST_ASSERT_EQUAL_INT(0, mdns_parse(packet, packet_len, &parsed));
    TEST_ASSERT_EQUAL_UINT16(0x4321, parsed.id);
    TEST_ASSERT_EQUAL_UINT16(1, parsed.qdcount);
    TEST_ASSERT_EQUAL_STRING("_http._tcp.local", parsed.questions[0].name);
    TEST_ASSERT_TRUE((parsed.questions[0].cls & DNS_QU_BIT) == 0);

    for (i = 0; i < parsed.nrr; i++) {
        TEST_ASSERT_TRUE(parsed.rr[i].ttl <= MDNS_LEGACY_TTL_CAP);
        TEST_ASSERT_TRUE((parsed.rr[i].cls & DNS_CACHE_FLUSH) == 0);
    }
}

void test_build_dns_sd_ptr(void)
{
    mdns_server_t server;
    mdns_server_service_t svc;
    mdns_server_interface_t iface;
    char *ips[1];
    mdns_server_want_t want;
    mdns_rr q;
    mdns_msg query;
    mdns_msg parsed;
    uint8_t packet[MDNS_MAX_PACKET_SIZE];
    size_t packet_len = 0;
    char rdata[MDNS_NAME_MAX];
    size_t i;
    int saw = 0;

    fill_server(&server, &svc, &iface, ips);
    mdns_server_want_clear(&want, 1);
    memset(&q, 0, sizeof q);
    snprintf(q.name, sizeof q.name, "%s", MDNS_DNS_SD_NAME);
    q.type = MDNS_TYPE_PTR;
    q.cls = DNS_CLASS_IN;
    mdns_server_want_add_question(&want, &server, &q);
    memset(&query, 0, sizeof query);
    query.questions[0] = q;
    query.nquestions = 1;

    mdns_server_build_query_response(packet, &packet_len, &server, &iface, &query, &want, 0);
    TEST_ASSERT_EQUAL_INT(0, mdns_parse(packet, packet_len, &parsed));
    for (i = 0; i < parsed.nrr; i++) {
        if (parsed.rr[i].type == MDNS_TYPE_PTR &&
            mdns_name_equal(parsed.rr[i].name, MDNS_DNS_SD_NAME)) {
            TEST_ASSERT_EQUAL_INT(0, mdns_rdata_name(packet, packet_len, &parsed.rr[i], rdata, sizeof rdata));
            TEST_ASSERT_TRUE(mdns_name_equal(rdata, "_http._tcp.local"));
            saw = 1;
        }
    }
    TEST_ASSERT_TRUE(saw);
}

void test_build_ipv4_only_aaaa_yields_nsec(void)
{
    mdns_server_t server;
    mdns_server_service_t svc;
    mdns_server_interface_t iface;
    char *ips[1];
    mdns_server_want_t want;
    mdns_rr q;
    mdns_msg query;
    mdns_msg parsed;
    uint8_t packet[MDNS_MAX_PACKET_SIZE];
    size_t packet_len = 0;
    size_t i;
    int saw_nsec = 0;
    int saw_aaaa = 0;

    fill_server(&server, &svc, &iface, ips);
    mdns_server_want_clear(&want, 1);
    memset(&q, 0, sizeof q);
    snprintf(q.name, sizeof q.name, "%s", "host.local");
    q.type = MDNS_TYPE_AAAA;
    q.cls = DNS_CLASS_IN;
    mdns_server_want_add_question(&want, &server, &q);
    memset(&query, 0, sizeof query);
    query.questions[0] = q;
    query.nquestions = 1;

    mdns_server_build_query_response(packet, &packet_len, &server, &iface, &query, &want, 0);
    TEST_ASSERT_TRUE(packet_len > 12);
    TEST_ASSERT_EQUAL_INT(0, mdns_parse(packet, packet_len, &parsed));
    for (i = 0; i < parsed.nrr; i++) {
        if (parsed.rr[i].type == MDNS_TYPE_AAAA) {
            saw_aaaa = 1;
        }
        if (parsed.rr[i].type == (uint16_t)RR_NSEC) {
            saw_nsec = 1;
            TEST_ASSERT_TRUE(mdns_name_equal(parsed.rr[i].name, "host.local"));
            TEST_ASSERT_EQUAL_INT(MDNS_SEC_ANSWER, parsed.rr[i].section);
        }
    }
    TEST_ASSERT_TRUE(saw_nsec);
    TEST_ASSERT_FALSE(saw_aaaa);
}

void test_build_overflow_sends_partial(void)
{
    mdns_server_t server;
    mdns_server_interface_t iface;
    char *ips[100];
    char addr_buf[100][16];
    mdns_server_want_t want;
    mdns_rr q;
    mdns_msg query;
    mdns_msg parsed;
    uint8_t packet[MDNS_MAX_PACKET_SIZE];
    size_t packet_len = 0;
    size_t i;
    int a_count = 0;

    memset(&server, 0, sizeof(server));
    server.hostname = (char *)"host.local";
    server.num_services = 0;
    server.services = NULL;

    memset(&iface, 0, sizeof(iface));
    iface.if_name = (char *)"eth0";
    iface.sockfd_v4 = -1;
    iface.sockfd_v6 = -1;
    iface.num_addresses = 100;
    iface.ip_addresses = ips;

    for (i = 0; i < 100; i++) {
        snprintf(addr_buf[i], sizeof(addr_buf[i]), "10.0.0.%d", (int)(i + 1));
        ips[i] = addr_buf[i];
    }

    mdns_server_want_clear(&want, 1);
    memset(&q, 0, sizeof q);
    snprintf(q.name, sizeof q.name, "%s", "host.local");
    q.type = MDNS_TYPE_A;
    q.cls = DNS_CLASS_IN;
    mdns_server_want_add_question(&want, &server, &q);

    memset(&query, 0, sizeof query);
    query.questions[0] = q;
    query.nquestions = 1;
    query.qdcount = 1;

    mdns_server_build_query_response(packet, &packet_len, &server, &iface, &query, &want, 0);

    TEST_ASSERT_TRUE(packet_len > 12);
    TEST_ASSERT_EQUAL_INT(0, mdns_parse(packet, packet_len, &parsed));
    TEST_ASSERT_TRUE(parsed.ancount > 0);

    for (i = 0; i < parsed.nrr; i++) {
        if (parsed.rr[i].type == MDNS_TYPE_A) {
            a_count++;
        }
    }
    TEST_ASSERT_TRUE(a_count > 0);
    TEST_ASSERT_TRUE(a_count < 100);
    TEST_ASSERT_EQUAL_INT(a_count, parsed.ancount);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_build_ptr_additional_and_flush);
    RUN_TEST(test_build_legacy_echo_ttl_no_flush);
    RUN_TEST(test_build_dns_sd_ptr);
    RUN_TEST(test_build_ipv4_only_aaaa_yields_nsec);
    RUN_TEST(test_build_overflow_sends_partial);
    return UNITY_END();
}
