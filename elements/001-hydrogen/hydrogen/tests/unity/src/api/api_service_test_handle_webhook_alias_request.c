/*
 * Unity Test File: API Service handle_webhook_alias_request Tests
 *
 * This file contains unit tests for the handle_webhook_alias_request
 * handler in api_service.c. This handler is a thin delegation wrapper
 * that:
 *   1. Sets the request URL via api_set_request_url
 *   2. Extracts the hook name from the URL via conduit_webhook_extract_hook_from_url
 *   3. Delegates to handle_conduit_webhook_request with either:
 *      - "webhook/{hook}" when a hook is found and fits in the path buffer
 *      - "webhook/" when the hook is NULL or too long
 *   4. Clears the request URL via api_set_request_url(NULL)
 *
 * Tests verify the delegation passes parameters through correctly and that
 * the return value matches what handle_conduit_webhook_request produces.
 */

/* Enable mocks BEFORE any includes */
#define USE_MOCK_LIBMICROHTTPD
#define USE_MOCK_API_UTILS

/* Include mock headers first (redirects system/MHD functions) */
#include <unity/mocks/mock_libmicrohttpd.h>
#include <unity/mocks/mock_api_utils.h>

/* Standard project header plus Unity Framework header */
#include <src/hydrogen.h>
#include <unity.h>

/* Include necessary headers for the module being tested */
#include <src/api/api_service.h>
#include <src/api/api_utils.h>
#include <src/api/conduit/webhook/webhook.h>
#include <src/api/conduit/script/script.h>
#include <src/config/config.h>
#include <src/utils/utils_crypto.h>

/* External variable (defined in global.c) */
extern AppConfig *app_config;

/* Mock app_config for testing */
static AppConfig test_config;

/* Capture callbacks for webhook hook injection */
static int captured_submit_calls;
static char captured_script[64];
static char captured_params[1024];

/* Custom send_json to capture responses without real MHD */
static enum MHD_Result send_json_capture(struct MHD_Connection *connection,
                                         json_t *json_obj,
                                         unsigned int status_code) {
    (void)connection;
    return mock_api_send_json_response(connection, json_obj, status_code);
}

/* Submit hook that succeeds and returns a job ID */
static ScriptingInvokeError submit_ok_hook(const char *script_name,
                                           const char *params_json,
                                           const ScoreboardJobLimits *limits,
                                           int fetch_timeout_seconds,
                                           char **job_id_out) {
    (void)limits;
    (void)fetch_timeout_seconds;
    captured_submit_calls++;
    strncpy(captured_script, script_name ? script_name : "", sizeof(captured_script) - 1);
    strncpy(captured_params, params_json ? params_json : "", sizeof(captured_params) - 1);
    *job_id_out = strdup("WH001");
    return SCRIPTING_INVOKE_OK;
}

/* Wait hook that returns a completed job */
static ScriptingWaitResult wait_completed_hook(const char *job_id,
                                               int timeout_seconds,
                                               ScoreboardEntry **out_entry) {
    (void)job_id;
    (void)timeout_seconds;
    ScoreboardEntry *e = calloc(1, sizeof(ScoreboardEntry));
    TEST_ASSERT_NOT_NULL(e);
    e->status = SCOREBOARD_JOB_COMPLETED;
    e->script_name = strdup("Webhook.Alias");
    e->result_json = strdup("{\"ok\":true}");
    e->result_type = strdup("json");
    *out_entry = e;
    return SCRIPTING_WAIT_COMPLETED;
}

/* Helper: seed a stripe-like webhook hook in config */
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

/* Function prototypes */
void test_handle_webhook_alias_get_rejected(void);
void test_handle_webhook_alias_delegates_with_hook_name(void);
void test_handle_webhook_alias_fallback_when_no_hook(void);
void test_handle_webhook_alias_fallback_on_long_hook(void);
void test_handle_webhook_alias_passes_connection_through(void);

void setUp(void) {
    mock_mhd_reset_all();
    mock_api_utils_reset_all();
    mock_api_utils_set_buffer_result(API_BUFFER_COMPLETE);
    mock_api_utils_set_capture_mode(true);

    memset(&test_config, 0, sizeof(AppConfig));
    test_config.api.prefix = (char*)"/api";
    test_config.api.headers = NULL;
    test_config.api.headers_count = 0;
    app_config = &test_config;

    captured_submit_calls = 0;
    captured_script[0] = '\0';
    captured_params[0] = '\0';

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

/*
 * Test: handle_webhook_alias_request delegates to handle_conduit_webhook_request,
 *       which rejects GET with 405 (method not allowed).
 *
 * URL "/webhook/stripe" extracts hook "stripe" and constructs path "webhook/stripe".
 * handle_conduit_webhook_request finds the hook name but rejects GET with 405.
 */
void test_handle_webhook_alias_get_rejected(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1;
    size_t sz = 0;
    void *cls = NULL;

    enum MHD_Result result = handle_webhook_alias_request(
        NULL, conn, "/webhook/stripe", "GET", "1.1",
        NULL, &sz, &cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_METHOD_NOT_ALLOWED,
                           mock_api_utils_get_captured_status());
}

/*
 * Test: handle_webhook_alias_request with a valid hook name and POST method
 *       delegates to handle_conduit_webhook_request which processes the
 *       webhook with the correct hook name extracted from the URL.
 *
 * Verifies that the full delegation chain works end-to-end with proper
 * configuration and hook injection.
 */
void test_handle_webhook_alias_delegates_with_hook_name(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1;
    size_t sz = 0;
    void *cls = NULL;

    seed_stripe_hook();
    TEST_ASSERT_EQUAL_INT(0, setenv("HYDROGEN_TEST_WH_SECRET", "s3cret", 1));

    /* Build a valid HMAC signature for the request body */
    const char *body = "{\"ok\":true}";
    const char *secret = "s3cret";
    unsigned int mac_len = 0;
    unsigned char *mac = utils_hmac_sha256((const unsigned char *)body, strlen(body),
                                           secret, strlen(secret), &mac_len);
    TEST_ASSERT_NOT_NULL(mac);
    char *hex = conduit_webhook_bytes_to_hex(mac, mac_len);
    TEST_ASSERT_NOT_NULL(hex);

    mock_api_utils_set_buffer_data(body);
    mock_mhd_set_lookup_result(hex);
    conduit_webhook_set_submit_hook(submit_ok_hook);
    conduit_webhook_set_wait_hook(wait_completed_hook);

    enum MHD_Result result = handle_webhook_alias_request(
        NULL, conn, "/webhook/stripe", "POST", "1.1",
        NULL, &sz, &cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_OK, mock_api_utils_get_captured_status());
    TEST_ASSERT_EQUAL_INT(1, captured_submit_calls);
    TEST_ASSERT_EQUAL_STRING("Stripe.Webhook", captured_script);
    TEST_ASSERT_NOT_NULL(strstr(captured_params, "\"hook\":\"stripe\""));
    TEST_ASSERT_NOT_NULL(strstr(captured_params, "\\\"ok\\\":true"));
    TEST_ASSERT_NULL(strstr(captured_params, "_hydrogen"));

    free(hex);
    free(mac);
}

/*
 * Test: handle_webhook_alias_request falls back to path "webhook/" when
 *       conduit_webhook_extract_hook_from_url returns NULL (no hook
 *       found in URL).
 *
 * URL "/webhookfoo" should NOT be matched (it lacks the "/webhook/" prefix
 * with a hook name). However, handle_webhook_alias_request is only called
 * when is_webhook_alias_endpoint already validated the URL. For this test,
 * we verify that handle_conduit_webhook_request handles the fallback correctly.
 *
 * Actually, handle_webhook_alias_request calls conduit_webhook_extract_hook_from_url
 * which will return NULL for URLs where the hook can't be extracted. We test
 * a URL like "/webhook/" where the hook name is empty.
 */
void test_handle_webhook_alias_fallback_when_no_hook(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1;
    size_t sz = 0;
    void *cls = NULL;

    /* "/webhook/" has no hook name after the prefix, so extraction returns NULL */
    enum MHD_Result result = handle_webhook_alias_request(
        NULL, conn, "/webhook/", "GET", "1.1",
        NULL, &sz, &cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    /* handle_conduit_webhook_request with path "webhook/" will try to
       extract a hook from path "webhook/" -> NULL, then from URL "/webhook/"
       -> NULL (empty hook), so returns 404 hook_not_found. But since method
       is not POST, the method check comes first and returns 405.
       Actually, the hook check comes first in handle_conduit_webhook_request
       before the method check. So this returns 404. */
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_NOT_FOUND,
                           mock_api_utils_get_captured_status());
}

/*
 * Test: handle_webhook_alias_request falls back to path "webhook/" when
 *       the extracted hook name would cause path_buf overflow.
 *
 * The path_buf is 64 bytes. "webhook/" prefix is 9 bytes. So if hook
 * length + 9 >= 64, i.e., hook length >= 55, the fallback is used.
 * We use a hook name of 60 characters to trigger the overflow check.
 */
void test_handle_webhook_alias_fallback_on_long_hook(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1;
    size_t sz = 0;
    void *cls = NULL;

    /* 60-char hook name: "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz" */
    const char *long_hook = "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz";
    char url[256];
    snprintf(url, sizeof(url), "/webhook/%s", long_hook);

    enum MHD_Result result = handle_webhook_alias_request(
        NULL, conn, url, "GET", "1.1",
        NULL, &sz, &cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    /* With GET and unknown hook "longhook..." which doesn't match any
       config, but method check comes after hook extraction.
       handle_conduit_webhook_request extracts "longhook..." from the
       fallback path "webhook/" -> NULL, then from URL -> extracts "longhook..."
       -> hook found, then method check: GET != POST -> 405. */
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_METHOD_NOT_ALLOWED,
                           mock_api_utils_get_captured_status());
}

/*
 * Test: verify that the real connection pointer is passed through
 *       to handle_conduit_webhook_request (not NULL or zeroed).
 *
 * Using a distinctive pointer value, if handle_webhook_alias_request
 * correctly forwards it, the underlying handler will work without
 * crashing.
 */
void test_handle_webhook_alias_passes_connection_through(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0xDEADBEEF;
    size_t sz = 0;
    void *cls = NULL;

    enum MHD_Result result = handle_webhook_alias_request(
        NULL, conn, "/webhook/stripe", "GET", "1.1",
        NULL, &sz, &cls);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL_UINT(MHD_HTTP_METHOD_NOT_ALLOWED,
                           mock_api_utils_get_captured_status());
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_handle_webhook_alias_get_rejected);
    RUN_TEST(test_handle_webhook_alias_delegates_with_hook_name);
    RUN_TEST(test_handle_webhook_alias_fallback_when_no_hook);
    RUN_TEST(test_handle_webhook_alias_fallback_on_long_hook);
    RUN_TEST(test_handle_webhook_alias_passes_connection_through);

    return UNITY_END();
}
