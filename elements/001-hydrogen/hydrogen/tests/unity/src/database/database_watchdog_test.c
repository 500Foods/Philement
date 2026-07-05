/*
 * Unity Test File: database_watchdog_test
 *
 * Unit tests for src/database/database_watchdog.c (step 1 + 2 of
 * the database timeout plan). Verifies:
 *
 *   - init / shutdown lifecycle and idempotency
 *   - register / deregister bookkeeping (active_count)
 *   - is_expired before and after the scan runs (test drives scan
 *     directly because pthread_create is mocked in unity builds and
 *     the watchdog timer thread never actually starts)
 *   - effective-timeout clamping to [MIN, MAX] with 0 -> DEFAULT
 *   - 30-second heartbeat re-alert behavior
 *   - set_bounds runtime override of clamp range
 *   - NULL-safety on all public entry points
 *
 * The mocked log_this (mock_log_this) records every call, so the
 * tests can also assert that the ALERT log fires on expiry and on
 * heartbeat re-fires.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database_watchdog.h>
#include <src/database/database_engine.h>
#include <tests/unity/mocks/mock_database_engine.h>
#include <tests/unity/mocks/mock_logging.h>
#include <tests/unity/mocks/mock_pthread.h>

/*
 * Test fixture: a stack-allocated DatabaseHandle with a designator
 * that the watchdog will copy. Nothing else about the handle is
 * used by the watchdog.
 */
static DatabaseHandle make_test_handle(const char* designator) {
    DatabaseHandle h = {0};
    h.designator = (char*)designator;
    return h;
}

/*
 * Test fixture: a stack-allocated QueryRequest with a query_id and
 * the desired timeout. The watchdog only reads these two fields.
 */
static QueryRequest make_test_request(const char* query_id, int timeout_seconds) {
    QueryRequest r = {0};
    r.query_id = (char*)query_id;
    r.timeout_seconds = timeout_seconds;
    return r;
}

/* Forward declarations for test functions (required by -Wmissing-prototypes). */
void test_watchdog_init_is_idempotent(void);
void test_watchdog_shutdown_without_init_is_safe(void);
void test_register_increments_active_count(void);
void test_deregister_null_is_safe(void);
void test_deregister_unknown_handle_does_not_double_free(void);
void test_register_with_null_request_returns_null(void);
void test_register_clamps_zero_to_default(void);
void test_register_clamps_negative_to_default(void);
void test_register_clamps_below_min_to_min(void);
void test_register_clamps_above_max_to_max(void);
void test_register_preserves_in_range_value(void);
void test_is_expired_false_for_fresh_entry(void);
void test_is_expired_null_handle_is_false(void);
void test_is_expired_true_after_age_exceeds_timeout(void);
void test_check_expirations_does_not_re_alert_within_heartbeat(void);
void test_check_expirations_re_alerts_after_heartbeat_elapses(void);
void test_set_bounds_overrides_clamp_range(void);
void test_set_bounds_ignores_zero_or_negative_arguments(void);
void test_set_bounds_clamps_min_when_min_exceeds_max(void);
void test_register_copies_query_id_and_designator(void);
void test_register_null_query_id_uses_placeholder(void);
void test_cancel_inflight_invoked_on_first_expiry(void);
void test_cancel_inflight_not_invoked_when_not_expired(void);
void test_cancel_inflight_invoked_only_once_per_entry(void);
void test_retry_transport_error_succeeds_on_second_attempt(void);
void test_retry_no_attempt_for_other_error(void);
void test_retry_zero_when_max_retries_zero(void);
void test_retry_exhausts_max_retries_on_persistent_transport(void);

void setUp(void) {
    mock_logging_reset_all();
    mock_pthread_reset_all();
    /*
     * Each test gets a fresh watchdog with compile-time default
     * bounds. The set_bounds tests override these themselves after
     * the shutdown below; resetting here prevents a test that
     * changed bounds from leaking values into the next test.
     */
    database_watchdog_shutdown();
    database_watchdog_set_bounds(DATABASE_WATCHDOG_MIN_TIMEOUT_SECONDS,
                                 DATABASE_WATCHDOG_MAX_TIMEOUT_SECONDS,
                                 DATABASE_WATCHDOG_DEFAULT_TIMEOUT_SECONDS);
    TEST_ASSERT_TRUE(database_watchdog_init());
}

void tearDown(void) {
    database_watchdog_shutdown();
    mock_logging_reset_all();
    mock_pthread_reset_all();
}

/* --- Lifecycle --- */

void test_watchdog_init_is_idempotent(void) {
    DatabaseHandle h = make_test_handle("DQM-TEST-SMFC");
    QueryRequest r = make_test_request("q1", DATABASE_WATCHDOG_MIN_TIMEOUT_SECONDS);
    DatabaseWatchdogHandle* entry = database_watchdog_register(&h, &r);
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_EQUAL_size_t(1, database_watchdog_active_count());

    TEST_ASSERT_TRUE(database_watchdog_init());

    TEST_ASSERT_EQUAL_size_t(1, database_watchdog_active_count());

    database_watchdog_deregister(entry);
}

void test_watchdog_shutdown_without_init_is_safe(void) {
    database_watchdog_shutdown();
    database_watchdog_shutdown();
    TEST_ASSERT_EQUAL_size_t(0, database_watchdog_active_count());
}

/* --- Register / Deregister --- */

void test_register_increments_active_count(void) {
    DatabaseHandle h = make_test_handle("DQM-TEST-SMFC");
    QueryRequest r = make_test_request("q1", DATABASE_WATCHDOG_MIN_TIMEOUT_SECONDS);

    TEST_ASSERT_EQUAL_size_t(0, database_watchdog_active_count());

    DatabaseWatchdogHandle* e1 = database_watchdog_register(&h, &r);
    TEST_ASSERT_NOT_NULL(e1);
    TEST_ASSERT_EQUAL_size_t(1, database_watchdog_active_count());

    DatabaseWatchdogHandle* e2 = database_watchdog_register(&h, &r);
    TEST_ASSERT_NOT_NULL(e2);
    TEST_ASSERT_EQUAL_size_t(2, database_watchdog_active_count());

    database_watchdog_deregister(e1);
    TEST_ASSERT_EQUAL_size_t(1, database_watchdog_active_count());

    database_watchdog_deregister(e2);
    TEST_ASSERT_EQUAL_size_t(0, database_watchdog_active_count());
}

void test_deregister_null_is_safe(void) {
    database_watchdog_deregister(NULL);
    TEST_ASSERT_EQUAL_size_t(0, database_watchdog_active_count());
}

void test_deregister_unknown_handle_does_not_double_free(void) {
    DatabaseWatchdogHandle fake = {0};
    database_watchdog_deregister(&fake);
    TEST_ASSERT_EQUAL_size_t(0, database_watchdog_active_count());
}

void test_register_with_null_request_returns_null(void) {
    TEST_ASSERT_NULL(database_watchdog_register(NULL, NULL));
    DatabaseHandle h = make_test_handle("DQM-TEST-SMFC");
    TEST_ASSERT_NULL(database_watchdog_register(&h, NULL));
    TEST_ASSERT_EQUAL_size_t(0, database_watchdog_active_count());
}

/* --- Timeout clamping (compile-time defaults) --- */

void test_register_clamps_zero_to_default(void) {
    DatabaseHandle h = make_test_handle("DQM-TEST-SMFC");
    QueryRequest r = make_test_request("q", 0);
    DatabaseWatchdogHandle* e = database_watchdog_register(&h, &r);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL_INT(DATABASE_WATCHDOG_DEFAULT_TIMEOUT_SECONDS, e->effective_timeout_seconds);
    database_watchdog_deregister(e);
}

void test_register_clamps_negative_to_default(void) {
    DatabaseHandle h = make_test_handle("DQM-TEST-SMFC");
    QueryRequest r = make_test_request("q", -5);
    DatabaseWatchdogHandle* e = database_watchdog_register(&h, &r);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL_INT(DATABASE_WATCHDOG_DEFAULT_TIMEOUT_SECONDS, e->effective_timeout_seconds);
    database_watchdog_deregister(e);
}

void test_register_clamps_below_min_to_min(void) {
    DatabaseHandle h = make_test_handle("DQM-TEST-SMFC");
    QueryRequest r = make_test_request("q", 1);
    DatabaseWatchdogHandle* e = database_watchdog_register(&h, &r);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL_INT(DATABASE_WATCHDOG_MIN_TIMEOUT_SECONDS, e->effective_timeout_seconds);
    database_watchdog_deregister(e);
}

void test_register_clamps_above_max_to_max(void) {
    DatabaseHandle h = make_test_handle("DQM-TEST-SMFC");
    QueryRequest r = make_test_request("q", DATABASE_WATCHDOG_MAX_TIMEOUT_SECONDS + 100);
    DatabaseWatchdogHandle* e = database_watchdog_register(&h, &r);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL_INT(DATABASE_WATCHDOG_MAX_TIMEOUT_SECONDS, e->effective_timeout_seconds);
    database_watchdog_deregister(e);
}

void test_register_preserves_in_range_value(void) {
    DatabaseHandle h = make_test_handle("DQM-TEST-SMFC");
    QueryRequest r = make_test_request("q", 120);
    DatabaseWatchdogHandle* e = database_watchdog_register(&h, &r);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL_INT(120, e->effective_timeout_seconds);
    database_watchdog_deregister(e);
}

/* --- is_expired semantics --- */

void test_is_expired_false_for_fresh_entry(void) {
    DatabaseHandle h = make_test_handle("DQM-TEST-SMFC");
    QueryRequest r = make_test_request("q", DATABASE_WATCHDOG_MIN_TIMEOUT_SECONDS);
    DatabaseWatchdogHandle* e = database_watchdog_register(&h, &r);
    TEST_ASSERT_NOT_NULL(e);

    database_watchdog_check_expirations();
    TEST_ASSERT_FALSE(database_watchdog_is_expired(e));

    database_watchdog_deregister(e);
}

void test_is_expired_null_handle_is_false(void) {
    TEST_ASSERT_FALSE(database_watchdog_is_expired(NULL));
}

void test_is_expired_true_after_age_exceeds_timeout(void) {
    DatabaseHandle h = make_test_handle("DQM-TEST-SMFC");
    QueryRequest r = make_test_request("q_late", DATABASE_WATCHDOG_MIN_TIMEOUT_SECONDS);
    DatabaseWatchdogHandle* e = database_watchdog_register(&h, &r);
    TEST_ASSERT_NOT_NULL(e);

    e->start_time = time(NULL) - (DATABASE_WATCHDOG_MIN_TIMEOUT_SECONDS + 5);

    int before = mock_logging_get_call_count();
    database_watchdog_check_expirations();

    TEST_ASSERT_TRUE(database_watchdog_is_expired(e));
    TEST_ASSERT_GREATER_THAN(before, mock_logging_get_call_count());

    database_watchdog_deregister(e);
}

/* --- Heartbeat re-alert --- */

void test_check_expirations_does_not_re_alert_within_heartbeat(void) {
    DatabaseHandle h = make_test_handle("DQM-TEST-SMFC");
    QueryRequest r = make_test_request("q_hb", DATABASE_WATCHDOG_MIN_TIMEOUT_SECONDS);
    DatabaseWatchdogHandle* e = database_watchdog_register(&h, &r);
    TEST_ASSERT_NOT_NULL(e);
    e->start_time = time(NULL) - (DATABASE_WATCHDOG_MIN_TIMEOUT_SECONDS + 5);

    database_watchdog_check_expirations();
    int after_first = mock_logging_get_call_count();
    TEST_ASSERT_TRUE(database_watchdog_is_expired(e));

    database_watchdog_check_expirations();
    database_watchdog_check_expirations();
    int after_more = mock_logging_get_call_count();
    TEST_ASSERT_EQUAL_INT(after_first, after_more);

    database_watchdog_deregister(e);
}

void test_check_expirations_re_alerts_after_heartbeat_elapses(void) {
    DatabaseHandle h = make_test_handle("DQM-TEST-SMFC");
    QueryRequest r = make_test_request("q_hb2", DATABASE_WATCHDOG_MIN_TIMEOUT_SECONDS);
    DatabaseWatchdogHandle* e = database_watchdog_register(&h, &r);
    TEST_ASSERT_NOT_NULL(e);
    e->start_time = time(NULL) - (DATABASE_WATCHDOG_MIN_TIMEOUT_SECONDS + 5);

    database_watchdog_check_expirations();
    int after_first = mock_logging_get_call_count();
    TEST_ASSERT_TRUE(database_watchdog_is_expired(e));

    /* Pretend the most recent alert was long enough ago to fire again. */
    e->alerted_at = time(NULL) - DATABASE_WATCHDOG_HEARTBEAT_SECONDS - 1;

    database_watchdog_check_expirations();
    int after_heartbeat = mock_logging_get_call_count();
    TEST_ASSERT_GREATER_THAN(after_first, after_heartbeat);

    database_watchdog_deregister(e);
}

/* --- set_bounds runtime override --- */

void test_set_bounds_overrides_clamp_range(void) {
    database_watchdog_shutdown();
    database_watchdog_set_bounds(60, 600, 120);
    TEST_ASSERT_TRUE(database_watchdog_init());

    DatabaseHandle h = make_test_handle("DQM-TEST-SMFC");

    QueryRequest r_in = make_test_request("in", 300);
    DatabaseWatchdogHandle* e_in = database_watchdog_register(&h, &r_in);
    TEST_ASSERT_NOT_NULL(e_in);
    TEST_ASSERT_EQUAL_INT(300, e_in->effective_timeout_seconds);
    database_watchdog_deregister(e_in);

    QueryRequest r_low = make_test_request("low", 5);
    DatabaseWatchdogHandle* e_low = database_watchdog_register(&h, &r_low);
    TEST_ASSERT_NOT_NULL(e_low);
    TEST_ASSERT_EQUAL_INT(60, e_low->effective_timeout_seconds);
    database_watchdog_deregister(e_low);

    QueryRequest r_high = make_test_request("high", 9999);
    DatabaseWatchdogHandle* e_high = database_watchdog_register(&h, &r_high);
    TEST_ASSERT_NOT_NULL(e_high);
    TEST_ASSERT_EQUAL_INT(600, e_high->effective_timeout_seconds);
    database_watchdog_deregister(e_high);

    QueryRequest r_zero = make_test_request("zero", 0);
    DatabaseWatchdogHandle* e_zero = database_watchdog_register(&h, &r_zero);
    TEST_ASSERT_NOT_NULL(e_zero);
    TEST_ASSERT_EQUAL_INT(120, e_zero->effective_timeout_seconds);
    database_watchdog_deregister(e_zero);
}

void test_set_bounds_ignores_zero_or_negative_arguments(void) {
    database_watchdog_shutdown();
    /*
     * All zeros - function must not change any field. The previous
     * values from setUp (compile-time defaults) must be retained.
     */
    database_watchdog_set_bounds(0, 0, 0);
    TEST_ASSERT_TRUE(database_watchdog_init());

    DatabaseHandle h = make_test_handle("DQM-TEST-SMFC");
    QueryRequest r = make_test_request("q", 0);
    DatabaseWatchdogHandle* e = database_watchdog_register(&h, &r);
    TEST_ASSERT_NOT_NULL(e);
    /* Default should still be the compile-time default (30). */
    TEST_ASSERT_EQUAL_INT(DATABASE_WATCHDOG_DEFAULT_TIMEOUT_SECONDS, e->effective_timeout_seconds);
    database_watchdog_deregister(e);
}

void test_set_bounds_clamps_min_when_min_exceeds_max(void) {
    database_watchdog_shutdown();
    database_watchdog_set_bounds(500, 100, 30);
    TEST_ASSERT_TRUE(database_watchdog_init());

    DatabaseHandle h = make_test_handle("DQM-TEST-SMFC");
    QueryRequest r = make_test_request("q", 50);
    DatabaseWatchdogHandle* e = database_watchdog_register(&h, &r);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL_INT(100, e->effective_timeout_seconds);
    database_watchdog_deregister(e);
}

/* --- Designator and query_id copy semantics --- */

void test_register_copies_query_id_and_designator(void) {
    char query_id[] = "q_orig";
    char designator[] = "DQM-ORIG";
    DatabaseHandle h = make_test_handle(designator);
    QueryRequest r = make_test_request(query_id, DATABASE_WATCHDOG_MIN_TIMEOUT_SECONDS);
    DatabaseWatchdogHandle* e = database_watchdog_register(&h, &r);
    TEST_ASSERT_NOT_NULL(e);

    query_id[0] = 'X';
    designator[0] = 'X';

    TEST_ASSERT_EQUAL_CHAR('X', query_id[0]);
    TEST_ASSERT_EQUAL_CHAR('X', designator[0]);

    TEST_ASSERT_EQUAL_STRING("q_orig", e->query_id_copy);
    TEST_ASSERT_EQUAL_STRING("DQM-ORIG", e->designator_copy);

    database_watchdog_deregister(e);
}

void test_register_null_query_id_uses_placeholder(void) {
    DatabaseHandle h = make_test_handle("DQM-TEST-SMFC");
    QueryRequest r = {0};
    r.timeout_seconds = DATABASE_WATCHDOG_MIN_TIMEOUT_SECONDS;
    DatabaseWatchdogHandle* e = database_watchdog_register(&h, &r);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_NOT_NULL(e->query_id_copy);
    database_watchdog_deregister(e);
}

/* --- Cancel hook orchestration --- */

/*
 * Wire the mock engine into the real engine registry so the
 * watchdog's check_expirations can look it up via
 * database_engine_get. database_engine_init() must run first to
 * set the engine_system_initialized flag. The watchdog unit tests
 * are the only consumer of this helper.
 */
static void setup_mock_engine_registry(void) {
    database_engine_init();
    /* Only register if not already present (idempotent across tests) */
    if (!database_engine_get(DB_ENGINE_POSTGRESQL)) {
        database_engine_register(mock_database_engine_get(DB_ENGINE_POSTGRESQL));
    }
}

void test_cancel_inflight_invoked_on_first_expiry(void) {
    /*
     * The mock engine's cancel_inflight bumps a counter. We register
     * a query against a handle whose engine_type is the mock's
     * (DB_ENGINE_POSTGRESQL by default), force the entry to be
     * expired by setting start_time in the past, then call
     * check_expirations and verify the counter incremented.
     */
    setup_mock_engine_registry();
    mock_database_engine_reset_cancel_call_count();

    DatabaseHandle h = make_test_handle("DQM-TEST-SMFC");
    h.engine_type = DB_ENGINE_POSTGRESQL;
    QueryRequest r = make_test_request("q_cancel", DATABASE_WATCHDOG_MIN_TIMEOUT_SECONDS);
    DatabaseWatchdogHandle* e = database_watchdog_register(&h, &r);
    TEST_ASSERT_NOT_NULL(e);
    e->start_time = time(NULL) - (DATABASE_WATCHDOG_MIN_TIMEOUT_SECONDS + 5);

    database_watchdog_check_expirations();

    TEST_ASSERT_EQUAL_INT(1, mock_database_engine_get_cancel_call_count());
    database_watchdog_deregister(e);
}

void test_cancel_inflight_not_invoked_when_not_expired(void) {
    /*
     * A fresh entry must NOT trigger the cancel hook. We call
     * check_expirations immediately after register (no time has
     * passed) and verify the counter stays at zero.
     */
    setup_mock_engine_registry();
    mock_database_engine_reset_cancel_call_count();

    DatabaseHandle h = make_test_handle("DQM-TEST-SMFC");
    h.engine_type = DB_ENGINE_POSTGRESQL;
    QueryRequest r = make_test_request("q_fresh", DATABASE_WATCHDOG_MIN_TIMEOUT_SECONDS);
    DatabaseWatchdogHandle* e = database_watchdog_register(&h, &r);
    TEST_ASSERT_NOT_NULL(e);

    database_watchdog_check_expirations();
    TEST_ASSERT_EQUAL_INT(0, mock_database_engine_get_cancel_call_count());

    database_watchdog_deregister(e);
}

void test_cancel_inflight_invoked_only_once_per_entry(void) {
    /*
     * After the first cancel, subsequent heartbeat re-alerts must
     * NOT re-invoke the cancel hook. We set start_time to far in
     * the past, run check_expirations three times, and verify the
     * counter is still 1.
     */
    setup_mock_engine_registry();
    mock_database_engine_reset_cancel_call_count();

    DatabaseHandle h = make_test_handle("DQM-TEST-SMFC");
    h.engine_type = DB_ENGINE_POSTGRESQL;
    QueryRequest r = make_test_request("q_once", DATABASE_WATCHDOG_MIN_TIMEOUT_SECONDS);
    DatabaseWatchdogHandle* e = database_watchdog_register(&h, &r);
    TEST_ASSERT_NOT_NULL(e);
    e->start_time = time(NULL) - (DATABASE_WATCHDOG_MIN_TIMEOUT_SECONDS + 5);

    database_watchdog_check_expirations();
    database_watchdog_check_expirations();
    database_watchdog_check_expirations();

    TEST_ASSERT_EQUAL_INT(1, mock_database_engine_get_cancel_call_count());
    database_watchdog_deregister(e);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_watchdog_init_is_idempotent);
    RUN_TEST(test_watchdog_shutdown_without_init_is_safe);

    RUN_TEST(test_register_increments_active_count);
    RUN_TEST(test_deregister_null_is_safe);
    RUN_TEST(test_deregister_unknown_handle_does_not_double_free);
    RUN_TEST(test_register_with_null_request_returns_null);

    RUN_TEST(test_register_clamps_zero_to_default);
    RUN_TEST(test_register_clamps_negative_to_default);
    RUN_TEST(test_register_clamps_below_min_to_min);
    RUN_TEST(test_register_clamps_above_max_to_max);
    RUN_TEST(test_register_preserves_in_range_value);

    RUN_TEST(test_is_expired_false_for_fresh_entry);
    RUN_TEST(test_is_expired_null_handle_is_false);
    RUN_TEST(test_is_expired_true_after_age_exceeds_timeout);
    RUN_TEST(test_check_expirations_does_not_re_alert_within_heartbeat);
    RUN_TEST(test_check_expirations_re_alerts_after_heartbeat_elapses);

    RUN_TEST(test_set_bounds_overrides_clamp_range);
    RUN_TEST(test_set_bounds_ignores_zero_or_negative_arguments);
    RUN_TEST(test_set_bounds_clamps_min_when_min_exceeds_max);

    RUN_TEST(test_register_copies_query_id_and_designator);
    RUN_TEST(test_register_null_query_id_uses_placeholder);

    RUN_TEST(test_cancel_inflight_invoked_on_first_expiry);
    RUN_TEST(test_cancel_inflight_not_invoked_when_not_expired);
    RUN_TEST(test_cancel_inflight_invoked_only_once_per_entry);

    return UNITY_END();
}
