/*
 * Unity Test File: WebSocket Compression (permessage-deflate) Tests
 * This file contains unit tests to verify WebSocket compression is properly
 * configured via the permessage-deflate extension (RFC 7692).
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the websocket startup module
#include <src/websocket/websocket_server_internal.h>
#include <src/websocket/websocket_server.h>

// Include libwebsockets for extension types
#include <libwebsockets.h>

// Forward declarations for functions being tested
void configure_lws_context_info(struct lws_context_creation_info* info,
                               struct lws_protocols* protocols,
                               WebSocketServerContext* context);
void configure_lws_vhost_info(struct lws_context_creation_info* vhost_info,
                              int port, struct lws_protocols* protocols,
                              WebSocketServerContext* context);

// Test function prototypes
void test_configure_lws_context_info_has_compression(void);
void test_configure_lws_vhost_info_has_compression(void);
void test_compression_extension_parameters(void);

// External variables that need to be accessible for testing
extern WebSocketServerContext *ws_context;
extern AppConfig* app_config;

// Test fixtures
static WebSocketServerContext *original_context;
static AppConfig* original_config;

void setUp(void) {
    // Save original state
    original_context = ws_context;
    original_config = app_config;

    // Initialize global state for testing
    ws_context = NULL;
    app_config = NULL;
}

void tearDown(void) {
    // Restore original state
    ws_context = original_context;
    app_config = original_config;
}

// Test that context info includes compression extensions
void test_configure_lws_context_info_has_compression(void) {
    // Create a minimal test context
    WebSocketServerContext test_context = {0};
    test_context.port = 8080;
    strncpy(test_context.protocol, "test-protocol", sizeof(test_context.protocol) - 1);
    strncpy(test_context.auth_key, "test-key", sizeof(test_context.auth_key) - 1);

    struct lws_protocols test_protocols[] = {
        { "http", NULL, 0, 0, 0, NULL, 0 },
        { "test-protocol", NULL, 0, 0, 0, NULL, 0 },
        { NULL, NULL, 0, 0, 0, NULL, 0 }
    };

    struct lws_context_creation_info info;
    configure_lws_context_info(&info, test_protocols, &test_context);

    // Verify extensions are configured
    TEST_ASSERT_NOT_NULL(info.extensions);
    
    // Verify the first extension is permessage-deflate
    TEST_ASSERT_NOT_NULL(info.extensions[0].name);
    TEST_ASSERT_EQUAL_STRING("permessage-deflate", info.extensions[0].name);
    
    // Verify the extension has a callback (not NULL)
    TEST_ASSERT_NOT_NULL(info.extensions[0].callback);
    
    // Verify terminator is present
    TEST_ASSERT_NULL(info.extensions[1].name);
    TEST_ASSERT_NULL(info.extensions[1].callback);
    TEST_ASSERT_NULL(info.extensions[1].client_offer);
}

// Test that vhost info includes compression extensions
void test_configure_lws_vhost_info_has_compression(void) {
    // Create a minimal test context
    WebSocketServerContext test_context = {0};
    test_context.port = 8080;
    strncpy(test_context.protocol, "test-protocol", sizeof(test_context.protocol) - 1);
    strncpy(test_context.auth_key, "test-key", sizeof(test_context.auth_key) - 1);

    struct lws_protocols test_protocols[] = {
        { "http", NULL, 0, 0, 0, NULL, 0 },
        { "test-protocol", NULL, 0, 0, 0, NULL, 0 },
        { NULL, NULL, 0, 0, 0, NULL, 0 }
    };

    struct lws_context_creation_info vhost_info;
    configure_lws_vhost_info(&vhost_info, 8080, test_protocols, &test_context);

    // Verify extensions are configured
    TEST_ASSERT_NOT_NULL(vhost_info.extensions);
    
    // Verify the first extension is permessage-deflate
    TEST_ASSERT_NOT_NULL(vhost_info.extensions[0].name);
    TEST_ASSERT_EQUAL_STRING("permessage-deflate", vhost_info.extensions[0].name);
    
    // Verify the extension has a callback
    TEST_ASSERT_NOT_NULL(vhost_info.extensions[0].callback);
    
    // Verify terminator is present
    TEST_ASSERT_NULL(vhost_info.extensions[1].name);
}

// Test that compression extension has proper parameters for memory efficiency
void test_compression_extension_parameters(void) {
    WebSocketServerContext test_context = {0};
    test_context.port = 8080;
    strncpy(test_context.protocol, "test-protocol", sizeof(test_context.protocol) - 1);
    strncpy(test_context.auth_key, "test-key", sizeof(test_context.auth_key) - 1);

    struct lws_protocols test_protocols[] = {
        { "http", NULL, 0, 0, 0, NULL, 0 },
        { "test-protocol", NULL, 0, 0, 0, NULL, 0 },
        { NULL, NULL, 0, 0, 0, NULL, 0 }
    };

    struct lws_context_creation_info info;
    configure_lws_context_info(&info, test_protocols, &test_context);

    // Verify extension parameters include no_context_takeover for memory efficiency
    TEST_ASSERT_NOT_NULL(info.extensions[0].client_offer);
    
    // Check that client_offer contains key parameters
    const char* offer = info.extensions[0].client_offer;
    TEST_ASSERT_NOT_NULL(strstr(offer, "permessage-deflate"));
    TEST_ASSERT_NOT_NULL(strstr(offer, "client_no_context_takeover"));
    TEST_ASSERT_NOT_NULL(strstr(offer, "server_no_context_takeover"));
}

int main(void) {
    UNITY_BEGIN();

    // WebSocket compression tests
    RUN_TEST(test_configure_lws_context_info_has_compression);
    RUN_TEST(test_configure_lws_vhost_info_has_compression);
    RUN_TEST(test_compression_extension_parameters);

    return UNITY_END();
}
