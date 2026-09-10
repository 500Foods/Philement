/*
  * Unity Test File: system_info_build_json helper tests
  *
  * Phase 6 of CHAT_FINALE. Tests the shared system_info_build_json
  * helper that both handle_system_info_request (REST) and
  * H_lua_system_info (Lua) call.
  *
 * Validates:
 *   - system_info_build_json(false, false) returns a non-NULL json_t* without scripting or terminal keys
 *   - system_info_build_json(true, true)  returns a non-NULL json_t* with scripting key
 *   - system_info_has_valid_jwt gracefully handles NULL connection
 *   - system_info_build_json returns a valid JSON object in both modes
 *   - system_info_build_json(true, true) with ws_context set includes terminal config
 *     (port + key) for the xterm.js iframe page
 *   - system_info_build_json(true, false) returns terminal config when ws_context is set,
 *     even when scripting is disabled (terminal is independent of scripting)
 */

#define USE_MOCK_INFO
#define UNITY_TEST_MODE

#include <unity/mocks/mock_info.h>

#include <src/hydrogen.h>
#include <unity.h>

#include <jansson.h>

#include <src/api/system/info/info.h>
#include <src/websocket/websocket_server_internal.h>

#include <tests/unity/mocks/mock_logging.h>

static AppConfig mock_app_config_storage = {0};
static WebSocketServerContext mock_ws_context_storage;
static WebSocketServerContext *original_ws_context;

extern WebSocketServerContext *ws_context;

void test_system_info_build_json_without_scripting(void);
void test_system_info_build_json_with_scripting(void);
void test_system_info_has_valid_jwt_null_connection(void);
void test_system_info_build_json_returns_object(void);
void test_system_info_build_json_terminal_config_with_ws_context(void);
void test_system_info_build_json_no_terminal_config_without_ws_context(void);
void test_system_info_build_json_terminal_config_independent_of_scripting(void);

void setUp(void) {
    memset(&mock_app_config_storage, 0, sizeof(mock_app_config_storage));
    app_config = &mock_app_config_storage;
    mock_logging_reset_all();

    original_ws_context = ws_context;
    memset(&mock_ws_context_storage, 0, sizeof(mock_ws_context_storage));
    mock_ws_context_storage.port = 5261;
    strncpy(mock_ws_context_storage.auth_key, "test_websocket_key_123",
            sizeof(mock_ws_context_storage.auth_key) - 1);
    mock_ws_context_storage.auth_key[sizeof(mock_ws_context_storage.auth_key) - 1] = '\0';
    pthread_mutex_init(&mock_ws_context_storage.mutex, NULL);
    ws_context = &mock_ws_context_storage;
}

void tearDown(void) {
    ws_context = original_ws_context;
    pthread_mutex_destroy(&mock_ws_context_storage.mutex);
    app_config = NULL;
}

void test_system_info_build_json_without_scripting(void) {
    json_t* root = system_info_build_json(false, false);
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_TRUE(json_is_object(root));

    TEST_ASSERT_NULL(json_object_get(root, "scripting"));

    json_decref(root);
}

void test_system_info_build_json_with_scripting(void) {
    json_t* root = system_info_build_json(true, true);
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_TRUE(json_is_object(root));

    json_t* scripting = json_object_get(root, "scripting");
    TEST_ASSERT_NOT_NULL(scripting);
    TEST_ASSERT_TRUE(json_is_object(scripting));

    TEST_ASSERT_NOT_NULL(json_object_get(scripting, "enabled"));

    json_decref(root);
}

void test_system_info_has_valid_jwt_null_connection(void) {
    bool result = system_info_has_valid_jwt(NULL);
    TEST_ASSERT_FALSE(result);
}

void test_system_info_build_json_returns_object(void) {
    json_t* root_no_script = system_info_build_json(false, false);
    TEST_ASSERT_NOT_NULL(root_no_script);
    TEST_ASSERT_TRUE(json_is_object(root_no_script));
    json_decref(root_no_script);

    json_t* root_with_script = system_info_build_json(true, true);
    TEST_ASSERT_NOT_NULL(root_with_script);
    TEST_ASSERT_TRUE(json_is_object(root_with_script));
    json_decref(root_with_script);
}

void test_system_info_build_json_terminal_config_with_ws_context(void) {
    json_t* root = system_info_build_json(true, true);
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_TRUE(json_is_object(root));

    json_t* terminal = json_object_get(root, "terminal");
    TEST_ASSERT_NOT_NULL(terminal);
    TEST_ASSERT_TRUE(json_is_object(terminal));

    json_t* port = json_object_get(terminal, "port");
    TEST_ASSERT_NOT_NULL(port);
    TEST_ASSERT_TRUE(json_is_integer(port));
    TEST_ASSERT_EQUAL_INT(5261, json_integer_value(port));

    json_t* key = json_object_get(terminal, "key");
    TEST_ASSERT_NOT_NULL(key);
    TEST_ASSERT_TRUE(json_is_string(key));
    TEST_ASSERT_EQUAL_STRING("test_websocket_key_123", json_string_value(key));

    json_decref(root);
}

void test_system_info_build_json_no_terminal_config_without_ws_context(void) {
    ws_context = NULL;

    json_t* root = system_info_build_json(true, true);
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_TRUE(json_is_object(root));

    json_t* terminal = json_object_get(root, "terminal");
    TEST_ASSERT_NULL(terminal);

    json_decref(root);
}

void test_system_info_build_json_terminal_config_independent_of_scripting(void) {
    json_t* root = system_info_build_json(false, true);
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_TRUE(json_is_object(root));

    TEST_ASSERT_NULL(json_object_get(root, "scripting"));

    json_t* terminal = json_object_get(root, "terminal");
    TEST_ASSERT_NOT_NULL(terminal);
    TEST_ASSERT_TRUE(json_is_object(terminal));

    json_t* port = json_object_get(terminal, "port");
    TEST_ASSERT_NOT_NULL(port);
    TEST_ASSERT_EQUAL_INT(5261, json_integer_value(port));

    json_decref(root);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_system_info_build_json_without_scripting);
    RUN_TEST(test_system_info_build_json_with_scripting);
    RUN_TEST(test_system_info_has_valid_jwt_null_connection);
    RUN_TEST(test_system_info_build_json_returns_object);
    RUN_TEST(test_system_info_build_json_terminal_config_with_ws_context);
    RUN_TEST(test_system_info_build_json_no_terminal_config_without_ws_context);
    RUN_TEST(test_system_info_build_json_terminal_config_independent_of_scripting);

    return UNITY_END();
}
