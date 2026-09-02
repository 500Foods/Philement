/*
  * Unity Test File: system_info_build_json helper tests
  *
  * Phase 6 of CHAT_FINALE. Tests the shared system_info_build_json
  * helper that both handle_system_info_request (REST) and
  * H_lua_system_info (Lua) call.
  *
  * Validates:
  *   - system_info_build_json(false) returns a non-NULL json_t* without scripting key
  *   - system_info_build_json(true)  returns a non-NULL json_t* with scripting key
  *   - system_info_has_valid_jwt gracefully handles NULL connection
  *   - system_info_build_json returns a valid JSON object in both modes
  */

#define USE_MOCK_INFO
#define UNITY_TEST_MODE

#include <unity/mocks/mock_info.h>

#include <src/hydrogen.h>
#include <unity.h>

#include <jansson.h>

#include <src/api/system/info/info.h>

#include <tests/unity/mocks/mock_logging.h>

static AppConfig mock_app_config_storage = {0};

void test_system_info_build_json_without_scripting(void);
void test_system_info_build_json_with_scripting(void);
void test_system_info_has_valid_jwt_null_connection(void);
void test_system_info_build_json_returns_object(void);

void setUp(void) {
    memset(&mock_app_config_storage, 0, sizeof(mock_app_config_storage));
    app_config = &mock_app_config_storage;
    mock_logging_reset_all();
}

void tearDown(void) {
    app_config = NULL;
}

void test_system_info_build_json_without_scripting(void) {
    json_t* root = system_info_build_json(false);
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_TRUE(json_is_object(root));

    TEST_ASSERT_NULL(json_object_get(root, "scripting"));

    json_decref(root);
}

void test_system_info_build_json_with_scripting(void) {
    json_t* root = system_info_build_json(true);
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
    json_t* root_no_script = system_info_build_json(false);
    TEST_ASSERT_NOT_NULL(root_no_script);
    TEST_ASSERT_TRUE(json_is_object(root_no_script));
    json_decref(root_no_script);

    json_t* root_with_script = system_info_build_json(true);
    TEST_ASSERT_NOT_NULL(root_with_script);
    TEST_ASSERT_TRUE(json_is_object(root_with_script));
    json_decref(root_with_script);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_system_info_build_json_without_scripting);
    RUN_TEST(test_system_info_build_json_with_scripting);
    RUN_TEST(test_system_info_has_valid_jwt_null_connection);
    RUN_TEST(test_system_info_build_json_returns_object);

    return UNITY_END();
}
