#include <src/hydrogen.h>
#include <unity.h>

#include <src/mdns/mdns_client.h>
#include <src/mdns/mdns_wire.h>

static char g_type_name[] = "_http._tcp.local";
static MDNSServiceType g_type;
static MDNSClientConfig g_cfg;
static mdns_client_t *g_client;

void test_mdns_client_handle_response_ptr_srv_txt_a(void);
void test_mdns_client_handle_response_goodbye(void);
void test_mdns_client_handle_response_ignore_query(void);
void test_mdns_client_handle_response_duplicate_addr(void);

void setUp(void) {
    memset(&g_cfg, 0, sizeof g_cfg);
    memset(&g_type, 0, sizeof g_type);
    g_type.type = g_type_name;
    g_cfg.max_services = 8;
    g_cfg.num_service_types = 1;
    g_cfg.service_types = &g_type;
    g_cfg.enable_ipv4 = true;
    g_client = mdns_client_create(&g_cfg);
    TEST_ASSERT_NOT_NULL(g_client);
}

void tearDown(void) {
    mdns_client_destroy(g_client);
    g_client = NULL;
}

static size_t build_full_response(uint8_t *storage, size_t cap)
{
    mdns_buf b;
    size_t pos;

    mdns_buf_init(&b, storage, cap);
    (void)mdns_put_u16(&b, 0);
    (void)mdns_put_u16(&b, DNS_FLAG_RESPONSE);
    (void)mdns_put_u16(&b, 0);
    (void)mdns_put_u16(&b, 4);
    (void)mdns_put_u16(&b, 0);
    (void)mdns_put_u16(&b, 0);

    (void)mdns_rr_head(&b, "_http._tcp.local", MDNS_TYPE_PTR, 4500, 0, &pos);
    (void)mdns_put_name(&b, "Inst._http._tcp.local");
    (void)mdns_rr_tail(&b, pos);

    (void)mdns_rr_head(&b, "Inst._http._tcp.local", MDNS_TYPE_SRV, 120, 1, &pos);
    (void)mdns_put_u16(&b, 0);
    (void)mdns_put_u16(&b, 0);
    (void)mdns_put_u16(&b, 8080);
    (void)mdns_put_name(&b, "host.local");
    (void)mdns_rr_tail(&b, pos);

    (void)mdns_rr_head(&b, "Inst._http._tcp.local", MDNS_TYPE_TXT, 4500, 1, &pos);
    (void)mdns_put_u8(&b, 9);
    (void)mdns_put_bytes(&b, "path=/api", 9);
    (void)mdns_rr_tail(&b, pos);

    (void)mdns_rr_head(&b, "host.local", MDNS_TYPE_A, 120, 1, &pos);
    (void)mdns_put_u32(&b, 0xc0000201u);
    (void)mdns_rr_tail(&b, pos);

    return b.len;
}

void test_mdns_client_handle_response_ptr_srv_txt_a(void) {
    uint8_t storage[512];
    size_t len = build_full_response(storage, sizeof storage);

    TEST_ASSERT_EQUAL_INT(0, mdns_client_handle_response(g_client, storage, len, 1));
    TEST_ASSERT_EQUAL_UINT(1, g_client->nservices);
    TEST_ASSERT_TRUE(mdns_name_equal(g_client->services[0].instance, "Inst._http._tcp.local"));
    TEST_ASSERT_TRUE(g_client->services[0].have_srv);
    TEST_ASSERT_TRUE(g_client->services[0].have_txt);
    TEST_ASSERT_EQUAL_UINT16(8080, g_client->services[0].port);
    TEST_ASSERT_TRUE(mdns_name_equal(g_client->services[0].host, "host.local"));
    TEST_ASSERT_EQUAL_STRING("/api", g_client->services[0].path);
    TEST_ASSERT_EQUAL_UINT(1, g_client->services[0].nendpoints);
}

void test_mdns_client_handle_response_goodbye(void) {
    uint8_t storage[512];
    uint8_t bye[256];
    mdns_buf b;
    size_t pos;
    size_t len = build_full_response(storage, sizeof storage);

    TEST_ASSERT_EQUAL_INT(0, mdns_client_handle_response(g_client, storage, len, 1));
    TEST_ASSERT_EQUAL_UINT(1, g_client->nservices);

    mdns_buf_init(&b, bye, sizeof bye);
    (void)mdns_put_u16(&b, 0);
    (void)mdns_put_u16(&b, DNS_FLAG_RESPONSE);
    (void)mdns_put_u16(&b, 0);
    (void)mdns_put_u16(&b, 1);
    (void)mdns_put_u16(&b, 0);
    (void)mdns_put_u16(&b, 0);
    (void)mdns_rr_head(&b, "_http._tcp.local", MDNS_TYPE_PTR, 0, 0, &pos);
    (void)mdns_put_name(&b, "Inst._http._tcp.local");
    (void)mdns_rr_tail(&b, pos);

    TEST_ASSERT_EQUAL_INT(0, mdns_client_handle_response(g_client, bye, b.len, 1));
    TEST_ASSERT_EQUAL_UINT(0, g_client->nservices);
}

void test_mdns_client_handle_response_ignore_query(void) {
    uint8_t storage[128];
    size_t len = 0;

    TEST_ASSERT_EQUAL_INT(0, mdns_client_build_query(storage, sizeof storage, &len,
                                                     "_http._tcp.local", MDNS_TYPE_PTR));
    TEST_ASSERT_EQUAL_INT(0, mdns_client_handle_response(g_client, storage, len, 1));
    TEST_ASSERT_EQUAL_UINT(0, g_client->nservices);
}

void test_mdns_client_handle_response_duplicate_addr(void) {
    uint8_t storage[512];
    size_t len = build_full_response(storage, sizeof storage);

    TEST_ASSERT_EQUAL_INT(0, mdns_client_handle_response(g_client, storage, len, 1));
    TEST_ASSERT_EQUAL_INT(0, mdns_client_handle_response(g_client, storage, len, 1));
    TEST_ASSERT_EQUAL_UINT(1, g_client->nservices);
    TEST_ASSERT_EQUAL_UINT(1, g_client->services[0].nendpoints);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_mdns_client_handle_response_ptr_srv_txt_a);
    RUN_TEST(test_mdns_client_handle_response_goodbye);
    RUN_TEST(test_mdns_client_handle_response_ignore_query);
    RUN_TEST(test_mdns_client_handle_response_duplicate_addr);
    return UNITY_END();
}
