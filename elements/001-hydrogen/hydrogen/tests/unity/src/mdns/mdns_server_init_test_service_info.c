/*
 * Unity Test: mdns_server_init_test_service_info.c
 * Tests mdns_server_init_service_info function
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/mdns/mdns_server.h>

// Forward declarations for helper functions being tested
int mdns_server_init_service_info(mdns_server_t *server, const char *app_name, const char *id,
                                  const char *friendly_name, const char *model, const char *manufacturer,
                                  const char *sw_version, const char *hw_version, const char *config_url);

// Test function prototypes
void test_mdns_server_init_service_info_success(void);
void test_mdns_server_init_service_info_all_params(void);
void test_mdns_server_init_service_info_long_strings(void);
void test_mdns_server_init_service_info_empty_strings(void);
void test_mdns_server_init_service_info_special_chars(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test mdns_server_init_service_info with basic success case
void test_mdns_server_init_service_info_success(void) {
    mdns_server_t server = {0};
    
    int result = mdns_server_init_service_info(&server, 
        "TestApp", "test123", "Test Printer", "Model-X", 
        "TestCorp", "1.0.0", "HW-1.0", "http://config.test");
    
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_NOT_NULL(server.service_name);
    TEST_ASSERT_EQUAL_STRING("TestApp", server.service_name);
    TEST_ASSERT_NOT_NULL(server.device_id);
    TEST_ASSERT_EQUAL_STRING("test123", server.device_id);
    TEST_ASSERT_NOT_NULL(server.friendly_name);
    TEST_ASSERT_EQUAL_STRING("Test Printer", server.friendly_name);
    TEST_ASSERT_NOT_NULL(server.model);
    TEST_ASSERT_EQUAL_STRING("Model-X", server.model);
    TEST_ASSERT_NOT_NULL(server.manufacturer);
    TEST_ASSERT_EQUAL_STRING("TestCorp", server.manufacturer);
    TEST_ASSERT_NOT_NULL(server.sw_version);
    TEST_ASSERT_EQUAL_STRING("1.0.0", server.sw_version);
    TEST_ASSERT_NOT_NULL(server.hw_version);
    TEST_ASSERT_EQUAL_STRING("HW-1.0", server.hw_version);
    TEST_ASSERT_NOT_NULL(server.config_url);
    TEST_ASSERT_EQUAL_STRING("http://config.test", server.config_url);
    TEST_ASSERT_NOT_NULL(server.secret_key);
    
    // Clean up
    free(server.service_name);
    free(server.device_id);
    free(server.friendly_name);
    free(server.model);
    free(server.manufacturer);
    free(server.sw_version);
    free(server.hw_version);
    free(server.config_url);
    free(server.secret_key);
}

// Test mdns_server_init_service_info with comprehensive parameters
void test_mdns_server_init_service_info_all_params(void) {
    mdns_server_t server = {0};
    
    int result = mdns_server_init_service_info(&server,
        "HydrogenPrinter", "HP-ABC123", "Hydrogen Network Printer",
        "HP-3000-Series", "Hydrogen Manufacturing", "2.1.5", "3.0.1",
        "https://printer.local/config");
    
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL_STRING("HydrogenPrinter", server.service_name);
    TEST_ASSERT_EQUAL_STRING("HP-ABC123", server.device_id);
    TEST_ASSERT_EQUAL_STRING("Hydrogen Network Printer", server.friendly_name);
    TEST_ASSERT_EQUAL_STRING("HP-3000-Series", server.model);
    TEST_ASSERT_EQUAL_STRING("Hydrogen Manufacturing", server.manufacturer);
    TEST_ASSERT_EQUAL_STRING("2.1.5", server.sw_version);
    TEST_ASSERT_EQUAL_STRING("3.0.1", server.hw_version);
    TEST_ASSERT_EQUAL_STRING("https://printer.local/config", server.config_url);
    
    // Clean up
    free(server.service_name);
    free(server.device_id);
    free(server.friendly_name);
    free(server.model);
    free(server.manufacturer);
    free(server.sw_version);
    free(server.hw_version);
    free(server.config_url);
    free(server.secret_key);
}

// Test mdns_server_init_service_info with long strings
void test_mdns_server_init_service_info_long_strings(void) {
    mdns_server_t server = {0};
    
    char long_name[100];
    char long_url[200];
    memset(long_name, 'A', 99);
    long_name[99] = '\0';
    memset(long_url, 'B', 199);
    long_url[199] = '\0';
    strcpy(long_url, "http://very-long-config-url-");
    
    int result = mdns_server_init_service_info(&server,
        long_name, "id123", "Long Name Printer", "Model123",
        "Very Long Manufacturer Name Inc.", "1.2.3", "4.5.6", long_url);
    
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL_STRING(long_name, server.service_name);
    TEST_ASSERT_EQUAL_STRING("id123", server.device_id);
    TEST_ASSERT_EQUAL_STRING("Long Name Printer", server.friendly_name);
    TEST_ASSERT_EQUAL_STRING("Model123", server.model);
    TEST_ASSERT_EQUAL_STRING("Very Long Manufacturer Name Inc.", server.manufacturer);
    TEST_ASSERT_EQUAL_STRING("1.2.3", server.sw_version);
    TEST_ASSERT_EQUAL_STRING("4.5.6", server.hw_version);
    TEST_ASSERT_EQUAL_STRING(long_url, server.config_url);
    
    // Clean up
    free(server.service_name);
    free(server.device_id);
    free(server.friendly_name);
    free(server.model);
    free(server.manufacturer);
    free(server.sw_version);
    free(server.hw_version);
    free(server.config_url);
    free(server.secret_key);
}

// Test mdns_server_init_service_info with empty strings
void test_mdns_server_init_service_info_empty_strings(void) {
    mdns_server_t server = {0};
    
    int result = mdns_server_init_service_info(&server,
        "", "", "", "", "", "", "", "");
    
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_NOT_NULL(server.service_name);
    TEST_ASSERT_EQUAL_STRING("", server.service_name);
    TEST_ASSERT_NOT_NULL(server.device_id);
    TEST_ASSERT_EQUAL_STRING("", server.device_id);
    TEST_ASSERT_NOT_NULL(server.friendly_name);
    TEST_ASSERT_EQUAL_STRING("", server.friendly_name);
    TEST_ASSERT_NOT_NULL(server.model);
    TEST_ASSERT_EQUAL_STRING("", server.model);
    TEST_ASSERT_NOT_NULL(server.manufacturer);
    TEST_ASSERT_EQUAL_STRING("", server.manufacturer);
    TEST_ASSERT_NOT_NULL(server.sw_version);
    TEST_ASSERT_EQUAL_STRING("", server.sw_version);
    TEST_ASSERT_NOT_NULL(server.hw_version);
    TEST_ASSERT_EQUAL_STRING("", server.hw_version);
    TEST_ASSERT_NOT_NULL(server.config_url);
    TEST_ASSERT_EQUAL_STRING("", server.config_url);
    
    // Clean up
    free(server.service_name);
    free(server.device_id);
    free(server.friendly_name);
    free(server.model);
    free(server.manufacturer);
    free(server.sw_version);
    free(server.hw_version);
    free(server.config_url);
    free(server.secret_key);
}

// Test mdns_server_init_service_info with special characters
void test_mdns_server_init_service_info_special_chars(void) {
    mdns_server_t server = {0};
    
    int result = mdns_server_init_service_info(&server,
        "App-Name_123", "ID#456", "Friendly & Name", "Model/Type",
        "Mfg & Co. Inc.", "v1.0-beta", "hw-2.0.1", "http://config.test?param=value&other=123");
    
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL_STRING("App-Name_123", server.service_name);
    TEST_ASSERT_EQUAL_STRING("ID#456", server.device_id);
    TEST_ASSERT_EQUAL_STRING("Friendly & Name", server.friendly_name);
    TEST_ASSERT_EQUAL_STRING("Model/Type", server.model);
    TEST_ASSERT_EQUAL_STRING("Mfg & Co. Inc.", server.manufacturer);
    TEST_ASSERT_EQUAL_STRING("v1.0-beta", server.sw_version);
    TEST_ASSERT_EQUAL_STRING("hw-2.0.1", server.hw_version);
    TEST_ASSERT_EQUAL_STRING("http://config.test?param=value&other=123", server.config_url);
    
    // Clean up
    free(server.service_name);
    free(server.device_id);
    free(server.friendly_name);
    free(server.model);
    free(server.manufacturer);
    free(server.sw_version);
    free(server.hw_version);
    free(server.config_url);
    free(server.secret_key);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mdns_server_init_service_info_success);
    RUN_TEST(test_mdns_server_init_service_info_all_params);
    RUN_TEST(test_mdns_server_init_service_info_long_strings);
    RUN_TEST(test_mdns_server_init_service_info_empty_strings);
    RUN_TEST(test_mdns_server_init_service_info_special_chars);

    return UNITY_END();
}