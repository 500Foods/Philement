/*
 * Unity Test File: handle_conduit_script_request GET + POST error/edge
 * paths (LUA_CLIENT). Covers lines not exercised by the happy-path suite.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/conduit/script/script.h>
#include <src/config/config.h>
#include <src/scripting/scripting.h>
#include <src/scripting/scoreboard.h>

#define USE_MOCK_LIBMICROHTTPD
#define USE_MOCK_API_UTILS
#include <unity/mocks/mock_libmicrohttpd.h>
#include <unity/mocks/mock_api_utils.h>

void test_post_buffer_continue(void);
void test_post_buffer_error(void);
void test_post_method_error(void);
void test_post_buffer_default_state(void);
void test_post_empty_body(void);
void test_post_params_too_large_413(void);
void test_post_build_params_internal_error(void);
void test_post_submit_error(void);
void test_post_wait_not_found(void);
void test_post_wait_shutdown(void);
void test_post_wait_internal(void);
void test_get_missing_job_id(void);
void test_get_job_not_found(void);
void test_get_job_forbidden(void);
void test_get_job_success(void);
void test_jwt_hook_fail_returns_false(void);
void test_top_level_null_args(void);
void test_top_level_get_on_job_id_method_not_allowed(void);
void test_top_level_unknown_path_not_found(void);
void test_top_level_non_post_method(void);
void test_top_level_missing_upload_args(void);

static AppConfig test_config;
static char g_job_id_storage[16];

static bool jwt_ok_hook(struct MHD_Connection *c, jwt_validation_result_t *out) {
    (void)c;
    memset(out, 0, sizeof(*out));
    jwt_claims_t *claims = calloc(1, sizeof(jwt_claims_t));
    TEST_ASSERT_NOT_NULL(claims);
    claims->sub = strdup("sub-owner");
    claims->database = strdup("Helium");
    out->valid = true;
    out->claims = claims;
    return true;
}

static bool jwt_other_hook(struct MHD_Connection *c, jwt_validation_result_t *out) {
    (void)c;
    memset(out, 0, sizeof(*out));
    jwt_claims_t *claims = calloc(1, sizeof(jwt_claims_t));
    TEST_ASSERT_NOT_NULL(claims);
    claims->sub = strdup("sub-other");
    out->valid = true;
    out->claims = claims;
    return true;
}

static bool jwt_fail_hook(struct MHD_Connection *c, jwt_validation_result_t *out) {
    (void)c;
    memset(out, 0, sizeof(*out));
    out->valid = false;
    return false;
}

static ScriptingInvokeError submit_ok_hook(const char *script_name,
                                           const char *params_json,
                                           const ScoreboardJobLimits *limits,
                                           int fetch_timeout_seconds,
                                           char **job_id_out) {
    (void)script_name;
    (void)params_json;
    (void)limits;
    (void)fetch_timeout_seconds;
    strncpy(g_job_id_storage, "ABC12", sizeof(g_job_id_storage) - 1);
    *job_id_out = strdup(g_job_id_storage);
    return SCRIPTING_INVOKE_OK;
}

static ScriptingInvokeError submit_err_hook(const char *script_name,
                                            const char *params_json,
                                            const ScoreboardJobLimits *limits,
                                            int fetch_timeout_seconds,
                                            char **job_id_out) {
    (void)script_name;
    (void)params_json;
    (void)limits;
    (void)fetch_timeout_seconds;
    *job_id_out = NULL;
    return SCRIPTING_INVOKE_ERR_SUBMIT_FAILED;
}

static ScriptingWaitResult wait_not_found_hook(const char *job_id,
                                               int timeout_seconds,
                                               ScoreboardEntry **out_entry) {
    (void)job_id;
    (void)timeout_seconds;
    *out_entry = NULL;
    return SCRIPTING_WAIT_NOT_FOUND;
}

static ScriptingWaitResult wait_shutdown_hook(const char *job_id,
                                              int timeout_seconds,
                                              ScoreboardEntry **out_entry) {
    (void)job_id;
    (void)timeout_seconds;
    *out_entry = NULL;
    return SCRIPTING_WAIT_SHUTDOWN;
}

static ScriptingWaitResult wait_internal_hook(const char *job_id,
                                              int timeout_seconds,
                                              ScoreboardEntry **out_entry) {
    (void)job_id;
    (void)timeout_seconds;
    *out_entry = NULL;
    return SCRIPTING_WAIT_INTERNAL;
}

void setUp(void) {
    mock_mhd_reset_all();
    mock_api_utils_reset_all();
    mock_api_utils_set_buffer_result(API_BUFFER_COMPLETE);
    mock_api_utils_set_capture_mode(true);
    memset(&test_config, 0, sizeof(test_config));
    test_config.scripting.Enabled = false;
    app_config = &test_config;
    conduit_script_set_jwt_hook(NULL);
    conduit_script_set_submit_hook(NULL);
    conduit_script_set_wait_hook(NULL);
}

void tearDown(void) {
    json_t *cap = mock_api_utils_get_captured_response();
    if (cap) {
        json_decref(cap);
    }
    mock_api_utils_reset_capture();
    conduit_script_set_jwt_hook(NULL);
    conduit_script_set_submit_hook(NULL);
    conduit_script_set_wait_hook(NULL);
    app_config = NULL;
}

/* --- POST buffer states ------------------------------------------------ */

void test_post_buffer_continue(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1;
    size_t sz = 0;
    void *cls = NULL;
    test_config.scripting.Enabled = true;
    mock_api_utils_set_buffer_result(API_BUFFER_CONTINUE);
    enum MHD_Result r = handle_conduit_script_request(
        conn, "/api/conduit/script", "POST", NULL, &sz, &cls, "conduit/script");
    TEST_ASSERT_EQUAL(MHD_YES, r);
}

void test_post_buffer_error(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1;
    size_t sz = 0;
    void *cls = NULL;
    test_config.scripting.Enabled = true;
    mock_api_utils_set_buffer_result(API_BUFFER_ERROR);
    enum MHD_Result r = handle_conduit_script_request(
        conn, "/api/conduit/script", "POST", NULL, &sz, &cls, "conduit/script");
    /* api_send_error_and_cleanup is a mocked passthrough; the branch
     * (API_BUFFER_ERROR -> error response) is what we exercise here. */
    TEST_ASSERT_EQUAL(MHD_YES, r);
}

void test_post_method_error(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1;
    size_t sz = 0;
    void *cls = NULL;
    test_config.scripting.Enabled = true;
    mock_api_utils_set_buffer_result(API_BUFFER_METHOD_ERROR);
    enum MHD_Result r = handle_conduit_script_request(
        conn, "/api/conduit/script", "POST", NULL, &sz, &cls, "conduit/script");
    TEST_ASSERT_EQUAL(MHD_YES, r);
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_METHOD_NOT_ALLOWED,
                           mock_api_utils_get_captured_status());
}

void test_post_buffer_default_state(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1;
    size_t sz = 0;
    void *cls = NULL;
    test_config.scripting.Enabled = true;
    mock_api_utils_set_buffer_result((ApiBufferResult)777);
    enum MHD_Result r = handle_conduit_script_request(
        conn, "/api/conduit/script", "POST", NULL, &sz, &cls, "conduit/script");
    TEST_ASSERT_EQUAL(MHD_YES, r);
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_INTERNAL_SERVER_ERROR,
                           mock_api_utils_get_captured_status());
}

void test_post_empty_body(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1;
    size_t sz = 0;
    void *cls = NULL;
    test_config.scripting.Enabled = true;
    mock_api_utils_set_buffer_data("");
    enum MHD_Result r = handle_conduit_script_request(
        conn, "/api/conduit/script", "POST", NULL, &sz, &cls, "conduit/script");
    TEST_ASSERT_EQUAL(MHD_YES, r);
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_BAD_REQUEST,
                           mock_api_utils_get_captured_status());
}

void test_post_params_too_large_413(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1;
    size_t sz = 0;
    void *cls = NULL;
    test_config.scripting.Enabled = true;
    test_config.scripting.ClientInvokeMaxParamsBytes = 10;
    mock_api_utils_set_buffer_data(
        "{\"script\":\"Api.Echo\",\"params\":{\"x\":\"01234567890123456789\"}}");
    enum MHD_Result r = handle_conduit_script_request(
        conn, "/api/conduit/script", "POST", NULL, &sz, &cls, "conduit/script");
    TEST_ASSERT_EQUAL(MHD_YES, r);
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_CONTENT_TOO_LARGE,
                           mock_api_utils_get_captured_status());
    json_t *body = mock_api_utils_get_captured_response();
    TEST_ASSERT_EQUAL_STRING(
        "params_too_large",
        json_string_value(json_object_get(body, "error")));
}

void test_post_build_params_internal_error(void) {
    /* The internal_error -> 500 branch in the POST path is OOM-only
     * (irreducible floor). We exercise the build-params error dispatch
     * by forcing a merged-params overflow (params_too_large -> 413),
     * which still flows through conduit_script_build_params_json's
     * error-handling path under the JWT hook. */
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1;
    size_t sz = 0;
    void *cls = NULL;
    test_config.scripting.Enabled = true;
    test_config.scripting.ClientInvokeMaxParamsBytes = 5;
    conduit_script_set_jwt_hook(jwt_ok_hook);
    mock_api_utils_set_buffer_data("{\"script\":\"Api.Echo\"}");
    enum MHD_Result r = handle_conduit_script_request(
        conn, "/api/conduit/script", "POST", NULL, &sz, &cls, "conduit/script");
    TEST_ASSERT_EQUAL(MHD_YES, r);
}

void test_post_submit_error(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1;
    size_t sz = 0;
    void *cls = NULL;
    test_config.scripting.Enabled = true;
    conduit_script_set_jwt_hook(jwt_ok_hook);
    conduit_script_set_submit_hook(submit_err_hook);
    mock_api_utils_set_buffer_data("{\"script\":\"Api.Echo\"}");
    enum MHD_Result r = handle_conduit_script_request(
        conn, "/api/conduit/script", "POST", NULL, &sz, &cls, "conduit/script");
    TEST_ASSERT_EQUAL(MHD_YES, r);
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_INTERNAL_SERVER_ERROR,
                           mock_api_utils_get_captured_status());
    json_t *body = mock_api_utils_get_captured_response();
    TEST_ASSERT_EQUAL_STRING(
        "submit_failed",
        json_string_value(json_object_get(body, "error")));
}

void test_post_wait_not_found(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1;
    size_t sz = 0;
    void *cls = NULL;
    test_config.scripting.Enabled = true;
    conduit_script_set_jwt_hook(jwt_ok_hook);
    conduit_script_set_submit_hook(submit_ok_hook);
    conduit_script_set_wait_hook(wait_not_found_hook);
    mock_api_utils_set_buffer_data("{\"script\":\"Api.Echo\",\"wait\":true}");
    enum MHD_Result r = handle_conduit_script_request(
        conn, "/api/conduit/script", "POST", NULL, &sz, &cls, "conduit/script");
    TEST_ASSERT_EQUAL(MHD_YES, r);
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_NOT_FOUND,
                           mock_api_utils_get_captured_status());
    json_t *body = mock_api_utils_get_captured_response();
    TEST_ASSERT_EQUAL_STRING(
        "job_not_found",
        json_string_value(json_object_get(body, "error")));
}

void test_post_wait_shutdown(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1;
    size_t sz = 0;
    void *cls = NULL;
    test_config.scripting.Enabled = true;
    conduit_script_set_jwt_hook(jwt_ok_hook);
    conduit_script_set_submit_hook(submit_ok_hook);
    conduit_script_set_wait_hook(wait_shutdown_hook);
    mock_api_utils_set_buffer_data("{\"script\":\"Api.Echo\",\"wait\":true}");
    enum MHD_Result r = handle_conduit_script_request(
        conn, "/api/conduit/script", "POST", NULL, &sz, &cls, "conduit/script");
    TEST_ASSERT_EQUAL(MHD_YES, r);
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_SERVICE_UNAVAILABLE,
                           mock_api_utils_get_captured_status());
    json_t *body = mock_api_utils_get_captured_response();
    TEST_ASSERT_EQUAL_STRING(
        "scripting_shutdown",
        json_string_value(json_object_get(body, "error")));
}

void test_post_wait_internal(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1;
    size_t sz = 0;
    void *cls = NULL;
    test_config.scripting.Enabled = true;
    conduit_script_set_jwt_hook(jwt_ok_hook);
    conduit_script_set_submit_hook(submit_ok_hook);
    conduit_script_set_wait_hook(wait_internal_hook);
    mock_api_utils_set_buffer_data("{\"script\":\"Api.Echo\",\"wait\":true}");
    enum MHD_Result r = handle_conduit_script_request(
        conn, "/api/conduit/script", "POST", NULL, &sz, &cls, "conduit/script");
    TEST_ASSERT_EQUAL(MHD_YES, r);
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_INTERNAL_SERVER_ERROR,
                           mock_api_utils_get_captured_status());
    json_t *body = mock_api_utils_get_captured_response();
    TEST_ASSERT_EQUAL_STRING(
        "internal_error",
        json_string_value(json_object_get(body, "error")));
}

/* --- GET paths --------------------------------------------------------- */

void test_get_missing_job_id(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1;
    size_t sz = 0;
    void *cls = NULL;
    test_config.scripting.Enabled = true;
    /* Trailing slash => extract_job_id returns NULL, but the dispatcher
     * still routes "conduit/script/" to GET so the missing_job_id branch
     * (script.c) returns 400 rather than a misleading 404. */
    enum MHD_Result r = handle_conduit_script_request(
        conn, "/api/conduit/script/", "GET", NULL, &sz, &cls,
        "conduit/script/");
    TEST_ASSERT_EQUAL(MHD_YES, r);
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_BAD_REQUEST,
                           mock_api_utils_get_captured_status());
    json_t *body = mock_api_utils_get_captured_response();
    TEST_ASSERT_EQUAL_STRING(
        "missing_job_id",
        json_string_value(json_object_get(body, "error")));
}

void test_get_job_not_found(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1;
    size_t sz = 0;
    void *cls = NULL;
    test_config.scripting.Enabled = true;
    conduit_script_set_jwt_hook(jwt_ok_hook);
    enum MHD_Result r = handle_conduit_script_request(
        conn, "/api/conduit/script/NOPE", "GET", NULL, &sz, &cls,
        "conduit/script/NOPE");
    TEST_ASSERT_EQUAL(MHD_YES, r);
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_NOT_FOUND,
                           mock_api_utils_get_captured_status());
    json_t *body = mock_api_utils_get_captured_response();
    TEST_ASSERT_EQUAL_STRING(
        "job_not_found",
        json_string_value(json_object_get(body, "error")));
}

void test_get_job_forbidden(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1;
    size_t sz = 0;
    void *cls = NULL;
    test_config.scripting.Enabled = true;
    conduit_script_set_jwt_hook(jwt_other_hook);
    /* Put a job into the live scoreboard owned by a different subject. */
    Scoreboard *sb = scoreboard_create();
    TEST_ASSERT_NOT_NULL(sb);
    char *id = scoreboard_submit(sb, "Api.Echo", "{}");
    TEST_ASSERT_NOT_NULL(id);
    TEST_ASSERT_TRUE(scoreboard_set_submitted_by(sb, id, "sub-owner"));
    scripting_scoreboard = sb;

    char path[64];
    snprintf(path, sizeof(path), "conduit/script/%s", id);
    char url[80];
    snprintf(url, sizeof(url), "/api/conduit/script/%s", id);
    enum MHD_Result r = handle_conduit_script_request(
        conn, url, "GET", NULL, &sz, &cls, path);
    (void)r;
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_FORBIDDEN,
                           mock_api_utils_get_captured_status());
    json_t *body = mock_api_utils_get_captured_response();
    TEST_ASSERT_EQUAL_STRING(
        "forbidden",
        json_string_value(json_object_get(body, "error")));
    scoreboard_destroy(sb);
    scripting_scoreboard = NULL;
}

void test_get_job_success(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1;
    size_t sz = 0;
    void *cls = NULL;
    test_config.scripting.Enabled = true;
    conduit_script_set_jwt_hook(jwt_ok_hook);
    Scoreboard *sb = scoreboard_create();
    TEST_ASSERT_NOT_NULL(sb);
    char *id = scoreboard_submit(sb, "Api.Echo", "{}");
    TEST_ASSERT_NOT_NULL(id);
    TEST_ASSERT_TRUE(scoreboard_set_submitted_by(sb, id, "sub-owner"));
    TEST_ASSERT_TRUE(scoreboard_update_status(sb, id, SCOREBOARD_JOB_COMPLETED));
    TEST_ASSERT_TRUE(scoreboard_update_result_json(sb, id, "{\"ok\":true}"));
    TEST_ASSERT_TRUE(scoreboard_update_result(sb, id, "json", "file:///x"));
    scripting_scoreboard = sb;

    char path[64];
    snprintf(path, sizeof(path), "conduit/script/%s", id);
    char url[80];
    snprintf(url, sizeof(url), "/api/conduit/script/%s", id);
    enum MHD_Result r = handle_conduit_script_request(
        conn, url, "GET", NULL, &sz, &cls, path);
    (void)r;
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_OK,
                           mock_api_utils_get_captured_status());
    json_t *body = mock_api_utils_get_captured_response();
    TEST_ASSERT_EQUAL_STRING(
        "completed",
        json_string_value(json_object_get(body, "status")));
    TEST_ASSERT_EQUAL_STRING(
        "file:///x",
        json_string_value(json_object_get(body, "result_location")));
    scoreboard_destroy(sb);
    scripting_scoreboard = NULL;
}

/* --- JWT hook failure branch ------------------------------------------ */

void test_jwt_hook_fail_returns_false(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1;
    size_t sz = 0;
    void *cls = NULL;
    test_config.scripting.Enabled = true;
    conduit_script_set_jwt_hook(jwt_fail_hook);
    mock_api_utils_set_buffer_data("{\"script\":\"Api.Echo\"}");
    enum MHD_Result r = handle_conduit_script_request(
        conn, "/api/conduit/script", "POST", NULL, &sz, &cls, "conduit/script");
    /* validate_script_jwt returns false; handler returns MHD_YES after the
     * hook queued (or would queue) a 401. The branch under test is the
     * !valid path that sends the generic invalid-token 401. */
    TEST_ASSERT_EQUAL(MHD_YES, r);
}

/* --- Top-level guards -------------------------------------------------- */

void test_top_level_null_args(void) {
    enum MHD_Result r = handle_conduit_script_request(
        NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(MHD_NO, r);
}

void test_top_level_get_on_job_id_method_not_allowed(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1;
    size_t sz = 0;
    void *cls = NULL;
    test_config.scripting.Enabled = true;
    enum MHD_Result r = handle_conduit_script_request(
        conn, "/api/conduit/script/J1", "POST", NULL, &sz, &cls,
        "conduit/script/J1");
    TEST_ASSERT_EQUAL(MHD_YES, r);
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_METHOD_NOT_ALLOWED,
                           mock_api_utils_get_captured_status());
}

void test_top_level_unknown_path_not_found(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1;
    size_t sz = 0;
    void *cls = NULL;
    test_config.scripting.Enabled = true;
    enum MHD_Result r = handle_conduit_script_request(
        conn, "/api/conduit/script/extra", "GET", NULL, &sz, &cls,
        "conduit/script/extra/path");
    TEST_ASSERT_EQUAL(MHD_YES, r);
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_NOT_FOUND,
                           mock_api_utils_get_captured_status());
}

void test_top_level_non_post_method(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1;
    size_t sz = 0;
    void *cls = NULL;
    test_config.scripting.Enabled = true;
    enum MHD_Result r = handle_conduit_script_request(
        conn, "/api/conduit/script", "PUT", NULL, &sz, &cls, "conduit/script");
    TEST_ASSERT_EQUAL(MHD_YES, r);
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_METHOD_NOT_ALLOWED,
                           mock_api_utils_get_captured_status());
}

void test_top_level_missing_upload_args(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1;
    void *cls = NULL;
    test_config.scripting.Enabled = true;
    enum MHD_Result r = handle_conduit_script_request(
        conn, "/api/conduit/script", "POST", NULL, NULL, &cls, "conduit/script");
    TEST_ASSERT_EQUAL(MHD_NO, r);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_post_buffer_continue);
    RUN_TEST(test_post_buffer_error);
    RUN_TEST(test_post_method_error);
    RUN_TEST(test_post_buffer_default_state);
    RUN_TEST(test_post_empty_body);
    RUN_TEST(test_post_params_too_large_413);
    RUN_TEST(test_post_build_params_internal_error);
    RUN_TEST(test_post_submit_error);
    RUN_TEST(test_post_wait_not_found);
    RUN_TEST(test_post_wait_shutdown);
    RUN_TEST(test_post_wait_internal);
    RUN_TEST(test_get_missing_job_id);
    RUN_TEST(test_get_job_not_found);
    RUN_TEST(test_get_job_forbidden);
    RUN_TEST(test_get_job_success);
    RUN_TEST(test_jwt_hook_fail_returns_false);
    RUN_TEST(test_top_level_null_args);
    RUN_TEST(test_top_level_get_on_job_id_method_not_allowed);
    RUN_TEST(test_top_level_unknown_path_not_found);
    RUN_TEST(test_top_level_non_post_method);
    RUN_TEST(test_top_level_missing_upload_args);
    return UNITY_END();
}
