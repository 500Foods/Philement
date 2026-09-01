/*
 * Unity Test: mdns_client_test_mdns_client_registry.c
 * Tests mdns_client_registry.c functions: add_browse_type, lookup_by_type,
 * service_visible, count, info_json
 *
 * CHANGELOG
 * 1.0.0 - 2026-09-01 - Initial creation
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mdns/mdns_client.h>
#include <src/mdns/mdns_server.h>
#include <src/state/state.h>

static mdns_client_t *g_client;

void test_mdns_client_add_browse_type_normal(void);
void test_mdns_client_add_browse_type_duplicate(void);
void test_mdns_client_add_browse_type_null_args(void);
void test_mdns_client_add_browse_type_empty_type(void);
void test_mdns_client_service_visible_null(void);
void test_mdns_client_service_visible_own_instance(void);
void test_mdns_client_service_visible_other(void);
void test_mdns_client_count_null(void);
void test_mdns_client_count_with_services(void);
void test_mdns_client_lookup_by_type_null_type(void);
void test_mdns_client_lookup_by_type_no_match(void);
void test_mdns_client_lookup_by_type_match(void);
void test_mdns_client_lookup_by_type_null_out(void);
void test_mdns_client_info_json_no_server(void);
void test_mdns_client_info_json_with_server(void);
void test_mdns_client_is_own_instance_unclaimed(void);
void test_mdns_client_info_json_with_endpoints(void);

void setUp(void) {
    g_client = calloc(1, sizeof(mdns_client_t));
    TEST_ASSERT_NOT_NULL(g_client);
    pthread_mutex_init(&g_client->lock, NULL);
    g_client->own_services = 1;
    mdns_client_instance = g_client;
    mdns_server = NULL;
}

void tearDown(void) {
    mdns_client_instance = NULL;
    mdns_server = NULL;
    if (g_client) {
        if (g_client->browse_types) {
            for (size_t i = 0; i < g_client->ntypes; i++) {
                free(g_client->browse_types[i]);
            }
            free(g_client->browse_types);
        }
        pthread_mutex_destroy(&g_client->lock);
        free(g_client);
        g_client = NULL;
    }
}

void test_mdns_client_add_browse_type_normal(void) {
    int rc = mdns_client_add_browse_type(g_client, "_http._tcp.local");
    TEST_ASSERT_EQUAL_INT(0, rc);
    TEST_ASSERT_EQUAL_UINT(1, g_client->ntypes);
}

void test_mdns_client_add_browse_type_duplicate(void) {
    mdns_client_add_browse_type(g_client, "_http._tcp.local");
    int rc = mdns_client_add_browse_type(g_client, "_http._tcp.local");
    TEST_ASSERT_EQUAL_INT(0, rc);
    TEST_ASSERT_EQUAL_UINT(1, g_client->ntypes);
}

void test_mdns_client_add_browse_type_null_args(void) {
    TEST_ASSERT_EQUAL_INT(-1, mdns_client_add_browse_type(NULL, "_http._tcp.local"));
    TEST_ASSERT_EQUAL_INT(-1, mdns_client_add_browse_type(g_client, NULL));
}

void test_mdns_client_add_browse_type_empty_type(void) {
    TEST_ASSERT_EQUAL_INT(-1, mdns_client_add_browse_type(g_client, ""));
}

void test_mdns_client_service_visible_null(void) {
    mdns_client_service_t svc;
    memset(&svc, 0, sizeof(svc));
    TEST_ASSERT_EQUAL_INT(0, mdns_client_service_visible(NULL, &svc));
    TEST_ASSERT_EQUAL_INT(0, mdns_client_service_visible(g_client, NULL));
}

void test_mdns_client_service_visible_own_instance(void) {
    mdns_server_t server;
    mdns_server_service_t service;
    char name[] = "MySvc";
    char type[] = "_http._tcp.local";

    memset(&server, 0, sizeof(server));
    memset(&service, 0, sizeof(service));
    service.name = name;
    service.type = type;
    service.claimed = 1;
    server.services = &service;
    server.num_services = 1;
    mdns_server = &server;

    g_client->own_services = 0;

    mdns_client_service_t svc;
    memset(&svc, 0, sizeof(svc));
    snprintf(svc.instance, sizeof svc.instance, "MySvc._http._tcp.local");
    TEST_ASSERT_EQUAL_INT(0, mdns_client_service_visible(g_client, &svc));
}

void test_mdns_client_service_visible_other(void) {
    g_client->own_services = 0;
    mdns_client_service_t svc;
    memset(&svc, 0, sizeof(svc));
    snprintf(svc.instance, sizeof svc.instance, "OtherSrv._http._tcp.local");
    TEST_ASSERT_EQUAL_INT(1, mdns_client_service_visible(g_client, &svc));
}

void test_mdns_client_count_null(void) {
    mdns_client_instance = NULL;
    TEST_ASSERT_EQUAL_UINT(0, mdns_client_count());
    mdns_client_instance = g_client;
}

void test_mdns_client_count_with_services(void) {
    mdns_client_service_t svc;
    memset(&svc, 0, sizeof(svc));
    g_client->services = &svc;
    g_client->nservices = 1;
    TEST_ASSERT_EQUAL_UINT(1, mdns_client_count());
}

void test_mdns_client_lookup_by_type_null_type(void) {
    mdns_client_service_t *out = (mdns_client_service_t *)0x1;
    size_t result = mdns_client_lookup_by_type(NULL, &out);
    TEST_ASSERT_EQUAL_UINT(0, result);
}

void test_mdns_client_lookup_by_type_no_match(void) {
    mdns_client_service_t *out = NULL;
    size_t result = mdns_client_lookup_by_type("nonexistent", &out);
    TEST_ASSERT_EQUAL_UINT(0, result);
    TEST_ASSERT_NULL(out);
}

void test_mdns_client_lookup_by_type_match(void) {
    mdns_client_service_t svc;
    memset(&svc, 0, sizeof(svc));
    snprintf(svc.type, sizeof svc.type, "_http._tcp.local");
    g_client->services = &svc;
    g_client->nservices = 1;

    mdns_client_service_t *out = NULL;
    size_t result = mdns_client_lookup_by_type("_http._tcp.local", &out);
    TEST_ASSERT_EQUAL_UINT(1, result);
    TEST_ASSERT_NOT_NULL(out);
    free(out);
}

void test_mdns_client_lookup_by_type_null_out(void) {
    mdns_client_service_t svc;
    memset(&svc, 0, sizeof(svc));
    snprintf(svc.type, sizeof svc.type, "_http._tcp.local");
    g_client->services = &svc;
    g_client->nservices = 1;

    size_t result = mdns_client_lookup_by_type("_http._tcp.local", NULL);
    TEST_ASSERT_EQUAL_UINT(1, result);
}

void test_mdns_client_info_json_no_server(void) {
    json_t *result = mdns_client_info_json();
    TEST_ASSERT_NOT_NULL(result);
    json_t *hostname = json_object_get(result, "hostname");
    TEST_ASSERT_NOT_NULL(hostname);
    json_t *claimed = json_object_get(result, "claimed");
    TEST_ASSERT_NOT_NULL(claimed);
    json_t *instances = json_object_get(result, "instances");
    TEST_ASSERT_NOT_NULL(instances);
    json_decref(result);
}

void test_mdns_client_info_json_with_server(void) {
    mdns_server_t server;
    mdns_server_service_t service;
    char hostname_buf[] = "host.local";
    char svc_name[] = "TestSvc";
    char svc_type[] = "_http._tcp.local";

    memset(&server, 0, sizeof(server));
    server.hostname = hostname_buf;

    memset(&service, 0, sizeof(service));
    service.name = svc_name;
    service.type = svc_type;
    service.claimed = 1;
    server.services = &service;
    server.num_services = 1;
    mdns_server = &server;

    json_t *result = mdns_client_info_json();
    TEST_ASSERT_NOT_NULL(result);
    json_t *hostname = json_object_get(result, "hostname");
    TEST_ASSERT_NOT_NULL(hostname);
    const char *hval = json_string_value(hostname);
    TEST_ASSERT_NOT_NULL(hval);
    TEST_ASSERT_EQUAL_STRING("host.local", hval);
    json_decref(result);
}

void test_mdns_client_is_own_instance_unclaimed(void) {
    mdns_server_t server;
    mdns_server_service_t service;
    char name[] = "MySvc";
    char type[] = "_http._tcp.local";

    memset(&server, 0, sizeof(server));
    memset(&service, 0, sizeof(service));
    service.name = name;
    service.type = type;
    service.claimed = 0;
    server.services = &service;
    server.num_services = 1;
    mdns_server = &server;

    TEST_ASSERT_EQUAL_INT(0, mdns_client_is_own_instance("MySvc._http._tcp.local"));
}

void test_mdns_client_info_json_with_endpoints(void) {
    mdns_client_service_t svc;
    memset(&svc, 0, sizeof(svc));
    snprintf(svc.instance, sizeof svc.instance, "Test._http._tcp.local");
    snprintf(svc.type, sizeof svc.type, "_http._tcp.local");
    svc.port = 8080;
    svc.healthy = 1;
    svc.nendpoints = 1;
    svc.endpoints[0].family = AF_INET;
    svc.endpoints[0].addrlen = 4;
    const uint8_t addr4[] = {192, 168, 1, 100};
    memcpy(svc.endpoints[0].addr, addr4, 4);

    g_client->services = &svc;
    g_client->nservices = 1;

    json_t *result = mdns_client_info_json();
    TEST_ASSERT_NOT_NULL(result);
    json_t *instances = json_object_get(result, "instances");
    TEST_ASSERT_NOT_NULL(instances);
    TEST_ASSERT_EQUAL_INT(1, json_array_size(instances));
    json_t *first = json_array_get(instances, 0);
    TEST_ASSERT_NOT_NULL(first);
    json_t *addrs = json_object_get(first, "addrs");
    TEST_ASSERT_NOT_NULL(addrs);
    TEST_ASSERT_EQUAL_INT(1, json_array_size(addrs));
    json_t *addr_str = json_array_get(addrs, 0);
    TEST_ASSERT_NOT_NULL(addr_str);
    TEST_ASSERT_EQUAL_STRING("192.168.1.100", json_string_value(addr_str));
    json_decref(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mdns_client_add_browse_type_normal);
    RUN_TEST(test_mdns_client_add_browse_type_duplicate);
    RUN_TEST(test_mdns_client_add_browse_type_null_args);
    RUN_TEST(test_mdns_client_add_browse_type_empty_type);
    RUN_TEST(test_mdns_client_service_visible_null);
    RUN_TEST(test_mdns_client_service_visible_own_instance);
    RUN_TEST(test_mdns_client_service_visible_other);
    RUN_TEST(test_mdns_client_count_null);
    RUN_TEST(test_mdns_client_count_with_services);
    RUN_TEST(test_mdns_client_lookup_by_type_null_type);
    RUN_TEST(test_mdns_client_lookup_by_type_no_match);
    RUN_TEST(test_mdns_client_lookup_by_type_match);
    RUN_TEST(test_mdns_client_lookup_by_type_null_out);
    RUN_TEST(test_mdns_client_info_json_no_server);
    RUN_TEST(test_mdns_client_info_json_with_server);
    RUN_TEST(test_mdns_client_is_own_instance_unclaimed);
    RUN_TEST(test_mdns_client_info_json_with_endpoints);

    return UNITY_END();
}
