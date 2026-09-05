/*
 * Mail Relay Repository unit tests — execution seam & infrastructure surface.
 *
 * Tests the swappable QueryRef execution seam (executor get/set, default
 * executor), database resolution (resolve_database), the low-level parameter
 * builder primitives (repo_add_string / repo_add_int / repo_add_int64), the
 * repo_execute_json / invoke_callback dispatch, and the engine-aware ISO 8601
 * -> MySQL DATETIME translator.
 *
 * The parameter-construction tests for each QueryRef helper live in
 * mailrelay_repository_test_params.c.
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

void setUp(void);
void tearDown(void);

void test_executor_seam_is_set_and_used(void);
void test_default_executor_returns_no_database(void);
void test_resolve_database_returns_explicit_database(void);
void test_resolve_database_falls_back_to_single_connection(void);
void test_resolve_database_returns_null_when_no_database(void);
void test_resolve_database_null_app_config(void);
void test_resolve_database_empty_database_name_falls_back(void);
void test_repo_add_string_null_root(void);
void test_repo_add_string_null_name(void);
void test_repo_add_string_missing_string_object(void);
void test_repo_add_string_null_value_adds_null(void);
void test_repo_add_int_null_root(void);
void test_repo_add_int_null_name(void);
void test_repo_add_int_missing_integer_object(void);
void test_repo_add_int64_null_root(void);
void test_repo_add_int64_null_name(void);
void test_repo_add_int64_missing_integer_object(void);
void test_repo_execute_json_null_params(void);
void test_invoke_callback_null_callback(void);

// PERSIST_PLAN Phase 2c: ISO 8601 -> MySQL DATETIME translator + engine-aware
// repo_add_datetime helper. See src/mailrelay/mailrelay_repository.{h,c}.
void test_translate_iso8601_basic(void);
void test_translate_iso8601_with_fractional(void);
void test_translate_iso8601_already_mysql_format(void);
void test_translate_iso8601_empty_string(void);
void test_translate_iso8601_null_input(void);
void test_translate_iso8601_no_t_separator_short(void);
void test_repo_add_datetime_no_app_config_passes_through(void);
void test_repo_add_datetime_non_mysql_engine_passes_through(void);
void test_repo_add_datetime_mysql_engine_translates(void);
void test_repo_add_datetime_mysql_engine_translates_fractional(void);
void test_repo_add_datetime_null_input_emits_null(void);

static int g_captured_query_ref;
static char* g_captured_params_json;
static bool g_executor_called;
static MailRelayRepoResult* g_captured_result;
static AppConfig* g_saved_app_config = NULL;

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
static bool mock_executor(int query_ref, const char* params_json, mailrelay_repo_callback_fn callback, void* user_data) {
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
    g_saved_app_config = app_config;
    reset_mock_state();
    mailrelay_repo_set_executor(mock_executor);
}

void tearDown(void) {
    reset_mock_state();
    mailrelay_repo_set_executor(NULL);
    app_config = g_saved_app_config;
}

// Executor seam tests

void test_executor_seam_is_set_and_used(void) {
    TEST_ASSERT_EQUAL_PTR(mock_executor, mailrelay_repo_get_executor());
    MailRelayRepoQueueGetByUuid params = { .message_uuid = "test-uuid" };
    bool result = mailrelay_repo_queue_get_by_uuid(&params, mock_callback, NULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(g_executor_called);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_QREF_QUEUE_GET_BY_UUID, g_captured_query_ref);
}

// Default executor error path test

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

// mailrelay_repo_resolve_database tests

void test_resolve_database_returns_explicit_database(void) {
    AppConfig* cfg = calloc(1, sizeof(AppConfig));
    TEST_ASSERT_NOT_NULL(cfg);
    cfg->mail_relay.Database = strdup("explicit_db");
    app_config = cfg;

    const char* result = mailrelay_repo_resolve_database();
    TEST_ASSERT_EQUAL_STRING("explicit_db", result);

    app_config = NULL;
    free(cfg->mail_relay.Database);
    free(cfg);
}

void test_resolve_database_falls_back_to_single_connection(void) {
    AppConfig* cfg = calloc(1, sizeof(AppConfig));
    TEST_ASSERT_NOT_NULL(cfg);
    cfg->mail_relay.Database = NULL;
    cfg->databases.connection_count = 1;
    cfg->databases.connections[0].name = strdup("fallback_db");
    app_config = cfg;

    const char* result = mailrelay_repo_resolve_database();
    TEST_ASSERT_EQUAL_STRING("fallback_db", result);

    app_config = NULL;
    free(cfg->databases.connections[0].name);
    free(cfg);
}

void test_resolve_database_returns_null_when_no_database(void) {
    AppConfig* cfg = calloc(1, sizeof(AppConfig));
    TEST_ASSERT_NOT_NULL(cfg);
    cfg->mail_relay.Database = NULL;
    cfg->databases.connection_count = 0;
    app_config = cfg;

    const char* result = mailrelay_repo_resolve_database();
    TEST_ASSERT_NULL(result);

    app_config = NULL;
    free(cfg);
}

void test_resolve_database_null_app_config(void) {
    app_config = NULL;
    const char* result = mailrelay_repo_resolve_database();
    TEST_ASSERT_NULL(result);
}

void test_resolve_database_empty_database_name_falls_back(void) {
    AppConfig* cfg = calloc(1, sizeof(AppConfig));
    TEST_ASSERT_NOT_NULL(cfg);
    cfg->mail_relay.Database = strdup("");
    cfg->databases.connection_count = 1;
    cfg->databases.connections[0].name = strdup("fallback_db");
    app_config = cfg;

    const char* result = mailrelay_repo_resolve_database();
    TEST_ASSERT_EQUAL_STRING("fallback_db", result);

    app_config = NULL;
    free(cfg->mail_relay.Database);
    free(cfg->databases.connections[0].name);
    free(cfg);
}

// repo_add_string tests

void test_repo_add_string_null_root(void) {
    TEST_ASSERT_FALSE(repo_add_string(NULL, "KEY", "value"));
}

void test_repo_add_string_null_name(void) {
    json_t* root = repo_params_new();
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_FALSE(repo_add_string(root, NULL, "value"));
    json_decref(root);
}

void test_repo_add_string_missing_string_object(void) {
    json_t* root = json_object();
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_FALSE(repo_add_string(root, "KEY", "value"));
    json_decref(root);
}

void test_repo_add_string_null_value_adds_null(void) {
    json_t* root = repo_params_new();
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_TRUE(repo_add_string(root, "NULL_KEY", NULL));

    json_t* string_obj = json_object_get(root, "STRING");
    TEST_ASSERT_NOT_NULL(string_obj);
    json_t* val = json_object_get(string_obj, "NULL_KEY");
    TEST_ASSERT_NOT_NULL(val);
    TEST_ASSERT_TRUE(json_is_null(val));
    json_decref(root);
}

// repo_add_int tests

void test_repo_add_int_null_root(void) {
    TEST_ASSERT_FALSE(repo_add_int(NULL, "KEY", 42));
}

void test_repo_add_int_null_name(void) {
    json_t* root = repo_params_new();
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_FALSE(repo_add_int(root, NULL, 42));
    json_decref(root);
}

void test_repo_add_int_missing_integer_object(void) {
    json_t* root = json_object();
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_FALSE(repo_add_int(root, "KEY", 42));
    json_decref(root);
}

// repo_add_int64 tests

void test_repo_add_int64_null_root(void) {
    TEST_ASSERT_FALSE(repo_add_int64(NULL, "KEY", 42));
}

void test_repo_add_int64_null_name(void) {
    json_t* root = repo_params_new();
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_FALSE(repo_add_int64(root, NULL, 42));
    json_decref(root);
}

void test_repo_add_int64_missing_integer_object(void) {
    json_t* root = json_object();
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_FALSE(repo_add_int64(root, "KEY", 42));
    json_decref(root);
}

// repo_execute_json and mailrelay_repo_invoke_callback tests

void test_repo_execute_json_null_params(void) {
    bool result = repo_execute_json(MAILRELAY_QREF_QUEUE_INSERT, NULL,
                                    mock_callback, NULL);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NOT_NULL(g_captured_result);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_REPO_INVALID_ARGS, g_captured_result->status);
}

void test_invoke_callback_null_callback(void) {
    mailrelay_repo_invoke_callback(NULL, NULL, MAILRELAY_REPO_OK,
                                   NULL, NULL, 0);
}

// ----------------------------------------------------------------------------
// PERSIST_PLAN Phase 2c: ISO 8601 -> MySQL DATETIME translator unit tests
// ----------------------------------------------------------------------------

void test_translate_iso8601_basic(void) {
    char* out = mailrelay_repo_translate_iso8601_to_mysql("2026-09-04T22:10:57Z");
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_EQUAL_STRING("2026-09-04 22:10:57", out);
    free(out);
}

void test_translate_iso8601_with_fractional(void) {
    char* out = mailrelay_repo_translate_iso8601_to_mysql("2026-09-04T22:10:57.123Z");
    TEST_ASSERT_NOT_NULL(out);
    // Fractional seconds are dropped (column is DATETIME, not DATETIME(6)).
    TEST_ASSERT_EQUAL_STRING("2026-09-04 22:10:57", out);
    free(out);
}

void test_translate_iso8601_already_mysql_format(void) {
    char* out = mailrelay_repo_translate_iso8601_to_mysql("2026-09-04 22:10:57");
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_EQUAL_STRING("2026-09-04 22:10:57", out);
    free(out);
}

void test_translate_iso8601_empty_string(void) {
    char* out = mailrelay_repo_translate_iso8601_to_mysql("");
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_EQUAL_STRING("", out);
    free(out);
}

void test_translate_iso8601_null_input(void) {
    char* out = mailrelay_repo_translate_iso8601_to_mysql(NULL);
    TEST_ASSERT_NULL(out);
}

void test_translate_iso8601_no_t_separator_short(void) {
    // "2026-09" is too short and has no 'T'; pass through unchanged.
    char* out = mailrelay_repo_translate_iso8601_to_mysql("2026-09");
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_EQUAL_STRING("2026-09", out);
    free(out);
}

// Helper for repo_add_datetime tests: build a one-connection AppConfig whose
// single database is named 'db' with the given engine type ("mysql", etc.).
// find_database_connection() matches on DatabaseConnection.connection_name
// (the JSON "Name" field), not the lower-level DatabaseConnection.name
// (KNOWN_DATABASES alias). Both must be set so the lookup works either way.
static AppConfig* make_datetime_test_config(const char* db_name, const char* engine_type) {
    AppConfig* cfg = calloc(1, sizeof(AppConfig));
    if (!cfg) return NULL;
    cfg->mail_relay.Database = strdup(db_name);
    cfg->databases.connection_count = 1;
    cfg->databases.connections[0].name = strdup(db_name);
    cfg->databases.connections[0].connection_name = strdup(db_name);
    cfg->databases.connections[0].type = strdup(engine_type);
    cfg->databases.connections[0].enabled = true;
    return cfg;
}

static void free_datetime_test_config(AppConfig* cfg) {
    if (!cfg) return;
    free(cfg->mail_relay.Database);
    free(cfg->databases.connections[0].name);
    free(cfg->databases.connections[0].connection_name);
    free(cfg->databases.connections[0].type);
    free(cfg);
}

void test_repo_add_datetime_no_app_config_passes_through(void) {
    app_config = NULL;
    json_t* root = repo_params_new();
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_TRUE(repo_add_datetime(root, "NEXT_ATTEMPT_AT",
                                       "2026-09-04T22:10:57Z", "Acuranzo"));
    json_t* string_obj = json_object_get(root, "STRING");
    TEST_ASSERT_NOT_NULL(string_obj);
    const char* value = json_string_value(json_object_get(string_obj, "NEXT_ATTEMPT_AT"));
    TEST_ASSERT_NOT_NULL(value);
    // Without app_config the helper cannot identify the engine -> pass-through.
    TEST_ASSERT_EQUAL_STRING("2026-09-04T22:10:57Z", value);
    json_decref(root);
}

void test_repo_add_datetime_non_mysql_engine_passes_through(void) {
    AppConfig* cfg = make_datetime_test_config("Acuranzo", "postgresql");
    TEST_ASSERT_NOT_NULL(cfg);
    app_config = cfg;

    json_t* root = repo_params_new();
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_TRUE(repo_add_datetime(root, "NEXT_ATTEMPT_AT",
                                       "2026-09-04T22:10:57Z", "Acuranzo"));
    json_t* string_obj = json_object_get(root, "STRING");
    TEST_ASSERT_NOT_NULL(string_obj);
    const char* value = json_string_value(json_object_get(string_obj, "NEXT_ATTEMPT_AT"));
    TEST_ASSERT_NOT_NULL(value);
    // 5 working engines accept ISO 8601 unchanged.
    TEST_ASSERT_EQUAL_STRING("2026-09-04T22:10:57Z", value);
    json_decref(root);

    app_config = NULL;
    free_datetime_test_config(cfg);
}

void test_repo_add_datetime_mysql_engine_translates(void) {
    AppConfig* cfg = make_datetime_test_config("Acuranzo", "mysql");
    TEST_ASSERT_NOT_NULL(cfg);
    app_config = cfg;

    json_t* root = repo_params_new();
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_TRUE(repo_add_datetime(root, "NEXT_ATTEMPT_AT",
                                       "2026-09-04T22:10:57Z", "Acuranzo"));
    json_t* string_obj = json_object_get(root, "STRING");
    TEST_ASSERT_NOT_NULL(string_obj);
    const char* value = json_string_value(json_object_get(string_obj, "NEXT_ATTEMPT_AT"));
    TEST_ASSERT_NOT_NULL(value);
    // MySQL/MariaDB DATETIME shape: 'T' -> ' ', trailing 'Z' stripped.
    TEST_ASSERT_EQUAL_STRING("2026-09-04 22:10:57", value);
    json_decref(root);

    app_config = NULL;
    free_datetime_test_config(cfg);
}

void test_repo_add_datetime_mysql_engine_translates_fractional(void) {
    AppConfig* cfg = make_datetime_test_config("Acuranzo", "mysql");
    TEST_ASSERT_NOT_NULL(cfg);
    app_config = cfg;

    json_t* root = repo_params_new();
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_TRUE(repo_add_datetime(root, "NEXT_ATTEMPT_AT",
                                       "2026-09-04T22:10:57.123Z", "Acuranzo"));
    json_t* string_obj = json_object_get(root, "STRING");
    TEST_ASSERT_NOT_NULL(string_obj);
    const char* value = json_string_value(json_object_get(string_obj, "NEXT_ATTEMPT_AT"));
    TEST_ASSERT_NOT_NULL(value);
    TEST_ASSERT_EQUAL_STRING("2026-09-04 22:10:57", value);
    json_decref(root);

    app_config = NULL;
    free_datetime_test_config(cfg);
}

void test_repo_add_datetime_null_input_emits_null(void) {
    AppConfig* cfg = make_datetime_test_config("Acuranzo", "mysql");
    TEST_ASSERT_NOT_NULL(cfg);
    app_config = cfg;

    json_t* root = repo_params_new();
    TEST_ASSERT_NOT_NULL(root);
    // NULL input on a MySQL engine still emits JSON null (preserve caller contract).
    TEST_ASSERT_TRUE(repo_add_datetime(root, "NEXT_ATTEMPT_AT", NULL, "Acuranzo"));
    json_t* string_obj = json_object_get(root, "STRING");
    TEST_ASSERT_NOT_NULL(string_obj);
    json_t* val = json_object_get(string_obj, "NEXT_ATTEMPT_AT");
    TEST_ASSERT_NOT_NULL(val);
    TEST_ASSERT_TRUE(json_is_null(val));
    json_decref(root);

    app_config = NULL;
    free_datetime_test_config(cfg);
}

int main(void) {
    UNITY_BEGIN();

    // Executor seam
    RUN_TEST(test_executor_seam_is_set_and_used);
    RUN_TEST(test_default_executor_returns_no_database);

    // resolve_database
    RUN_TEST(test_resolve_database_returns_explicit_database);
    RUN_TEST(test_resolve_database_falls_back_to_single_connection);
    RUN_TEST(test_resolve_database_returns_null_when_no_database);
    RUN_TEST(test_resolve_database_null_app_config);
    RUN_TEST(test_resolve_database_empty_database_name_falls_back);

    // repo_add_string / repo_add_int / repo_add_int64
    RUN_TEST(test_repo_add_string_null_root);
    RUN_TEST(test_repo_add_string_null_name);
    RUN_TEST(test_repo_add_string_missing_string_object);
    RUN_TEST(test_repo_add_string_null_value_adds_null);
    RUN_TEST(test_repo_add_int_null_root);
    RUN_TEST(test_repo_add_int_null_name);
    RUN_TEST(test_repo_add_int_missing_integer_object);
    RUN_TEST(test_repo_add_int64_null_root);
    RUN_TEST(test_repo_add_int64_null_name);
    RUN_TEST(test_repo_add_int64_missing_integer_object);

    // repo_execute_json and invoke_callback
    RUN_TEST(test_repo_execute_json_null_params);
    RUN_TEST(test_invoke_callback_null_callback);

    // PERSIST_PLAN Phase 2c: ISO 8601 -> MySQL DATETIME translator
    RUN_TEST(test_translate_iso8601_basic);
    RUN_TEST(test_translate_iso8601_with_fractional);
    RUN_TEST(test_translate_iso8601_already_mysql_format);
    RUN_TEST(test_translate_iso8601_empty_string);
    RUN_TEST(test_translate_iso8601_null_input);
    RUN_TEST(test_translate_iso8601_no_t_separator_short);
    RUN_TEST(test_repo_add_datetime_no_app_config_passes_through);
    RUN_TEST(test_repo_add_datetime_non_mysql_engine_passes_through);
    RUN_TEST(test_repo_add_datetime_mysql_engine_translates);
    RUN_TEST(test_repo_add_datetime_mysql_engine_translates_fractional);
    RUN_TEST(test_repo_add_datetime_null_input_emits_null);

    return UNITY_END();
}
