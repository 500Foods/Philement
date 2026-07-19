/*
 * Unity Test File: auth_chats_parse_request
 * Tests multi-chat request JSON parsing and validation.
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include source header
#include <src/api/wschat/auth_chats/auth_chats.h>

void test_auth_chats_parse_request_rejects_invalid_parameters(void);
void test_auth_chats_parse_request_rejects_missing_engines(void);
void test_auth_chats_parse_request_rejects_empty_engines(void);
void test_auth_chats_parse_request_rejects_too_many_engines(void);
void test_auth_chats_parse_request_rejects_missing_messages(void);
void test_auth_chats_parse_request_applies_defaults(void);
void test_auth_chats_parse_request_extracts_optional_values(void);

void setUp(void) {
}

void tearDown(void) {
}

static bool parse(json_t *request, size_t *count, double *temperature,
                  int *max_tokens, char **error) {
    json_t *engines = NULL;
    json_t *messages = NULL;
    return auth_chats_parse_request(request, &engines, &messages, count,
                                    temperature, max_tokens, error);
}

void test_auth_chats_parse_request_rejects_invalid_parameters(void) {
    char *error = NULL;
    json_t *engines = NULL;
    json_t *messages = NULL;
    size_t count = 0;
    double temperature = 0.0;
    int max_tokens = 0;

    TEST_ASSERT_FALSE(auth_chats_parse_request(NULL, &engines, &messages, &count,
                                               &temperature, &max_tokens, &error));
    TEST_ASSERT_EQUAL_STRING("Invalid parse request parameters", error);
    free(error);
}

void test_auth_chats_parse_request_rejects_missing_engines(void) {
    json_t *request = json_pack("{s:[]}", "messages");
    size_t count = 0;
    double temperature = 0.0;
    int max_tokens = 0;
    char *error = NULL;

    TEST_ASSERT_FALSE(parse(request, &count, &temperature, &max_tokens, &error));
    TEST_ASSERT_EQUAL_STRING("Missing or invalid 'engines' array", error);
    free(error);
    json_decref(request);
}

void test_auth_chats_parse_request_rejects_empty_engines(void) {
    json_t *request = json_pack("{s:[],s:[]}", "engines", "messages");
    size_t count = 0;
    double temperature = 0.0;
    int max_tokens = 0;
    char *error = NULL;

    TEST_ASSERT_FALSE(parse(request, &count, &temperature, &max_tokens, &error));
    TEST_ASSERT_EQUAL_STRING("Engines array must contain 1-10 engine names", error);
    free(error);
    json_decref(request);
}

void test_auth_chats_parse_request_rejects_too_many_engines(void) {
    json_t *request = json_pack("{s:[],s:[]}", "engines", "messages");
    json_t *engines = json_object_get(request, "engines");
    for (int i = 0; i < 11; i++) json_array_append_new(engines, json_string("engine"));
    size_t count = 0;
    double temperature = 0.0;
    int max_tokens = 0;
    char *error = NULL;

    TEST_ASSERT_FALSE(parse(request, &count, &temperature, &max_tokens, &error));
    TEST_ASSERT_EQUAL_STRING("Engines array must contain 1-10 engine names", error);
    free(error);
    json_decref(request);
}

void test_auth_chats_parse_request_rejects_missing_messages(void) {
    json_t *request = json_pack("{s:[s]}", "engines", "engine");
    size_t count = 0;
    double temperature = 0.0;
    int max_tokens = 0;
    char *error = NULL;

    TEST_ASSERT_FALSE(parse(request, &count, &temperature, &max_tokens, &error));
    TEST_ASSERT_EQUAL_STRING("Missing or invalid 'messages' array", error);
    free(error);
    json_decref(request);
}

void test_auth_chats_parse_request_applies_defaults(void) {
    json_t *request = json_pack("{s:[s],s:[]}", "engines", "engine", "messages");
    size_t count = 0;
    double temperature = 9.0;
    int max_tokens = 9;
    char *error = NULL;

    TEST_ASSERT_TRUE(parse(request, &count, &temperature, &max_tokens, &error));
    TEST_ASSERT_EQUAL_UINT(1, count);
    TEST_ASSERT_EQUAL_DOUBLE(-1.0, temperature);
    TEST_ASSERT_EQUAL_INT(-1, max_tokens);
    TEST_ASSERT_NULL(error);
    json_decref(request);
}

void test_auth_chats_parse_request_extracts_optional_values(void) {
    json_t *request = json_pack("{s:[s,s],s:[],s:f,s:i}",
                                "engines", "one", "two", "messages",
                                "temperature", 0.4, "max_tokens", 321);
    size_t count = 0;
    double temperature = 0.0;
    int max_tokens = 0;
    char *error = NULL;

    TEST_ASSERT_TRUE(parse(request, &count, &temperature, &max_tokens, &error));
    TEST_ASSERT_EQUAL_UINT(2, count);
    TEST_ASSERT_EQUAL_DOUBLE(0.4, temperature);
    TEST_ASSERT_EQUAL_INT(321, max_tokens);
    json_decref(request);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_auth_chats_parse_request_rejects_invalid_parameters);
    RUN_TEST(test_auth_chats_parse_request_rejects_missing_engines);
    RUN_TEST(test_auth_chats_parse_request_rejects_empty_engines);
    RUN_TEST(test_auth_chats_parse_request_rejects_too_many_engines);
    RUN_TEST(test_auth_chats_parse_request_rejects_missing_messages);
    RUN_TEST(test_auth_chats_parse_request_applies_defaults);
    RUN_TEST(test_auth_chats_parse_request_extracts_optional_values);
    return UNITY_END();
}
