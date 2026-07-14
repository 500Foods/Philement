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

void test_queue_get_by_idempotency_builds_correct_params(void);
void test_queue_mark_sent_builds_correct_params(void);
void test_queue_mark_failed_builds_correct_params(void);
void test_queue_reschedule_builds_correct_params(void);
void test_template_list_active_has_no_params(void);
void test_template_insert_builds_correct_params(void);
void test_template_update_builds_correct_params(void);
void test_template_soft_delete_builds_correct_params(void);
void test_event_insert_builds_correct_params(void);
void test_event_list_pending_has_no_params(void);
void test_event_mark_processed_builds_correct_params(void);
void test_event_mark_suppressed_builds_correct_params(void);
void test_otp_insert_builds_correct_params(void);
void test_otp_get_active_builds_correct_params(void);
void test_otp_consume_builds_correct_params(void);
void test_otp_increment_attempts_builds_correct_params(void);
void test_otp_expire_old_builds_correct_params(void);
void test_otp_get_by_id_builds_correct_params(void);
void test_otp_mark_max_attempts_builds_correct_params(void);
void test_route_get_by_sender_domain_builds_correct_params(void);
void test_route_list_active_has_no_params(void);
void test_route_insert_builds_correct_params(void);
void test_route_update_builds_correct_params(void);
void test_route_soft_delete_builds_correct_params(void);
void test_cleanup_events_builds_correct_params(void);
void test_cleanup_attempts_builds_correct_params(void);
void test_cleanup_otp_builds_correct_params(void);
void test_role_get_by_name_builds_correct_params(void);

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

// Helper: load the captured params JSON into a json_t (caller must decref).
static json_t* load_captured_params(void) {
    if (!g_captured_params_json) {
        return NULL;
    }
    json_error_t err;
    return json_loads(g_captured_params_json, 0, &err);
}

// Helper: fetch a STRING param value from the captured params.
// Copies the value into a static buffer so the JSON can be released safely.
static char g_captured_buf[1024];
static const char* captured_string(const char* name) {
    json_t* root = load_captured_params();
    if (!root) {
        return NULL;
    }
    json_t* string_obj = json_object_get(root, "STRING");
    const char* value = string_obj
        ? json_string_value(json_object_get(string_obj, name))
        : NULL;
    if (value) {
        strncpy(g_captured_buf, value, sizeof(g_captured_buf) - 1);
        g_captured_buf[sizeof(g_captured_buf) - 1] = '\0';
    } else {
        g_captured_buf[0] = '\0';
    }
    json_decref(root);
    return value ? g_captured_buf : NULL;
}

// Helper: fetch an INTEGER param value from the captured params.
static long long captured_integer(const char* name) {
    json_t* root = load_captured_params();
    if (!root) {
        return -1;
    }
    json_t* integer_obj = json_object_get(root, "INTEGER");
    long long value = integer_obj
        ? json_integer_value(json_object_get(integer_obj, name))
        : -1;
    json_decref(root);
    return value;
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
// Queue helper parameter JSON tests (remaining functions)
// ----------------------------------------------------------------------------

void test_queue_get_by_idempotency_builds_correct_params(void) {
    MailRelayRepoQueueGetByIdempotency params = { .idempotency_key = "idem-xyz" };
    bool result = mailrelay_repo_queue_get_by_idempotency(&params, mock_callback, NULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_QUEUE_GET_BY_IDEMPOTENCY, g_captured_query_ref);
    TEST_ASSERT_EQUAL_STRING("idem-xyz", captured_string("IDEMPOTENCY_KEY"));
}

void test_queue_mark_sent_builds_correct_params(void) {
    MailRelayRepoQueueMarkSent params = { .queue_id = 7, .smtp_code = 250, .smtp_text = "OK" };
    bool result = mailrelay_repo_queue_mark_sent(&params, mock_callback, NULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_QUEUE_MARK_SENT, g_captured_query_ref);
    TEST_ASSERT_EQUAL_INT64(7, captured_integer("QUEUE_ID"));
    TEST_ASSERT_EQUAL_INT(250, captured_integer("SMTP_CODE"));
    TEST_ASSERT_EQUAL_STRING("OK", captured_string("SMTP_TEXT"));
}

void test_queue_mark_failed_builds_correct_params(void) {
    MailRelayRepoQueueMarkFailed params = { .queue_id = 8, .smtp_code = 550, .smtp_text = "Rejected" };
    bool result = mailrelay_repo_queue_mark_failed(&params, mock_callback, NULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_QUEUE_MARK_FAILED, g_captured_query_ref);
    TEST_ASSERT_EQUAL_INT64(8, captured_integer("QUEUE_ID"));
    TEST_ASSERT_EQUAL_INT(550, captured_integer("SMTP_CODE"));
    TEST_ASSERT_EQUAL_STRING("Rejected", captured_string("SMTP_TEXT"));
}

void test_queue_reschedule_builds_correct_params(void) {
    MailRelayRepoQueueReschedule params = {
        .queue_id = 9, .next_attempt_at = "2026-08-01T00:00:00Z", .attempts = 3
    };
    bool result = mailrelay_repo_queue_reschedule(&params, mock_callback, NULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_QUEUE_RESCHEDULE, g_captured_query_ref);
    TEST_ASSERT_EQUAL_INT64(9, captured_integer("QUEUE_ID"));
    TEST_ASSERT_EQUAL_INT(3, captured_integer("ATTEMPTS"));
    TEST_ASSERT_EQUAL_STRING("2026-08-01T00:00:00Z", captured_string("NEXT_ATTEMPT_AT"));
}

// ----------------------------------------------------------------------------
// Template helper parameter JSON tests (remaining functions)
// ----------------------------------------------------------------------------

void test_template_list_active_has_no_params(void) {
    bool result = mailrelay_repo_template_list_active(mock_callback, NULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_TEMPLATE_LIST_ACTIVE, g_captured_query_ref);
    json_t* root = load_captured_params();
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_NOT_NULL(json_object_get(root, "STRING"));
    TEST_ASSERT_NOT_NULL(json_object_get(root, "INTEGER"));
    TEST_ASSERT_NOT_NULL(json_object_get(root, "BOOLEAN"));
    json_decref(root);
}

void test_template_insert_builds_correct_params(void) {
    MailRelayRepoTemplateInsert params = {
        .template_key = "mail.welcome",
        .name = "Welcome",
        .status_a64 = 1,
        .subject_template = "Hi {{name}}",
        .text_template = "Hello",
        .html_template = "<p>Hello</p>",
        .collection = "default"
    };
    bool result = mailrelay_repo_template_insert(&params, mock_callback, NULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_TEMPLATE_INSERT, g_captured_query_ref);
    TEST_ASSERT_EQUAL_STRING("mail.welcome", captured_string("TEMPLATE_KEY"));
    TEST_ASSERT_EQUAL_STRING("Welcome", captured_string("NAME"));
    TEST_ASSERT_EQUAL_INT(1, captured_integer("STATUS_A64"));
    TEST_ASSERT_EQUAL_STRING("Hi {{name}}", captured_string("SUBJECT_TEMPLATE"));
    TEST_ASSERT_EQUAL_STRING("Hello", captured_string("TEXT_TEMPLATE"));
    TEST_ASSERT_EQUAL_STRING("<p>Hello</p>", captured_string("HTML_TEMPLATE"));
    TEST_ASSERT_EQUAL_STRING("default", captured_string("COLLECTION"));
}

void test_template_update_builds_correct_params(void) {
    MailRelayRepoTemplateUpdate params = {
        .template_key = "mail.welcome",
        .name = "Welcome v2",
        .status_a64 = 0,
        .subject_template = "Hello {{name}}",
        .text_template = "Hi there",
        .html_template = "<p>Hi</p>",
        .collection = "marketing"
    };
    bool result = mailrelay_repo_template_update(&params, mock_callback, NULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_TEMPLATE_UPDATE, g_captured_query_ref);
    TEST_ASSERT_EQUAL_STRING("mail.welcome", captured_string("TEMPLATE_KEY"));
    TEST_ASSERT_EQUAL_STRING("Welcome v2", captured_string("NAME"));
    TEST_ASSERT_EQUAL_INT(0, captured_integer("STATUS_A64"));
    TEST_ASSERT_EQUAL_STRING("Hello {{name}}", captured_string("SUBJECT_TEMPLATE"));
    TEST_ASSERT_EQUAL_STRING("Hi there", captured_string("TEXT_TEMPLATE"));
    TEST_ASSERT_EQUAL_STRING("<p>Hi</p>", captured_string("HTML_TEMPLATE"));
    TEST_ASSERT_EQUAL_STRING("marketing", captured_string("COLLECTION"));
}

void test_template_soft_delete_builds_correct_params(void) {
    MailRelayRepoTemplateSoftDelete params = { .template_key = "mail.old" };
    bool result = mailrelay_repo_template_soft_delete(&params, mock_callback, NULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_TEMPLATE_SOFT_DELETE, g_captured_query_ref);
    TEST_ASSERT_EQUAL_STRING("mail.old", captured_string("TEMPLATE_KEY"));
}

// ----------------------------------------------------------------------------
// Event helper parameter JSON tests
// ----------------------------------------------------------------------------

void test_event_insert_builds_correct_params(void) {
    MailRelayRepoEventInsert params = {
        .event_key = "evt-1",
        .status_a65 = 1,
        .template_key = "mail.welcome",
        .from_addr = "from@example.com",
        .reply_to = "reply@example.com",
        .recipients_json = "[\"to@example.com\"]",
        .subject = "Subject",
        .body_text = "Text",
        .body_html = "<p>HTML</p>",
        .headers_json = "{}",
        .params_json = "{\"a\":1}",
        .debounce_key = "deb-1",
        .idempotency_key = "idem-1",
        .priority = 5
    };
    bool result = mailrelay_repo_event_insert(&params, mock_callback, NULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_EVENT_INSERT, g_captured_query_ref);
    TEST_ASSERT_EQUAL_STRING("evt-1", captured_string("EVENT_KEY"));
    TEST_ASSERT_EQUAL_INT(1, captured_integer("STATUS_A65"));
    TEST_ASSERT_EQUAL_STRING("mail.welcome", captured_string("TEMPLATE_KEY"));
    TEST_ASSERT_EQUAL_STRING("from@example.com", captured_string("FROM_ADDR"));
    TEST_ASSERT_EQUAL_STRING("reply@example.com", captured_string("REPLY_TO"));
    TEST_ASSERT_EQUAL_STRING("[\"to@example.com\"]", captured_string("RECIPIENTS_JSON"));
    TEST_ASSERT_EQUAL_STRING("Subject", captured_string("SUBJECT"));
    TEST_ASSERT_EQUAL_STRING("Text", captured_string("BODY_TEXT"));
    TEST_ASSERT_EQUAL_STRING("<p>HTML</p>", captured_string("BODY_HTML"));
    TEST_ASSERT_EQUAL_STRING("{}", captured_string("HEADERS_JSON"));
    TEST_ASSERT_EQUAL_STRING("{\"a\":1}", captured_string("PARAMS_JSON"));
    TEST_ASSERT_EQUAL_STRING("deb-1", captured_string("DEBOUNCE_KEY"));
    TEST_ASSERT_EQUAL_STRING("idem-1", captured_string("IDEMPOTENCY_KEY"));
    TEST_ASSERT_EQUAL_INT(5, captured_integer("PRIORITY"));
}

void test_event_list_pending_has_no_params(void) {
    bool result = mailrelay_repo_event_list_pending(mock_callback, NULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_EVENT_LIST_PENDING, g_captured_query_ref);
    json_t* root = load_captured_params();
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_NOT_NULL(json_object_get(root, "STRING"));
    TEST_ASSERT_NOT_NULL(json_object_get(root, "INTEGER"));
    TEST_ASSERT_NOT_NULL(json_object_get(root, "BOOLEAN"));
    json_decref(root);
}

void test_event_mark_processed_builds_correct_params(void) {
    MailRelayRepoEventMarkProcessed params = {
        .event_id = 11, .status_a65 = 2, .queue_id = 22
    };
    bool result = mailrelay_repo_event_mark_processed(&params, mock_callback, NULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_EVENT_MARK_PROCESSED, g_captured_query_ref);
    TEST_ASSERT_EQUAL_INT64(11, captured_integer("EVENT_ID"));
    TEST_ASSERT_EQUAL_INT(2, captured_integer("STATUS_A65"));
    TEST_ASSERT_EQUAL_INT64(22, captured_integer("QUEUE_ID"));
}

void test_event_mark_suppressed_builds_correct_params(void) {
    MailRelayRepoEventMarkSuppressed params = { .event_id = 33 };
    bool result = mailrelay_repo_event_mark_suppressed(&params, mock_callback, NULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_EVENT_MARK_SUPPRESSED, g_captured_query_ref);
    TEST_ASSERT_EQUAL_INT64(33, captured_integer("EVENT_ID"));
}

// ----------------------------------------------------------------------------
// OTP helper parameter JSON tests
// ----------------------------------------------------------------------------

void test_otp_insert_builds_correct_params(void) {
    MailRelayRepoOtpInsert params = {
        .code_hash = "abc123",
        .email = "user@example.com",
        .account_id = 5,
        .purpose_a66 = 1,
        .status_a67 = 0,
        .expiry_at = "2026-09-01T00:00:00Z",
        .max_attempts = 3
    };
    bool result = mailrelay_repo_otp_insert(&params, mock_callback, NULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_OTP_INSERT, g_captured_query_ref);
    TEST_ASSERT_EQUAL_STRING("abc123", captured_string("CODE_HASH"));
    TEST_ASSERT_EQUAL_STRING("user@example.com", captured_string("EMAIL"));
    TEST_ASSERT_EQUAL_INT64(5, captured_integer("ACCOUNT_ID"));
    TEST_ASSERT_EQUAL_INT(1, captured_integer("PURPOSE_A66"));
    TEST_ASSERT_EQUAL_INT(0, captured_integer("STATUS_A67"));
    TEST_ASSERT_EQUAL_STRING("2026-09-01T00:00:00Z", captured_string("EXPIRY_AT"));
    TEST_ASSERT_EQUAL_INT(3, captured_integer("MAX_ATTEMPTS"));
}

void test_otp_get_active_builds_correct_params(void) {
    MailRelayRepoOtpGetActive params = { .email = "user@example.com", .purpose_a66 = 2 };
    bool result = mailrelay_repo_otp_get_active(&params, mock_callback, NULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_OTP_GET_ACTIVE, g_captured_query_ref);
    TEST_ASSERT_EQUAL_STRING("user@example.com", captured_string("EMAIL"));
    TEST_ASSERT_EQUAL_INT(2, captured_integer("PURPOSE_A66"));
}

void test_otp_consume_builds_correct_params(void) {
    MailRelayRepoOtpConsume params = { .otp_id = 44 };
    bool result = mailrelay_repo_otp_consume(&params, mock_callback, NULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_OTP_CONSUME, g_captured_query_ref);
    TEST_ASSERT_EQUAL_INT64(44, captured_integer("OTP_ID"));
}

void test_otp_increment_attempts_builds_correct_params(void) {
    MailRelayRepoOtpIncrementAttempts params = { .otp_id = 55 };
    bool result = mailrelay_repo_otp_increment_attempts(&params, mock_callback, NULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_OTP_INCREMENT_ATTEMPTS, g_captured_query_ref);
    TEST_ASSERT_EQUAL_INT64(55, captured_integer("OTP_ID"));
}

void test_otp_expire_old_builds_correct_params(void) {
    MailRelayRepoOtpExpireOld params = { .expiry_cutoff_at = "2026-10-01T00:00:00Z" };
    bool result = mailrelay_repo_otp_expire_old(&params, mock_callback, NULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_OTP_EXPIRE_OLD, g_captured_query_ref);
    TEST_ASSERT_EQUAL_STRING("2026-10-01T00:00:00Z", captured_string("EXPIRY_CUTOFF_AT"));
}

void test_otp_get_by_id_builds_correct_params(void) {
    MailRelayRepoOtpGetById params = { .otp_id = 66 };
    bool result = mailrelay_repo_otp_get_by_id(&params, mock_callback, NULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_OTP_GET_BY_ID, g_captured_query_ref);
    TEST_ASSERT_EQUAL_INT64(66, captured_integer("OTP_ID"));
}

void test_otp_mark_max_attempts_builds_correct_params(void) {
    MailRelayRepoOtpMarkMaxAttempts params = { .otp_id = 77 };
    bool result = mailrelay_repo_otp_mark_max_attempts(&params, mock_callback, NULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_OTP_MARK_MAX_ATTEMPTS, g_captured_query_ref);
    TEST_ASSERT_EQUAL_INT64(77, captured_integer("OTP_ID"));
}

// ----------------------------------------------------------------------------
// Route helper parameter JSON tests
// ----------------------------------------------------------------------------

void test_route_get_by_sender_domain_builds_correct_params(void) {
    MailRelayRepoRouteGetBySenderDomain params = { .sender_domain = "example.com" };
    bool result = mailrelay_repo_route_get_by_sender_domain(&params, mock_callback, NULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_ROUTE_GET_BY_SENDER_DOMAIN, g_captured_query_ref);
    TEST_ASSERT_EQUAL_STRING("example.com", captured_string("SENDER_DOMAIN"));
}

void test_route_list_active_has_no_params(void) {
    bool result = mailrelay_repo_route_list_active(mock_callback, NULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_ROUTE_LIST_ACTIVE, g_captured_query_ref);
    json_t* root = load_captured_params();
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_NOT_NULL(json_object_get(root, "STRING"));
    TEST_ASSERT_NOT_NULL(json_object_get(root, "INTEGER"));
    TEST_ASSERT_NOT_NULL(json_object_get(root, "BOOLEAN"));
    json_decref(root);
}

void test_route_insert_builds_correct_params(void) {
    MailRelayRepoRouteInsert params = {
        .status_a68 = 1,
        .source_network = "10.0.0.0/8",
        .sender_domain = "example.com",
        .sender_pattern = "*@example.com",
        .recipient_domain = "internal.com",
        .recipient_pattern = "*@internal.com",
        .auth_required = true,
        .require_tls = true,
        .template_key = "mail.welcome",
        .rewrite_from = "no-reply@example.com",
        .rewrite_to = "real@example.com",
        .add_recipients_json = "[]",
        .priority = 10,
        .sort_seq = 5
    };
    bool result = mailrelay_repo_route_insert(&params, mock_callback, NULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_ROUTE_INSERT, g_captured_query_ref);
    TEST_ASSERT_EQUAL_INT(1, captured_integer("STATUS_A68"));
    TEST_ASSERT_EQUAL_STRING("10.0.0.0/8", captured_string("SOURCE_NETWORK"));
    TEST_ASSERT_EQUAL_STRING("example.com", captured_string("SENDER_DOMAIN"));
    TEST_ASSERT_EQUAL_STRING("*@example.com", captured_string("SENDER_PATTERN"));
    TEST_ASSERT_EQUAL_STRING("internal.com", captured_string("RECIPIENT_DOMAIN"));
    TEST_ASSERT_EQUAL_STRING("*@internal.com", captured_string("RECIPIENT_PATTERN"));
    TEST_ASSERT_EQUAL_INT(1, captured_integer("AUTH_REQUIRED"));
    TEST_ASSERT_EQUAL_INT(1, captured_integer("REQUIRE_TLS"));
    TEST_ASSERT_EQUAL_STRING("mail.welcome", captured_string("TEMPLATE_KEY"));
    TEST_ASSERT_EQUAL_STRING("no-reply@example.com", captured_string("REWRITE_FROM"));
    TEST_ASSERT_EQUAL_STRING("real@example.com", captured_string("REWRITE_TO"));
    TEST_ASSERT_EQUAL_STRING("[]", captured_string("ADD_RECIPIENTS_JSON"));
    TEST_ASSERT_EQUAL_INT(10, captured_integer("PRIORITY"));
    TEST_ASSERT_EQUAL_INT(5, captured_integer("SORT_SEQ"));
}

void test_route_update_builds_correct_params(void) {
    MailRelayRepoRouteUpdate params = {
        .route_id = 88,
        .status_a68 = 0,
        .source_network = "192.168.0.0/16",
        .sender_domain = "example.org",
        .sender_pattern = "*@example.org",
        .recipient_domain = "internal.org",
        .recipient_pattern = "*@internal.org",
        .auth_required = false,
        .require_tls = false,
        .template_key = "mail.other",
        .rewrite_from = "noreply@example.org",
        .rewrite_to = "real@example.org",
        .add_recipients_json = "[]",
        .priority = 20,
        .sort_seq = 1
    };
    bool result = mailrelay_repo_route_update(&params, mock_callback, NULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_ROUTE_UPDATE, g_captured_query_ref);
    TEST_ASSERT_EQUAL_INT64(88, captured_integer("ROUTE_ID"));
    TEST_ASSERT_EQUAL_INT(0, captured_integer("STATUS_A68"));
    TEST_ASSERT_EQUAL_STRING("192.168.0.0/16", captured_string("SOURCE_NETWORK"));
    TEST_ASSERT_EQUAL_STRING("example.org", captured_string("SENDER_DOMAIN"));
    TEST_ASSERT_EQUAL_STRING("*@example.org", captured_string("SENDER_PATTERN"));
    TEST_ASSERT_EQUAL_STRING("internal.org", captured_string("RECIPIENT_DOMAIN"));
    TEST_ASSERT_EQUAL_STRING("*@internal.org", captured_string("RECIPIENT_PATTERN"));
    TEST_ASSERT_EQUAL_INT(0, captured_integer("AUTH_REQUIRED"));
    TEST_ASSERT_EQUAL_INT(0, captured_integer("REQUIRE_TLS"));
    TEST_ASSERT_EQUAL_STRING("mail.other", captured_string("TEMPLATE_KEY"));
    TEST_ASSERT_EQUAL_STRING("noreply@example.org", captured_string("REWRITE_FROM"));
    TEST_ASSERT_EQUAL_STRING("real@example.org", captured_string("REWRITE_TO"));
    TEST_ASSERT_EQUAL_STRING("[]", captured_string("ADD_RECIPIENTS_JSON"));
    TEST_ASSERT_EQUAL_INT(20, captured_integer("PRIORITY"));
    TEST_ASSERT_EQUAL_INT(1, captured_integer("SORT_SEQ"));
}

void test_route_soft_delete_builds_correct_params(void) {
    MailRelayRepoRouteSoftDelete params = { .route_id = 99 };
    bool result = mailrelay_repo_route_soft_delete(&params, mock_callback, NULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_ROUTE_SOFT_DELETE, g_captured_query_ref);
    TEST_ASSERT_EQUAL_INT64(99, captured_integer("ROUTE_ID"));
}

// ----------------------------------------------------------------------------
// Cleanup helper parameter JSON tests (remaining functions)
// ----------------------------------------------------------------------------

void test_cleanup_events_builds_correct_params(void) {
    MailRelayRepoCleanupEvents params = { .cutoff_at = "2026-11-01T00:00:00Z" };
    bool result = mailrelay_repo_cleanup_events(&params, mock_callback, NULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_CLEANUP_EVENTS, g_captured_query_ref);
    TEST_ASSERT_EQUAL_STRING("2026-11-01T00:00:00Z", captured_string("CUTOFF_AT"));
}

void test_cleanup_attempts_builds_correct_params(void) {
    MailRelayRepoCleanupAttempts params = { .cutoff_at = "2026-12-01T00:00:00Z" };
    bool result = mailrelay_repo_cleanup_attempts(&params, mock_callback, NULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_CLEANUP_ATTEMPTS, g_captured_query_ref);
    TEST_ASSERT_EQUAL_STRING("2026-12-01T00:00:00Z", captured_string("CUTOFF_AT"));
}

void test_cleanup_otp_builds_correct_params(void) {
    MailRelayRepoCleanupOtp params = { .cutoff_at = "2027-01-01T00:00:00Z" };
    bool result = mailrelay_repo_cleanup_otp(&params, mock_callback, NULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_CLEANUP_OTP, g_captured_query_ref);
    TEST_ASSERT_EQUAL_STRING("2027-01-01T00:00:00Z", captured_string("CUTOFF_AT"));
}

// ----------------------------------------------------------------------------
// Role helper parameter JSON tests
// ----------------------------------------------------------------------------

void test_role_get_by_name_builds_correct_params(void) {
    bool result = mailrelay_repo_role_get_by_name("admin", mock_callback, NULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_ROLE_GET_BY_NAME, g_captured_query_ref);
    TEST_ASSERT_EQUAL_STRING("admin", captured_string("ROLENAME"));
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

    RUN_TEST(test_queue_get_by_idempotency_builds_correct_params);
    RUN_TEST(test_queue_mark_sent_builds_correct_params);
    RUN_TEST(test_queue_mark_failed_builds_correct_params);
    RUN_TEST(test_queue_reschedule_builds_correct_params);
    RUN_TEST(test_template_list_active_has_no_params);
    RUN_TEST(test_template_insert_builds_correct_params);
    RUN_TEST(test_template_update_builds_correct_params);
    RUN_TEST(test_template_soft_delete_builds_correct_params);
    RUN_TEST(test_event_insert_builds_correct_params);
    RUN_TEST(test_event_list_pending_has_no_params);
    RUN_TEST(test_event_mark_processed_builds_correct_params);
    RUN_TEST(test_event_mark_suppressed_builds_correct_params);
    RUN_TEST(test_otp_insert_builds_correct_params);
    RUN_TEST(test_otp_get_active_builds_correct_params);
    RUN_TEST(test_otp_consume_builds_correct_params);
    RUN_TEST(test_otp_increment_attempts_builds_correct_params);
    RUN_TEST(test_otp_expire_old_builds_correct_params);
    RUN_TEST(test_otp_get_by_id_builds_correct_params);
    RUN_TEST(test_otp_mark_max_attempts_builds_correct_params);
    RUN_TEST(test_route_get_by_sender_domain_builds_correct_params);
    RUN_TEST(test_route_list_active_has_no_params);
    RUN_TEST(test_route_insert_builds_correct_params);
    RUN_TEST(test_route_update_builds_correct_params);
    RUN_TEST(test_route_soft_delete_builds_correct_params);
    RUN_TEST(test_cleanup_events_builds_correct_params);
    RUN_TEST(test_cleanup_attempts_builds_correct_params);
    RUN_TEST(test_cleanup_otp_builds_correct_params);
    RUN_TEST(test_role_get_by_name_builds_correct_params);

    return UNITY_END();
}
