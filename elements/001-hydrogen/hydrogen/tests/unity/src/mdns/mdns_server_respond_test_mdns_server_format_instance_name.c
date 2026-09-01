/*
 * Unity Test: mdns_server_respond_test_mdns_server_format_instance_name.c
 * Tests mdns_server_format_instance_name from mdns_server_respond.c
 *
 * CHANGELOG
 * 1.0.0 - 2026-09-01 - Initial creation
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mdns/mdns_keys.h>
#include <src/mdns/mdns_server.h>

void test_format_instance_name_null_args(void);
void test_format_instance_name_null_cap(void);
void test_format_instance_name_null_svc(void);
void test_format_instance_name_null_name(void);
void test_format_instance_name_null_type(void);
void test_format_instance_name_normal(void);
void test_format_instance_name_truncating(void);

void setUp(void) {}
void tearDown(void) {}

void test_format_instance_name_null_args(void) {
    mdns_server_service_t svc;
    memset(&svc, 0, sizeof(svc));
    svc.name = (char *)"TestService";
    svc.type = (char *)"_http._tcp.local";

    mdns_server_format_instance_name(&svc, NULL, 256);
}

void test_format_instance_name_null_cap(void) {
    mdns_server_service_t svc;
    memset(&svc, 0, sizeof(svc));
    svc.name = (char *)"TestService";
    svc.type = (char *)"_http._tcp.local";

    char out[256];
    out[0] = 'X';
    mdns_server_format_instance_name(&svc, out, 0);
    TEST_ASSERT_EQUAL_UINT('X', out[0]);
}

void test_format_instance_name_null_svc(void) {
    char out[256];
    out[0] = 'X';
    mdns_server_format_instance_name(NULL, out, sizeof(out));
    TEST_ASSERT_EQUAL_UINT(0, out[0]);
}

void test_format_instance_name_null_name(void) {
    mdns_server_service_t svc;
    memset(&svc, 0, sizeof(svc));
    svc.name = NULL;
    svc.type = (char *)"_http._tcp.local";

    char out[256];
    out[0] = 'X';
    mdns_server_format_instance_name(&svc, out, sizeof(out));
    TEST_ASSERT_EQUAL_UINT(0, out[0]);
}

void test_format_instance_name_null_type(void) {
    mdns_server_service_t svc;
    memset(&svc, 0, sizeof(svc));
    svc.name = (char *)"TestService";
    svc.type = NULL;

    char out[256];
    out[0] = 'X';
    mdns_server_format_instance_name(&svc, out, sizeof(out));
    TEST_ASSERT_EQUAL_UINT(0, out[0]);
}

void test_format_instance_name_normal(void) {
    mdns_server_service_t svc;
    memset(&svc, 0, sizeof(svc));
    svc.name = (char *)"TestService";
    svc.type = (char *)"_http._tcp.local";

    char out[256];
    mdns_server_format_instance_name(&svc, out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING("TestService._http._tcp.local", out);
}

void test_format_instance_name_truncating(void) {
    mdns_server_service_t svc;
    memset(&svc, 0, sizeof(svc));
    svc.name = (char *)"TestService";
    svc.type = (char *)"_http._tcp.local";

    char out[8];
    mdns_server_format_instance_name(&svc, out, sizeof(out));
    TEST_ASSERT_EQUAL_UINT(0, out[7]);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_format_instance_name_null_args);
    RUN_TEST(test_format_instance_name_null_cap);
    RUN_TEST(test_format_instance_name_null_svc);
    RUN_TEST(test_format_instance_name_null_name);
    RUN_TEST(test_format_instance_name_null_type);
    RUN_TEST(test_format_instance_name_normal);
    RUN_TEST(test_format_instance_name_truncating);

    return UNITY_END();
}
