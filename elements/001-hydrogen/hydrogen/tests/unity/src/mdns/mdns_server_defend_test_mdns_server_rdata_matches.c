/*
 * Unity Test File: mdns_server_defend_test_mdns_server_rdata_matches.c
 * Tests TXT and SRV rdata byte-level comparison helpers.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mdns/mdns_server.h>
#include <src/mdns/mdns_wire.h>

void test_txt_rdata_matches_exact(void);
void test_txt_rdata_mismatch_length(void);
void test_txt_rdata_mismatch_bytes(void);
void test_txt_rdata_null_svc(void);
void test_srv_rdata_matches_exact(void);
void test_srv_rdata_port_mismatch(void);
void test_srv_rdata_null_args(void);

void setUp(void)
{
}

void tearDown(void)
{
}

void test_txt_rdata_matches_exact(void)
{
    mdns_server_service_t svc;
    const char *txt = "path=/api";
    uint8_t rdata[16];
    size_t txt_len = strlen(txt);

    memset(&svc, 0, sizeof svc);
    svc.txt_records = (char **)malloc(sizeof(char *));
    svc.txt_records[0] = (char *)txt;
    svc.num_txt_records = 1;

    rdata[0] = (uint8_t)txt_len;
    memcpy(rdata + 1, txt, txt_len);

    TEST_ASSERT_EQUAL_INT(1, mdns_server_txt_rdata_matches(&svc, rdata, txt_len + 1));
    free(svc.txt_records);
}

void test_txt_rdata_mismatch_length(void)
{
    mdns_server_service_t svc;
    const char *txt = "path=/api";
    uint8_t rdata[16];
    size_t txt_len = strlen(txt);

    memset(&svc, 0, sizeof svc);
    svc.txt_records = (char **)malloc(sizeof(char *));
    svc.txt_records[0] = (char *)txt;
    svc.num_txt_records = 1;

    rdata[0] = (uint8_t)(txt_len + 1);
    memcpy(rdata + 1, txt, txt_len);

    TEST_ASSERT_EQUAL_INT(0, mdns_server_txt_rdata_matches(&svc, rdata, txt_len + 2));
    free(svc.txt_records);
}

void test_txt_rdata_mismatch_bytes(void)
{
    mdns_server_service_t svc;
    const char *txt = "path=/api";
    uint8_t rdata[16];
    size_t txt_len = strlen(txt);

    memset(&svc, 0, sizeof svc);
    svc.txt_records = (char **)malloc(sizeof(char *));
    svc.txt_records[0] = (char *)txt;
    svc.num_txt_records = 1;

    rdata[0] = (uint8_t)txt_len;
    memcpy(rdata + 1, txt, txt_len);
    rdata[txt_len + 1 - 1] = (uint8_t)(rdata[txt_len + 1 - 1] ^ 0xFF); /* flip a byte */

    TEST_ASSERT_EQUAL_INT(0, mdns_server_txt_rdata_matches(&svc, rdata, txt_len + 1));
    free(svc.txt_records);
}

void test_txt_rdata_null_svc(void)
{
    const uint8_t rdata[4] = {0};
    TEST_ASSERT_EQUAL_INT(0, mdns_server_txt_rdata_matches(NULL, rdata, 4));
}

void test_srv_rdata_matches_exact(void)
{
    mdns_server_t server;
    mdns_server_service_t svc;
    uint8_t msg[32];
    mdns_rr rr;

    memset(&server, 0, sizeof server);
    memset(&svc, 0, sizeof svc);
    memset(msg, 0, sizeof msg);
    memset(&rr, 0, sizeof rr);

    server.hostname = (char *)"host.local";
    svc.port = 80;
    svc.name = (char *)"Printer";
    svc.type = (char *)"_http._tcp.local";

    /* SRV: priority(2)=0, weight(2)=0, port(2)=80, target=host.local encoded */
    msg[4] = 0; msg[5] = 80; /* port 80 */
    msg[6] = 4; /* "host" label */
    memcpy(msg + 7, "host", 4);
    msg[11] = 5; /* "local" label */
    memcpy(msg + 12, "local", 5);
    msg[17] = 0; /* null root */

    rr.type = MDNS_TYPE_SRV;
    rr.rdlen = 18;
    rr.rdoff = 0;

    TEST_ASSERT_EQUAL_INT(1, mdns_server_srv_rdata_matches(&server, &svc, msg, 18, &rr));
}

void test_srv_rdata_port_mismatch(void)
{
    mdns_server_t server;
    mdns_server_service_t svc;
    uint8_t msg[32];
    mdns_rr rr;

    memset(&server, 0, sizeof server);
    memset(&svc, 0, sizeof svc);
    memset(msg, 0, sizeof msg);
    memset(&rr, 0, sizeof rr);

    server.hostname = (char *)"host.local";
    svc.port = 80;

    /* Port 9999 */
    msg[4] = 0x27; msg[5] = 0x0F;

    rr.type = MDNS_TYPE_SRV;
    rr.rdlen = 18;
    rr.rdoff = 0;

    /* Port mismatch, should return 0 */
    TEST_ASSERT_EQUAL_INT(0, mdns_server_srv_rdata_matches(&server, &svc, msg, 18, &rr));
}

void test_srv_rdata_null_args(void)
{
    mdns_rr rr;
    memset(&rr, 0, sizeof rr);
    TEST_ASSERT_EQUAL_INT(0, mdns_server_srv_rdata_matches(NULL, NULL, NULL, 0, &rr));
    TEST_ASSERT_EQUAL_INT(0, mdns_server_srv_rdata_matches(NULL, NULL, NULL, 0, NULL));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_txt_rdata_matches_exact);
    RUN_TEST(test_txt_rdata_mismatch_length);
    RUN_TEST(test_txt_rdata_mismatch_bytes);
    RUN_TEST(test_txt_rdata_null_svc);
    RUN_TEST(test_srv_rdata_matches_exact);
    RUN_TEST(test_srv_rdata_port_mismatch);
    RUN_TEST(test_srv_rdata_null_args);
    return UNITY_END();
}
