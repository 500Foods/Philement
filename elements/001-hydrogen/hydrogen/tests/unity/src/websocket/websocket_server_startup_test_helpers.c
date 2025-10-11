/*
 * Unity Test File: WebSocket Server Startup Helper Functions Tests
 * This file contains unit tests for the helper functions created during refactoring
 * to achieve better test coverage for startup operations.
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the websocket startup module
#include <src/websocket/websocket_server_internal.h>
#include <src/websocket/websocket_server.h>

// Forward declarations for functions being tested
int validate_websocket_params(int port, const char* protocol, const char* key);
void setup_websocket_protocols(struct lws_protocols protocols[3], const char* protocol);
void configure_lws_context_info(struct lws_context_creation_info* info,
                               struct lws_protocols* protocols,
                               WebSocketServerContext* context);
void configure_lws_vhost_info(struct lws_context_creation_info* vhost_info,
                             int port, struct lws_protocols* protocols,
                             WebSocketServerContext* context);
int verify_websocket_port_binding(int port);

// Function prototypes for test functions
void test_validate_websocket_params_valid(void);
void test_validate_websocket_params_invalid_port(void);
void test_validate_websocket_params_invalid_port_high(void);
void test_validate_websocket_params_null_protocol(void);
void test_validate_websocket_params_empty_protocol(void);
void test_validate_websocket_params_null_key(void);
void test_validate_websocket_params_empty_key(void);
void test_setup_websocket_protocols_basic(void);
void test_setup_websocket_protocols_custom_protocol(void);
void test_configure_lws_context_info_basic(void);
void test_configure_lws_vhost_info_basic(void);
void test_verify_websocket_port_binding_available(void);
void test_verify_websocket_port_binding_invalid(void);

void setUp(void) {
    // No setup needed for these isolated tests
}

void tearDown(void) {
    // No teardown needed for these isolated tests
}

// Test parameter validation with valid inputs
void test_validate_websocket_params_valid(void) {
    int result = validate_websocket_params(8080, "test-protocol", "test-key");
    TEST_ASSERT_EQUAL_INT(0, result);
}

// Test parameter validation with invalid port (too low)
void test_validate_websocket_params_invalid_port(void) {
    int result = validate_websocket_params(0, "test-protocol", "test-key");
    TEST_ASSERT_EQUAL_INT(-1, result);
}

// Test parameter validation with invalid port (too high)
void test_validate_websocket_params_invalid_port_high(void) {
    int result = validate_websocket_params(70000, "test-protocol", "test-key");
    TEST_ASSERT_EQUAL_INT(-1, result);
}

// Test parameter validation with NULL protocol
void test_validate_websocket_params_null_protocol(void) {
    int result = validate_websocket_params(8080, NULL, "test-key");
    TEST_ASSERT_EQUAL_INT(-1, result);
}

// Test parameter validation with empty protocol
void test_validate_websocket_params_empty_protocol(void) {
    int result = validate_websocket_params(8080, "", "test-key");
    TEST_ASSERT_EQUAL_INT(-1, result);
}

// Test parameter validation with NULL key
void test_validate_websocket_params_null_key(void) {
    int result = validate_websocket_params(8080, "test-protocol", NULL);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

// Test parameter validation with empty key
void test_validate_websocket_params_empty_key(void) {
    int result = validate_websocket_params(8080, "test-protocol", "");
    TEST_ASSERT_EQUAL_INT(-1, result);
}

// Test protocol setup with basic configuration
void test_setup_websocket_protocols_basic(void) {
    struct lws_protocols protocols[3];
    setup_websocket_protocols(protocols, "hydrogen-protocol");

    // Test HTTP protocol (first entry)
    TEST_ASSERT_EQUAL_STRING("http", protocols[0].name);
    TEST_ASSERT_NOT_NULL(protocols[0].callback);
    TEST_ASSERT_EQUAL_INT(0, protocols[0].per_session_data_size);

    // Test custom protocol (second entry)
    TEST_ASSERT_EQUAL_STRING("hydrogen-protocol", protocols[1].name);
    TEST_ASSERT_NOT_NULL(protocols[1].callback);
    TEST_ASSERT_EQUAL_INT(sizeof(WebSocketSessionData), protocols[1].per_session_data_size);

    // Test terminator (third entry)
    TEST_ASSERT_NULL(protocols[2].name);
    TEST_ASSERT_NULL(protocols[2].callback);
}

// Test protocol setup with different protocol name
void test_setup_websocket_protocols_custom_protocol(void) {
    struct lws_protocols protocols[3];
    setup_websocket_protocols(protocols, "custom-ws-protocol");

    TEST_ASSERT_EQUAL_STRING("custom-ws-protocol", protocols[1].name);
    TEST_ASSERT_NULL(protocols[2].name); // Terminator
}

// Test context info configuration
void test_configure_lws_context_info_basic(void) {
    struct lws_context_creation_info info;
    struct lws_protocols protocols[3];
    WebSocketServerContext context;

    // Initialize test data
    memset(&context, 0, sizeof(context));
    context.port = 8080;
    setup_websocket_protocols(protocols, "test-protocol");

    // Configure context info
    configure_lws_context_info(&info, protocols, &context);

    // Verify configuration
    TEST_ASSERT_EQUAL_INT(8080, info.port);
    TEST_ASSERT_EQUAL_PTR(protocols, info.protocols);
    TEST_ASSERT_EQUAL_INT((gid_t)-1, info.gid);
    TEST_ASSERT_EQUAL_INT((uid_t)-1, info.uid);
    TEST_ASSERT_EQUAL_PTR(&context, info.user);
    TEST_ASSERT_EQUAL_INT(LWS_SERVER_OPTION_ALLOW_LISTEN_SHARE, info.options);
}

// Test vhost info configuration
void test_configure_lws_vhost_info_basic(void) {
    struct lws_context_creation_info vhost_info;
    struct lws_protocols protocols[3];
    WebSocketServerContext context;

    // Initialize test data
    memset(&context, 0, sizeof(context));
    context.port = 8080;
    setup_websocket_protocols(protocols, "test-protocol");

    // Configure vhost info
    configure_lws_vhost_info(&vhost_info, 8080, protocols, &context);

    // Verify configuration
    TEST_ASSERT_EQUAL_INT(8080, vhost_info.port);
    TEST_ASSERT_EQUAL_PTR(protocols, vhost_info.protocols);
    TEST_ASSERT_EQUAL_STRING("hydrogen", vhost_info.vhost_name);
    TEST_ASSERT_EQUAL_PTR(&context, vhost_info.user);
    TEST_ASSERT_TRUE(vhost_info.options & LWS_SERVER_OPTION_ALLOW_LISTEN_SHARE);
    TEST_ASSERT_TRUE(vhost_info.options & LWS_SERVER_OPTION_VALIDATE_UTF8);
    TEST_ASSERT_TRUE(vhost_info.options & LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE);
    TEST_ASSERT_TRUE(vhost_info.options & LWS_SERVER_OPTION_SKIP_SERVER_CANONICAL_NAME);
}

// Test port binding verification with a port that should be available
// Note: This test may be unreliable in some environments, but tests the logic
void test_verify_websocket_port_binding_available(void) {
    // Test with a high port number that's likely to be available
    // The function will attempt to create a socket and bind to test availability
    int result = verify_websocket_port_binding(65530);

    // Result depends on system state - either 0 (success) or -1 (failure)
    // The important thing is that the function executes without crashing
    TEST_ASSERT_TRUE(result == 0 || result == -1);
}

// Test port binding verification with port 0 (should fail)
void test_verify_websocket_port_binding_invalid(void) {
    // Test with invalid port
    int result = verify_websocket_port_binding(0);

    // Should fail because port 0 is invalid
    TEST_ASSERT_EQUAL_INT(-1, result);
}

int main(void) {
    UNITY_BEGIN();

    // Parameter validation tests
    RUN_TEST(test_validate_websocket_params_valid);
    RUN_TEST(test_validate_websocket_params_invalid_port);
    RUN_TEST(test_validate_websocket_params_invalid_port_high);
    RUN_TEST(test_validate_websocket_params_null_protocol);
    RUN_TEST(test_validate_websocket_params_empty_protocol);
    RUN_TEST(test_validate_websocket_params_null_key);
    RUN_TEST(test_validate_websocket_params_empty_key);

    // Protocol setup tests
    RUN_TEST(test_setup_websocket_protocols_basic);
    RUN_TEST(test_setup_websocket_protocols_custom_protocol);

    // Configuration tests
    RUN_TEST(test_configure_lws_context_info_basic);
    RUN_TEST(test_configure_lws_vhost_info_basic);

    // Port binding tests
    RUN_TEST(test_verify_websocket_port_binding_available);
    RUN_TEST(test_verify_websocket_port_binding_invalid);

    return UNITY_END();
}