/*
 * Unity Tests for Chat Request Builder — Phase 8b: Hosted MCP connector
 *
 * Verifies that when `hosted_mcp_enabled` is set, the Responses API
 * builder injects a `type: mcp` connector carrying:
 *   - server_url = mcp_auth_resource(cfg)
 *   - authorization = "Bearer <minted JWT>"
 *   - allowed_tools = ["System.Info"]
 *
 * Also verifies fail-closed behavior: NULL cfg, unreachable resource
 * URL, or mint failure all cause the builder to return NULL.
 */
#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/wschat/helpers/engine_cache.h>
#include <src/api/wschat/helpers/req_builder.h>
#include <src/config/config_mcp.h>
#include <src/mcp/mcp_auth.h>
#include <openssl/bio.h>
#include <openssl/evp.h>

static AppConfig test_app;

void setUp(void) {
    memset(&test_app, 0, sizeof(test_app));
    mcp_config_apply_defaults(&test_app.mcp);
    test_app.mcp.Enabled = true;
    test_app.mcp.Resource = strdup("https://hydrogen.example.com/mcp");
    app_config = &test_app;
}

void tearDown(void) {
    free(test_app.mcp.Resource);
    test_app.mcp.Resource = NULL;
    app_config = NULL;
}

void test_hosted_mcp_injects_type_mcp(void);
void test_hosted_mcp_authorization_is_bearer_jwt(void);
void test_hosted_mcp_allowed_tools_only_system_info(void);
void test_hosted_mcp_server_url_matches_resource(void);
void test_hosted_mcp_skipped_when_disabled(void);
void test_hosted_mcp_fails_closed_on_null_cfg(void);
void test_hosted_mcp_fails_closed_on_unreachable_url(void);
void test_hosted_mcp_fails_closed_on_loopback(void);
void test_hosted_mcp_mint_jwt_has_correct_aud(void);
void test_hosted_mcp_server_label_hydrogen(void);
void test_apply_hosted_mcp_only_on_responses_api(void);
void test_correlation_id_is_uuid_v4_shape(void);

static ChatEngineConfig *create_test_engine(void) {
    return chat_engine_config_create(
        1, "grok", CEC_PROVIDER_OPENAI, "grok-4.6",
        "https://api.x.ai/v1/responses", "sk-test",
        4096, 0.7, true, 300, 10, 10, 100, MODALITY_DEFAULT, false);
}

void test_hosted_mcp_injects_type_mcp(void) {
    ChatEngineConfig *engine = create_test_engine();
    ChatMessage *messages = chat_message_create(CHAT_ROLE_USER, "Hello", NULL);
    ChatRequestParams params = chat_request_params_default();
    params.hosted_mcp_enabled = true;
    params.hosted_mcp_sub = "user-1";
    params.hosted_mcp_database = "acuranzo";
    params.hosted_mcp_correlation_id = "cid-inject";

    json_t *request = chat_request_build_responses(engine, messages, &params);
    TEST_ASSERT_NOT_NULL(request);

    json_t *tools = json_object_get(request, "tools");
    TEST_ASSERT_NOT_NULL(tools);
    TEST_ASSERT_TRUE(json_is_array(tools));
    TEST_ASSERT_EQUAL_UINT(1, json_array_size(tools));

    json_t *mcp_tool = json_array_get(tools, 0);
    TEST_ASSERT_NOT_NULL(mcp_tool);
    json_t *type_obj = json_object_get(mcp_tool, "type");
    TEST_ASSERT_EQUAL_STRING("mcp", json_string_value(type_obj));
    TEST_ASSERT_EQUAL_STRING("hydrogen", json_string_value(json_object_get(mcp_tool, "server_label")));

    json_decref(request);
    chat_message_list_destroy(messages);
    chat_engine_config_destroy(engine);
}

void test_hosted_mcp_authorization_is_bearer_jwt(void) {
    ChatEngineConfig *engine = create_test_engine();
    ChatMessage *messages = chat_message_create(CHAT_ROLE_USER, "Hi", NULL);
    ChatRequestParams params = chat_request_params_default();
    params.hosted_mcp_enabled = true;
    params.hosted_mcp_sub = "user-1";
    params.hosted_mcp_database = "acuranzo";
    params.hosted_mcp_correlation_id = "cid-bearer";

    json_t *request = chat_request_build_responses(engine, messages, &params);
    TEST_ASSERT_NOT_NULL(request);

    json_t *mcp_tool = json_array_get(json_object_get(request, "tools"), 0);
    json_t *authz = json_object_get(mcp_tool, "authorization");
    TEST_ASSERT_NOT_NULL(authz);
    const char *authz_str = json_string_value(authz);
    TEST_ASSERT_NOT_NULL(authz_str);
    TEST_ASSERT_EQUAL_INT(0, strncmp(authz_str, "Bearer ", 7));
    TEST_ASSERT_TRUE(strlen(authz_str) > 7);

    json_decref(request);
    chat_message_list_destroy(messages);
    chat_engine_config_destroy(engine);
}

void test_hosted_mcp_allowed_tools_only_system_info(void) {
    ChatEngineConfig *engine = create_test_engine();
    ChatMessage *messages = chat_message_create(CHAT_ROLE_USER, "Hi", NULL);
    ChatRequestParams params = chat_request_params_default();
    params.hosted_mcp_enabled = true;
    params.hosted_mcp_sub = "u";
    params.hosted_mcp_database = "d";
    params.hosted_mcp_correlation_id = "cid-tools";

    json_t *request = chat_request_build_responses(engine, messages, &params);
    TEST_ASSERT_NOT_NULL(request);
    json_t *mcp_tool = json_array_get(json_object_get(request, "tools"), 0);
    json_t *allowed = json_object_get(mcp_tool, "allowed_tools");
    TEST_ASSERT_TRUE(json_is_array(allowed));
    TEST_ASSERT_EQUAL_UINT(1, json_array_size(allowed));
    TEST_ASSERT_EQUAL_STRING("System.Info", json_string_value(json_array_get(allowed, 0)));

    json_decref(request);
    chat_message_list_destroy(messages);
    chat_engine_config_destroy(engine);
}

void test_hosted_mcp_server_url_matches_resource(void) {
    ChatEngineConfig *engine = create_test_engine();
    ChatMessage *messages = chat_message_create(CHAT_ROLE_USER, "Hi", NULL);
    ChatRequestParams params = chat_request_params_default();
    params.hosted_mcp_enabled = true;
    params.hosted_mcp_sub = "u";
    params.hosted_mcp_database = "d";
    params.hosted_mcp_correlation_id = "cid-url";

    json_t *request = chat_request_build_responses(engine, messages, &params);
    TEST_ASSERT_NOT_NULL(request);
    json_t *mcp_tool = json_array_get(json_object_get(request, "tools"), 0);
    json_t *url = json_object_get(mcp_tool, "server_url");
    TEST_ASSERT_EQUAL_STRING("https://hydrogen.example.com/mcp", json_string_value(url));

    json_decref(request);
    chat_message_list_destroy(messages);
    chat_engine_config_destroy(engine);
}

void test_hosted_mcp_skipped_when_disabled(void) {
    ChatEngineConfig *engine = create_test_engine();
    ChatMessage *messages = chat_message_create(CHAT_ROLE_USER, "Hi", NULL);
    ChatRequestParams params = chat_request_params_default();
    params.hosted_mcp_enabled = false;

    json_t *request = chat_request_build_responses(engine, messages, &params);
    TEST_ASSERT_NOT_NULL(request);
    json_t *tools = json_object_get(request, "tools");
    TEST_ASSERT_NULL(tools);

    json_decref(request);
    chat_message_list_destroy(messages);
    chat_engine_config_destroy(engine);
}

void test_hosted_mcp_fails_closed_on_null_cfg(void) {
    app_config = NULL;
    ChatEngineConfig *engine = create_test_engine();
    ChatMessage *messages = chat_message_create(CHAT_ROLE_USER, "Hi", NULL);
    ChatRequestParams params = chat_request_params_default();
    params.hosted_mcp_enabled = true;
    params.hosted_mcp_sub = "u";
    params.hosted_mcp_database = "d";
    params.hosted_mcp_correlation_id = "cid-nocfg";

    json_t *request = chat_request_build_responses(engine, messages, &params);
    TEST_ASSERT_NULL(request);

    chat_message_list_destroy(messages);
    chat_engine_config_destroy(engine);
}

void test_hosted_mcp_fails_closed_on_unreachable_url(void) {
    free(test_app.mcp.Resource);
    test_app.mcp.Resource = strdup("https://localhost:3100/mcp");
    ChatEngineConfig *engine = create_test_engine();
    ChatMessage *messages = chat_message_create(CHAT_ROLE_USER, "Hi", NULL);
    ChatRequestParams params = chat_request_params_default();
    params.hosted_mcp_enabled = true;
    params.hosted_mcp_sub = "u";
    params.hosted_mcp_database = "d";
    params.hosted_mcp_correlation_id = "cid-loopback";

    json_t *request = chat_request_build_responses(engine, messages, &params);
    TEST_ASSERT_NULL(request);

    chat_message_list_destroy(messages);
    chat_engine_config_destroy(engine);
}

void test_hosted_mcp_fails_closed_on_loopback(void) {
    free(test_app.mcp.Resource);
    test_app.mcp.Resource = strdup("https://127.0.0.1:3100/mcp");
    ChatEngineConfig *engine = create_test_engine();
    ChatMessage *messages = chat_message_create(CHAT_ROLE_USER, "Hi", NULL);
    ChatRequestParams params = chat_request_params_default();
    params.hosted_mcp_enabled = true;
    params.hosted_mcp_sub = "u";
    params.hosted_mcp_database = "d";
    params.hosted_mcp_correlation_id = "cid-127";

    json_t *request = chat_request_build_responses(engine, messages, &params);
    TEST_ASSERT_NULL(request);

    chat_message_list_destroy(messages);
    chat_engine_config_destroy(engine);
}

void test_hosted_mcp_mint_jwt_has_correct_aud(void) {
    ChatEngineConfig *engine = create_test_engine();
    ChatMessage *messages = chat_message_create(CHAT_ROLE_USER, "Hi", NULL);
    ChatRequestParams params = chat_request_params_default();
    params.hosted_mcp_enabled = true;
    params.hosted_mcp_sub = "user-1";
    params.hosted_mcp_database = "acuranzo";
    params.hosted_mcp_roles = "chat";
    params.hosted_mcp_correlation_id = "cid-aud";

    json_t *request = chat_request_build_responses(engine, messages, &params);
    TEST_ASSERT_NOT_NULL(request);
    json_t *mcp_tool = json_array_get(json_object_get(request, "tools"), 0);
    const char *authz = json_string_value(json_object_get(mcp_tool, "authorization"));
    TEST_ASSERT_NOT_NULL(authz);
    TEST_ASSERT_EQUAL_INT(0, strncmp(authz, "Bearer ", 7));
    const char *jwt = authz + 7;
    const char *p1 = strchr(jwt, '.');
    TEST_ASSERT_NOT_NULL(p1);
    const char *p2 = p1 ? strchr(p1 + 1, '.') : NULL;
    TEST_ASSERT_NOT_NULL(p2);
    size_t b64_len = (size_t)(p2 - (p1 + 1));
    char *b64 = calloc(1, b64_len + 1);
    TEST_ASSERT_NOT_NULL(b64);
    memcpy(b64, p1 + 1, b64_len);

    char *padded = calloc(1, b64_len + 8);
    TEST_ASSERT_NOT_NULL(padded);
    size_t pad = (4 - (b64_len % 4)) % 4;
    memcpy(padded, b64, b64_len);
    for (size_t i = 0; i < pad; i++) padded[b64_len + i] = '=';
    padded[b64_len + pad] = '\0';
    free(b64);

    BIO *bmem = BIO_new_mem_buf(padded, -1);
    BIO *b64f = BIO_new(BIO_f_base64());
    BIO_set_flags(b64f, BIO_FLAGS_BASE64_NO_NL);
    BIO *chain = BIO_push(b64f, bmem);
    char decoded[4096];
    int n = BIO_read(chain, decoded, sizeof(decoded) - 1);
    BIO_free_all(chain);
    free(padded);
    TEST_ASSERT_GREATER_THAN(0, n);
    decoded[n] = '\0';

    json_error_t err;
    json_t *payload = json_loadb(decoded, (size_t)n, 0, &err);
    TEST_ASSERT_NOT_NULL(payload);
    TEST_ASSERT_EQUAL_STRING("hydrogen-auth", json_string_value(json_object_get(payload, "iss")));
    TEST_ASSERT_EQUAL_STRING("user-1", json_string_value(json_object_get(payload, "sub")));
    TEST_ASSERT_EQUAL_STRING("https://hydrogen.example.com/mcp", json_string_value(json_object_get(payload, "aud")));
    TEST_ASSERT_EQUAL_STRING("acuranzo", json_string_value(json_object_get(payload, "database")));
    TEST_ASSERT_EQUAL_STRING("chat", json_string_value(json_object_get(payload, "roles")));
    json_decref(payload);

    json_decref(request);
    chat_message_list_destroy(messages);
    chat_engine_config_destroy(engine);
}

void test_hosted_mcp_server_label_hydrogen(void) {
    ChatEngineConfig *engine = create_test_engine();
    ChatMessage *messages = chat_message_create(CHAT_ROLE_USER, "Hi", NULL);
    ChatRequestParams params = chat_request_params_default();
    params.hosted_mcp_enabled = true;
    params.hosted_mcp_sub = "u";
    params.hosted_mcp_database = "d";
    params.hosted_mcp_correlation_id = "cid-label";
    json_t *request = chat_request_build_responses(engine, messages, &params);
    TEST_ASSERT_NOT_NULL(request);
    json_t *mcp_tool = json_array_get(json_object_get(request, "tools"), 0);
    TEST_ASSERT_EQUAL_STRING("hydrogen", json_string_value(json_object_get(mcp_tool, "server_label")));
    json_decref(request);
    chat_message_list_destroy(messages);
    chat_engine_config_destroy(engine);
}

void test_apply_hosted_mcp_only_on_responses_api(void) {
    ChatEngineConfig *engine = create_test_engine();
    ChatRequestParams params = chat_request_params_default();
    chat_request_params_apply_hosted_mcp(&params, engine, "u", "d", "chat", "cid");
    TEST_ASSERT_FALSE(params.hosted_mcp_enabled);
    engine->use_responses_api = true;
    chat_request_params_apply_hosted_mcp(&params, engine, "u", "d", "chat", "cid");
    TEST_ASSERT_TRUE(params.hosted_mcp_enabled);
    TEST_ASSERT_EQUAL_STRING("u", params.hosted_mcp_sub);
    TEST_ASSERT_EQUAL_STRING("d", params.hosted_mcp_database);
    chat_engine_config_destroy(engine);
}

void test_correlation_id_is_uuid_v4_shape(void) {
    char cid[37];
    chat_correlation_id_generate(cid, sizeof(cid));
    TEST_ASSERT_EQUAL_INT(36, (int)strlen(cid));
    TEST_ASSERT_EQUAL_CHAR('-', cid[8]);
    TEST_ASSERT_EQUAL_CHAR('-', cid[13]);
    TEST_ASSERT_EQUAL_CHAR('4', cid[14]);
    TEST_ASSERT_EQUAL_CHAR('-', cid[18]);
    TEST_ASSERT_EQUAL_CHAR('-', cid[23]);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_hosted_mcp_injects_type_mcp);
    RUN_TEST(test_hosted_mcp_authorization_is_bearer_jwt);
    RUN_TEST(test_hosted_mcp_allowed_tools_only_system_info);
    RUN_TEST(test_hosted_mcp_server_url_matches_resource);
    RUN_TEST(test_hosted_mcp_skipped_when_disabled);
    RUN_TEST(test_hosted_mcp_fails_closed_on_null_cfg);
    RUN_TEST(test_hosted_mcp_fails_closed_on_unreachable_url);
    RUN_TEST(test_hosted_mcp_fails_closed_on_loopback);
    RUN_TEST(test_hosted_mcp_mint_jwt_has_correct_aud);
    RUN_TEST(test_hosted_mcp_server_label_hydrogen);
    RUN_TEST(test_apply_hosted_mcp_only_on_responses_api);
    RUN_TEST(test_correlation_id_is_uuid_v4_shape);
    return UNITY_END();
}
