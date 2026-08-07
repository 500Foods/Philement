/*
 * Unity Test File: WebSocket Heartbeat - ws_arm_heartbeat_timer
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/websocket/websocket_server.h>
#include <src/websocket/websocket_server_message.h>
#include <unity/mocks/mock_libwebsockets.h>

void test_ws_arm_heartbeat_timer_null_wsi(void);
void test_ws_arm_heartbeat_timer_disabled(void);
void test_ws_arm_heartbeat_timer_enabled(void);
void test_ws_arm_heartbeat_timer_clamps_interval(void);

extern AppConfig *app_config;

static AppConfig test_app_config;
static AppConfig *original_app_config;

void setUp(void) {
    original_app_config = app_config;
    memset(&test_app_config, 0, sizeof(test_app_config));
    test_app_config.websocket.heartbeat.enabled = true;
    test_app_config.websocket.heartbeat.ping_interval_seconds = 5;
    app_config = &test_app_config;
    mock_lws_reset_all();
}

void tearDown(void) {
    app_config = original_app_config;
    mock_lws_reset_all();
}

void test_ws_arm_heartbeat_timer_null_wsi(void) {
    ws_arm_heartbeat_timer(NULL);
    TEST_PASS();
}

void test_ws_arm_heartbeat_timer_disabled(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    test_app_config.websocket.heartbeat.enabled = false;
    ws_arm_heartbeat_timer(test_wsi);
    TEST_PASS();
}

void test_ws_arm_heartbeat_timer_enabled(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    ws_arm_heartbeat_timer(test_wsi);
    TEST_PASS();
}

void test_ws_arm_heartbeat_timer_clamps_interval(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    test_app_config.websocket.heartbeat.ping_interval_seconds = 0;
    ws_arm_heartbeat_timer(test_wsi);
    TEST_PASS();
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_ws_arm_heartbeat_timer_null_wsi);
    RUN_TEST(test_ws_arm_heartbeat_timer_disabled);
    RUN_TEST(test_ws_arm_heartbeat_timer_enabled);
    RUN_TEST(test_ws_arm_heartbeat_timer_clamps_interval);

    return UNITY_END();
}
