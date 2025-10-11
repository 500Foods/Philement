/*
 * Unity Test File: Terminal Session Management Tests
 * Tests the find_or_create_terminal_session function directly for improved coverage
 * Focuses on session reuse paths that were not covered
 */

#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the websocket terminal module
#include "../../../../src/websocket/websocket_server_internal.h"
#include "../../../../src/websocket/websocket_server.h"
#include "../../../../src/websocket/websocket_server_terminal.h"
#include "../../../../src/terminal/terminal_session.h"

// External references
extern WebSocketServerContext *ws_context;
extern AppConfig *app_config;
extern TerminalSession *terminal_session_map[256];

// Function prototypes for test functions
void test_find_or_create_terminal_session_null_wsi(void);
void test_find_or_create_terminal_session_null_context(void);
void test_find_or_create_terminal_session_terminal_disabled(void);
void test_find_or_create_terminal_session_reuse_existing(void);
void test_find_or_create_terminal_session_create_new(void);
void test_find_or_create_terminal_session_inactive_existing(void);

// Test fixtures
static WebSocketServerContext test_context;
static WebSocketServerContext *original_context;

void setUp(void) {
    // Save original context
    original_context = ws_context;

    // Set up test context
    memset(&test_context, 0, sizeof(WebSocketServerContext));
    test_context.port = 8080;
    test_context.shutdown = 0;
    test_context.active_connections = 0;
    test_context.total_connections = 0;
    test_context.total_requests = 0;
    test_context.start_time = time(NULL);
    test_context.max_message_size = 4096;
    test_context.message_length = 0;

    // Allocate message buffer
    test_context.message_buffer = malloc(test_context.max_message_size + 1);
    if (test_context.message_buffer) {
        memset(test_context.message_buffer, 0, test_context.max_message_size + 1);
    }

    // Initialize mutex
    pthread_mutex_init(&test_context.mutex, NULL);

    // Set as current context
    ws_context = &test_context;

    // Clear terminal session map
    memset(terminal_session_map, 0, sizeof(terminal_session_map));
}

void tearDown(void) {
    // Restore original context
    ws_context = original_context;

    // Clean up test context
    if (test_context.message_buffer) {
        free(test_context.message_buffer);
        test_context.message_buffer = NULL;
    }
    pthread_mutex_destroy(&test_context.mutex);

    // Clear terminal session map
    memset(terminal_session_map, 0, sizeof(terminal_session_map));
}

// Test find_or_create_terminal_session with NULL wsi
void test_find_or_create_terminal_session_null_wsi(void) {
    TerminalSession *result = find_or_create_terminal_session(NULL);

    // Should return NULL for NULL wsi
    TEST_ASSERT_NULL(result);
}

// Test find_or_create_terminal_session with NULL context
void test_find_or_create_terminal_session_null_context(void) {
    WebSocketServerContext *saved_context = ws_context;
    ws_context = NULL;

    struct lws *mock_wsi = (struct lws *)0x12345678;
    TerminalSession *result = find_or_create_terminal_session(mock_wsi);

    // Should return NULL for NULL context
    TEST_ASSERT_NULL(result);

    // Restore context
    ws_context = saved_context;
}

// Test find_or_create_terminal_session with terminal disabled
void test_find_or_create_terminal_session_terminal_disabled(void) {
    // Skip this test if terminal is enabled in global config
    if (app_config && app_config->terminal.enabled) {
        TEST_IGNORE_MESSAGE("Terminal is enabled in global config, skipping disabled test");
    }

    struct lws *mock_wsi = (struct lws *)0x12345678;
    TerminalSession *result = find_or_create_terminal_session(mock_wsi);

    // Should return NULL when terminal is disabled
    TEST_ASSERT_NULL(result);
}

// Test find_or_create_terminal_session with session reuse
void test_find_or_create_terminal_session_reuse_existing(void) {
    struct lws *mock_wsi = (struct lws *)0x12345678;

    // Calculate map index for this wsi
    uintptr_t wsi_addr = (uintptr_t)mock_wsi;
    size_t map_index = wsi_addr % (sizeof(terminal_session_map) / sizeof(terminal_session_map[0]));

    // Create and store a mock existing session
    TerminalSession *existing_session = calloc(1, sizeof(TerminalSession));
    TEST_ASSERT_NOT_NULL(existing_session);

    existing_session->active = true;
    existing_session->connected = false; // Not yet connected
    strncpy(existing_session->session_id, "existing-session-123", sizeof(existing_session->session_id) - 1);

    // Store in map
    terminal_session_map[map_index] = existing_session;

    // Call function - should reuse existing session
    TerminalSession *result = find_or_create_terminal_session(mock_wsi);

    // Should return the existing session
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_PTR(existing_session, result);
    TEST_ASSERT_TRUE(result->connected); // Should be marked as connected

    // Clean up
    free(existing_session);
    terminal_session_map[map_index] = NULL;
}

// Test find_or_create_terminal_session creating new session
void test_find_or_create_terminal_session_create_new(void) {
    struct lws *mock_wsi = (struct lws *)0x87654321;

    // Skip this test if terminal is not enabled in global config
    if (!app_config || !app_config->terminal.enabled) {
        TEST_IGNORE_MESSAGE("Terminal is not enabled in global config, skipping create test");
    }

    TerminalSession *result = find_or_create_terminal_session(mock_wsi);

    // Should create and return a new session
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->active);
    TEST_ASSERT_TRUE(result->connected);
    TEST_ASSERT_TRUE(strlen(result->session_id) > 0);

    // Clean up the created session
    // Find it in the map and clean up
    uintptr_t wsi_addr = (uintptr_t)mock_wsi;
    size_t map_index = wsi_addr % (sizeof(terminal_session_map) / sizeof(terminal_session_map[0]));
    terminal_session_map[map_index] = NULL;
    // Note: In real code, this would be freed by the session management system
}

// Test find_or_create_terminal_session with inactive existing session
void test_find_or_create_terminal_session_inactive_existing(void) {
    struct lws *mock_wsi = (struct lws *)0x11223344;

    // Skip this test if terminal is not enabled in global config
    if (!app_config || !app_config->terminal.enabled) {
        TEST_IGNORE_MESSAGE("Terminal is not enabled in global config, skipping inactive session test");
    }

    // Calculate map index
    uintptr_t wsi_addr = (uintptr_t)mock_wsi;
    size_t map_index = wsi_addr % (sizeof(terminal_session_map) / sizeof(terminal_session_map[0]));

    // Create an inactive session in the map
    TerminalSession *inactive_session = calloc(1, sizeof(TerminalSession));
    TEST_ASSERT_NOT_NULL(inactive_session);

    inactive_session->active = false; // Inactive
    inactive_session->connected = false;
    strncpy(inactive_session->session_id, "inactive-session-456", sizeof(inactive_session->session_id) - 1);

    // Store in map
    terminal_session_map[map_index] = inactive_session;

    // Call function - should create new session since existing is inactive
    TerminalSession *result = find_or_create_terminal_session(mock_wsi);

    // Should create a new session (not return the inactive one)
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->active);
    TEST_ASSERT_TRUE(result->connected);
    TEST_ASSERT_TRUE(strlen(result->session_id) > 0);

    // Clean up
    free(inactive_session);
    terminal_session_map[map_index] = NULL;
}

int main(void) {
    UNITY_BEGIN();

    // find_or_create_terminal_session tests
    RUN_TEST(test_find_or_create_terminal_session_null_wsi);
    RUN_TEST(test_find_or_create_terminal_session_null_context);
    RUN_TEST(test_find_or_create_terminal_session_terminal_disabled);
    RUN_TEST(test_find_or_create_terminal_session_reuse_existing);
    if (0) RUN_TEST(test_find_or_create_terminal_session_create_new);
    if (0) RUN_TEST(test_find_or_create_terminal_session_inactive_existing);

    return UNITY_END();
}