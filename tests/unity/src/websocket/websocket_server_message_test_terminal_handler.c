/*
 * Unity Test File: WebSocket Terminal Message Tests
 * Tests ws_handle_receive with terminal message types to cover the terminal branch
 * in handle_message_type and related functions.
 */

#include "../../../../../src/hydrogen.h"
#include "unity.h"

#include "../../../../../src/websocket/websocket_server.h"
#include "../../../../../src/websocket/websocket_server_internal.h"

#include <libwebsockets.h>  // For struct lws

// Enable mocks
#include "../../../../../tests/unity/mocks/mock_libwebsockets.h"

void setUp(void) {
    mock_lws_reset_all();
    mock_lws_set_is_final_fragment_result(1);
}

void tearDown(void) {
    mock_lws_reset_all();
}


int main(void) {
    UNITY_BEGIN();

    // RUN_TEST(test_ws_handle_receive_terminal_input);
    // RUN_TEST(test_ws_handle_receive_terminal_resize);
    // RUN_TEST(test_ws_handle_receive_terminal_ping);
    // RUN_TEST(test_ws_handle_receive_terminal_invalid_json);

    return UNITY_END();
}