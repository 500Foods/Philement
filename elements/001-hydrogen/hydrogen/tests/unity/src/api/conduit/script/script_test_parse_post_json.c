/*
 * Unity Test File: conduit_script_parse_post_json
 * src/api/conduit/script/script.c
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <stdlib.h>
#include <string.h>

#include <src/api/conduit/script/script.h>
#include <src/config/config.h>
#include <src/scripting/scoreboard.h>

extern AppConfig *app_config;

void test_parse_missing_script(void);
void test_parse_empty_script(void);
void test_parse_valid_defaults(void);
void test_parse_wait_false_and_timeout_clamp(void);
void test_parse_invalid_params_type(void);
void test_parse_null_body(void);
void test_parse_rejects_client_hydrogen(void);
void test_parse_params_too_large(void);
void test_parse_timeout_uses_config_cap(void);
void test_extract_job_id(void);
void test_error_json_shape(void);
void test_map_invoke_error(void);
void test_map_timeout_wait_name(void);
void test_claims_to_hydrogen(void);
void test_job_response_omits_traceback(void);
void test_limit_helpers_defaults(void);

static AppConfig g_cfg;

void setUp(void) {
    memset(&g_cfg, 0, sizeof(g_cfg));
    g_cfg.scripting.ClientInvokeDefaultTimeout = CONDUIT_SCRIPT_TIMEOUT_DEFAULT_S;
    g_cfg.scripting.ClientInvokeMaxTimeout = CONDUIT_SCRIPT_TIMEOUT_MAX_S;
    g_cfg.scripting.ClientInvokeMaxParamsBytes = CONDUIT_SCRIPT_PARAMS_MAX_BYTES;
    g_cfg.scripting.ClientInvokeMaxResultBytes = CONDUIT_SCRIPT_RESULT_MAX_BYTES;
    app_config = &g_cfg;
}

void tearDown(void) {
    app_config = NULL;
}

void test_parse_missing_script(void) {
    json_t *body = json_loads("{\"params\":{}}", 0, NULL);
    TEST_ASSERT_NOT_NULL(body);
    ConduitScriptRequest req;
    const char *code = NULL;
    const char *msg = NULL;
    TEST_ASSERT_FALSE(conduit_script_parse_post_json(body, &req, &code, &msg));
    TEST_ASSERT_EQUAL_STRING("missing_script", code);
    TEST_ASSERT_NOT_NULL(msg);
    json_decref(body);
}

void test_parse_empty_script(void) {
    json_t *body = json_loads("{\"script\":\"\"}", 0, NULL);
    TEST_ASSERT_NOT_NULL(body);
    ConduitScriptRequest req;
    const char *code = NULL;
    const char *msg = NULL;
    TEST_ASSERT_FALSE(conduit_script_parse_post_json(body, &req, &code, &msg));
    TEST_ASSERT_EQUAL_STRING("missing_script", code);
    json_decref(body);
}

void test_parse_valid_defaults(void) {
    json_t *body = json_loads("{\"script\":\"Api.Echo\"}", 0, NULL);
    TEST_ASSERT_NOT_NULL(body);
    ConduitScriptRequest req;
    const char *code = NULL;
    const char *msg = NULL;
    TEST_ASSERT_TRUE(conduit_script_parse_post_json(body, &req, &code, &msg));
    TEST_ASSERT_NULL(code);
    TEST_ASSERT_EQUAL_STRING("Api.Echo", req.script);
    TEST_ASSERT_TRUE(req.wait);
    TEST_ASSERT_EQUAL_INT(CONDUIT_SCRIPT_TIMEOUT_DEFAULT_S, req.timeout_seconds);
    TEST_ASSERT_NULL(req.params);
    json_decref(body);
}

void test_parse_wait_false_and_timeout_clamp(void) {
    json_t *body = json_loads(
        "{\"script\":\"G.S\",\"wait\":false,\"timeout_seconds\":99}", 0, NULL);
    TEST_ASSERT_NOT_NULL(body);
    ConduitScriptRequest req;
    const char *code = NULL;
    const char *msg = NULL;
    TEST_ASSERT_TRUE(conduit_script_parse_post_json(body, &req, &code, &msg));
    TEST_ASSERT_FALSE(req.wait);
    TEST_ASSERT_EQUAL_INT(CONDUIT_SCRIPT_TIMEOUT_MAX_S, req.timeout_seconds);
    json_decref(body);
}

void test_parse_invalid_params_type(void) {
    json_t *body = json_loads("{\"script\":\"Api.Echo\",\"params\":[]}", 0, NULL);
    TEST_ASSERT_NOT_NULL(body);
    ConduitScriptRequest req;
    const char *code = NULL;
    const char *msg = NULL;
    TEST_ASSERT_FALSE(conduit_script_parse_post_json(body, &req, &code, &msg));
    TEST_ASSERT_EQUAL_STRING("invalid_params", code);
    json_decref(body);
}

void test_parse_null_body(void) {
    ConduitScriptRequest req;
    const char *code = NULL;
    const char *msg = NULL;
    TEST_ASSERT_FALSE(conduit_script_parse_post_json(NULL, &req, &code, &msg));
    TEST_ASSERT_EQUAL_STRING("invalid_json", code);
}

void test_extract_job_id(void) {
    char *id = conduit_script_extract_job_id("conduit/script/ABC12");
    TEST_ASSERT_NOT_NULL(id);
    TEST_ASSERT_EQUAL_STRING("ABC12", id);
    free(id);

    TEST_ASSERT_NULL(conduit_script_extract_job_id("conduit/script"));
    TEST_ASSERT_NULL(conduit_script_extract_job_id("conduit/script/"));
    TEST_ASSERT_NULL(conduit_script_extract_job_id("conduit/script/a/b"));
    TEST_ASSERT_NULL(conduit_script_extract_job_id(NULL));
}

void test_error_json_shape(void) {
    json_t *err = conduit_script_error_json("missing_script", "nope");
    TEST_ASSERT_NOT_NULL(err);
    TEST_ASSERT_FALSE(json_is_true(json_object_get(err, "success")));
    TEST_ASSERT_EQUAL_STRING("missing_script",
                             json_string_value(json_object_get(err, "error")));
    TEST_ASSERT_EQUAL_STRING("nope",
                             json_string_value(json_object_get(err, "message")));
    json_decref(err);
}

void test_parse_rejects_client_hydrogen(void) {
    json_t *body = json_loads(
        "{\"script\":\"Api.Echo\",\"params\":{\"_hydrogen\":{}}}", 0, NULL);
    TEST_ASSERT_NOT_NULL(body);
    ConduitScriptRequest req;
    const char *code = NULL;
    const char *msg = NULL;
    TEST_ASSERT_FALSE(conduit_script_parse_post_json(body, &req, &code, &msg));
    TEST_ASSERT_EQUAL_STRING("reserved_params", code);
    json_decref(body);
}

void test_map_invoke_error(void) {
    unsigned int st = 0;
    const char *code = NULL;
    const char *msg = NULL;
    conduit_script_map_invoke_error(SCRIPTING_INVOKE_ERR_NOT_FOUND, &st, &code, &msg);
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_NOT_FOUND, st);
    TEST_ASSERT_EQUAL_STRING("script_not_found", code);
    conduit_script_map_invoke_error(SCRIPTING_INVOKE_ERR_DISABLED, &st, &code, &msg);
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_SERVICE_UNAVAILABLE, st);
}

void test_claims_to_hydrogen(void) {
    jwt_claims_t claims;
    memset(&claims, 0, sizeof(claims));
    claims.sub = (char *)"user-1";
    claims.username = (char *)"alice";
    claims.database = (char *)"Helium";
    claims.user_id = 42;
    json_t *h = conduit_script_claims_to_hydrogen(&claims);
    TEST_ASSERT_NOT_NULL(h);
    TEST_ASSERT_EQUAL_STRING("user-1",
                             json_string_value(json_object_get(h, "sub")));
    TEST_ASSERT_EQUAL_STRING("alice",
                             json_string_value(json_object_get(h, "username")));
    TEST_ASSERT_EQUAL(42, json_integer_value(json_object_get(h, "user_id")));
    TEST_ASSERT_NULL(json_object_get(h, "id_token"));
    json_decref(h);
}

void test_parse_params_too_large(void) {
    g_cfg.scripting.ClientInvokeMaxParamsBytes = 40;
    json_t *body = json_loads(
        "{\"script\":\"Api.Echo\",\"params\":{\"pad\":\"0123456789012345678901234567890123456789\"}}",
        0, NULL);
    TEST_ASSERT_NOT_NULL(body);
    ConduitScriptRequest req;
    const char *code = NULL;
    const char *msg = NULL;
    TEST_ASSERT_FALSE(conduit_script_parse_post_json(body, &req, &code, &msg));
    TEST_ASSERT_EQUAL_STRING("params_too_large", code);
    json_decref(body);
}

void test_parse_timeout_uses_config_cap(void) {
    g_cfg.scripting.ClientInvokeDefaultTimeout = 3;
    g_cfg.scripting.ClientInvokeMaxTimeout = 7;
    json_t *body = json_loads(
        "{\"script\":\"Api.Echo\",\"timeout_seconds\":99}", 0, NULL);
    TEST_ASSERT_NOT_NULL(body);
    ConduitScriptRequest req;
    const char *code = NULL;
    const char *msg = NULL;
    TEST_ASSERT_TRUE(conduit_script_parse_post_json(body, &req, &code, &msg));
    TEST_ASSERT_EQUAL_INT(7, req.timeout_seconds);
    json_decref(body);

    body = json_loads("{\"script\":\"Api.Echo\"}", 0, NULL);
    TEST_ASSERT_TRUE(conduit_script_parse_post_json(body, &req, &code, &msg));
    TEST_ASSERT_EQUAL_INT(3, req.timeout_seconds);
    json_decref(body);
}

void test_map_timeout_wait_name(void) {
    TEST_ASSERT_EQUAL_STRING("timeout",
        conduit_script_wait_status_name(SCRIPTING_WAIT_TIMEOUT));
    TEST_ASSERT_EQUAL_STRING("shutdown",
        conduit_script_wait_status_name(SCRIPTING_WAIT_SHUTDOWN));
}

void test_job_response_omits_traceback(void) {
    ScoreboardEntry e;
    memset(&e, 0, sizeof(e));
    e.script_name = (char *)"Api.Echo";
    e.error_message = (char *)"runtime boom";
    e.error_traceback = (char *)"stack\nline1\nline2\nline3";
    e.result_json = (char *)"{}";
    json_t *resp = conduit_script_build_job_response(
        "failed", "AB12C", "Api.Echo", &e, 12);
    TEST_ASSERT_NOT_NULL(resp);
    TEST_ASSERT_EQUAL_STRING("runtime boom",
        json_string_value(json_object_get(resp, "error")));
    TEST_ASSERT_NULL(json_object_get(resp, "error_traceback"));
    TEST_ASSERT_NULL(json_object_get(resp, "traceback"));
    char *dump = json_dumps(resp, JSON_COMPACT);
    TEST_ASSERT_NOT_NULL(dump);
    TEST_ASSERT_NULL(strstr(dump, "stack"));
    free(dump);
    json_decref(resp);
}

void test_limit_helpers_defaults(void) {
    app_config = NULL;
    TEST_ASSERT_EQUAL_INT(CONDUIT_SCRIPT_PARAMS_MAX_BYTES,
                          conduit_script_params_max_bytes());
    TEST_ASSERT_EQUAL_INT(CONDUIT_SCRIPT_TIMEOUT_DEFAULT_S,
                          conduit_script_timeout_default_s());
    TEST_ASSERT_EQUAL_INT(CONDUIT_SCRIPT_TIMEOUT_MAX_S,
                          conduit_script_timeout_max_s());
    TEST_ASSERT_EQUAL_INT(CONDUIT_SCRIPT_RESULT_MAX_BYTES,
                          conduit_script_result_max_bytes());
    app_config = &g_cfg;
    g_cfg.scripting.ClientInvokeMaxParamsBytes = 100;
    TEST_ASSERT_EQUAL_INT(100, conduit_script_params_max_bytes());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_parse_missing_script);
    RUN_TEST(test_parse_empty_script);
    RUN_TEST(test_parse_valid_defaults);
    RUN_TEST(test_parse_wait_false_and_timeout_clamp);
    RUN_TEST(test_parse_invalid_params_type);
    RUN_TEST(test_parse_null_body);
    RUN_TEST(test_parse_rejects_client_hydrogen);
    RUN_TEST(test_parse_params_too_large);
    RUN_TEST(test_parse_timeout_uses_config_cap);
    RUN_TEST(test_extract_job_id);
    RUN_TEST(test_error_json_shape);
    RUN_TEST(test_map_invoke_error);
    RUN_TEST(test_map_timeout_wait_name);
    RUN_TEST(test_claims_to_hydrogen);
    RUN_TEST(test_job_response_omits_traceback);
    RUN_TEST(test_limit_helpers_defaults);
    return UNITY_END();
}
