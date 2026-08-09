/*
 * Unity Test File: handle_conduit_script_request (Phases 4–5)
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/conduit/script/script.h>
#include <src/config/config.h>
#include <src/scripting/scoreboard.h>

#define USE_MOCK_LIBMICROHTTPD
#define USE_MOCK_API_UTILS
#include <unity/mocks/mock_libmicrohttpd.h>
#include <unity/mocks/mock_api_utils.h>

void test_method_not_allowed_on_post_path(void);
void test_get_job_scripting_disabled(void);
void test_post_scripting_disabled(void);
void test_post_missing_script_field(void);
void test_post_invalid_json_body(void);
void test_post_jwt_failure(void);
void test_post_wait_true_happy(void);
void test_post_wait_false_accepted(void);
void test_post_script_not_found(void);
void test_post_reserved_hydrogen(void);

static AppConfig test_config;
static char g_job_id_storage[16];

static bool jwt_ok_hook(struct MHD_Connection *c, jwt_validation_result_t *out) {
    (void)c;
    memset(out, 0, sizeof(*out));
    jwt_claims_t *claims = calloc(1, sizeof(jwt_claims_t));
    TEST_ASSERT_NOT_NULL(claims);
    claims->sub = strdup("sub-1");
    claims->database = strdup("Helium");
    claims->username = strdup("tester");
    out->valid = true;
    out->claims = claims;
    return true;
}

static bool jwt_fail_hook(struct MHD_Connection *c, jwt_validation_result_t *out) {
    (void)c;
    memset(out, 0, sizeof(*out));
    out->valid = false;
    out->error = JWT_ERROR_INVALID_SIGNATURE;
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
    TEST_ASSERT_NOT_NULL(params_json);
    TEST_ASSERT_NOT_NULL(strstr(params_json, "\"_hydrogen\""));
    strncpy(g_job_id_storage, "ABC12", sizeof(g_job_id_storage) - 1);
    *job_id_out = strdup(g_job_id_storage);
    return SCRIPTING_INVOKE_OK;
}

static ScriptingInvokeError submit_not_found_hook(const char *script_name,
                                                  const char *params_json,
                                                  const ScoreboardJobLimits *limits,
                                                  int fetch_timeout_seconds,
                                                  char **job_id_out) {
    (void)script_name;
    (void)params_json;
    (void)limits;
    (void)fetch_timeout_seconds;
    *job_id_out = NULL;
    return SCRIPTING_INVOKE_ERR_NOT_FOUND;
}

static ScriptingWaitResult wait_completed_hook(const char *job_id,
                                               int timeout_seconds,
                                               ScoreboardEntry **out_entry) {
    (void)job_id;
    (void)timeout_seconds;
    ScoreboardEntry *e = calloc(1, sizeof(ScoreboardEntry));
    TEST_ASSERT_NOT_NULL(e);
    e->status = SCOREBOARD_JOB_COMPLETED;
    e->script_name = strdup("Api.Echo");
    e->result_json = strdup("{\"ok\":true}");
    e->result_type = strdup("json");
    *out_entry = e;
    return SCRIPTING_WAIT_COMPLETED;
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

void test_method_not_allowed_on_post_path(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1;
    size_t sz = 0;
    void *cls = NULL;
    enum MHD_Result r = handle_conduit_script_request(
        conn, "/api/conduit/script", "GET", NULL, &sz, &cls, "conduit/script");
    TEST_ASSERT_EQUAL(MHD_YES, r);
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_METHOD_NOT_ALLOWED,
                           mock_api_utils_get_captured_status());
}

void test_get_job_scripting_disabled(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1;
    size_t sz = 0;
    void *cls = NULL;
    enum MHD_Result r = handle_conduit_script_request(
        conn, "/api/conduit/script/JOB1", "GET", NULL, &sz, &cls,
        "conduit/script/JOB1");
    TEST_ASSERT_EQUAL(MHD_YES, r);
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_SERVICE_UNAVAILABLE,
                           mock_api_utils_get_captured_status());
}

void test_post_scripting_disabled(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1;
    size_t sz = 0;
    void *cls = NULL;
    mock_api_utils_set_buffer_data("{\"script\":\"Api.Echo\"}");
    enum MHD_Result r = handle_conduit_script_request(
        conn, "/api/conduit/script", "POST", NULL, &sz, &cls, "conduit/script");
    TEST_ASSERT_EQUAL(MHD_YES, r);
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_SERVICE_UNAVAILABLE,
                           mock_api_utils_get_captured_status());
}

void test_post_missing_script_field(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1;
    size_t sz = 0;
    void *cls = NULL;
    test_config.scripting.Enabled = true;
    mock_api_utils_set_buffer_data("{\"params\":{}}");
    enum MHD_Result r = handle_conduit_script_request(
        conn, "/api/conduit/script", "POST", NULL, &sz, &cls, "conduit/script");
    TEST_ASSERT_EQUAL(MHD_YES, r);
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_BAD_REQUEST,
                           mock_api_utils_get_captured_status());
    json_t *body = mock_api_utils_get_captured_response();
    TEST_ASSERT_EQUAL_STRING(
        "missing_script",
        json_string_value(json_object_get(body, "error")));
}

void test_post_invalid_json_body(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1;
    size_t sz = 0;
    void *cls = NULL;
    test_config.scripting.Enabled = true;
    mock_api_utils_set_buffer_data("{\"script\":");
    enum MHD_Result r = handle_conduit_script_request(
        conn, "/api/conduit/script", "POST", NULL, &sz, &cls, "conduit/script");
    TEST_ASSERT_EQUAL(MHD_YES, r);
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_BAD_REQUEST,
                           mock_api_utils_get_captured_status());
}

void test_post_jwt_failure(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1;
    size_t sz = 0;
    void *cls = NULL;
    test_config.scripting.Enabled = true;
    conduit_script_set_jwt_hook(jwt_fail_hook);
    mock_api_utils_set_buffer_data("{\"script\":\"Api.Echo\"}");
    enum MHD_Result r = handle_conduit_script_request(
        conn, "/api/conduit/script", "POST", NULL, &sz, &cls, "conduit/script");
    TEST_ASSERT_EQUAL(MHD_YES, r);
    /* send_jwt_error_response does not use api_send_json_response mock */
    (void)r;
}

void test_post_wait_true_happy(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1;
    size_t sz = 0;
    void *cls = NULL;
    test_config.scripting.Enabled = true;
    conduit_script_set_jwt_hook(jwt_ok_hook);
    conduit_script_set_submit_hook(submit_ok_hook);
    conduit_script_set_wait_hook(wait_completed_hook);
    mock_api_utils_set_buffer_data(
        "{\"script\":\"Api.Echo\",\"params\":{\"x\":1},\"wait\":true}");

    enum MHD_Result r = handle_conduit_script_request(
        conn, "/api/conduit/script", "POST", NULL, &sz, &cls, "conduit/script");
    TEST_ASSERT_EQUAL(MHD_YES, r);
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_OK, mock_api_utils_get_captured_status());
    json_t *body = mock_api_utils_get_captured_response();
    TEST_ASSERT_NOT_NULL(body);
    TEST_ASSERT_EQUAL_STRING(
        "completed",
        json_string_value(json_object_get(body, "status")));
    TEST_ASSERT_EQUAL_STRING(
        "ABC12",
        json_string_value(json_object_get(body, "job_id")));
    json_t *result = json_object_get(body, "result");
    TEST_ASSERT_TRUE(json_is_object(result));
    TEST_ASSERT_TRUE(json_is_true(json_object_get(result, "ok")));
}

void test_post_wait_false_accepted(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1;
    size_t sz = 0;
    void *cls = NULL;
    test_config.scripting.Enabled = true;
    conduit_script_set_jwt_hook(jwt_ok_hook);
    conduit_script_set_submit_hook(submit_ok_hook);
    mock_api_utils_set_buffer_data(
        "{\"script\":\"Api.Echo\",\"wait\":false}");

    enum MHD_Result r = handle_conduit_script_request(
        conn, "/api/conduit/script", "POST", NULL, &sz, &cls, "conduit/script");
    TEST_ASSERT_EQUAL(MHD_YES, r);
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_ACCEPTED,
                           mock_api_utils_get_captured_status());
    json_t *body = mock_api_utils_get_captured_response();
    TEST_ASSERT_EQUAL_STRING(
        "pending",
        json_string_value(json_object_get(body, "status")));
}

void test_post_script_not_found(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1;
    size_t sz = 0;
    void *cls = NULL;
    test_config.scripting.Enabled = true;
    conduit_script_set_jwt_hook(jwt_ok_hook);
    conduit_script_set_submit_hook(submit_not_found_hook);
    mock_api_utils_set_buffer_data("{\"script\":\"Missing.Script\"}");

    enum MHD_Result r = handle_conduit_script_request(
        conn, "/api/conduit/script", "POST", NULL, &sz, &cls, "conduit/script");
    TEST_ASSERT_EQUAL(MHD_YES, r);
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_NOT_FOUND,
                           mock_api_utils_get_captured_status());
    json_t *body = mock_api_utils_get_captured_response();
    TEST_ASSERT_EQUAL_STRING(
        "script_not_found",
        json_string_value(json_object_get(body, "error")));
}

void test_post_reserved_hydrogen(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1;
    size_t sz = 0;
    void *cls = NULL;
    test_config.scripting.Enabled = true;
    mock_api_utils_set_buffer_data(
        "{\"script\":\"Api.Echo\",\"params\":{\"_hydrogen\":{\"x\":1}}}");

    enum MHD_Result r = handle_conduit_script_request(
        conn, "/api/conduit/script", "POST", NULL, &sz, &cls, "conduit/script");
    TEST_ASSERT_EQUAL(MHD_YES, r);
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_BAD_REQUEST,
                           mock_api_utils_get_captured_status());
    json_t *body = mock_api_utils_get_captured_response();
    TEST_ASSERT_EQUAL_STRING(
        "reserved_params",
        json_string_value(json_object_get(body, "error")));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_method_not_allowed_on_post_path);
    RUN_TEST(test_get_job_scripting_disabled);
    RUN_TEST(test_post_scripting_disabled);
    RUN_TEST(test_post_missing_script_field);
    RUN_TEST(test_post_invalid_json_body);
    RUN_TEST(test_post_jwt_failure);
    RUN_TEST(test_post_wait_true_happy);
    RUN_TEST(test_post_wait_false_accepted);
    RUN_TEST(test_post_script_not_found);
    RUN_TEST(test_post_reserved_hydrogen);
    return UNITY_END();
}
