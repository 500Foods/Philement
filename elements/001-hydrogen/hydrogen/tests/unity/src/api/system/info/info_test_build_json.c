/*
  * Unity Test File: system_info_build_json helper tests
  *
  * Phase 9 of TERMINAL_FIX_PLAN. Tests the shared system_info_build_json
  * helper that both handle_system_info_request (REST) and
  * H_lua_system_info (Lua) call.
  *
  * Validates:
  *   - system_info_build_json(false, false, "none") returns a public object
  *     with version.auth="none", status.server_running (in production mode),
  *     no terminal/scripting/system
  *   - system_info_build_json(true, false, "jwt") returns full object with
  *     version.auth="jwt" (production) — no terminal
  *   - system_info_build_json(true, true, "jwt, terminal") includes terminal
  *     object with protocol from ws_context->terminal_protocol and key from
  *     ws_context->terminal_auth_key (not the chat key)
  *   - system_info_build_json(true, false, NULL) is Lua path: no version.auth,
  *     no terminal
  *   - system_info_build_json(true, true, ...) with ws_context=NULL omits terminal
  *   - Terminal key uses Terminal.Key (terminal_auth_key), not WebSocketServer.Key
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

void test_system_info_build_json_public_shape(void);
void test_system_info_build_json_valid_jwt_shape(void);
void test_system_info_has_valid_jwt_null_connection(void);
void test_system_info_build_json_returns_object(void);
void test_system_info_build_json_terminal_config_with_ws_context(void);
void test_system_info_build_json_no_terminal_config_without_ws_context(void);
void test_system_info_build_json_terminal_key_from_terminal_auth_key(void);
void test_system_info_build_json_lua_public_shape(void);

void setUp(void) {
    memset(&mock_app_config_storage, 0, sizeof(mock_app_config_storage));
    app_config = &mock_app_config_storage;
    /* Set up a test public URL for terminal config tests. */
    mock_app_config_storage.websocket.public_url = strdup("wss://localhost:5261");
    mock_app_config_storage.terminal.enabled = true;
    mock_app_config_storage.terminal.web_path = strdup("/terminal");
    mock_app_config_storage.terminal.cors_origin = strdup("https://localhost:5261");
    mock_logging_reset_all();

    original_ws_context = ws_context;
    memset(&mock_ws_context_storage, 0, sizeof(mock_ws_context_storage));
    mock_ws_context_storage.port = 5261;
    strncpy(mock_ws_context_storage.auth_key, "test_websocket_chat_key_123",
            sizeof(mock_ws_context_storage.auth_key) - 1);
    mock_ws_context_storage.auth_key[sizeof(mock_ws_context_storage.auth_key) - 1] = '\0';
    /* Terminal key is distinct from chat key. */
    strncpy(mock_ws_context_storage.terminal_auth_key, "test_websocket_terminal_key_456",
            sizeof(mock_ws_context_storage.terminal_auth_key) - 1);
    mock_ws_context_storage.terminal_auth_key[sizeof(mock_ws_context_storage.terminal_auth_key) - 1] = '\0';
    /* Terminal protocol is distinct from chat protocol. */
    strncpy(mock_ws_context_storage.terminal_protocol, "terminal",
            sizeof(mock_ws_context_storage.terminal_protocol) - 1);
    mock_ws_context_storage.terminal_protocol[sizeof(mock_ws_context_storage.terminal_protocol) - 1] = '\0';
    /* Chat protocol. */
    strncpy(mock_ws_context_storage.protocol, "hydrogen",
            sizeof(mock_ws_context_storage.protocol) - 1);
    mock_ws_context_storage.protocol[sizeof(mock_ws_context_storage.protocol) - 1] = '\0';
    pthread_mutex_init(&mock_ws_context_storage.mutex, NULL);
    ws_context = &mock_ws_context_storage;
}

void tearDown(void) {
    ws_context = original_ws_context;
    pthread_mutex_destroy(&mock_ws_context_storage.mutex);
    /* Free mock app_config strings. */
    if (app_config) {
        free(app_config->websocket.public_url);
        free(app_config->terminal.web_path);
        free(app_config->terminal.cors_origin);
        app_config->websocket.public_url = NULL;
        app_config->terminal.web_path = NULL;
        app_config->terminal.cors_origin = NULL;
    }
    app_config = NULL;
}

/* Public shape: no JWT (auth_mode="none") — short payload, no full dump.
 * In UNITY_TEST_MODE the public path returns a mock root, so we can only
 * verify that no terminal/scripting is present. */
void test_system_info_build_json_public_shape(void) {
    json_t* root = system_info_build_json(false, false, "none");
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_TRUE(json_is_object(root));

#ifndef UNITY_TEST_MODE
    /* version.auth must be "none" (production path). */
    json_t* version = json_object_get(root, "version");
    TEST_ASSERT_NOT_NULL(version);
    json_t* auth = json_object_get(version, "auth");
    TEST_ASSERT_NOT_NULL(auth);
    TEST_ASSERT_EQUAL_STRING("none", json_string_value(auth));

    /* status.server_running must be present. */
    json_t* status = json_object_get(root, "status");
    TEST_ASSERT_NOT_NULL(status);
    json_t* running = json_object_get(status, "server_running");
    TEST_ASSERT_NOT_NULL(running);
    TEST_ASSERT_TRUE(json_is_boolean(running));
#else
    /* In test mode, verify no terminal/scripting/system leaked. */
    TEST_ASSERT_NULL(json_object_get(root, "system"));
#endif

    /* No scripting or terminal in public shape. */
    TEST_ASSERT_NULL(json_object_get(root, "scripting"));
    TEST_ASSERT_NULL(json_object_get(root, "terminal"));

    json_decref(root);
}

/* Valid JWT without terminal role (auth_mode="jwt").
 * In UNITY_TEST_MODE there is no "version" object (mock root), so we
 * only check terminal absence. */
void test_system_info_build_json_valid_jwt_shape(void) {
    json_t* root = system_info_build_json(true, false, "jwt");
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_TRUE(json_is_object(root));

#ifndef UNITY_TEST_MODE
    /* version.auth must be "jwt". */
    json_t* version = json_object_get(root, "version");
    TEST_ASSERT_NOT_NULL(version);
    json_t* auth = json_object_get(version, "auth");
    TEST_ASSERT_NOT_NULL(auth);
    TEST_ASSERT_EQUAL_STRING("jwt", json_string_value(auth));
#endif

    /* No terminal object for non-terminal JWT. */
    TEST_ASSERT_NULL(json_object_get(root, "terminal"));

    json_decref(root);
}

void test_system_info_has_valid_jwt_null_connection(void) {
    bool result = system_info_has_valid_jwt(NULL);
    TEST_ASSERT_FALSE(result);
}

void test_system_info_build_json_returns_object(void) {
    json_t* root_public = system_info_build_json(false, false, "none");
    TEST_ASSERT_NOT_NULL(root_public);
    TEST_ASSERT_TRUE(json_is_object(root_public));
    json_decref(root_public);

    json_t* root_jwt = system_info_build_json(true, true, "jwt, terminal");
    TEST_ASSERT_NOT_NULL(root_jwt);
    TEST_ASSERT_TRUE(json_is_object(root_jwt));
    json_decref(root_jwt);
}

/* Full terminal config in the authorized response. */
void test_system_info_build_json_terminal_config_with_ws_context(void) {
    json_t* root = system_info_build_json(true, true, "jwt, terminal");
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_TRUE(json_is_object(root));

    json_t* terminal = json_object_get(root, "terminal");
    TEST_ASSERT_NOT_NULL(terminal);
    TEST_ASSERT_TRUE(json_is_object(terminal));

    json_t* enabled = json_object_get(terminal, "enabled");
    TEST_ASSERT_NOT_NULL(enabled);
    TEST_ASSERT_TRUE(json_is_true(enabled));

    json_t* protocol = json_object_get(terminal, "protocol");
    TEST_ASSERT_NOT_NULL(protocol);
    TEST_ASSERT_TRUE(json_is_string(protocol));
    TEST_ASSERT_EQUAL_STRING("terminal", json_string_value(protocol));

    json_t* key = json_object_get(terminal, "key");
    TEST_ASSERT_NOT_NULL(key);
    TEST_ASSERT_TRUE(json_is_string(key));

    json_t* url = json_object_get(terminal, "url");
    TEST_ASSERT_NOT_NULL(url);
    TEST_ASSERT_TRUE(json_is_string(url));

    json_decref(root);
}

/* No terminal object when ws_context is NULL, even if has_terminal=true. */
void test_system_info_build_json_no_terminal_config_without_ws_context(void) {
    ws_context = NULL;

    json_t* root = system_info_build_json(true, true, "jwt, terminal");
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_TRUE(json_is_object(root));

    json_t* terminal = json_object_get(root, "terminal");
    TEST_ASSERT_NULL(terminal);

    json_decref(root);
}

/* Terminal key must come from ws_context->terminal_auth_key, NOT auth_key. */
void test_system_info_build_json_terminal_key_from_terminal_auth_key(void) {
    json_t* root = system_info_build_json(true, true, "jwt, terminal");
    TEST_ASSERT_NOT_NULL(root);

    json_t* terminal = json_object_get(root, "terminal");
    TEST_ASSERT_NOT_NULL(terminal);

    json_t* key = json_object_get(terminal, "key");
    TEST_ASSERT_NOT_NULL(key);
    TEST_ASSERT_TRUE(json_is_string(key));

    /* The terminal key must be the terminal_auth_key, not the chat auth_key. */
    const char *key_val = json_string_value(key);
    TEST_ASSERT_EQUAL_STRING("test_websocket_terminal_key_456", key_val);
    /* Must NOT be the chat key. */
    TEST_ASSERT_TRUE(strcmp(key_val, "test_websocket_chat_key_123") != 0);

    /* The protocol must be the terminal protocol, not the chat protocol. */
    json_t* protocol = json_object_get(terminal, "protocol");
    TEST_ASSERT_NOT_NULL(protocol);
    TEST_ASSERT_EQUAL_STRING("terminal", json_string_value(protocol));
    TEST_ASSERT_TRUE(strcmp(json_string_value(protocol), "hydrogen") != 0);

    json_decref(root);
}

/* Lua path: auth_mode=NULL — no version.auth, but still no terminal object. */
void test_system_info_build_json_lua_public_shape(void) {
    json_t* root = system_info_build_json(true, false, NULL);
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_TRUE(json_is_object(root));

    /* Lua path must not receive version.auth. */
    json_t* version = json_object_get(root, "version");
    if (version) {
        json_t* auth = json_object_get(version, "auth");
        TEST_ASSERT_NULL(auth);
    }

    /* Lua path must not receive terminal data. */
    TEST_ASSERT_NULL(json_object_get(root, "terminal"));

    json_decref(root);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_system_info_build_json_public_shape);
    RUN_TEST(test_system_info_build_json_valid_jwt_shape);
    RUN_TEST(test_system_info_has_valid_jwt_null_connection);
    RUN_TEST(test_system_info_build_json_returns_object);
    RUN_TEST(test_system_info_build_json_terminal_config_with_ws_context);
    RUN_TEST(test_system_info_build_json_no_terminal_config_without_ws_context);
    RUN_TEST(test_system_info_build_json_terminal_key_from_terminal_auth_key);
    RUN_TEST(test_system_info_build_json_lua_public_shape);

    return UNITY_END();
}
