/*
 * Unity Test File: chat request builder
 * This file contains unit tests for src/api/wschat/helpers/req_builder.c
 *
 * Coverage focus: message list append, role string helpers, the OpenAI /
 * Anthropic / Ollama request builders (including streaming + additional
 * params), the OpenAI->Anthropic multimodal content converter, image
 * counting, the generic dispatcher, and request validation error paths.
 */

#include <src/hydrogen.h>
#include <unity.h>
#include <stdlib.h>
#include <string.h>

#include <src/api/wschat/helpers/req_builder.h>
#include <src/api/wschat/helpers/engine_cache.h>

// Forward declarations for tests
void test_list_append_null_and_chain(void);
void test_role_to_string(void);
void test_role_from_string(void);
void test_estimate_tokens(void);
void test_build_openai_with_params(void);
void test_build_openai_additional_params(void);
void test_build_openai_null_args(void);
void test_convert_content_plain_text(void);
void test_convert_content_text_part(void);
void test_convert_content_image_data_url(void);
void test_convert_content_image_no_comma(void);
void test_convert_content_non_object_and_no_type(void);
void test_convert_content_unknown_type(void);
void test_build_anthropic_with_system_and_stream(void);
void test_build_anthropic_multimodal(void);
void test_build_ollama_native(void);
void test_build_ollama_stream(void);
void test_build_dispatch(void);
void test_to_json_string(void);
void test_count_images(void);
void test_count_all_images(void);
void test_validate_success(void);
void test_validate_null_engine(void);
void test_validate_null_messages(void);
void test_validate_token_limit(void);
void test_validate_image_count_limit(void);
void test_validate_modality_unsupported(void);

static ChatEngineConfig* g_engine = NULL;

static ChatEngineConfig* make_engine(ChatEngineProvider provider, int max_tokens,
                                     int max_images, int modalities, bool native) {
    return chat_engine_config_create(1, "eng", provider, "test-model",
        "http://localhost", "key", max_tokens, 0.0, false, 0,
        max_images, 0, 0, modalities, native);
}

void setUp(void) {
    g_engine = make_engine(CEC_PROVIDER_OPENAI, 1000, 4, MODALITY_ALL, false);
}

void tearDown(void) {
    if (g_engine) {
        chat_engine_config_destroy(g_engine);
        g_engine = NULL;
    }
}

/* ---- message list append ---- */
void test_list_append_null_and_chain(void) {
    // append NULL new_message returns head unchanged
    ChatMessage* head = chat_message_create(CHAT_ROLE_USER, "a", NULL);
    TEST_ASSERT_EQUAL_PTR(head, chat_message_list_append(head, NULL));
    // append to NULL head returns new message
    ChatMessage* solo = chat_message_create(CHAT_ROLE_USER, "b", NULL);
    TEST_ASSERT_EQUAL_PTR(solo, chat_message_list_append(NULL, solo));
    chat_message_list_destroy(solo);

    // chain several and confirm traversal to the tail
    ChatMessage* m2 = chat_message_create(CHAT_ROLE_ASSISTANT, "c", NULL);
    ChatMessage* m3 = chat_message_create(CHAT_ROLE_USER, "d", NULL);
    head = chat_message_list_append(head, m2);
    head = chat_message_list_append(head, m3);
    TEST_ASSERT_EQUAL_PTR(m3, head->next->next);
    TEST_ASSERT_NULL(head->next->next->next);
    chat_message_list_destroy(head);
}

/* ---- role helpers ---- */
void test_role_to_string(void) {
    TEST_ASSERT_EQUAL_STRING("system", chat_message_role_to_string(CHAT_ROLE_SYSTEM));
    TEST_ASSERT_EQUAL_STRING("user", chat_message_role_to_string(CHAT_ROLE_USER));
    TEST_ASSERT_EQUAL_STRING("assistant", chat_message_role_to_string(CHAT_ROLE_ASSISTANT));
    TEST_ASSERT_EQUAL_STRING("tool", chat_message_role_to_string(CHAT_ROLE_TOOL));
    TEST_ASSERT_EQUAL_STRING("user", chat_message_role_to_string(CHAT_ROLE_UNKNOWN));
    TEST_ASSERT_EQUAL_STRING("user", chat_message_role_to_string((ChatMessageRole)999));
}

void test_role_from_string(void) {
    TEST_ASSERT_EQUAL_INT(CHAT_ROLE_USER, chat_message_role_from_string(NULL));
    TEST_ASSERT_EQUAL_INT(CHAT_ROLE_SYSTEM, chat_message_role_from_string("SYSTEM"));
    TEST_ASSERT_EQUAL_INT(CHAT_ROLE_USER, chat_message_role_from_string("user"));
    TEST_ASSERT_EQUAL_INT(CHAT_ROLE_ASSISTANT, chat_message_role_from_string("Assistant"));
    TEST_ASSERT_EQUAL_INT(CHAT_ROLE_TOOL, chat_message_role_from_string("tool"));
    TEST_ASSERT_EQUAL_INT(CHAT_ROLE_UNKNOWN, chat_message_role_from_string("bogus"));
}

/* ---- token estimation ---- */
void test_estimate_tokens(void) {
    TEST_ASSERT_EQUAL_INT(0, chat_message_estimate_tokens(NULL));
    TEST_ASSERT_EQUAL_INT(1, chat_message_estimate_tokens(""));
    // "12345678" = 8 chars -> 8/4 + 1 = 3
    TEST_ASSERT_EQUAL_INT(3, chat_message_estimate_tokens("12345678"));

    ChatMessage* head = chat_message_create(CHAT_ROLE_USER, "hi", NULL);
    // per-message overhead (4) + estimate(2 chars=1) + base(2) = 7
    TEST_ASSERT_EQUAL_INT(7, chat_request_estimate_tokens(head));
    chat_message_list_destroy(head);
}

/* ---- OpenAI builder ---- */
void test_build_openai_with_params(void) {
    ChatMessage* head = chat_message_create(CHAT_ROLE_USER, "hello", NULL);
    ChatRequestParams params = chat_request_params_default();
    params.max_tokens = 50;
    params.stream = true;

    json_t* root = chat_request_build_openai(g_engine, head, &params);
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_EQUAL_STRING("test-model",
        json_string_value(json_object_get(root, "model")));
    TEST_ASSERT_EQUAL_INT(50, json_integer_value(json_object_get(root, "max_tokens")));
    TEST_ASSERT_TRUE(json_is_true(json_object_get(root, "stream")));
    TEST_ASSERT_TRUE(json_is_true(json_object_get(root, "include_retrieval_info")));

    json_decref(root);
    chat_message_list_destroy(head);
}

void test_build_openai_additional_params(void) {
    ChatMessage* head = chat_message_create(CHAT_ROLE_USER, "hello", NULL);
    ChatRequestParams params = chat_request_params_default();
    params.model = "override-model";
    params.additional_params = json_object();
    json_object_set_new(params.additional_params, "top_p", json_real(0.9));

    json_t* root = chat_request_build_openai(g_engine, head, &params);
    TEST_ASSERT_NOT_NULL(root);
    // model override applied
    TEST_ASSERT_EQUAL_STRING("override-model",
        json_string_value(json_object_get(root, "model")));
    // additional param merged
    TEST_ASSERT_EQUAL_DOUBLE(0.9, json_real_value(json_object_get(root, "top_p")));
    // engine max_tokens (1000) used when params has none
    TEST_ASSERT_EQUAL_INT(1000, json_integer_value(json_object_get(root, "max_tokens")));

    json_decref(root);
    json_decref(params.additional_params);
    chat_message_list_destroy(head);
}

void test_build_openai_null_args(void) {
    ChatRequestParams params = chat_request_params_default();
    ChatMessage* head = chat_message_create(CHAT_ROLE_USER, "x", NULL);
    TEST_ASSERT_NULL(chat_request_build_openai(NULL, head, &params));
    TEST_ASSERT_NULL(chat_request_build_openai(g_engine, NULL, &params));
    chat_message_list_destroy(head);
}

/* ---- OpenAI->Anthropic content converter ---- */
void test_convert_content_plain_text(void) {
    // NULL input
    TEST_ASSERT_NULL(chat_request_convert_openai_content_to_anthropic(NULL));
    // Non-JSON-array input returns NULL (treated as plain text)
    TEST_ASSERT_NULL(chat_request_convert_openai_content_to_anthropic("just plain text"));
    // A JSON object (not array) is also not an array
    TEST_ASSERT_NULL(chat_request_convert_openai_content_to_anthropic("{\"a\":1}"));
}

void test_convert_content_text_part(void) {
    const char* content = "[{\"type\":\"text\",\"text\":\"hi\"}]";
    json_t* arr = chat_request_convert_openai_content_to_anthropic(content);
    TEST_ASSERT_NOT_NULL(arr);
    TEST_ASSERT_TRUE(json_is_array(arr));
    TEST_ASSERT_EQUAL_INT(1, json_array_size(arr));
    json_t* item = json_array_get(arr, 0);
    TEST_ASSERT_EQUAL_STRING("text", json_string_value(json_object_get(item, "type")));
    json_decref(arr);
}

void test_convert_content_image_data_url(void) {
    const char* content =
        "[{\"type\":\"image_url\",\"image_url\":"
        "{\"url\":\"data:image/png;base64,QUJD\"}}]";
    json_t* arr = chat_request_convert_openai_content_to_anthropic(content);
    TEST_ASSERT_NOT_NULL(arr);
    json_t* item = json_array_get(arr, 0);
    TEST_ASSERT_EQUAL_STRING("image", json_string_value(json_object_get(item, "type")));
    json_t* source = json_object_get(item, "source");
    TEST_ASSERT_NOT_NULL(source);
    TEST_ASSERT_EQUAL_STRING("base64", json_string_value(json_object_get(source, "type")));
    TEST_ASSERT_EQUAL_STRING("image/png",
        json_string_value(json_object_get(source, "media_type")));
    TEST_ASSERT_EQUAL_STRING("QUJD", json_string_value(json_object_get(source, "data")));
    json_decref(arr);
}

void test_convert_content_image_no_comma(void) {
    // data URL without a comma -> conversion fails, original kept via deep copy
    const char* content =
        "[{\"type\":\"image_url\",\"image_url\":{\"url\":\"data:image/png\"}}]";
    json_t* arr = chat_request_convert_openai_content_to_anthropic(content);
    TEST_ASSERT_NOT_NULL(arr);
    json_t* item = json_array_get(arr, 0);
    // Kept as original image_url object
    TEST_ASSERT_EQUAL_STRING("image_url", json_string_value(json_object_get(item, "type")));
    json_decref(arr);

    // non-data URL image_url also kept as original
    const char* http_content =
        "[{\"type\":\"image_url\",\"image_url\":{\"url\":\"http://x/y.png\"}}]";
    json_t* arr2 = chat_request_convert_openai_content_to_anthropic(http_content);
    TEST_ASSERT_NOT_NULL(arr2);
    TEST_ASSERT_EQUAL_STRING("image_url",
        json_string_value(json_object_get(json_array_get(arr2, 0), "type")));
    json_decref(arr2);
}

void test_convert_content_non_object_and_no_type(void) {
    // Array with a non-object element and an object without a "type" field
    const char* content = "[\"raw string\",{\"foo\":\"bar\"}]";
    json_t* arr = chat_request_convert_openai_content_to_anthropic(content);
    TEST_ASSERT_NOT_NULL(arr);
    TEST_ASSERT_EQUAL_INT(2, json_array_size(arr));
    json_decref(arr);
}

void test_convert_content_unknown_type(void) {
    const char* content = "[{\"type\":\"video\",\"data\":\"x\"}]";
    json_t* arr = chat_request_convert_openai_content_to_anthropic(content);
    TEST_ASSERT_NOT_NULL(arr);
    TEST_ASSERT_EQUAL_STRING("video",
        json_string_value(json_object_get(json_array_get(arr, 0), "type")));
    json_decref(arr);
}

/* ---- Anthropic builder ---- */
void test_build_anthropic_with_system_and_stream(void) {
    ChatEngineConfig* eng = make_engine(CEC_PROVIDER_ANTHROPIC, 0, 4, MODALITY_ALL, false);
    ChatMessage* sys = chat_message_create(CHAT_ROLE_SYSTEM, "be nice", NULL);
    ChatMessage* usr = chat_message_create(CHAT_ROLE_USER, "hello", NULL);
    sys = chat_message_list_append(sys, usr);

    ChatRequestParams params = chat_request_params_default();
    params.stream = true;

    json_t* root = chat_request_build_anthropic(eng, sys, &params);
    TEST_ASSERT_NOT_NULL(root);
    // Default max_tokens 4096 applied when engine/params have none
    TEST_ASSERT_EQUAL_INT(4096, json_integer_value(json_object_get(root, "max_tokens")));
    // system extracted to its own field
    TEST_ASSERT_EQUAL_STRING("be nice", json_string_value(json_object_get(root, "system")));
    // messages array contains only the user message
    TEST_ASSERT_EQUAL_INT(1, json_array_size(json_object_get(root, "messages")));
    TEST_ASSERT_TRUE(json_is_true(json_object_get(root, "stream")));

    json_decref(root);
    chat_message_list_destroy(sys);
    chat_engine_config_destroy(eng);
}

void test_build_anthropic_multimodal(void) {
    ChatEngineConfig* eng = make_engine(CEC_PROVIDER_ANTHROPIC, 500, 4, MODALITY_ALL, false);
    const char* content =
        "[{\"type\":\"image_url\",\"image_url\":"
        "{\"url\":\"data:image/jpeg;base64,ZZ\"}}]";
    ChatMessage* usr = chat_message_create(CHAT_ROLE_USER, content, NULL);

    ChatRequestParams params = chat_request_params_default();
    json_t* root = chat_request_build_anthropic(eng, usr, &params);
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_EQUAL_INT(500, json_integer_value(json_object_get(root, "max_tokens")));
    json_t* msgs = json_object_get(root, "messages");
    json_t* msg0 = json_array_get(msgs, 0);
    // content became an array (multimodal converted)
    TEST_ASSERT_TRUE(json_is_array(json_object_get(msg0, "content")));

    json_decref(root);
    chat_message_list_destroy(usr);
    chat_engine_config_destroy(eng);
}

/* ---- Ollama builder ---- */
void test_build_ollama_native(void) {
    ChatEngineConfig* eng = make_engine(CEC_PROVIDER_OLLAMA, 800, 4, MODALITY_ALL, true);
    ChatMessage* usr = chat_message_create(CHAT_ROLE_USER, "hey", NULL);
    ChatRequestParams params = chat_request_params_default();

    json_t* root = chat_request_build_ollama(eng, usr, &params);
    TEST_ASSERT_NOT_NULL(root);
    json_t* options = json_object_get(root, "options");
    TEST_ASSERT_NOT_NULL(options);
    TEST_ASSERT_EQUAL_INT(800, json_integer_value(json_object_get(options, "num_predict")));
    TEST_ASSERT_EQUAL_DOUBLE(1.0, json_real_value(json_object_get(options, "temperature")));

    json_decref(root);
    chat_message_list_destroy(usr);
    chat_engine_config_destroy(eng);
}

void test_build_ollama_stream(void) {
    ChatEngineConfig* eng = make_engine(CEC_PROVIDER_OLLAMA, 0, 4, MODALITY_ALL, true);
    ChatMessage* usr = chat_message_create(CHAT_ROLE_USER, "hey", NULL);
    ChatRequestParams params = chat_request_params_default();
    params.stream = true;

    json_t* root = chat_request_build_ollama(eng, usr, &params);
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_TRUE(json_is_true(json_object_get(root, "stream")));
    // No num_predict when neither params nor engine set a positive limit
    json_t* options = json_object_get(root, "options");
    TEST_ASSERT_NULL(json_object_get(options, "num_predict"));

    TEST_ASSERT_NULL(chat_request_build_ollama(NULL, usr, &params));

    json_decref(root);
    chat_message_list_destroy(usr);
    chat_engine_config_destroy(eng);
}

/* ---- generic dispatch ---- */
void test_build_dispatch(void) {
    ChatMessage* usr = chat_message_create(CHAT_ROLE_USER, "hi", NULL);
    ChatRequestParams params = chat_request_params_default();

    TEST_ASSERT_NULL(chat_request_build(NULL, usr, &params));

    // OpenAI provider
    json_t* r_openai = chat_request_build(g_engine, usr, &params);
    TEST_ASSERT_NOT_NULL(r_openai);
    json_decref(r_openai);

    // Anthropic -> builds an Anthropic-shaped request (has max_tokens set)
    ChatEngineConfig* ant = make_engine(CEC_PROVIDER_ANTHROPIC, 100, 4, MODALITY_ALL, false);
    json_t* r_ant = chat_request_build(ant, usr, &params);
    TEST_ASSERT_NOT_NULL(r_ant);
    TEST_ASSERT_NOT_NULL(json_object_get(r_ant, "max_tokens"));
    json_decref(r_ant);
    chat_engine_config_destroy(ant);

    // Ollama native -> has options
    ChatEngineConfig* olln = make_engine(CEC_PROVIDER_OLLAMA, 100, 4, MODALITY_ALL, true);
    json_t* r_olln = chat_request_build(olln, usr, &params);
    TEST_ASSERT_NOT_NULL(json_object_get(r_olln, "options"));
    json_decref(r_olln);
    chat_engine_config_destroy(olln);

    // Ollama non-native -> falls back to OpenAI (no options key)
    ChatEngineConfig* ollo = make_engine(CEC_PROVIDER_OLLAMA, 100, 4, MODALITY_ALL, false);
    json_t* r_ollo = chat_request_build(ollo, usr, &params);
    TEST_ASSERT_NULL(json_object_get(r_ollo, "options"));
    json_decref(r_ollo);
    chat_engine_config_destroy(ollo);

    // Unknown provider -> OpenAI
    ChatEngineConfig* unk = make_engine(CEC_PROVIDER_UNKNOWN, 100, 4, MODALITY_ALL, false);
    json_t* r_unk = chat_request_build(unk, usr, &params);
    TEST_ASSERT_NOT_NULL(r_unk);
    json_decref(r_unk);
    chat_engine_config_destroy(unk);

    chat_message_list_destroy(usr);
}

/* ---- json string ---- */
void test_to_json_string(void) {
    TEST_ASSERT_NULL(chat_request_to_json_string(NULL, true));
    json_t* obj = json_object();
    json_object_set_new(obj, "k", json_string("v"));
    char* compact = chat_request_to_json_string(obj, true);
    TEST_ASSERT_NOT_NULL(compact);
    TEST_ASSERT_NOT_NULL(strstr(compact, "\"k\""));
    free(compact);
    char* pretty = chat_request_to_json_string(obj, false);
    TEST_ASSERT_NOT_NULL(pretty);
    free(pretty);
    json_decref(obj);
}

/* ---- image counting ---- */
void test_count_images(void) {
    TEST_ASSERT_EQUAL_INT(0, chat_request_message_count_images(NULL));
    TEST_ASSERT_EQUAL_INT(0, chat_request_message_count_images("plain text"));
    const char* content =
        "[{\"type\":\"text\",\"text\":\"a\"},"
        "{\"type\":\"image_url\",\"image_url\":{\"url\":\"x\"}},"
        "{\"type\":\"image\"},"
        "\"raw\","
        "{\"notype\":1}]";
    TEST_ASSERT_EQUAL_INT(2, chat_request_message_count_images(content));
}

void test_count_all_images(void) {
    const char* img = "[{\"type\":\"image\"}]";
    ChatMessage* m1 = chat_message_create(CHAT_ROLE_USER, img, NULL);
    ChatMessage* m2 = chat_message_create(CHAT_ROLE_USER, img, NULL);
    m1 = chat_message_list_append(m1, m2);
    TEST_ASSERT_EQUAL_INT(2, chat_request_count_all_images(m1));
    TEST_ASSERT_EQUAL_INT(0, chat_request_count_all_images(NULL));
    chat_message_list_destroy(m1);
}

/* ---- validation ---- */
void test_validate_success(void) {
    ChatMessage* usr = chat_message_create(CHAT_ROLE_USER, "hi", NULL);
    ChatRequestParams params = chat_request_params_default();
    char* err = NULL;
    TEST_ASSERT_TRUE(chat_request_validate(g_engine, usr, &params, &err));
    TEST_ASSERT_NULL(err);
    chat_message_list_destroy(usr);
}

void test_validate_null_engine(void) {
    ChatMessage* usr = chat_message_create(CHAT_ROLE_USER, "hi", NULL);
    ChatRequestParams params = chat_request_params_default();
    char* err = NULL;
    TEST_ASSERT_FALSE(chat_request_validate(NULL, usr, &params, &err));
    TEST_ASSERT_EQUAL_STRING("No engine configuration", err);
    free(err);
    // NULL error_message pointer path is also safe
    TEST_ASSERT_FALSE(chat_request_validate(NULL, usr, &params, NULL));
    chat_message_list_destroy(usr);
}

void test_validate_null_messages(void) {
    ChatRequestParams params = chat_request_params_default();
    char* err = NULL;
    TEST_ASSERT_FALSE(chat_request_validate(g_engine, NULL, &params, &err));
    TEST_ASSERT_EQUAL_STRING("No messages provided", err);
    free(err);
}

void test_validate_token_limit(void) {
    ChatEngineConfig* small = make_engine(CEC_PROVIDER_OPENAI, 5, 4, MODALITY_ALL, false);
    ChatMessage* usr = chat_message_create(CHAT_ROLE_USER,
        "a very long message that will exceed the tiny token budget here", NULL);
    ChatRequestParams params = chat_request_params_default();
    params.max_tokens = 100;  // pushes estimate over the limit
    char* err = NULL;
    TEST_ASSERT_FALSE(chat_request_validate(small, usr, &params, &err));
    TEST_ASSERT_NOT_NULL(strstr(err, "exceeds engine limit"));
    free(err);
    chat_message_list_destroy(usr);
    chat_engine_config_destroy(small);
}

void test_validate_image_count_limit(void) {
    ChatEngineConfig* eng = make_engine(CEC_PROVIDER_OPENAI, 10000, 1, MODALITY_ALL, false);
    const char* content =
        "[{\"type\":\"image\"},{\"type\":\"image\"}]";  // 2 images, limit 1
    ChatMessage* usr = chat_message_create(CHAT_ROLE_USER, content, NULL);
    ChatRequestParams params = chat_request_params_default();
    char* err = NULL;
    TEST_ASSERT_FALSE(chat_request_validate(eng, usr, &params, &err));
    TEST_ASSERT_NOT_NULL(strstr(err, "exceeds limit"));
    free(err);
    chat_message_list_destroy(usr);
    chat_engine_config_destroy(eng);
}

void test_validate_modality_unsupported(void) {
    // Engine allows the image count but does NOT support the image modality
    ChatEngineConfig* eng = make_engine(CEC_PROVIDER_OPENAI, 10000, 5, MODALITY_TEXT, false);
    const char* content = "[{\"type\":\"image\"}]";
    ChatMessage* usr = chat_message_create(CHAT_ROLE_USER, content, NULL);
    ChatRequestParams params = chat_request_params_default();
    char* err = NULL;
    TEST_ASSERT_FALSE(chat_request_validate(eng, usr, &params, &err));
    TEST_ASSERT_EQUAL_STRING("Engine does not support image modality", err);
    free(err);
    chat_message_list_destroy(usr);
    chat_engine_config_destroy(eng);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_list_append_null_and_chain);
    RUN_TEST(test_role_to_string);
    RUN_TEST(test_role_from_string);
    RUN_TEST(test_estimate_tokens);
    RUN_TEST(test_build_openai_with_params);
    RUN_TEST(test_build_openai_additional_params);
    RUN_TEST(test_build_openai_null_args);
    RUN_TEST(test_convert_content_plain_text);
    RUN_TEST(test_convert_content_text_part);
    RUN_TEST(test_convert_content_image_data_url);
    RUN_TEST(test_convert_content_image_no_comma);
    RUN_TEST(test_convert_content_non_object_and_no_type);
    RUN_TEST(test_convert_content_unknown_type);
    RUN_TEST(test_build_anthropic_with_system_and_stream);
    RUN_TEST(test_build_anthropic_multimodal);
    RUN_TEST(test_build_ollama_native);
    RUN_TEST(test_build_ollama_stream);
    RUN_TEST(test_build_dispatch);
    RUN_TEST(test_to_json_string);
    RUN_TEST(test_count_images);
    RUN_TEST(test_count_all_images);
    RUN_TEST(test_validate_success);
    RUN_TEST(test_validate_null_engine);
    RUN_TEST(test_validate_null_messages);
    RUN_TEST(test_validate_token_limit);
    RUN_TEST(test_validate_image_count_limit);
    RUN_TEST(test_validate_modality_unsupported);
    return UNITY_END();
}
