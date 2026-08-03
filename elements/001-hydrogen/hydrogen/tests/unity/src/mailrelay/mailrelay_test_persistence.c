/*
 * Mail Relay persistence and startup recovery unit tests.
 *
 * Tests that the Mail Relay runtime persists pending rows when persistence is
 * enabled, fails fast when persistence fails, and recovers stale 'sending'
 * rows on startup.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/mailrelay/mailrelay.h>
#include <src/mailrelay/mailrelay_internal.h>
#include <src/mailrelay/mailrelay_message.h>
#include <src/mailrelay/mailrelay_queue.h>
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

void test_recovery_skips_when_persistence_disabled(void);
void test_recovery_runs_when_persistence_enabled(void);
void test_enqueue_to_queue_skips_persist_when_disabled(void);
void test_enqueue_to_queue_persists_when_enabled(void);
void test_enqueue_to_queue_fails_when_persist_fails(void);
void test_enqueue_persists_direct_path(void);

static AppConfig* g_saved_app_config = NULL;
static AppConfig g_test_config;

static int g_captured_query_ref;
static char* g_captured_params_json;
static bool g_executor_called;
static MailRelayRepoStatus g_executor_status;
static int g_executor_affected_rows;

static void reset_mock_state(void) {
    g_captured_query_ref = -1;
    free(g_captured_params_json);
    g_captured_params_json = NULL;
    g_executor_called = false;
    g_executor_status = MAILRELAY_REPO_OK;
    g_executor_affected_rows = 0;
}

static bool mock_executor(int query_ref, const char* params_json,
                          mailrelay_repo_callback_fn callback, void* user_data) {
    g_captured_query_ref = query_ref;
    free(g_captured_params_json);
    g_captured_params_json = params_json ? strdup(params_json) : NULL;
    g_executor_called = true;

    json_t* data = NULL;
    if (query_ref == MAILRELAY_QREF_QUEUE_INSERT) {
        data = json_object();
        if (data) {
            json_object_set_new(data, "queue_id", json_integer(42));
        }
    }

    MailRelayRepoResult result = {
        .status = g_executor_status,
        .error_message = NULL,
        .data = data,
        .affected_rows = g_executor_affected_rows
    };
    if (callback) {
        callback(&result, user_data);
    }
    if (data) {
        json_decref(data);
    }
    return g_executor_status == MAILRELAY_REPO_OK;
}

static void setup_config_persist_enabled(void) {
    memset(&g_test_config, 0, sizeof(g_test_config));
    g_test_config.mail_relay.Enabled = true;
    g_test_config.mail_relay.Queue.Persist = true;
    g_test_config.mail_relay.Database = (char*)"acuranzo";
    g_test_config.mail_relay.Queue.StaleTimeoutSeconds = 300;
    g_saved_app_config = app_config;
    app_config = &g_test_config;
}

static void setup_config_persist_disabled(void) {
    memset(&g_test_config, 0, sizeof(g_test_config));
    g_test_config.mail_relay.Enabled = true;
    g_test_config.mail_relay.Queue.Persist = false;
    g_saved_app_config = app_config;
    app_config = &g_test_config;
}

void setUp(void) {
    reset_mock_state();
    mailrelay_repo_set_executor(mock_executor);
}

void tearDown(void) {
    reset_mock_state();
    mailrelay_repo_set_executor(NULL);
    if (g_saved_app_config) {
        app_config = g_saved_app_config;
        g_saved_app_config = NULL;
    }
    if (mailrelay_runtime_is_initialized()) {
        mailrelay_shutdown();
    }
}

// ----------------------------------------------------------------------------
// Startup recovery tests
// ----------------------------------------------------------------------------

void test_recovery_skips_when_persistence_disabled(void) {
    setup_config_persist_disabled();

    bool result = mailrelay_recover_stale_sending_rows();

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(g_executor_called);
}

void test_recovery_runs_when_persistence_enabled(void) {
    setup_config_persist_enabled();
    g_executor_affected_rows = 3;

    bool result = mailrelay_recover_stale_sending_rows();

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(g_executor_called);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_QUEUE_RECOVER_STALE, g_captured_query_ref);

    json_error_t err;
    json_t* root = json_loads(g_captured_params_json, 0, &err);
    TEST_ASSERT_NOT_NULL(root);
    json_t* string_obj = json_object_get(root, "STRING");
    TEST_ASSERT_NOT_NULL(json_object_get(string_obj, "STALE_BEFORE"));
    json_decref(root);
}

// ----------------------------------------------------------------------------
// Enqueue persistence tests
// ----------------------------------------------------------------------------

static void build_test_message(MailRelayMessage* msg) {
    mailrelay_message_init(msg);
    mailrelay_message_set_from(msg, "from@example.com");
    mailrelay_message_add_to(msg, "to@example.com");
    msg->subject = strdup("Test Subject");
    msg->text_body = strdup("Test body");
}

void test_enqueue_to_queue_skips_persist_when_disabled(void) {
    setup_config_persist_disabled();
    MailRelayQueue* queue = mailrelay_queue_create(10);
    TEST_ASSERT_NOT_NULL(queue);

    MailRelayMessage msg;
    build_test_message(&msg);

    MailRelayStatus status = mailrelay_enqueue_to_queue(&msg, 0, queue);

    TEST_ASSERT_EQUAL_INT(MAILRELAY_OK, status);
    TEST_ASSERT_FALSE(g_executor_called);
    TEST_ASSERT_EQUAL_INT(1, mailrelay_queue_size(queue));

    mailrelay_message_free(&msg);
    mailrelay_queue_destroy(queue);
}

void test_enqueue_to_queue_persists_when_enabled(void) {
    setup_config_persist_enabled();
    MailRelayQueue* queue = mailrelay_queue_create(10);
    TEST_ASSERT_NOT_NULL(queue);

    MailRelayMessage msg;
    build_test_message(&msg);

    MailRelayStatus status = mailrelay_enqueue_to_queue(&msg, 0, queue);

    TEST_ASSERT_EQUAL_INT(MAILRELAY_OK, status);
    TEST_ASSERT_TRUE(g_executor_called);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_QUEUE_INSERT, g_captured_query_ref);
    TEST_ASSERT_EQUAL_INT(1, mailrelay_queue_size(queue));

    json_error_t err;
    json_t* root = json_loads(g_captured_params_json, 0, &err);
    TEST_ASSERT_NOT_NULL(root);
    json_t* string_obj = json_object_get(root, "STRING");
    TEST_ASSERT_NOT_NULL(json_object_get(string_obj, "MESSAGE_UUID"));
    TEST_ASSERT_NOT_NULL(json_object_get(string_obj, "RECIPIENTS_JSON"));
    TEST_ASSERT_NOT_NULL(json_object_get(string_obj, "NEXT_ATTEMPT_AT"));
    json_decref(root);

    mailrelay_message_free(&msg);
    mailrelay_queue_destroy(queue);
}

void test_enqueue_to_queue_fails_when_persist_fails(void) {
    setup_config_persist_enabled();
    g_executor_status = MAILRELAY_REPO_QUERY_ERROR;
    MailRelayQueue* queue = mailrelay_queue_create(10);
    TEST_ASSERT_NOT_NULL(queue);

    MailRelayMessage msg;
    build_test_message(&msg);

    MailRelayStatus status = mailrelay_enqueue_to_queue(&msg, 0, queue);

    TEST_ASSERT_EQUAL_INT(MAILRELAY_PERSIST_FAILED, status);
    TEST_ASSERT_TRUE(g_executor_called);
    TEST_ASSERT_EQUAL_INT(0, mailrelay_queue_size(queue));

    mailrelay_message_free(&msg);
    mailrelay_queue_destroy(queue);
}

void test_enqueue_persists_direct_path(void) {
    setup_config_persist_enabled();
    TEST_ASSERT_TRUE(mailrelay_init());

    MailRelayMessage msg;
    build_test_message(&msg);

    MailRelayStatus status = mailrelay_enqueue(&msg, 0);

    TEST_ASSERT_EQUAL_INT(MAILRELAY_OK, status);
    TEST_ASSERT_TRUE(g_executor_called);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_QUEUE_INSERT, g_captured_query_ref);

    mailrelay_message_free(&msg);
    mailrelay_shutdown();
}

// ----------------------------------------------------------------------------
// Main
// ----------------------------------------------------------------------------

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_recovery_skips_when_persistence_disabled);
    RUN_TEST(test_recovery_runs_when_persistence_enabled);
    RUN_TEST(test_enqueue_to_queue_skips_persist_when_disabled);
    RUN_TEST(test_enqueue_to_queue_persists_when_enabled);
    RUN_TEST(test_enqueue_to_queue_fails_when_persist_fails);
    RUN_TEST(test_enqueue_persists_direct_path);

    return UNITY_END();
}
