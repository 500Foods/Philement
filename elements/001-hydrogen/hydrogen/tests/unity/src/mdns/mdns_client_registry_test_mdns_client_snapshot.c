#include <src/hydrogen.h>
#include <unity.h>

#include <src/mdns/mdns_client.h>
#include <src/mdns/mdns_server.h>
#include <src/state/state.h>

static char g_type_name[] = "_http._tcp.local";
static MDNSServiceType g_type;
static MDNSClientConfig g_cfg;
static mdns_client_t *g_client;
static int g_evt;
static char g_evt_name[MDNS_NAME_MAX];

void test_mdns_client_snapshot_copy(void);
void test_mdns_client_own_services_filter(void);
void test_mdns_client_on_change_found(void);

static void on_change(mdns_client_event_t ev, const mdns_client_service_t *svc)
{
    g_evt = (int)ev;
    if (svc) {
        strncpy(g_evt_name, svc->instance, sizeof g_evt_name - 1);
    }
}

void setUp(void) {
    memset(&g_cfg, 0, sizeof g_cfg);
    memset(&g_type, 0, sizeof g_type);
    g_type.type = g_type_name;
    g_cfg.max_services = 8;
    g_cfg.num_service_types = 1;
    g_cfg.service_types = &g_type;
    g_cfg.enable_ipv4 = true;
    g_cfg.own_services = true;
    g_client = mdns_client_create(&g_cfg);
    TEST_ASSERT_NOT_NULL(g_client);
    mdns_client_instance = g_client;
    g_evt = 0;
    g_evt_name[0] = '\0';
    mdns_client_on_change = NULL;
}

void tearDown(void) {
    mdns_client_on_change = NULL;
    mdns_client_instance = NULL;
    mdns_client_destroy(g_client);
    g_client = NULL;
    mdns_server = NULL;
}

void test_mdns_client_snapshot_copy(void) {
    mdns_client_service_t *svc;
    mdns_client_service_t *snap;
    size_t n = 0;

    svc = mdns_client_insert_instance(g_client, "A._http._tcp.local", "_http._tcp.local");
    TEST_ASSERT_NOT_NULL(svc);
    snap = mdns_client_snapshot(&n);
    TEST_ASSERT_EQUAL_UINT(1, n);
    TEST_ASSERT_NOT_NULL(snap);
    TEST_ASSERT_TRUE(mdns_name_equal(snap[0].instance, "A._http._tcp.local"));
    TEST_ASSERT_EQUAL_UINT(1, mdns_client_count());
    mdns_client_snapshot_free(snap);
}

void test_mdns_client_own_services_filter(void) {
    mdns_server_t server;
    mdns_server_service_t svc;
    mdns_client_service_t *snap;
    size_t n = 0;
    char name[] = "Mine";
    char type[] = "_http._tcp.local";

    memset(&server, 0, sizeof server);
    memset(&svc, 0, sizeof svc);
    svc.name = name;
    svc.type = type;
    svc.claimed = 1;
    server.services = &svc;
    server.num_services = 1;
    mdns_server = &server;

    g_client->own_services = 0;
    TEST_ASSERT_NULL(mdns_client_insert_instance(g_client, "Mine._http._tcp.local", "_http._tcp.local"));
    TEST_ASSERT_NOT_NULL(mdns_client_insert_instance(g_client, "Other._http._tcp.local", "_http._tcp.local"));
    snap = mdns_client_snapshot(&n);
    TEST_ASSERT_EQUAL_UINT(1, n);
    TEST_ASSERT_TRUE(mdns_name_equal(snap[0].instance, "Other._http._tcp.local"));
    mdns_client_snapshot_free(snap);
}

void test_mdns_client_on_change_found(void) {
    mdns_client_on_change = on_change;
    TEST_ASSERT_NOT_NULL(mdns_client_insert_instance(g_client, "X._http._tcp.local", "_http._tcp.local"));
    mdns_client_drop_service(g_client, 0, "test");
    TEST_ASSERT_EQUAL_INT(MDNS_CLIENT_EVT_LOST, g_evt);
    TEST_ASSERT_TRUE(mdns_name_equal(g_evt_name, "X._http._tcp.local"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_mdns_client_snapshot_copy);
    RUN_TEST(test_mdns_client_own_services_filter);
    RUN_TEST(test_mdns_client_on_change_found);
    return UNITY_END();
}
