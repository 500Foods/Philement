/*
 * Unity Test: mdns_server_init_test_allocate_services.c
 * Tests mdns_server_allocate_services and mdns_server_init_services functions
 */

#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../src/mdns/mdns_server.h"

// Forward declarations for helper functions being tested
int mdns_server_allocate_services(mdns_server_t *server, mdns_server_service_t *services, size_t num_services);
int mdns_server_init_services(mdns_server_t *server, const mdns_server_service_t *services, size_t num_services);

// Test function prototypes
void test_mdns_server_allocate_services_zero_services(void);
void test_mdns_server_allocate_services_multiple_services(void);
void test_mdns_server_init_services_single_service(void);
void test_mdns_server_init_services_multiple_services_with_txt(void);
void test_mdns_server_init_services_no_txt_records(void);
void test_mdns_server_init_services_empty_services(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test mdns_server_allocate_services with zero services
void test_mdns_server_allocate_services_zero_services(void) {
    mdns_server_t server = {0};
    
    int result = mdns_server_allocate_services(&server, NULL, 0);
    
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL(0, server.num_services);
    TEST_ASSERT_NULL(server.services);
}

// Test mdns_server_allocate_services with multiple services
void test_mdns_server_allocate_services_multiple_services(void) {
    mdns_server_t server = {0};
    
    int result = mdns_server_allocate_services(&server, NULL, 3);
    
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL(3, server.num_services);
    TEST_ASSERT_NOT_NULL(server.services);
    
    // Clean up
    free(server.services);
}

// Test mdns_server_init_services with single service
void test_mdns_server_init_services_single_service(void) {
    mdns_server_t server = {0};
    server.num_services = 1;
    server.services = malloc(sizeof(mdns_server_service_t) * 1);
    TEST_ASSERT_NOT_NULL(server.services);
    
    char name[] = "test_service";
    char type[] = "_http._tcp.local";
    char txt[] = "version=1.0";
    mdns_server_service_t input_service = {
        .name = name,
        .type = type,
        .port = 8080,
        .txt_records = (char*[]){txt},
        .num_txt_records = 1
    };
    
    int result = mdns_server_init_services(&server, &input_service, 1);
    
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_NOT_NULL(server.services[0].name);
    TEST_ASSERT_EQUAL_STRING("test_service", server.services[0].name);
    TEST_ASSERT_NOT_NULL(server.services[0].type);
    TEST_ASSERT_EQUAL_STRING("_http._tcp.local", server.services[0].type);
    TEST_ASSERT_EQUAL(8080, server.services[0].port);
    TEST_ASSERT_EQUAL(1, server.services[0].num_txt_records);
    TEST_ASSERT_NOT_NULL(server.services[0].txt_records);
    TEST_ASSERT_EQUAL_STRING("version=1.0", server.services[0].txt_records[0]);
    
    // Clean up
    free(server.services[0].name);
    free(server.services[0].type);
    free(server.services[0].txt_records[0]);
    free(server.services[0].txt_records);
    free(server.services);
}

// Test mdns_server_init_services with multiple services and TXT records
void test_mdns_server_init_services_multiple_services_with_txt(void) {
    mdns_server_t server = {0};
    server.num_services = 2;
    server.services = malloc(sizeof(mdns_server_service_t) * 2);
    TEST_ASSERT_NOT_NULL(server.services);
    
    char name1[] = "service1";
    char type1[] = "_http._tcp.local";
    char txt1_1[] = "path=/api";
    char txt1_2[] = "version=2.0";
    
    char name2[] = "service2";
    char type2[] = "_ssh._tcp.local";
    char txt2_1[] = "txtvers=1";
    
    mdns_server_service_t input_services[] = {
        {
            .name = name1,
            .type = type1,
            .port = 8080,
            .txt_records = (char*[]){txt1_1, txt1_2},
            .num_txt_records = 2
        },
        {
            .name = name2,
            .type = type2,
            .port = 22,
            .txt_records = (char*[]){txt2_1},
            .num_txt_records = 1
        }
    };
    
    int result = mdns_server_init_services(&server, input_services, 2);
    
    TEST_ASSERT_EQUAL(0, result);
    
    // Verify service 1
    TEST_ASSERT_EQUAL_STRING("service1", server.services[0].name);
    TEST_ASSERT_EQUAL_STRING("_http._tcp.local", server.services[0].type);
    TEST_ASSERT_EQUAL(8080, server.services[0].port);
    TEST_ASSERT_EQUAL(2, server.services[0].num_txt_records);
    TEST_ASSERT_EQUAL_STRING("path=/api", server.services[0].txt_records[0]);
    TEST_ASSERT_EQUAL_STRING("version=2.0", server.services[0].txt_records[1]);
    
    // Verify service 2
    TEST_ASSERT_EQUAL_STRING("service2", server.services[1].name);
    TEST_ASSERT_EQUAL_STRING("_ssh._tcp.local", server.services[1].type);
    TEST_ASSERT_EQUAL(22, server.services[1].port);
    TEST_ASSERT_EQUAL(1, server.services[1].num_txt_records);
    TEST_ASSERT_EQUAL_STRING("txtvers=1", server.services[1].txt_records[0]);
    
    // Clean up
    for (size_t i = 0; i < 2; i++) {
        free(server.services[i].name);
        free(server.services[i].type);
        for (size_t j = 0; j < server.services[i].num_txt_records; j++) {
            free(server.services[i].txt_records[j]);
        }
        free(server.services[i].txt_records);
    }
    free(server.services);
}

// Test mdns_server_init_services with no TXT records
void test_mdns_server_init_services_no_txt_records(void) {
    mdns_server_t server = {0};
    server.num_services = 1;
    server.services = malloc(sizeof(mdns_server_service_t) * 1);
    TEST_ASSERT_NOT_NULL(server.services);
    
    char name[] = "simple_service";
    char type[] = "_ftp._tcp.local";
    mdns_server_service_t input_service = {
        .name = name,
        .type = type,
        .port = 21,
        .txt_records = NULL,
        .num_txt_records = 0
    };
    
    int result = mdns_server_init_services(&server, &input_service, 1);
    
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL_STRING("simple_service", server.services[0].name);
    TEST_ASSERT_EQUAL_STRING("_ftp._tcp.local", server.services[0].type);
    TEST_ASSERT_EQUAL(21, server.services[0].port);
    TEST_ASSERT_EQUAL(0, server.services[0].num_txt_records);
    TEST_ASSERT_NULL(server.services[0].txt_records);
    
    // Clean up
    free(server.services[0].name);
    free(server.services[0].type);
    free(server.services);
}

// Test mdns_server_init_services with zero services
void test_mdns_server_init_services_empty_services(void) {
    mdns_server_t server = {0};
    server.num_services = 0;
    server.services = NULL;
    
    int result = mdns_server_init_services(&server, NULL, 0);
    
    TEST_ASSERT_EQUAL(0, result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mdns_server_allocate_services_zero_services);
    RUN_TEST(test_mdns_server_allocate_services_multiple_services);
    RUN_TEST(test_mdns_server_init_services_single_service);
    RUN_TEST(test_mdns_server_init_services_multiple_services_with_txt);
    RUN_TEST(test_mdns_server_init_services_no_txt_records);
    RUN_TEST(test_mdns_server_init_services_empty_services);

    return UNITY_END();
}