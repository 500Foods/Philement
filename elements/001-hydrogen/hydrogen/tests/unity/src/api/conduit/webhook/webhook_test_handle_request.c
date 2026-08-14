/*
 * Unity Test File: handle_conduit_webhook_request
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/conduit/webhook/webhook.h>
#include <src/config/config.h>
#include <src/scripting/scoreboard.h>
#include <src/utils/utils_crypto.h>

#define USE_MOCK_LIBMICROHTTPD
#define USE_MOCK_API_UTILS
#include <unity/mocks/mock_libmicrohttpd.h>
#include <unity/mocks/mock_api_utils.h>

void test_method_not_allowed(void);
void test_unknown_hook_404(void);
void test_disabled_503(void);
void test_bad_signature_401_no_submit(void);
void test_good_signature_submits_configured_script(void);

static AppConfig test_config;
static int submit_calls;
static char last_script[64];
static char last_params[1024];

static enum MHD_Result send_json_capture(struct MHD_Connection *connection,
                                         json_t *json_obj,
                                         unsigned int status_code) {
    (void)connection;
    return mock_api_send_json_response(connection, json_obj, status_code);
}

static ScriptingInvokeError submit_ok_hook(const char *script_name,
                                           const char *params_json,
                                           const ScoreboardJobLimits *limits,
                                           int fetch_timeout_seconds,
                                           char **job_id_out) {
    (void)limits;
    (void)fetch_timeout_seconds;
    submit_calls++;
    strncpy(last_script, script_name ? script_name : "", sizeof(last_script) - 1);
    strncpy(last_params, params_json ? params_json : "", sizeof(last_params) - 1);
    *job_id_out = strdup("WH001");
    return SCRIPTING_INVOKE_OK;
}

static ScriptingWaitResult wait_completed_hook(const char *job_id,
                                               int timeout_seconds,
                                               ScoreboardEntry **out_entry) {
    (void)job_id;
    (void)timeout_seconds;
    ScoreboardEntry *e = calloc(1, sizeof(ScoreboardEntry));
    TEST_ASSERT_NOT_NULL(e);
    e->status = SCOREBOARD_JOB_COMPLETED;
    e->script_name = strdup("Stripe.Webhook");
    e->result_json = strdup("{\"ok\":true}");
    e->result_type = strdup("json");
    *out_entry = e;
    return SCRIPTING_WAIT_COMPLETED;
}

static void seed_stripe_hook(void) {
    test_config.webhooks.Enabled = true;
    test_config.webhooks.HookCount = 1;
    test_config.webhooks.Hooks[0].Name = strdup("stripe");
    test_config.webhooks.Hooks[0].SecretEnv = strdup("HYDROGEN_TEST_WH_SECRET");
    test_config.webhooks.Hooks[0].SignatureHeader = strdup("Stripe-Signature");
    test_config.webhooks.Hooks[0].Hmac = strdup("sha256");
    test_config.webhooks.Hooks[0].Script = strdup("Stripe.Webhook");
    test_config.scripting.Enabled = true;
}

void setUp(void) {
    mock_mhd_reset_all();
    mock_api_utils_reset_all();
    mock_api_utils_set_buffer_result(API_BUFFER_COMPLETE);
    mock_api_utils_set_capture_mode(true);
    memset(&test_config, 0, sizeof(test_config));
    app_config = &test_config;
    submit_calls = 0;
    last_script[0] = '\0';
    last_params[0] = '\0';
    conduit_webhook_set_submit_hook(NULL);
    conduit_webhook_set_wait_hook(NULL);
    conduit_webhook_set_send_json_hook(send_json_capture);
    conduit_webhook_set_buffer_hook(mock_api_buffer_post_data);
}

void tearDown(void) {
    json_t *cap = mock_api_utils_get_captured_response();
    if (cap) {
        json_decref(cap);
    }
    mock_api_utils_reset_capture();
    cleanup_webhooks_config(&test_config.webhooks);
    conduit_webhook_set_submit_hook(NULL);
    conduit_webhook_set_wait_hook(NULL);
    conduit_webhook_set_send_json_hook(NULL);
    conduit_webhook_set_buffer_hook(NULL);
    app_config = NULL;
}

void test_method_not_allowed(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1;
    size_t sz = 0;
    void *cls = NULL;
    enum MHD_Result r = handle_conduit_webhook_request(
        conn, "/api/conduit/webhook/stripe", "GET", NULL, &sz, &cls,
        "conduit/webhook/stripe");
    TEST_ASSERT_EQUAL(MHD_YES, r);
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_METHOD_NOT_ALLOWED,
                           mock_api_utils_get_captured_status());
}

void test_unknown_hook_404(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1;
    size_t sz = 0;
    void *cls = NULL;
    seed_stripe_hook();
    mock_api_utils_set_buffer_data("{}");
    enum MHD_Result r = handle_conduit_webhook_request(
        conn, "/api/conduit/webhook/github", "POST", NULL, &sz, &cls,
        "conduit/webhook/github");
    TEST_ASSERT_EQUAL(MHD_YES, r);
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_NOT_FOUND,
                           mock_api_utils_get_captured_status());
    TEST_ASSERT_EQUAL_INT(0, submit_calls);
}

void test_disabled_503(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1;
    size_t sz = 0;
    void *cls = NULL;
    mock_api_utils_set_buffer_data("{}");
    enum MHD_Result r = handle_conduit_webhook_request(
        conn, "/api/conduit/webhook/stripe", "POST", NULL, &sz, &cls,
        "conduit/webhook/stripe");
    TEST_ASSERT_EQUAL(MHD_YES, r);
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_SERVICE_UNAVAILABLE,
                           mock_api_utils_get_captured_status());
}

void test_bad_signature_401_no_submit(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1;
    size_t sz = 0;
    void *cls = NULL;
    seed_stripe_hook();
    TEST_ASSERT_EQUAL_INT(0, setenv("HYDROGEN_TEST_WH_SECRET", "s3cret", 1));
    mock_api_utils_set_buffer_data("{\"ok\":true}");
    mock_mhd_set_lookup_result("00");
    conduit_webhook_set_submit_hook(submit_ok_hook);
    enum MHD_Result r = handle_conduit_webhook_request(
        conn, "/api/conduit/webhook/stripe", "POST", NULL, &sz, &cls,
        "conduit/webhook/stripe");
    TEST_ASSERT_EQUAL(MHD_YES, r);
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_UNAUTHORIZED,
                           mock_api_utils_get_captured_status());
    TEST_ASSERT_EQUAL_INT(0, submit_calls);
}

void test_good_signature_submits_configured_script(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1;
    size_t sz = 0;
    void *cls = NULL;
    const char *body = "{\"ok\":true}";
    const char *secret = "s3cret";
    unsigned int mac_len = 0;
    unsigned char *mac;
    char *hex;

    seed_stripe_hook();
    TEST_ASSERT_EQUAL_INT(0, setenv("HYDROGEN_TEST_WH_SECRET", secret, 1));
    mac = utils_hmac_sha256((const unsigned char *)body, strlen(body), secret,
                            strlen(secret), &mac_len);
    TEST_ASSERT_NOT_NULL(mac);
    hex = conduit_webhook_bytes_to_hex(mac, mac_len);
    TEST_ASSERT_NOT_NULL(hex);

    mock_api_utils_set_buffer_data(body);
    mock_mhd_set_lookup_result(hex);
    conduit_webhook_set_submit_hook(submit_ok_hook);
    conduit_webhook_set_wait_hook(wait_completed_hook);

    enum MHD_Result r = handle_conduit_webhook_request(
        conn, "/api/conduit/webhook/stripe", "POST", NULL, &sz, &cls,
        "conduit/webhook/stripe");
    TEST_ASSERT_EQUAL(MHD_YES, r);
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_OK, mock_api_utils_get_captured_status());
    TEST_ASSERT_EQUAL_INT(1, submit_calls);
    TEST_ASSERT_EQUAL_STRING("Stripe.Webhook", last_script);
    TEST_ASSERT_NOT_NULL(strstr(last_params, "\"hook\":\"stripe\""));
    TEST_ASSERT_NOT_NULL(strstr(last_params, "\\\"ok\\\":true"));
    TEST_ASSERT_NULL(strstr(last_params, "_hydrogen"));

    free(hex);
    free(mac);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_method_not_allowed);
    RUN_TEST(test_unknown_hook_404);
    RUN_TEST(test_disabled_503);
    RUN_TEST(test_bad_signature_401_no_submit);
    RUN_TEST(test_good_signature_submits_configured_script);
    return UNITY_END();
}
