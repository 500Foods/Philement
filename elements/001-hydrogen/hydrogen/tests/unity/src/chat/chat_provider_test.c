/*
 * Unity Tests for Chat Provider Support (Phase 5)
 *
 * Tests provider-specific request building and response parsing for
 * Anthropic native format and Ollama native API support.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/conduit/chat_common/chat_engine_cache.h>
#include <src/api/conduit/chat_common/chat_request_builder.h>
#include <src/api/conduit/chat_common/chat_response_parser.h>

// Test function prototypes
void setUp(void);
void tearDown(void);
void test_chat_request_build_openai_basic(void);
void test_chat_request_build_anthropic_native_format(void);
void test_chat_request_build_anthropic_extracts_system(void);
void test_chat_request_build_ollama_native(void);
void test_chat_request_build_ollama_openai_compatible(void);
void test_chat_response_parse_openai(void);
void test_chat_response_parse_anthropic(void);
void test_chat_response_parse_ollama(void);
void test_chat_request_validate_image_count(void);
void test_chat_engine_config_use_native_api(void);

// Required by Unity framework
void setUp(void) { }
void tearDown(void) { }

// Helper to create a test engine
static ChatEngineConfig* create_test_engine(ChatEngineProvider provider, bool use_native) {
    return chat_engine_config_create(
        1, "test-engine", provider, "test-model",
        "https://api.test.com/v1/chat", "sk-test123",
        4096, 0.7, true, 300, 10, 10, 100, use_native);
}

// Test OpenAI request building
void test_chat_request_build_openai_basic(void) {
    ChatEngineConfig* engine = create_test_engine(CEC_PROVIDER_OPENAI, false);
    TEST_ASSERT_NOT_NULL(engine);

    ChatMessage* messages = chat_message_create(CHAT_ROLE_USER, "Hello, world!", NULL);
    ChatRequestParams params = chat_request_params_default();

    json_t* request = chat_request_build_openai(engine, messages, &params);
    TEST_ASSERT_NOT_NULL(request);

    // Check model
    json_t* model = json_object_get(request, "model");
    TEST_ASSERT_NOT_NULL(model);
    TEST_ASSERT_EQUAL_STRING("test-model", json_string_value(model));

    // Check messages array
    json_t* messages_arr = json_object_get(request, "messages");
    TEST_ASSERT_NOT_NULL(messages_arr);
    TEST_ASSERT_EQUAL(1, json_array_size(messages_arr));

    json_t* first_msg = json_array_get(messages_arr, 0);
    json_t* role = json_object_get(first_msg, "role");
    json_t* content = json_object_get(first_msg, "content");
    TEST_ASSERT_EQUAL_STRING("user", json_string_value(role));
    TEST_ASSERT_EQUAL_STRING("Hello, world!", json_string_value(content));

    json_decref(request);
    chat_message_list_destroy(messages);
    chat_engine_config_destroy(engine);
}

// Test Anthropic native format building
void test_chat_request_build_anthropic_native_format(void) {
    ChatEngineConfig* engine = create_test_engine(CEC_PROVIDER_ANTHROPIC, false);
    TEST_ASSERT_NOT_NULL(engine);

    ChatMessage* msg1 = chat_message_create(CHAT_ROLE_USER, "Hello", NULL);
    ChatMessage* msg2 = chat_message_create(CHAT_ROLE_ASSISTANT, "Hi there!", NULL);
    ChatMessage* messages = msg1;
    msg1->next = msg2;

    ChatRequestParams params = chat_request_params_default();
    params.max_tokens = 1000;

    json_t* request = chat_request_build_anthropic(engine, messages, &params);
    TEST_ASSERT_NOT_NULL(request);

    // Check model
    json_t* model = json_object_get(request, "model");
    TEST_ASSERT_EQUAL_STRING("test-model", json_string_value(model));

    // Check max_tokens (required for Anthropic)
    json_t* max_tokens = json_object_get(request, "max_tokens");
    TEST_ASSERT_NOT_NULL(max_tokens);
    TEST_ASSERT_EQUAL(1000, json_integer_value(max_tokens));

    // Check messages array
    json_t* messages_arr = json_object_get(request, "messages");
    TEST_ASSERT_NOT_NULL(messages_arr);
    TEST_ASSERT_EQUAL(2, json_array_size(messages_arr));

    json_decref(request);
    chat_message_list_destroy(messages);
    chat_engine_config_destroy(engine);
}

// Test Anthropic extracts system message to separate field
void test_chat_request_build_anthropic_extracts_system(void) {
    ChatEngineConfig* engine = create_test_engine(CEC_PROVIDER_ANTHROPIC, false);
    TEST_ASSERT_NOT_NULL(engine);

    ChatMessage* system_msg = chat_message_create(CHAT_ROLE_SYSTEM, "You are a helpful assistant.", NULL);
    ChatMessage* user_msg = chat_message_create(CHAT_ROLE_USER, "Hello", NULL);
    system_msg->next = user_msg;

    ChatRequestParams params = chat_request_params_default();

    json_t* request = chat_request_build_anthropic(engine, system_msg, &params);
    TEST_ASSERT_NOT_NULL(request);

    // Check system field is present
    json_t* system = json_object_get(request, "system");
    TEST_ASSERT_NOT_NULL(system);
    TEST_ASSERT_EQUAL_STRING("You are a helpful assistant.", json_string_value(system));

    // Check messages array does NOT contain system message
    json_t* messages_arr = json_object_get(request, "messages");
    TEST_ASSERT_NOT_NULL(messages_arr);
    TEST_ASSERT_EQUAL(1, json_array_size(messages_arr));

    json_t* first_msg = json_array_get(messages_arr, 0);
    json_t* role = json_object_get(first_msg, "role");
    TEST_ASSERT_EQUAL_STRING("user", json_string_value(role));

    json_decref(request);
    chat_message_list_destroy(system_msg);
    chat_engine_config_destroy(engine);
}

// Test Ollama native format
void test_chat_request_build_ollama_native(void) {
    ChatEngineConfig* engine = create_test_engine(CEC_PROVIDER_OLLAMA, true);
    TEST_ASSERT_NOT_NULL(engine);
    TEST_ASSERT_TRUE(engine->use_native_api);

    ChatMessage* messages = chat_message_create(CHAT_ROLE_USER, "Hello, Ollama!", NULL);
    ChatRequestParams params = chat_request_params_default();
    params.max_tokens = 500;
    params.temperature = 0.8;

    json_t* request = chat_request_build_ollama(engine, messages, &params);
    TEST_ASSERT_NOT_NULL(request);

    // Check model
    json_t* model = json_object_get(request, "model");
    TEST_ASSERT_EQUAL_STRING("test-model", json_string_value(model));

    // Check options object
    json_t* options = json_object_get(request, "options");
    TEST_ASSERT_NOT_NULL(options);

    // Check num_predict (Ollama's max_tokens equivalent)
    json_t* num_predict = json_object_get(options, "num_predict");
    TEST_ASSERT_NOT_NULL(num_predict);
    TEST_ASSERT_EQUAL(500, json_integer_value(num_predict));

    // Check temperature in options
    json_t* temperature = json_object_get(options, "temperature");
    TEST_ASSERT_NOT_NULL(temperature);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.8, json_real_value(temperature));

    json_decref(request);
    chat_message_list_destroy(messages);
    chat_engine_config_destroy(engine);
}

// Test Ollama uses OpenAI format when use_native_api is false
void test_chat_request_build_ollama_openai_compatible(void) {
    ChatEngineConfig* engine = create_test_engine(CEC_PROVIDER_OLLAMA, false);
    TEST_ASSERT_NOT_NULL(engine);
    TEST_ASSERT_FALSE(engine->use_native_api);

    ChatMessage* messages = chat_message_create(CHAT_ROLE_USER, "Hello", NULL);
    ChatRequestParams params = chat_request_params_default();

    // Use the generic builder which should choose OpenAI format
    json_t* request = chat_request_build(engine, messages, &params);
    TEST_ASSERT_NOT_NULL(request);

    // Should use OpenAI format (no options object, max_tokens instead of num_predict)
    json_t* options = json_object_get(request, "options");
    TEST_ASSERT_NULL(options);  // OpenAI format doesn't have options

    // Should have messages array
    json_t* messages_arr = json_object_get(request, "messages");
    TEST_ASSERT_NOT_NULL(messages_arr);

    json_decref(request);
    chat_message_list_destroy(messages);
    chat_engine_config_destroy(engine);
}

// Test OpenAI response parsing
void test_chat_response_parse_openai(void) {
    const char* json_response = "{\n"
        "  \"model\": \"gpt-4\",\n"
        "  \"choices\": [{\n"
        "    \"message\": {\n"
        "      \"role\": \"assistant\",\n"
        "      \"content\": \"Hello! How can I help you today?\"\n"
        "    },\n"
        "    \"finish_reason\": \"stop\"\n"
        "  }],\n"
        "  \"usage\": {\n"
        "    \"prompt_tokens\": 10,\n"
        "    \"completion_tokens\": 20,\n"
        "    \"total_tokens\": 30\n"
        "  }\n"
        "}";

    ChatParsedResponse* response = chat_response_parse_openai(json_response);
    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_TRUE(response->success);
    TEST_ASSERT_EQUAL_STRING("Hello! How can I help you today?", response->content);
    TEST_ASSERT_EQUAL_STRING("gpt-4", response->model);
    TEST_ASSERT_EQUAL_STRING("stop", response->finish_reason);
    TEST_ASSERT_EQUAL(10, response->prompt_tokens);
    TEST_ASSERT_EQUAL(20, response->completion_tokens);
    TEST_ASSERT_EQUAL(30, response->total_tokens);

    chat_parsed_response_destroy(response);
}

// Test Anthropic response parsing
void test_chat_response_parse_anthropic(void) {
    const char* json_response = "{\n"
        "  \"model\": \"claude-3-opus\",\n"
        "  \"content\": [{\n"
        "    \"type\": \"text\",\n"
        "    \"text\": \"Hello! I'm Claude.\"\n"
        "  }],\n"
        "  \"stop_reason\": \"end_turn\",\n"
        "  \"usage\": {\n"
        "    \"input_tokens\": 15,\n"
        "    \"output_tokens\": 25\n"
        "  }\n"
        "}";

    ChatParsedResponse* response = chat_response_parse_anthropic(json_response);
    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_TRUE(response->success);
    TEST_ASSERT_EQUAL_STRING("Hello! I'm Claude.", response->content);
    TEST_ASSERT_EQUAL_STRING("claude-3-opus", response->model);
    TEST_ASSERT_EQUAL_STRING("end_turn", response->finish_reason);
    TEST_ASSERT_EQUAL(15, response->prompt_tokens);
    TEST_ASSERT_EQUAL(25, response->completion_tokens);

    chat_parsed_response_destroy(response);
}

// Test Ollama response parsing
void test_chat_response_parse_ollama(void) {
    const char* json_response = "{\n"
        "  \"model\": \"llama2\",\n"
        "  \"message\": {\n"
        "    \"role\": \"assistant\",\n"
        "    \"content\": \"Hello from Ollama!\"\n"
        "  }\n"
        "}";

    ChatParsedResponse* response = chat_response_parse_ollama(json_response);
    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_TRUE(response->success);
    TEST_ASSERT_EQUAL_STRING("Hello from Ollama!", response->content);
    TEST_ASSERT_EQUAL_STRING("llama2", response->model);

    chat_parsed_response_destroy(response);
}

// Test image count validation
void test_chat_request_validate_image_count(void) {
    // Create engine with max 2 images
    ChatEngineConfig* engine = chat_engine_config_create(
        1, "test-engine", CEC_PROVIDER_OPENAI, "test-model",
        "https://api.test.com/v1/chat", "sk-test123",
        4096, 0.7, true, 300, 2, 10, 100, false);  // max_images = 2

    // Message with 3 images (should fail validation)
    const char* multimodal_content = "[\n"
        "  {\"type\": \"text\", \"text\": \"What do you see?\"},\n"
        "  {\"type\": \"image_url\", \"image_url\": {\"url\": \"http://example.com/1.jpg\"}},\n"
        "  {\"type\": \"image_url\", \"image_url\": {\"url\": \"http://example.com/2.jpg\"}},\n"
        "  {\"type\": \"image_url\", \"image_url\": {\"url\": \"http://example.com/3.jpg\"}}\n"
        "]";

    ChatMessage* messages = chat_message_create(CHAT_ROLE_USER, multimodal_content, NULL);
    ChatRequestParams params = chat_request_params_default();

    char* error = NULL;
    bool valid = chat_request_validate(engine, messages, &params, &error);
    TEST_ASSERT_FALSE(valid);
    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_NOT_NULL(strstr(error, "exceeds limit"));

    free(error);
    chat_message_list_destroy(messages);

    // Message with 1 image (should pass)
    const char* single_image = "[{\"type\": \"text\", \"text\": \"Look\"}, {\"type\": \"image_url\", \"image_url\": {\"url\": \"http://example.com/1.jpg\"}}]";
    messages = chat_message_create(CHAT_ROLE_USER, single_image, NULL);

    valid = chat_request_validate(engine, messages, &params, &error);
    TEST_ASSERT_TRUE(valid);

    chat_message_list_destroy(messages);
    chat_engine_config_destroy(engine);
}

// Test use_native_api flag
void test_chat_engine_config_use_native_api(void) {
    ChatEngineConfig* engine_native = create_test_engine(CEC_PROVIDER_OLLAMA, true);
    TEST_ASSERT_NOT_NULL(engine_native);
    TEST_ASSERT_TRUE(engine_native->use_native_api);
    chat_engine_config_destroy(engine_native);

    ChatEngineConfig* engine_compat = create_test_engine(CEC_PROVIDER_OLLAMA, false);
    TEST_ASSERT_NOT_NULL(engine_compat);
    TEST_ASSERT_FALSE(engine_compat->use_native_api);
    chat_engine_config_destroy(engine_compat);
}

// Main function
int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_chat_request_build_openai_basic);
    RUN_TEST(test_chat_request_build_anthropic_native_format);
    RUN_TEST(test_chat_request_build_anthropic_extracts_system);
    RUN_TEST(test_chat_request_build_ollama_native);
    RUN_TEST(test_chat_request_build_ollama_openai_compatible);
    RUN_TEST(test_chat_response_parse_openai);
    RUN_TEST(test_chat_response_parse_anthropic);
    RUN_TEST(test_chat_response_parse_ollama);
    RUN_TEST(test_chat_request_validate_image_count);
    RUN_TEST(test_chat_engine_config_use_native_api);

    return UNITY_END();
}
