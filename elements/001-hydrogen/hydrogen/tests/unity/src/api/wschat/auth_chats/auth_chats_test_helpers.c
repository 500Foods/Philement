/*
 * Unity Test File: auth_chats helpers
 * Tests message conversion, request parameters, fan-out construction and IDs.
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include source headers
#include <src/api/wschat/auth_chats/auth_chats.h>
#include <unity/mocks/mock_auth_service_jwt.h>

void test_auth_chats_generate_broadcast_id_formats_uuid(void);
void test_auth_chats_generate_broadcast_id_handles_small_buffer(void);
void test_auth_chats_messages_json_to_list_filters_invalid_messages(void);
void test_auth_chats_messages_json_to_list_serializes_array_content(void);
void test_auth_chats_resolve_request_params_uses_defaults_and_overrides(void);
void test_auth_chats_build_multi_requests_filters_engines(void);
void test_auth_chats_copy_engine_names_copies_values(void);
void test_auth_chats_validate_jwt_from_header_rejects_invalid_format(void);
void test_auth_chats_validate_jwt_from_header_validates_bearer_token(void);

void setUp(void) {
    mock_auth_service_jwt_reset_all();
}

void tearDown(void) {
}

static ChatEngineConfig *make_engine(int id, const char *name, bool healthy) {
    ChatEngineConfig *engine = chat_engine_config_create(
        id, name, CEC_PROVIDER_OPENAI, "test-model", "https://example.test", "key",
        1000, 0.7, false, 0, 4, 8, 4, MODALITY_TEXT, false);
    if (engine) engine->is_healthy = healthy;
    return engine;
}

void test_auth_chats_generate_broadcast_id_formats_uuid(void) {
    char id[37];
    auth_chats_generate_broadcast_id(id, sizeof(id));
    TEST_ASSERT_EQUAL_UINT(36, strlen(id));
    TEST_ASSERT_EQUAL_CHAR('-', id[8]);
    TEST_ASSERT_EQUAL_CHAR('-', id[13]);
    TEST_ASSERT_EQUAL_CHAR('-', id[18]);
    TEST_ASSERT_EQUAL_CHAR('-', id[23]);
}

void test_auth_chats_generate_broadcast_id_handles_small_buffer(void) {
    char id[5] = "xxxx";
    auth_chats_generate_broadcast_id(id, sizeof(id));
    TEST_ASSERT_EQUAL_UINT(4, strlen(id));
    auth_chats_generate_broadcast_id(NULL, 0);
}

void test_auth_chats_messages_json_to_list_filters_invalid_messages(void) {
    json_t *messages = json_loads(
        "[null,{\"role\":\"user\"},{\"role\":\"user\",\"content\":7},"
        "{\"role\":\"user\",\"content\":\"hello\"}]", 0, NULL);
    ChatMessage *list = auth_chats_messages_json_to_list(messages);

    TEST_ASSERT_NOT_NULL(list);
    TEST_ASSERT_EQUAL_INT(CHAT_ROLE_USER, list->role);
    TEST_ASSERT_EQUAL_STRING("hello", list->content);
    TEST_ASSERT_NULL(list->next);
    chat_message_list_destroy(list);
    json_decref(messages);
    TEST_ASSERT_NULL(auth_chats_messages_json_to_list(NULL));
}

void test_auth_chats_messages_json_to_list_serializes_array_content(void) {
    json_t *messages = json_loads(
        "[{\"role\":\"assistant\",\"content\":[{\"type\":\"text\",\"text\":\"hi\"}]}]",
        0, NULL);
    ChatMessage *list = auth_chats_messages_json_to_list(messages);

    TEST_ASSERT_NOT_NULL(list);
    TEST_ASSERT_EQUAL_INT(CHAT_ROLE_ASSISTANT, list->role);
    TEST_ASSERT_NOT_NULL(strstr(list->content, "text"));
    chat_message_list_destroy(list);
    json_decref(messages);
}

void test_auth_chats_resolve_request_params_uses_defaults_and_overrides(void) {
    ChatEngineConfig engine = {0};
    engine.temperature_default = 0.8;
    engine.max_tokens = 2048;

    ChatRequestParams defaults = auth_chats_resolve_request_params(&engine, -1.0, -1);
    TEST_ASSERT_EQUAL_DOUBLE(0.8, defaults.temperature);
    TEST_ASSERT_EQUAL_INT(2048, defaults.max_tokens);

    ChatRequestParams overrides = auth_chats_resolve_request_params(&engine, 0.2, 99);
    TEST_ASSERT_EQUAL_DOUBLE(0.2, overrides.temperature);
    TEST_ASSERT_EQUAL_INT(99, overrides.max_tokens);
    (void)auth_chats_resolve_request_params(NULL, 0.2, 99);
}

void test_auth_chats_build_multi_requests_filters_engines(void) {
    ChatEngineCache *cache = chat_engine_cache_create("db");
    chat_engine_cache_add_engine(cache, make_engine(1, "healthy", true));
    chat_engine_cache_add_engine(cache, make_engine(2, "unhealthy", false));
    json_t *engines = json_pack("[s,s,s,i]", "healthy", "unhealthy", "missing", 7);
    ChatMultiRequest *requests = calloc(4, sizeof(ChatMultiRequest));
    ChatMessage *message = chat_message_create(CHAT_ROLE_USER, "hello", NULL);
    TEST_ASSERT_NOT_NULL(requests);
    TEST_ASSERT_NOT_NULL(message);

    size_t count = auth_chats_build_multi_requests(cache, engines, message, 0.3, 64, requests);
    TEST_ASSERT_EQUAL_UINT(1, count);
    TEST_ASSERT_EQUAL_STRING("healthy", requests[0].engine_name);
    TEST_ASSERT_NOT_NULL(strstr(requests[0].request_json, "test-model"));

    auth_chats_free_multi_requests(requests, count);
    chat_message_destroy(message);
    json_decref(engines);
    chat_engine_cache_destroy(cache);
    TEST_ASSERT_EQUAL_UINT(0, auth_chats_build_multi_requests(NULL, NULL, NULL, 0, 0, NULL));
}

void test_auth_chats_copy_engine_names_copies_values(void) {
    ChatMultiRequest requests[2] = {{0}};
    requests[0].engine_name = (char *)"one";
    char **names = auth_chats_copy_engine_names(requests, 2);

    TEST_ASSERT_NOT_NULL(names);
    TEST_ASSERT_EQUAL_STRING("one", names[0]);
    TEST_ASSERT_NULL(names[1]);
    TEST_ASSERT_NOT_EQUAL(requests[0].engine_name, names[0]);
    auth_chats_free_engine_names(names, 2);
    TEST_ASSERT_NULL(auth_chats_copy_engine_names(NULL, 0));
    auth_chats_free_engine_names(NULL, 0);
    auth_chats_free_multi_requests(NULL, 0);
}

void test_auth_chats_validate_jwt_from_header_rejects_invalid_format(void) {
    jwt_validation_result_t result = auth_chats_validate_jwt_from_header(NULL);
    TEST_ASSERT_FALSE(result.valid);
    TEST_ASSERT_EQUAL_INT(JWT_ERROR_INVALID_FORMAT, result.error);
    result = auth_chats_validate_jwt_from_header("Basic token");
    TEST_ASSERT_FALSE(result.valid);
}

void test_auth_chats_validate_jwt_from_header_validates_bearer_token(void) {
    jwt_claims_t claims = {0};
    claims.database = (char *)"db";
    mock_auth_service_jwt_set_validation_result(
        (jwt_validation_result_t){true, &claims, JWT_ERROR_NONE});

    jwt_validation_result_t result = auth_chats_validate_jwt_from_header("Bearer token");
    TEST_ASSERT_TRUE(result.valid);
    TEST_ASSERT_EQUAL_STRING("db", result.claims->database);
    free_jwt_validation_result(&result);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_auth_chats_generate_broadcast_id_formats_uuid);
    RUN_TEST(test_auth_chats_generate_broadcast_id_handles_small_buffer);
    RUN_TEST(test_auth_chats_messages_json_to_list_filters_invalid_messages);
    RUN_TEST(test_auth_chats_messages_json_to_list_serializes_array_content);
    RUN_TEST(test_auth_chats_resolve_request_params_uses_defaults_and_overrides);
    RUN_TEST(test_auth_chats_build_multi_requests_filters_engines);
    RUN_TEST(test_auth_chats_copy_engine_names_copies_values);
    RUN_TEST(test_auth_chats_validate_jwt_from_header_rejects_invalid_format);
    RUN_TEST(test_auth_chats_validate_jwt_from_header_validates_bearer_token);
    return UNITY_END();
}
