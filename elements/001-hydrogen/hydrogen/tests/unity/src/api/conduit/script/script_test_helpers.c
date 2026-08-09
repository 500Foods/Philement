/*
 * Unity Test File: conduit_script pure helpers (build params, status/map
 * strings, job response, elapsed) — src/api/conduit/script/script.c
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <stdlib.h>
#include <string.h>

#include <src/api/conduit/script/script.h>
#include <src/config/config.h>
#include <src/scripting/scoreboard.h>

extern AppConfig *app_config;

void test_build_params_null_guard(void);
void test_build_params_claims_oom(void);
void test_build_params_merged_too_large(void);
void test_build_params_merges_client_and_hydrogen(void);
void test_job_status_string_all(void);
void test_wait_status_name_all(void);
void test_map_invoke_error_all(void);
void test_set_string_if_empty(void);
void test_build_job_response_null_entry(void);
void test_build_job_response_no_script_fallback(void);
void test_build_job_response_result_location(void);
void test_elapsed_from_entry_null(void);
void test_elapsed_from_entry_uses_finished(void);
void test_is_enabled_false_without_config(void);

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

void test_build_params_null_guard(void) {
    const char *code = NULL;
    const char *msg = NULL;
    jwt_claims_t claims;
    memset(&claims, 0, sizeof(claims));
    TEST_ASSERT_NULL(conduit_script_build_params_json(NULL, &claims, &code, &msg));
    TEST_ASSERT_NULL(conduit_script_build_params_json(
        (const ConduitScriptRequest *)"x", NULL, &code, &msg));
}

void test_build_params_claims_oom(void) {
    /* Claims NULL -> conduit_script_claims_to_hydrogen returns NULL
     * (NULL guard returns before setting error_code). */
    const char *code = NULL;
    const char *msg = NULL;
    ConduitScriptRequest req;
    memset(&req, 0, sizeof(req));
    TEST_ASSERT_NULL(conduit_script_build_params_json(&req, NULL, &code, &msg));
}

void test_build_params_merged_too_large(void) {
    /* Cap the merged params limit very small so any _hydrogen blows it. */
    g_cfg.scripting.ClientInvokeMaxParamsBytes = 5;
    const char *code = NULL;
    const char *msg = NULL;
    ConduitScriptRequest req;
    memset(&req, 0, sizeof(req));
    jwt_claims_t claims;
    memset(&claims, 0, sizeof(claims));
    claims.sub = (char *)"user-1";
    char *out = conduit_script_build_params_json(&req, &claims, &code, &msg);
    TEST_ASSERT_NULL(out);
    TEST_ASSERT_EQUAL_STRING("params_too_large", code);
    TEST_ASSERT_EQUAL_STRING(
        "Merged params (including _hydrogen) exceed size limit", msg);
}

void test_build_params_merges_client_and_hydrogen(void) {
    const char *code = NULL;
    const char *msg = NULL;
    ConduitScriptRequest req;
    memset(&req, 0, sizeof(req));
    json_t *params = json_pack("{s:i}", "x", 1);
    TEST_ASSERT_NOT_NULL(params);
    req.params = params;

    jwt_claims_t claims;
    memset(&claims, 0, sizeof(claims));
    claims.sub = (char *)"user-1";
    claims.username = (char *)"alice";

    char *out = conduit_script_build_params_json(&req, &claims, &code, &msg);
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_NULL(code);
    json_t *merged = json_loads(out, 0, NULL);
    TEST_ASSERT_NOT_NULL(merged);
    TEST_ASSERT_EQUAL_INT(1, json_integer_value(json_object_get(merged, "x")));
    json_t *h = json_object_get(merged, "_hydrogen");
    TEST_ASSERT_NOT_NULL(h);
    TEST_ASSERT_EQUAL_STRING("user-1",
                             json_string_value(json_object_get(h, "sub")));
    TEST_ASSERT_EQUAL_STRING("alice",
                             json_string_value(json_object_get(h, "username")));
    json_decref(merged);
    free(out);
    json_decref(params);
}

void test_job_status_string_all(void) {
    TEST_ASSERT_EQUAL_STRING(
        "pending", conduit_script_job_status_string(SCOREBOARD_JOB_PENDING));
    TEST_ASSERT_EQUAL_STRING(
        "running", conduit_script_job_status_string(SCOREBOARD_JOB_RUNNING));
    TEST_ASSERT_EQUAL_STRING(
        "completed", conduit_script_job_status_string(SCOREBOARD_JOB_COMPLETED));
    TEST_ASSERT_EQUAL_STRING(
        "failed", conduit_script_job_status_string(SCOREBOARD_JOB_FAILED));
    TEST_ASSERT_EQUAL_STRING(
        "killed", conduit_script_job_status_string(SCOREBOARD_JOB_KILLED));
    TEST_ASSERT_EQUAL_STRING(
        "unknown", (conduit_script_job_status_string(
            (ScoreboardJobStatus)999)));
}

void test_wait_status_name_all(void) {
    TEST_ASSERT_EQUAL_STRING(
        "completed", conduit_script_wait_status_name(SCRIPTING_WAIT_COMPLETED));
    TEST_ASSERT_EQUAL_STRING(
        "failed", conduit_script_wait_status_name(SCRIPTING_WAIT_FAILED));
    TEST_ASSERT_EQUAL_STRING(
        "killed", conduit_script_wait_status_name(SCRIPTING_WAIT_KILLED));
    TEST_ASSERT_EQUAL_STRING(
        "timeout", conduit_script_wait_status_name(SCRIPTING_WAIT_TIMEOUT));
    TEST_ASSERT_EQUAL_STRING(
        "not_found", conduit_script_wait_status_name(SCRIPTING_WAIT_NOT_FOUND));
    TEST_ASSERT_EQUAL_STRING(
        "shutdown", conduit_script_wait_status_name(SCRIPTING_WAIT_SHUTDOWN));
    TEST_ASSERT_EQUAL_STRING(
        "internal_error",
        conduit_script_wait_status_name(SCRIPTING_WAIT_INTERNAL));
    TEST_ASSERT_EQUAL_STRING(
        "internal_error",
        conduit_script_wait_status_name((ScriptingWaitResult)999));
}

void test_map_invoke_error_all(void) {
    unsigned int st = 0;
    const char *code = NULL;
    const char *msg = NULL;

    conduit_script_map_invoke_error(SCRIPTING_INVOKE_OK, &st, &code, &msg);
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_OK, st);
    TEST_ASSERT_EQUAL_STRING("ok", code);

    conduit_script_map_invoke_error(SCRIPTING_INVOKE_ERR_DISABLED, &st, &code, &msg);
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_SERVICE_UNAVAILABLE, st);
    TEST_ASSERT_EQUAL_STRING("scripting_disabled", code);

    conduit_script_map_invoke_error(SCRIPTING_INVOKE_ERR_INVALID_NAME, &st, &code, &msg);
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_BAD_REQUEST, st);
    TEST_ASSERT_EQUAL_STRING("invalid_script_name", code);

    conduit_script_map_invoke_error(SCRIPTING_INVOKE_ERR_NO_DATABASE, &st, &code, &msg);
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_SERVICE_UNAVAILABLE, st);
    TEST_ASSERT_EQUAL_STRING("no_database", code);

    conduit_script_map_invoke_error(SCRIPTING_INVOKE_ERR_NOT_FOUND, &st, &code, &msg);
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_NOT_FOUND, st);
    TEST_ASSERT_EQUAL_STRING("script_not_found", code);

    conduit_script_map_invoke_error(SCRIPTING_INVOKE_ERR_DB_TIMEOUT, &st, &code, &msg);
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_GATEWAY_TIMEOUT, st);
    TEST_ASSERT_EQUAL_STRING("script_fetch_timeout", code);

    conduit_script_map_invoke_error(SCRIPTING_INVOKE_ERR_SUBMIT_FAILED, &st, &code, &msg);
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_INTERNAL_SERVER_ERROR, st);
    TEST_ASSERT_EQUAL_STRING("submit_failed", code);

    conduit_script_map_invoke_error(SCRIPTING_INVOKE_ERR_INTERNAL, &st, &code, &msg);
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_INTERNAL_SERVER_ERROR, st);
    TEST_ASSERT_EQUAL_STRING("internal_error", code);

    /* NULL out-params must be safe. */
    conduit_script_map_invoke_error(SCRIPTING_INVOKE_ERR_DISABLED, NULL, NULL, NULL);
}

void test_set_string_if_empty(void) {
    json_t *obj = json_object();
    TEST_ASSERT_NOT_NULL(obj);
    conduit_script_set_string_if(obj, "k", NULL);
    conduit_script_set_string_if(obj, "k", "");
    TEST_ASSERT_NULL(json_object_get(obj, "k"));
    conduit_script_set_string_if(obj, "k", "v");
    TEST_ASSERT_EQUAL_STRING("v", json_string_value(json_object_get(obj, "k")));
    json_decref(obj);
}

void test_build_job_response_null_entry(void) {
    json_t *resp = conduit_script_build_job_response(
        "completed", "J1", "Api.Echo", NULL, 7);
    TEST_ASSERT_NOT_NULL(resp);
    TEST_ASSERT_EQUAL_STRING("completed",
                             json_string_value(json_object_get(resp, "status")));
    TEST_ASSERT_EQUAL_STRING("J1",
                             json_string_value(json_object_get(resp, "job_id")));
    TEST_ASSERT_EQUAL_STRING("Api.Echo",
                             json_string_value(json_object_get(resp, "script")));
    json_t *result = json_object_get(resp, "result");
    TEST_ASSERT_TRUE(json_is_object(result));
    TEST_ASSERT_EQUAL_INT(0, json_object_size(result)); /* defaults to {} */
    TEST_ASSERT_TRUE(json_is_null(json_object_get(resp, "result_type")));
    TEST_ASSERT_TRUE(json_is_null(json_object_get(resp, "result_location")));
    TEST_ASSERT_TRUE(json_is_null(json_object_get(resp, "error")));
    TEST_ASSERT_EQUAL_INT(7, json_integer_value(json_object_get(resp, "elapsed_ms")));
    json_decref(resp);
}

void test_build_job_response_no_script_fallback(void) {
    ScoreboardEntry e;
    memset(&e, 0, sizeof(e));
    e.script_name = (char *)"FromEntry.Script";
    json_t *resp = conduit_script_build_job_response(
        "running", "J2", NULL, &e, 0);
    TEST_ASSERT_NOT_NULL(resp);
    TEST_ASSERT_EQUAL_STRING("FromEntry.Script",
                             json_string_value(json_object_get(resp, "script")));
    json_decref(resp);
}

void test_build_job_response_result_location(void) {
    ScoreboardEntry e;
    memset(&e, 0, sizeof(e));
    e.result_json = (char *)"{\"ok\":true}";
    e.result_type = (char *)"json";
    e.result_location = (char *)"file:///tmp/out";
    e.error_message = (char *)"boom";
    json_t *resp = conduit_script_build_job_response(
        "failed", "J3", "Api.Echo", &e, 3);
    TEST_ASSERT_NOT_NULL(resp);
    TEST_ASSERT_EQUAL_STRING("file:///tmp/out",
                             json_string_value(json_object_get(resp, "result_location")));
    TEST_ASSERT_EQUAL_STRING("json",
                             json_string_value(json_object_get(resp, "result_type")));
    TEST_ASSERT_EQUAL_STRING("boom",
                             json_string_value(json_object_get(resp, "error")));
    json_t *result = json_object_get(resp, "result");
    TEST_ASSERT_TRUE(json_is_object(result));
    TEST_ASSERT_TRUE(json_is_true(json_object_get(result, "ok")));
    json_decref(resp);
}

void test_elapsed_from_entry_null(void) {
    TEST_ASSERT_EQUAL_INT(0, conduit_script_elapsed_ms_from_entry(NULL));
}

void test_elapsed_from_entry_uses_finished(void) {
    ScoreboardEntry e;
    memset(&e, 0, sizeof(e));
    e.started_at.tv_sec = 100;
    e.finished_at.tv_sec = 103;
    e.finished_at.tv_nsec = 500000000L;
    long ms = conduit_script_elapsed_ms_from_entry(&e);
    TEST_ASSERT_EQUAL_INT(3500, ms);
}

void test_is_enabled_false_without_config(void) {
    app_config = NULL;
    TEST_ASSERT_FALSE(conduit_script_is_enabled());
    g_cfg.scripting.Enabled = true;
    app_config = &g_cfg;
    TEST_ASSERT_TRUE(conduit_script_is_enabled());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_build_params_null_guard);
    RUN_TEST(test_build_params_claims_oom);
    RUN_TEST(test_build_params_merged_too_large);
    RUN_TEST(test_build_params_merges_client_and_hydrogen);
    RUN_TEST(test_job_status_string_all);
    RUN_TEST(test_wait_status_name_all);
    RUN_TEST(test_map_invoke_error_all);
    RUN_TEST(test_set_string_if_empty);
    RUN_TEST(test_build_job_response_null_entry);
    RUN_TEST(test_build_job_response_no_script_fallback);
    RUN_TEST(test_build_job_response_result_location);
    RUN_TEST(test_elapsed_from_entry_null);
    RUN_TEST(test_elapsed_from_entry_uses_finished);
    RUN_TEST(test_is_enabled_false_without_config);
    return UNITY_END();
}
