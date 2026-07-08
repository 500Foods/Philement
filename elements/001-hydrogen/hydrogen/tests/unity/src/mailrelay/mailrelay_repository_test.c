/*
 * Mail Relay Repository unit tests.
 *
 * Tests the callback-based QueryRef execution seam and parameter JSON
 * construction for representative helpers.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/mailrelay/mailrelay_repository.h>

// Unity includes
#include <unity.h>

// Third-party includes
#include <jansson.h>

// System includes
#include <stdbool.h>
#include <string.h>

// Forward declarations
void setUp(void);
void tearDown(void);

void test_executor_seam_is_set_and_used(void);
void test_queue_insert_builds_correct_params(void);
void test_queue_get_by_uuid_builds_correct_params(void);
void test_queue_select_next_pending_has_no_params(void);
void test_queue_mark_sending_builds_correct_params(void);
void test_attempt_insert_builds_correct_params(void);
void test_template_get_by_key_builds_correct_params(void);
void test_cleanup_queue_builds_correct_params(void);
void test_queue_recover_stale_builds_correct_params(void);
void test_default_executor_returns_no_database(void);
void test_helpers_reject_null_params(void);
void test_helpers_reject_null_callback(void);

static int g_captured_query_ref;
static char* g_captured_params_json;
static bool g_executor_called;
static MailRelayRepoResult* g_captured_result;

static void reset_mock_state(void) {
    g_captured_query_ref = -1;
    free(g_captured_params_json);
    g_captured_params_json = NULL;
    g_executor_called = false;
    g_captured_result = NULL;
}

static void mock_callback(MailRelayRepoResult* result, void* user_data) {
    (void)user_data;
    g_captured_result = result;
}

static bool mock_executor(int query_ref, const char* params_json,
                          mailrelay_repo_callback_fn callback, void* user_data) {
    (void)callback;
    (void)user_data;
    g_captured_query_ref = query_ref;
    free(g_captured_params_json);
    g_captured_params_json = params_json ? strdup(params_json) : NULL;
    g_executor_called = true;

    // Simulate a successful empty result.
    MailRelayRepoResult result = {
        .status = MAILRELAY_REPO_OK,
        .error_message = NULL,
        .data = NULL,
        .affected_rows = 1
    };
    if (callback) {
        callback(&result, user_data);
    }
    return true;
}

void setUp(void) {
    reset_mock_state();
    mailrelay_repo_set_executor(mock_executor);
}

void tearDown(void) {
    reset_mock_state();
    mailrelay_repo_set_executor(NULL);
}

// ----------------------------------------------------------------------------
// Executor seam tests
// ----------------------------------------------------------------------------

void test_executor_seam_is_set_and_used(void) {
    TEST_ASSERT_EQUAL_PTR(mock_executor, mailrelay_repo_get_executor());

    MailRelayRepoQueueGetByUuid params = { .message_uuid = "test-uuid" };
    bool result = mailrelay_repo_queue_get_by_uuid(&params, mock_callback, NULL);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(g_executor_called);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_QUEUE_GET_BY_UUID, g_captured_query_ref);
}

// ----------------------------------------------------------------------------
// Queue helper parameter JSON tests
// ----------------------------------------------------------------------------

void test_queue_insert_builds_correct_params(void) {
    MailRelayRepoQueueInsert params = {
        .message_uuid = "msg-123",
        .priority = 5,
        .template_key = "mail.test",
        .from_addr = "from@example.com",
        .reply_to = "reply@example.com",
        .recipients_json = "[\"to@example.com\"]",
        .subject = "Hello",
        .body_text = "Text body",
        .body_html = "<p>HTML body</p>",
        .headers_json = "{}",
        .idempotency_key = "idem-123",
        .next_attempt_at = "2026-07-07T12:00:00Z"
    };

    bool result = mailrelay_repo_queue_insert(&params, mock_callback, NULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_QUEUE_INSERT, g_captured_query_ref);
    TEST_ASSERT_NOT_NULL(g_captured_params_json);

    json_error_t err;
    json_t* root = json_loads(g_captured_params_json, 0, &err);
    TEST_ASSERT_NOT_NULL(root);

    json_t* string_obj = json_object_get(root, "STRING");
    TEST_ASSERT_NOT_NULL(string_obj);
    TEST_ASSERT_EQUAL_STRING("msg-123", json_string_value(json_object_get(string_obj, "MESSAGE_UUID")));
    TEST_ASSERT_EQUAL_STRING("mail.test", json_string_value(json_object_get(string_obj, "TEMPLATE_KEY")));
    TEST_ASSERT_EQUAL_STRING("from@example.com", json_string_value(json_object_get(string_obj, "FROM_ADDR")));
    TEST_ASSERT_EQUAL_STRING("reply@example.com", json_string_value(json_object_get(string_obj, "REPLY_TO")));
    TEST_ASSERT_EQUAL_STRING("[\"to@example.com\"]", json_string_value(json_object_get(string_obj, "RECIPIENTS_JSON")));
    TEST_ASSERT_EQUAL_STRING("Hello", json_string_value(json_object_get(string_obj, "SUBJECT")));
    TEST_ASSERT_EQUAL_STRING("Text body", json_string_value(json_object_get(string_obj, "BODY_TEXT")));
    TEST_ASSERT_EQUAL_STRING("<p>HTML body</p>", json_string_value(json_object_get(string_obj, "BODY_HTML")));
    TEST_ASSERT_EQUAL_STRING("{}", json_string_value(json_object_get(string_obj, "HEADERS_JSON")));
    TEST_ASSERT_EQUAL_STRING("idem-123", json_string_value(json_object_get(string_obj, "IDEMPOTENCY_KEY")));
    TEST_ASSERT_EQUAL_STRING("2026-07-07T12:00:00Z", json_string_value(json_object_get(string_obj, "NEXT_ATTEMPT_AT")));

    json_t* integer_obj = json_object_get(root, "INTEGER");
    TEST_ASSERT_NOT_NULL(integer_obj);
    TEST_ASSERT_EQUAL_INT(5, json_integer_value(json_object_get(integer_obj, "PRIORITY")));

    json_decref(root);
}

void test_queue_get_by_uuid_builds_correct_params(void) {
    MailRelayRepoQueueGetByUuid params = { .message_uuid = "uuid-abc" };
    bool result = mailrelay_repo_queue_get_by_uuid(&params, mock_callback, NULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_QUEUE_GET_BY_UUID, g_captured_query_ref);

    json_error_t err;
    json_t* root = json_loads(g_captured_params_json, 0, &err);
    TEST_ASSERT_NOT_NULL(root);
    json_t* string_obj = json_object_get(root, "STRING");
    TEST_ASSERT_EQUAL_STRING("uuid-abc", json_string_value(json_object_get(string_obj, "MESSAGE_UUID")));
    json_decref(root);
}

void test_queue_select_next_pending_has_no_params(void) {
    bool result = mailrelay_repo_queue_select_next_pending(mock_callback, NULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_QUEUE_SELECT_NEXT_PENDING, g_captured_query_ref);

    json_error_t err;
    json_t* root = json_loads(g_captured_params_json, 0, &err);
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_NOT_NULL(json_object_get(root, "STRING"));
    TEST_ASSERT_NOT_NULL(json_object_get(root, "INTEGER"));
    TEST_ASSERT_NOT_NULL(json_object_get(root, "BOOLEAN"));
    json_decref(root);
}

void test_queue_mark_sending_builds_correct_params(void) {
    MailRelayRepoQueueMarkSending params = {
        .queue_id = 42,
        .instance_id = "instance-1",
        .claim_token = "token-1"
    };

    bool result = mailrelay_repo_queue_mark_sending(&params, mock_callback, NULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_QUEUE_MARK_SENDING, g_captured_query_ref);

    json_error_t err;
    json_t* root = json_loads(g_captured_params_json, 0, &err);
    TEST_ASSERT_NOT_NULL(root);
    json_t* integer_obj = json_object_get(root, "INTEGER");
    TEST_ASSERT_EQUAL_INT(42, json_integer_value(json_object_get(integer_obj, "QUEUE_ID")));
    json_t* string_obj = json_object_get(root, "STRING");
    TEST_ASSERT_EQUAL_STRING("instance-1", json_string_value(json_object_get(string_obj, "INSTANCE_ID")));
    TEST_ASSERT_EQUAL_STRING("token-1", json_string_value(json_object_get(string_obj, "CLAIM_TOKEN")));
    json_decref(root);
}

// ----------------------------------------------------------------------------
// Attempts helper tests
// ----------------------------------------------------------------------------

void test_attempt_insert_builds_correct_params(void) {
    MailRelayRepoAttemptInsert params = {
        .queue_id = 10,
        .attempt_number = 1,
        .server_index = 0,
        .success = true,
        .smtp_code = 250,
        .smtp_text = "OK",
        .duration_ms = 123,
        .error_class = "none"
    };

    bool result = mailrelay_repo_attempt_insert(&params, mock_callback, NULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_ATTEMPT_INSERT, g_captured_query_ref);

    json_error_t err;
    json_t* root = json_loads(g_captured_params_json, 0, &err);
    TEST_ASSERT_NOT_NULL(root);
    json_t* integer_obj = json_object_get(root, "INTEGER");
    TEST_ASSERT_EQUAL_INT(10, json_integer_value(json_object_get(integer_obj, "QUEUE_ID")));
    TEST_ASSERT_EQUAL_INT(1, json_integer_value(json_object_get(integer_obj, "ATTEMPT_NUMBER")));
    TEST_ASSERT_EQUAL_INT(0, json_integer_value(json_object_get(integer_obj, "SERVER_INDEX")));
    TEST_ASSERT_EQUAL_INT(1, json_integer_value(json_object_get(integer_obj, "SUCCESS")));
    TEST_ASSERT_EQUAL_INT(250, json_integer_value(json_object_get(integer_obj, "SMTP_CODE")));
    TEST_ASSERT_EQUAL_INT(123, json_integer_value(json_object_get(integer_obj, "DURATION_MS")));
    json_t* string_obj = json_object_get(root, "STRING");
    TEST_ASSERT_EQUAL_STRING("OK", json_string_value(json_object_get(string_obj, "SMTP_TEXT")));
    TEST_ASSERT_EQUAL_STRING("none", json_string_value(json_object_get(string_obj, "ERROR_CLASS")));
    json_decref(root);
}

// ----------------------------------------------------------------------------
// Template helper tests
// ----------------------------------------------------------------------------

void test_template_get_by_key_builds_correct_params(void) {
    MailRelayRepoTemplateGetByKey params = { .template_key = "mail.test" };
    bool result = mailrelay_repo_template_get_by_key(&params, mock_callback, NULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_TEMPLATE_GET_BY_KEY, g_captured_query_ref);

    json_error_t err;
    json_t* root = json_loads(g_captured_params_json, 0, &err);
    TEST_ASSERT_NOT_NULL(root);
    json_t* string_obj = json_object_get(root, "STRING");
    TEST_ASSERT_EQUAL_STRING("mail.test", json_string_value(json_object_get(string_obj, "TEMPLATE_KEY")));
    json_decref(root);
}

// ----------------------------------------------------------------------------
// Cleanup helper tests
// ----------------------------------------------------------------------------

void test_cleanup_queue_builds_correct_params(void) {
    MailRelayRepoCleanupQueue params = { .cutoff_at = "2026-07-01T00:00:00Z" };
    bool result = mailrelay_repo_cleanup_queue(&params, mock_callback, NULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_CLEANUP_QUEUE, g_captured_query_ref);

    json_error_t err;
    json_t* root = json_loads(g_captured_params_json, 0, &err);
    TEST_ASSERT_NOT_NULL(root);
    json_t* string_obj = json_object_get(root, "STRING");
    TEST_ASSERT_EQUAL_STRING("2026-07-01T00:00:00Z", json_string_value(json_object_get(string_obj, "CUTOFF_AT")));
    json_decref(root);
}

void test_queue_recover_stale_builds_correct_params(void) {
    MailRelayRepoQueueRecoverStale params = { .stale_before = "2026-07-07T10:00:00Z" };
    bool result = mailrelay_repo_queue_recover_stale(&params, mock_callback, NULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_QUEUE_RECOVER_STALE, g_captured_query_ref);

    json_error_t err;
    json_t* root = json_loads(g_captured_params_json, 0, &err);
    TEST_ASSERT_NOT_NULL(root);
    json_t* string_obj = json_object_get(root, "STRING");
    TEST_ASSERT_EQUAL_STRING("2026-07-07T10:00:00Z", json_string_value(json_object_get(string_obj, "STALE_BEFORE")));
    json_decref(root);
}

// ----------------------------------------------------------------------------
// Default executor error path test
// ----------------------------------------------------------------------------

void test_default_executor_returns_no_database(void) {
    // Ensure the default executor is installed by clearing the seam.
    mailrelay_repo_set_executor(NULL);
    TEST_ASSERT_NOT_NULL(mailrelay_repo_get_executor());

    MailRelayRepoQueueGetByUuid params = { .message_uuid = "uuid" };
    bool result = mailrelay_repo_queue_get_by_uuid(&params, mock_callback, NULL);

    // The default executor invokes the callback with NO_DATABASE and returns false.
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NOT_NULL(g_captured_result);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_REPO_NO_DATABASE, g_captured_result->status);
}

// ----------------------------------------------------------------------------
// NULL argument tests
// ----------------------------------------------------------------------------

void test_helpers_reject_null_params(void) {
    TEST_ASSERT_FALSE(mailrelay_repo_queue_insert(NULL, mock_callback, NULL));
    TEST_ASSERT_FALSE(mailrelay_repo_queue_get_by_uuid(NULL, mock_callback, NULL));
    TEST_ASSERT_FALSE(mailrelay_repo_cleanup_queue(NULL, mock_callback, NULL));
}

void test_helpers_reject_null_callback(void) {
    MailRelayRepoQueueGetByUuid params = { .message_uuid = "uuid" };
    TEST_ASSERT_FALSE(mailrelay_repo_queue_get_by_uuid(&params, NULL, NULL));
}

// ----------------------------------------------------------------------------
// Main
// ----------------------------------------------------------------------------

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_executor_seam_is_set_and_used);
    RUN_TEST(test_queue_insert_builds_correct_params);
    RUN_TEST(test_queue_get_by_uuid_builds_correct_params);
    RUN_TEST(test_queue_select_next_pending_has_no_params);
    RUN_TEST(test_queue_mark_sending_builds_correct_params);
    RUN_TEST(test_attempt_insert_builds_correct_params);
    RUN_TEST(test_template_get_by_key_builds_correct_params);
    RUN_TEST(test_cleanup_queue_builds_correct_params);
    RUN_TEST(test_queue_recover_stale_builds_correct_params);
    RUN_TEST(test_default_executor_returns_no_database);
    RUN_TEST(test_helpers_reject_null_params);
    RUN_TEST(test_helpers_reject_null_callback);

    return UNITY_END();
}
