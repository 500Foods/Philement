/*
 * Unity Test File: mailrelay_test_recover_stale_sending_rows.c
 *
 * Tests the public mailrelay_recover_stale_sending_rows() function. Covers the
 * early-return guards (no app_config, persistence disabled, missing database
 * target) and the database-backed recovery path through a mock repository
 * executor that drives both the success and failure recovery callbacks.
 */

// Project header + Unity framework
#include <src/hydrogen.h>
#include <unity.h>

// Module under test
#include <src/mailrelay/mailrelay.h>
#include <src/mailrelay/mailrelay_internal.h>
#include <src/mailrelay/mailrelay_repository.h>

// System includes
#include <stdlib.h>
#include <string.h>

// Forward declarations for test functions
void test_recover_no_app_config_returns_false(void);
void test_recover_persist_disabled_returns_true(void);
void test_recover_persist_enabled_no_database_returns_false(void);
void test_recover_persist_enabled_recovers_success(void);
void test_recover_persist_enabled_recovery_failure(void);

// Saved global config / executor so each test can restore afterwards.
static AppConfig* g_saved_app_config = NULL;
static AppConfig g_test_config = { 0 };
static mailrelay_repo_execute_fn g_original_executor = NULL;

// Mutable database name buffer (Database is a non-const char*).
static char g_database[] = "main";
static char g_empty_database[] = "";

// Controls the mock executor outcome for the stale-recovery query.
static MailRelayRepoStatus g_recover_status = MAILRELAY_REPO_OK;
static bool g_recover_called = false;

static bool mock_executor(int query_ref,
                          const char* params_json,
                          mailrelay_repo_callback_fn callback,
                          void* user_data) {
    (void)params_json;
    if (query_ref != MAILRELAY_QREF_QUEUE_RECOVER_STALE) {
        return false;
    }
    TEST_ASSERT_NOT_NULL(callback);

    g_recover_called = true;
    MailRelayRepoResult result = { 0 };
    result.status = g_recover_status;
    result.affected_rows = 3;
    callback(&result, user_data);
    return true;
}

void setUp(void) {
    g_saved_app_config = app_config;
    memset(&g_test_config, 0, sizeof(g_test_config));
    mailrelay_shutdown();

    g_original_executor = mailrelay_repo_get_executor();
    mailrelay_repo_set_executor(mock_executor);
    g_recover_called = false;
    g_recover_status = MAILRELAY_REPO_OK;
}

void tearDown(void) {
    mailrelay_repo_set_executor(g_original_executor);
    mailrelay_shutdown();
    app_config = g_saved_app_config;
}

void test_recover_no_app_config_returns_false(void) {
    app_config = NULL;
    TEST_ASSERT_FALSE(mailrelay_recover_stale_sending_rows());
    TEST_ASSERT_FALSE(g_recover_called);
}

void test_recover_persist_disabled_returns_true(void) {
    g_test_config.mail_relay.Enabled = true;
    g_test_config.mail_relay.Queue.Persist = false;
    g_test_config.mail_relay.Database = g_database;
    app_config = &g_test_config;

    TEST_ASSERT_TRUE(mailrelay_recover_stale_sending_rows());
    TEST_ASSERT_FALSE(g_recover_called);
}

void test_recover_persist_enabled_no_database_returns_false(void) {
    g_test_config.mail_relay.Enabled = true;
    g_test_config.mail_relay.Queue.Persist = true;
    g_test_config.mail_relay.Database = g_empty_database;
    app_config = &g_test_config;

    TEST_ASSERT_FALSE(mailrelay_recover_stale_sending_rows());
    TEST_ASSERT_FALSE(g_recover_called);
}

void test_recover_persist_enabled_recovers_success(void) {
    g_test_config.mail_relay.Enabled = true;
    g_test_config.mail_relay.Queue.Persist = true;
    g_test_config.mail_relay.Database = g_database;
    // Non-positive timeout exercises the 300-second default branch.
    g_test_config.mail_relay.Queue.StaleTimeoutSeconds = 0;
    app_config = &g_test_config;

    g_recover_status = MAILRELAY_REPO_OK;
    TEST_ASSERT_TRUE(mailrelay_recover_stale_sending_rows());
    TEST_ASSERT_TRUE(g_recover_called);
}

void test_recover_persist_enabled_recovery_failure(void) {
    g_test_config.mail_relay.Enabled = true;
    g_test_config.mail_relay.Queue.Persist = true;
    g_test_config.mail_relay.Database = g_database;
    g_test_config.mail_relay.Queue.StaleTimeoutSeconds = 600;
    app_config = &g_test_config;

    g_recover_status = MAILRELAY_REPO_QUERY_ERROR;
    // Recovery still reports success; the failed callback only logs.
    TEST_ASSERT_TRUE(mailrelay_recover_stale_sending_rows());
    TEST_ASSERT_TRUE(g_recover_called);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_recover_no_app_config_returns_false);
    RUN_TEST(test_recover_persist_disabled_returns_true);
    RUN_TEST(test_recover_persist_enabled_no_database_returns_false);
    RUN_TEST(test_recover_persist_enabled_recovers_success);
    RUN_TEST(test_recover_persist_enabled_recovery_failure);
    return UNITY_END();
}
