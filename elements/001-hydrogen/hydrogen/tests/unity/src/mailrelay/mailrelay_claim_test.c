/*
 * Mail Relay atomic claim unit tests.
 *
 * Tests Phase 11.1-11.3: HA atomic claim for multi-instance mail queue safety.
 * Covers:
 * - mailrelay_repo_queue_claim_next(): always uses single QueryRef #154
 *   (MAILRELAY_QREF_QUEUE_CLAIM_NEXT), no engine-specific branching.
 * - worker_claim_cb(): callback that sets *claimed based on affected_rows.
 * - mailrelay_repo_queue_claim_next(): param construction, engine resolution,
 *   no-database paths, and null-argument rejection.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/mailrelay/mailrelay_repository.h>
#include <src/mailrelay/mailrelay_workers.h>
#include <src/database/database_types.h>
#include <src/database/dbqueue/dbqueue.h>
#include <src/config/config_mail_relay.h>

// Unity includes
#include <unity.h>

// Third-party includes
#include <jansson.h>

// System includes
#include <stdbool.h>
#include <string.h>
#include <pthread.h>

void setUp(void);
void tearDown(void);

void test_claim_next_uses_single_query_ref(void);
void test_claim_next_uses_single_query_ref_mysql(void);
void test_claim_next_uses_single_query_ref_sqlite(void);
void test_claim_next_uses_single_query_ref_db2(void);

void test_worker_claim_cb_success(void);
void test_worker_claim_cb_lost_race(void);
void test_worker_claim_cb_null_result(void);
void test_worker_claim_cb_null_user_data(void);
void test_worker_claim_cb_error_status_no_claim(void);

void test_claim_next_builds_correct_params(void);
void test_claim_next_no_database_configured(void);
void test_claim_next_null_params_rejected(void);
void test_claim_next_null_callback_rejected(void);

// ---------------------------------------------------------------------------
// Mock executor infrastructure
// ---------------------------------------------------------------------------

static int g_captured_query_ref;
static char* g_captured_params_json;
static bool g_executor_called;
static MailRelayRepoStatus g_executor_status;
static int g_executor_affected_rows;
static json_t* g_executor_data;
static AppConfig* g_saved_app_config = NULL;

// Track the global_queue_manager so we can save/restore it.
extern DatabaseQueueManager* global_queue_manager;
static DatabaseQueueManager* g_saved_queue_manager = NULL;

static void reset_mock_state(void) {
    g_captured_query_ref = -1;
    free(g_captured_params_json);
    g_captured_params_json = NULL;
    g_executor_called = false;
    g_executor_status = MAILRELAY_REPO_OK;
    g_executor_affected_rows = 0;
    if (g_executor_data) {
        json_decref(g_executor_data);
        g_executor_data = NULL;
    }
}

static bool mock_executor(int query_ref, const char* params_json,
                          mailrelay_repo_callback_fn callback, void* user_data) {
    (void)callback;
    (void)user_data;
    g_captured_query_ref = query_ref;
    free(g_captured_params_json);
    g_captured_params_json = params_json ? strdup(params_json) : NULL;
    g_executor_called = true;

    MailRelayRepoResult result = {
        .status = g_executor_status,
        .error_message = NULL,
        .data = g_executor_data,
        .affected_rows = g_executor_affected_rows
    };
    if (callback) {
        callback(&result, user_data);
    }
    return g_executor_status == MAILRELAY_REPO_OK;
}

static void mock_callback(MailRelayRepoResult* result, void* user_data) {
    (void)user_data;
    (void)result;
}

// ---------------------------------------------------------------------------
// Mock DatabaseQueueManager + DatabaseQueue setup helpers
// ---------------------------------------------------------------------------

// Creates a minimal DatabaseQueueManager with one DatabaseQueue registered,
// matching the database name and engine type. The caller must free via
// teardown_mock_queue_manager().
static DatabaseQueueManager* g_mock_manager = NULL;
static DatabaseQueue* g_mock_db_queue = NULL;

static void setup_mock_db_queue(const char* database_name, DatabaseEngine engine) {
    // Allocate and zero the manager.
    g_mock_manager = malloc(sizeof(DatabaseQueueManager));
    TEST_ASSERT_NOT_NULL(g_mock_manager);
    memset(g_mock_manager, 0, sizeof(DatabaseQueueManager));

    g_mock_manager->max_databases = 2;
    g_mock_manager->databases = calloc(g_mock_manager->max_databases, sizeof(DatabaseQueue*));
    TEST_ASSERT_NOT_NULL(g_mock_manager->databases);

    // Initialize the mutex that database_queue_manager_get_database() locks.
    int rc = pthread_mutex_init(&g_mock_manager->manager_lock, NULL);
    TEST_ASSERT_EQUAL_INT(0, rc);
    g_mock_manager->initialized = true;

    // Allocate and zero a DatabaseQueue.
    g_mock_db_queue = malloc(sizeof(DatabaseQueue));
    TEST_ASSERT_NOT_NULL(g_mock_db_queue);
    memset(g_mock_db_queue, 0, sizeof(DatabaseQueue));

    // Use strdup so the manager lookup (strcmp on database_name) works.
    g_mock_db_queue->database_name = strdup(database_name);
    g_mock_db_queue->engine_type = engine;

    g_mock_manager->databases[0] = g_mock_db_queue;
    g_mock_manager->database_count = 1;

    // Swap in our mock manager.
    g_saved_queue_manager = global_queue_manager;
    global_queue_manager = g_mock_manager;
}

static void teardown_mock_db_queue(void) {
    if (g_mock_db_queue) {
        free(g_mock_db_queue->database_name);
        // Note: We don't destroy the queue's internal mutexes since we never
        // initialized them (only manager_lock was used).
        free(g_mock_db_queue);
        g_mock_db_queue = NULL;
    }
    if (g_mock_manager) {
        pthread_mutex_destroy(&g_mock_manager->manager_lock);
        free(g_mock_manager->databases);
        free(g_mock_manager);
        g_mock_manager = NULL;
    }
    global_queue_manager = g_saved_queue_manager;
    g_saved_queue_manager = NULL;
}

// ---------------------------------------------------------------------------
// setUp / tearDown
// ---------------------------------------------------------------------------

void setUp(void) {
    g_saved_app_config = app_config;
    reset_mock_state();
    mailrelay_repo_set_executor(mock_executor);
}

void tearDown(void) {
    reset_mock_state();
    mailrelay_repo_set_executor(NULL);
    app_config = g_saved_app_config;
    teardown_mock_db_queue();
}

// ---------------------------------------------------------------------------
// Tests: mailrelay_repo_queue_claim_next always uses single QueryRef #154
// ---------------------------------------------------------------------------

void test_claim_next_uses_single_query_ref(void) {
    setup_mock_db_queue("acuranzo", DB_ENGINE_POSTGRESQL);

    AppConfig* cfg = calloc(1, sizeof(AppConfig));
    TEST_ASSERT_NOT_NULL(cfg);
    cfg->mail_relay.Database = strdup("acuranzo");
    app_config = cfg;

    MailRelayRepoQueueClaimNext params = {
        .instance_id = "inst-1",
        .claim_token = "token-abc"
    };
    bool result = mailrelay_repo_queue_claim_next(&params, mock_callback, NULL);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(g_executor_called);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_QUEUE_CLAIM_NEXT, g_captured_query_ref);

    free(cfg->mail_relay.Database);
    free(cfg);
}

void test_claim_next_uses_single_query_ref_mysql(void) {
    setup_mock_db_queue("acuranzo", DB_ENGINE_MYSQL);

    AppConfig* cfg = calloc(1, sizeof(AppConfig));
    TEST_ASSERT_NOT_NULL(cfg);
    cfg->mail_relay.Database = strdup("acuranzo");
    app_config = cfg;

    MailRelayRepoQueueClaimNext params = {
        .instance_id = "inst-1",
        .claim_token = "token-abc"
    };
    bool result = mailrelay_repo_queue_claim_next(&params, mock_callback, NULL);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(g_executor_called);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_QUEUE_CLAIM_NEXT, g_captured_query_ref);

    free(cfg->mail_relay.Database);
    free(cfg);
}

void test_claim_next_uses_single_query_ref_sqlite(void) {
    setup_mock_db_queue("acuranzo", DB_ENGINE_SQLITE);

    AppConfig* cfg = calloc(1, sizeof(AppConfig));
    TEST_ASSERT_NOT_NULL(cfg);
    cfg->mail_relay.Database = strdup("acuranzo");
    app_config = cfg;

    MailRelayRepoQueueClaimNext params = {
        .instance_id = "inst-1",
        .claim_token = "token-abc"
    };
    bool result = mailrelay_repo_queue_claim_next(&params, mock_callback, NULL);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(g_executor_called);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_QUEUE_CLAIM_NEXT, g_captured_query_ref);

    free(cfg->mail_relay.Database);
    free(cfg);
}

void test_claim_next_uses_single_query_ref_db2(void) {
    setup_mock_db_queue("acuranzo", DB_ENGINE_DB2);

    AppConfig* cfg = calloc(1, sizeof(AppConfig));
    TEST_ASSERT_NOT_NULL(cfg);
    cfg->mail_relay.Database = strdup("acuranzo");
    app_config = cfg;

    MailRelayRepoQueueClaimNext params = {
        .instance_id = "inst-1",
        .claim_token = "token-abc"
    };
    bool result = mailrelay_repo_queue_claim_next(&params, mock_callback, NULL);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(g_executor_called);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_QUEUE_CLAIM_NEXT, g_captured_query_ref);

    free(cfg->mail_relay.Database);
    free(cfg);
}

// ---------------------------------------------------------------------------
// Tests: worker_claim_cb
// ---------------------------------------------------------------------------

void test_worker_claim_cb_success(void) {
    bool claimed = false;
    MailRelayRepoResult result = {
        .status = MAILRELAY_REPO_OK,
        .affected_rows = 1,
        .error_message = NULL,
        .data = NULL
    };
    worker_claim_cb(&result, &claimed);
    TEST_ASSERT_TRUE(claimed);
}

void test_worker_claim_cb_lost_race(void) {
    bool claimed = false;
    MailRelayRepoResult result = {
        .status = MAILRELAY_REPO_OK,
        .affected_rows = 0,
        .error_message = NULL,
        .data = NULL
    };
    worker_claim_cb(&result, &claimed);
    TEST_ASSERT_FALSE(claimed);
}

void test_worker_claim_cb_null_result(void) {
    bool claimed = false;
    // Should not crash; claimed stays false.
    worker_claim_cb(NULL, &claimed);
    TEST_ASSERT_FALSE(claimed);
}

void test_worker_claim_cb_null_user_data(void) {
    MailRelayRepoResult result = {
        .status = MAILRELAY_REPO_OK,
        .affected_rows = 1,
        .error_message = NULL,
        .data = NULL
    };
    // Should not crash when user_data is NULL.
    worker_claim_cb(&result, NULL);
}

void test_worker_claim_cb_error_status_no_claim(void) {
    bool claimed = false;
    MailRelayRepoResult result = {
        .status = MAILRELAY_REPO_QUERY_ERROR,
        .affected_rows = 1,
        .error_message = "simulated error",
        .data = NULL
    };
    worker_claim_cb(&result, &claimed);
    // Error status should not claim, even with affected_rows > 0.
    TEST_ASSERT_FALSE(claimed);
}

// ---------------------------------------------------------------------------
// Tests: mailrelay_repo_queue_claim_next param construction
// ---------------------------------------------------------------------------

void test_claim_next_builds_correct_params(void) {
    setup_mock_db_queue("acuranzo", DB_ENGINE_POSTGRESQL);

    AppConfig* cfg = calloc(1, sizeof(AppConfig));
    TEST_ASSERT_NOT_NULL(cfg);
    cfg->mail_relay.Database = strdup("acuranzo");
    app_config = cfg;

    MailRelayRepoQueueClaimNext params = {
        .instance_id = "inst-1",
        .claim_token = "token-abc"
    };
    bool result = mailrelay_repo_queue_claim_next(&params, mock_callback, NULL);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(g_executor_called);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_QUEUE_CLAIM_NEXT, g_captured_query_ref);

    json_error_t err;
    json_t* root = json_loads(g_captured_params_json, 0, &err);
    TEST_ASSERT_NOT_NULL(root);
    json_t* string_obj = json_object_get(root, "STRING");
    TEST_ASSERT_NOT_NULL(string_obj);
    TEST_ASSERT_EQUAL_STRING("inst-1", json_string_value(json_object_get(string_obj, "INSTANCE_ID")));
    TEST_ASSERT_EQUAL_STRING("token-abc", json_string_value(json_object_get(string_obj, "CLAIM_TOKEN")));
    json_decref(root);

    free(cfg->mail_relay.Database);
    free(cfg);
}

void test_claim_next_no_database_configured(void) {
    // No app_config set up -> resolve_database returns NULL -> callback fires
    // with NO_DATABASE before db_queue lookup.
    app_config = NULL;

    MailRelayRepoQueueClaimNext params = {
        .instance_id = "inst-1",
        .claim_token = "token-abc"
    };
    bool result = mailrelay_repo_queue_claim_next(&params, mock_callback, NULL);

    TEST_ASSERT_FALSE(result);
    // The no-database path calls mailrelay_repo_invoke_callback directly,
    // bypassing the executor seam entirely.
    TEST_ASSERT_FALSE(g_executor_called);
}

void test_claim_next_null_params_rejected(void) {
    // NULL params should return false without calling executor.
    bool result = mailrelay_repo_queue_claim_next(NULL, mock_callback, NULL);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_FALSE(g_executor_called);
}

void test_claim_next_null_callback_rejected(void) {
    MailRelayRepoQueueClaimNext params = {
        .instance_id = "inst-1",
        .claim_token = "token-abc"
    };
    bool result = mailrelay_repo_queue_claim_next(&params, NULL, NULL);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_FALSE(g_executor_called);
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main(void) {
    UNITY_BEGIN();

    // mailrelay_repo_queue_claim_next always uses single QueryRef #154
    RUN_TEST(test_claim_next_uses_single_query_ref);
    RUN_TEST(test_claim_next_uses_single_query_ref_mysql);
    RUN_TEST(test_claim_next_uses_single_query_ref_sqlite);
    RUN_TEST(test_claim_next_uses_single_query_ref_db2);

    // worker_claim_cb
    RUN_TEST(test_worker_claim_cb_success);
    RUN_TEST(test_worker_claim_cb_lost_race);
    RUN_TEST(test_worker_claim_cb_null_result);
    RUN_TEST(test_worker_claim_cb_null_user_data);
    RUN_TEST(test_worker_claim_cb_error_status_no_claim);

    // mailrelay_repo_queue_claim_next param construction
    RUN_TEST(test_claim_next_builds_correct_params);
    RUN_TEST(test_claim_next_no_database_configured);
    RUN_TEST(test_claim_next_null_params_rejected);
    RUN_TEST(test_claim_next_null_callback_rejected);

    return UNITY_END();
}
