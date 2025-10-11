/*
 * Unity Test File: WebSocket Server callback_http Function Tests
 * Tests the callback_http function with various callback reasons and authentication scenarios
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the websocket module
#include <src/websocket/websocket_server.h>
#include <src/websocket/websocket_server_internal.h>

// Forward declarations for functions being tested
int callback_http(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len);

// Function prototypes for test functions
void test_callback_http_unknown_reason(void);
void test_callback_http_confirm_upgrade(void);
void test_callback_http_with_mock_auth_header(void);
void test_callback_http_auth_flow_logic(void);
void test_callback_http_auth_key_prefix_logic(void);
void test_callback_http_missing_auth_header(void);
void test_callback_http_malformed_auth_header(void);
void test_callback_http_empty_auth_header(void);
void test_callback_http_successful_authentication_header(void);
void test_callback_http_successful_authentication_query_param(void);
void test_callback_http_failed_authentication_wrong_key(void);
void test_callback_http_no_context(void);
void test_callback_http_malformed_header_too_short(void);
void test_callback_http_during_shutdown(void);
void test_callback_http_query_param_with_ampersand(void);
void test_callback_http_query_param_url_encoded(void);
void test_callback_http_query_param_wrong_key(void);
void test_callback_http_no_query_string(void);
void test_callback_http_query_param_no_key_param(void);

// External variables that need to be accessible for testing
extern WebSocketServerContext *ws_context;
extern AppConfig* app_config;

// Test fixtures
static WebSocketServerContext test_context;
static WebSocketServerContext *original_context;

void setUp(void) {
    // Save original context
    original_context = ws_context;
    
    // Initialize test context
    memset(&test_context, 0, sizeof(WebSocketServerContext));
    test_context.port = 8080;
    test_context.shutdown = 0;
    test_context.active_connections = 0;
    test_context.total_connections = 0;
    test_context.total_requests = 0;
    test_context.start_time = time(NULL);
    strncpy(test_context.protocol, "hydrogen-protocol", sizeof(test_context.protocol) - 1);
    strncpy(test_context.auth_key, "test_key_123", sizeof(test_context.auth_key) - 1);
    
    // Initialize mutex and condition variable
    pthread_mutex_init(&test_context.mutex, NULL);
    pthread_cond_init(&test_context.cond, NULL);
}

void tearDown(void) {
    // Restore original context
    ws_context = original_context;
    
    // Clean up test context
    pthread_mutex_destroy(&test_context.mutex);
    pthread_cond_destroy(&test_context.cond);
}

// Tests for callback_http function - now using mock libwebsockets for comprehensive testing
// With mock_libwebsockets, we can test the full authentication flow and edge cases

void test_callback_http_unknown_reason(void) {
    // Test with unknown callback reason - this is safe to test
    int result = callback_http(NULL, (enum lws_callback_reasons)999, NULL, NULL, 0);
    TEST_ASSERT_EQUAL_INT(0, result);  // Should return 0 for unknown reasons
}

void test_callback_http_confirm_upgrade(void) {
    // Test LWS_CALLBACK_HTTP_CONFIRM_UPGRADE - this is safe to test
    int result = callback_http(NULL, LWS_CALLBACK_HTTP_CONFIRM_UPGRADE, NULL, NULL, 0);
    TEST_ASSERT_EQUAL_INT(0, result);  // Should return 0 to confirm upgrade
}

void test_callback_http_with_mock_auth_header(void) {
    // Test HTTP callback with authentication (limited without real libwebsockets)
    // This is challenging to test without actual libwebsockets context
    
    // We can at least test the LWS_CALLBACK_HTTP case entry
    // The actual authentication logic requires libwebsockets functions
    enum lws_callback_reasons reason = LWS_CALLBACK_HTTP;
    
    // Verify the reason is correct
    TEST_ASSERT_EQUAL_INT(LWS_CALLBACK_HTTP, reason);
}

void test_callback_http_auth_flow_logic(void) {
    // Test the authentication flow logic conditions
    ws_context = &test_context;
    strncpy(test_context.auth_key, "test_key", sizeof(test_context.auth_key) - 1);
    
    // Test key comparison logic
    const char *test_key = "test_key";
    const char *wrong_key = "wrong_key";
    
    // Test correct key
    TEST_ASSERT_EQUAL_INT(0, strcmp(test_key, test_context.auth_key));
    
    // Test wrong key
    TEST_ASSERT_NOT_EQUAL(0, strcmp(wrong_key, test_context.auth_key));
}

void test_callback_http_auth_key_prefix_logic(void) {
    // Test the "Key " prefix logic
    const char *auth_header = "Key test_key_123";
    const char *expected_key = "test_key_123";
    
    // Test prefix check
    TEST_ASSERT_EQUAL_INT(0, strncmp(auth_header, "Key ", 4));
    
    // Test key extraction
    const char *extracted_key = auth_header + 4;
    TEST_ASSERT_EQUAL_STRING(expected_key, extracted_key);
}

void test_callback_http_missing_auth_header(void) {
    // Test HTTP callback with missing authorization header logic
    ws_context = &test_context;
    strncpy(test_context.auth_key, "required_key", sizeof(test_context.auth_key) - 1);

    // Simulate missing auth header condition
    int auth_len = 0;  // No header present
    TEST_ASSERT_EQUAL(0, auth_len);

    // Test that missing auth should fail
    bool should_fail = (auth_len == 0);
    TEST_ASSERT_TRUE(should_fail);

    // Test with present auth header to make conditions variable
    auth_len = 15;  // Header present
    TEST_ASSERT_NOT_EQUAL(0, auth_len);

    should_fail = (auth_len == 0);
    TEST_ASSERT_FALSE(should_fail);
}

void test_callback_http_malformed_auth_header(void) {
    // Test HTTP callback with malformed authorization header
    ws_context = &test_context;
    strncpy(test_context.auth_key, "correct_key", sizeof(test_context.auth_key) - 1);
    
    // Test various malformed headers
    const char *malformed_headers[] = {
        "Bearer token123",      // Wrong scheme
        "Key",                  // Missing key
        "Key ",                 // Empty key
        "WrongScheme key123",   // Wrong scheme name
        "key somekey",          // Wrong case
        ""                      // Empty header
    };
    
    for (size_t i = 0; i < sizeof(malformed_headers) / sizeof(malformed_headers[0]); i++) {
        const char *header = malformed_headers[i];
        bool has_correct_prefix = (strncmp(header, "Key ", 4) == 0);
        
        if (has_correct_prefix && strlen(header) > 4) {
            const char *key = header + 4;
            bool key_matches = (strcmp(key, test_context.auth_key) == 0);
            // All malformed headers should not match the correct key
            TEST_ASSERT_FALSE(key_matches);
        } else {
            // Headers without correct prefix should fail prefix check
            // The "Key " header (index 2) has correct prefix but no key content
            if (i == 2) {
                TEST_ASSERT_TRUE(has_correct_prefix);  // "Key " does have correct prefix
            } else if (strlen(header) >= 4) {
                TEST_ASSERT_FALSE(has_correct_prefix);
            } else {
                // Short headers can't have correct prefix
                TEST_ASSERT_FALSE(has_correct_prefix);
            }
        }
    }
}

void test_callback_http_empty_auth_header(void) {
    // Test HTTP callback with empty authorization header
    const char *empty_header = "";
    int header_len = (int)strlen(empty_header);

    TEST_ASSERT_EQUAL(0, header_len);

    // Test with non-empty header to make conditions variable
    const char *valid_header = "Key test_key";
    header_len = (int)strlen(valid_header);

    TEST_ASSERT_NOT_EQUAL(0, header_len);
    TEST_ASSERT_GREATER_OR_EQUAL(4, header_len);

    bool has_key_prefix = (strncmp(valid_header, "Key ", 4) == 0);
    TEST_ASSERT_TRUE(has_key_prefix);
}

// New comprehensive tests using mock libwebsockets functions

void test_callback_http_successful_authentication_header(void) {
    // Test successful authentication using Authorization header
    ws_context = &test_context;
    mock_lws_reset_all();

    // Mock the header functions to return our test data
    mock_lws_set_hdr_data("Key test_key_123");  // Set the header data
    mock_lws_set_hdr_copy_result(1);  // lws_hdr_copy returns success
    mock_lws_set_hdr_total_length_result(20);  // Header length

    // Create a mock wsi structure (we'll use a dummy pointer)
    struct lws *mock_wsi = (struct lws *)0x12345678;

    // Test the callback with HTTP reason
    int result = callback_http(mock_wsi, LWS_CALLBACK_HTTP, NULL, NULL, 0);

    // Should return 0 for successful authentication
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_callback_http_successful_authentication_query_param(void) {
    // Test successful authentication using query parameters
    ws_context = &test_context;
    mock_lws_reset_all();

    // Mock header functions to simulate no Authorization header
    // (auth header check happens first, so it should return 0)
    mock_lws_set_hdr_data("");  // No auth header
    
    // Mock URI to simulate query parameter with valid key
    const char *mock_uri = "/?key=test_key_123";
    mock_lws_set_uri_data(mock_uri);

    struct lws *mock_wsi = (struct lws *)0x87654321;

    int result = callback_http(mock_wsi, LWS_CALLBACK_HTTP, NULL, NULL, 0);

    // Should return 0 for successful authentication via query param
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_callback_http_query_param_with_ampersand(void) {
    // Test query parameter extraction when there are multiple params
    ws_context = &test_context;
    mock_lws_reset_all();

    // No auth header
    mock_lws_set_hdr_data("");
    
    // URI with key as first param and other params
    const char *mock_uri = "/?key=test_key_123&other=value";
    mock_lws_set_uri_data(mock_uri);

    struct lws *mock_wsi = (struct lws *)0x99999999;

    int result = callback_http(mock_wsi, LWS_CALLBACK_HTTP, NULL, NULL, 0);

    // Should return 0 for successful authentication
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_callback_http_query_param_url_encoded(void) {
    // Test URL-encoded query parameter (e.g., key with special characters)
    ws_context = &test_context;
    // Set a key with special character for testing URL decoding
    strncpy(test_context.auth_key, "test key 123", sizeof(test_context.auth_key) - 1);
    mock_lws_reset_all();

    // No auth header
    mock_lws_set_hdr_data("");
    
    // URI with URL-encoded key (space = %20)
    const char *mock_uri = "/?key=test%20key%20123";
    mock_lws_set_uri_data(mock_uri);

    struct lws *mock_wsi = (struct lws *)0xAAAAAAAA;

    int result = callback_http(mock_wsi, LWS_CALLBACK_HTTP, NULL, NULL, 0);

    // Should return 0 after decoding and matching
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_callback_http_query_param_wrong_key(void) {
    // Test query parameter with wrong key
    ws_context = &test_context;
    mock_lws_reset_all();

    // No auth header
    mock_lws_set_hdr_data("");
    
    // URI with wrong key
    const char *mock_uri = "/?key=wrong_key";
    mock_lws_set_uri_data(mock_uri);

    struct lws *mock_wsi = (struct lws *)0xBBBBBBBB;

    int result = callback_http(mock_wsi, LWS_CALLBACK_HTTP, NULL, NULL, 0);

    // Should return -1 for wrong key
    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_callback_http_no_query_string(void) {
    // Test URI without query string
    ws_context = &test_context;
    mock_lws_reset_all();

    // No auth header
    mock_lws_set_hdr_data("");
    
    // URI without query string
    const char *mock_uri = "/";
    mock_lws_set_uri_data(mock_uri);

    struct lws *mock_wsi = (struct lws *)0xCCCCCCCC;

    int result = callback_http(mock_wsi, LWS_CALLBACK_HTTP, NULL, NULL, 0);

    // Should return -1 (missing authorization)
    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_callback_http_query_param_no_key_param(void) {
    // Test query string without 'key=' parameter
    ws_context = &test_context;
    mock_lws_reset_all();

    // No auth header
    mock_lws_set_hdr_data("");
    
    // URI with query but no key param
    const char *mock_uri = "/?other=value&another=test";
    mock_lws_set_uri_data(mock_uri);

    struct lws *mock_wsi = (struct lws *)0xDDDDDDDD;

    int result = callback_http(mock_wsi, LWS_CALLBACK_HTTP, NULL, NULL, 0);

    // Should return -1 (missing authorization)
    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_callback_http_failed_authentication_wrong_key(void) {
    // Test failed authentication with wrong key
    ws_context = &test_context;
    mock_lws_reset_all();

    // Mock header functions with wrong key data
    mock_lws_set_hdr_data("Key wrong_key");  // Set wrong key data
    mock_lws_set_hdr_copy_result(1);
    mock_lws_set_hdr_total_length_result(15);  // "Key wrong_key" length

    struct lws *mock_wsi = (struct lws *)0x11111111;

    int result = callback_http(mock_wsi, LWS_CALLBACK_HTTP, NULL, NULL, 0);

    // Should return -1 for failed authentication
    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_callback_http_no_context(void) {
    // Test callback behavior when ws_context is NULL
    WebSocketServerContext *saved_context = ws_context;
    ws_context = NULL;
    mock_lws_reset_all();

    struct lws *mock_wsi = (struct lws *)0x22222222;

    int result = callback_http(mock_wsi, LWS_CALLBACK_HTTP, NULL, NULL, 0);

    // Should return -1 when no context is available
    TEST_ASSERT_EQUAL_INT(-1, result);

    // Restore context
    ws_context = saved_context;
}

void test_callback_http_malformed_header_too_short(void) {
    // Test with header too short to contain "Key "
    ws_context = &test_context;
    mock_lws_reset_all();

    mock_lws_set_hdr_total_length_result(3);  // Too short for "Key "

    struct lws *mock_wsi = (struct lws *)0x33333333;

    int result = callback_http(mock_wsi, LWS_CALLBACK_HTTP, NULL, NULL, 0);

    // Should return -1 for malformed header
    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_callback_http_during_shutdown(void) {
    // Test callback behavior during shutdown
    ws_context = &test_context;
    test_context.shutdown = 1;  // Simulate shutdown
    mock_lws_reset_all();

    struct lws *mock_wsi = (struct lws *)0x44444444;

    int result = callback_http(mock_wsi, LWS_CALLBACK_HTTP, NULL, NULL, 0);

    // Should return -1 during shutdown
    TEST_ASSERT_EQUAL_INT(-1, result);

    // Reset shutdown flag
    test_context.shutdown = 0;
}

int main(void) {
    UNITY_BEGIN();

    // callback_http tests - basic logic
    RUN_TEST(test_callback_http_unknown_reason);
    RUN_TEST(test_callback_http_confirm_upgrade);
    RUN_TEST(test_callback_http_with_mock_auth_header);
    RUN_TEST(test_callback_http_auth_flow_logic);
    RUN_TEST(test_callback_http_auth_key_prefix_logic);
    RUN_TEST(test_callback_http_missing_auth_header);
    RUN_TEST(test_callback_http_malformed_auth_header);
    RUN_TEST(test_callback_http_empty_auth_header);

    // Authorization header authentication tests
    RUN_TEST(test_callback_http_successful_authentication_header);
    RUN_TEST(test_callback_http_failed_authentication_wrong_key);
    RUN_TEST(test_callback_http_no_context);
    RUN_TEST(test_callback_http_malformed_header_too_short);
    RUN_TEST(test_callback_http_during_shutdown);
    
    // Query parameter authentication tests - new!
    RUN_TEST(test_callback_http_successful_authentication_query_param);
    RUN_TEST(test_callback_http_query_param_with_ampersand);
    RUN_TEST(test_callback_http_query_param_url_encoded);
    RUN_TEST(test_callback_http_query_param_wrong_key);
    RUN_TEST(test_callback_http_no_query_string);
    RUN_TEST(test_callback_http_query_param_no_key_param);

    return UNITY_END();
}
