/*
 * Unity Test File: execute_helpers_test_migration_ranges
 * Unit tests for the per-band migration watermark helpers in execute_helpers.c:
 * migration_ref_band(), migration_ranges_reset(), migration_ranges_clear_progress(),
 * migration_range_for_ref(), migration_ranges_note_query(),
 * migration_ranges_sync_globals(), migration_range_skip_through(),
 * migration_ranges_action() and migration_ranges_log()
 */

#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>
#include <unity/mocks/mock_logging.h>

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/dbqueue/dbqueue.h>

// Forward declarations for functions being tested
int migration_ref_band(long long ref);
void migration_ranges_reset(DatabaseQueue* db_queue);
void migration_ranges_clear_progress(DatabaseQueue* db_queue);
const MigrationRange* migration_range_for_ref(const DatabaseQueue* db_queue, long long query_ref);
void migration_ranges_note_query(DatabaseQueue* db_queue, long long query_ref, long long query_type);
void migration_ranges_sync_globals(DatabaseQueue* db_queue);
long long migration_range_skip_through(const DatabaseQueue* db_queue, long long query_ref);
MigrationAction migration_ranges_action(const DatabaseQueue* db_queue);
void migration_ranges_log(const DatabaseQueue* db_queue, const char* dqm_label, const char* verb, int level);

// Forward declarations for test functions
void test_migration_ref_band_negative(void);
void test_migration_ref_band_zero(void);
void test_migration_ref_band_boundaries(void);
void test_migration_ref_band_large_values(void);
void test_migration_ranges_reset_null_queue(void);
void test_migration_ranges_reset_clears_ranges(void);
void test_migration_ranges_clear_progress_null_queue(void);
void test_migration_ranges_clear_progress_zeroes_watermarks(void);
void test_migration_range_for_ref_null_queue(void);
void test_migration_range_for_ref_no_ranges(void);
void test_migration_range_for_ref_match(void);
void test_migration_range_for_ref_no_match(void);
void test_migration_ranges_note_query_null_queue(void);
void test_migration_ranges_note_query_zero_ref(void);
void test_migration_ranges_note_query_negative_ref(void);
void test_migration_ranges_note_query_other_band(void);
void test_migration_ranges_note_query_load_type(void);
void test_migration_ranges_note_query_load_type_lower_ref(void);
void test_migration_ranges_note_query_apply_type(void);
void test_migration_ranges_note_query_apply_type_lower_ref(void);
void test_migration_ranges_note_query_unknown_type(void);
void test_migration_ranges_sync_globals_null_queue(void);
void test_migration_ranges_sync_globals_takes_maximum(void);
void test_migration_range_skip_through_null_queue(void);
void test_migration_range_skip_through_no_match(void);
void test_migration_range_skip_through_loaded_only(void);
void test_migration_range_skip_through_applied_only(void);
void test_migration_range_skip_through_applied_higher(void);
void test_migration_ranges_action_null_queue(void);
void test_migration_ranges_action_no_ranges(void);
void test_migration_ranges_action_nothing_to_do(void);
void test_migration_ranges_action_available_equals_applied(void);
void test_migration_ranges_action_need_load(void);
void test_migration_ranges_action_need_apply(void);
void test_migration_ranges_action_loaded_falls_back_to_applied(void);
void test_migration_ranges_action_load_wins_over_apply(void);
void test_migration_ranges_log_null_queue(void);
void test_migration_ranges_log_null_verb(void);
void test_migration_ranges_log_reports_each_range(void);

/* Minimal queue holder: the range helpers never touch the queue locks. */
static DatabaseQueue* make_queue(void) {
    DatabaseQueue* db_queue = calloc(1, sizeof(DatabaseQueue));
    TEST_ASSERT_NOT_NULL(db_queue);
    return db_queue;
}

static void free_queue(DatabaseQueue* db_queue) {
    free(db_queue);
}

/* Build a queue holding count zeroed ranges. */
static DatabaseQueue* make_ranged_queue(size_t count) {
    DatabaseQueue* db_queue = make_queue();
    db_queue->migration_range_count = count;
    for (size_t i = 0; i < count; i++) {
        db_queue->migration_ranges[i].band = (int)i + 1;
    }
    return db_queue;
}

void setUp(void) {
    mock_system_reset_all();
    mock_logging_reset_all();
}

void tearDown(void) {
    mock_logging_reset_all();
    mock_system_reset_all();
}

// ===== migration_ref_band TESTS =====

void test_migration_ref_band_negative(void) {
    TEST_ASSERT_EQUAL(-1, migration_ref_band(-1));
    TEST_ASSERT_EQUAL(-1, migration_ref_band(-1000000));
}

void test_migration_ref_band_zero(void) {
    TEST_ASSERT_EQUAL(0, migration_ref_band(0));
    TEST_ASSERT_EQUAL(0, migration_ref_band(999));
}

void test_migration_ref_band_boundaries(void) {
    TEST_ASSERT_EQUAL(1, migration_ref_band(1000));
    TEST_ASSERT_EQUAL(1, migration_ref_band(1999));
    TEST_ASSERT_EQUAL(2, migration_ref_band(2000));
    TEST_ASSERT_EQUAL(2, migration_ref_band(2999));
}

void test_migration_ref_band_large_values(void) {
    TEST_ASSERT_EQUAL(12, migration_ref_band(12345));
    TEST_ASSERT_EQUAL(123, migration_ref_band(123456));
}

// ===== migration_ranges_reset TESTS =====

void test_migration_ranges_reset_null_queue(void) {
    /* Must be a no-op rather than a crash. */
    migration_ranges_reset(NULL);
    TEST_ASSERT_TRUE(true);
}

void test_migration_ranges_reset_clears_ranges(void) {
    DatabaseQueue* db_queue = make_ranged_queue(2);
    db_queue->migration_ranges[0].available = 1004;
    db_queue->migration_ranges[0].loaded = 1002;
    db_queue->migration_ranges[0].applied = 1001;
    snprintf(db_queue->migration_ranges[0].design, sizeof(db_queue->migration_ranges[0].design),
             "%s", "acuranzo");
    db_queue->migration_ranges[1].available = 2001;
    db_queue->latest_available_migration = 2001;
    db_queue->latest_loaded_migration = 1002;
    db_queue->latest_applied_migration = 1001;

    migration_ranges_reset(db_queue);

    TEST_ASSERT_EQUAL(0, db_queue->migration_range_count);
    TEST_ASSERT_EQUAL(0, db_queue->migration_ranges[0].band);
    TEST_ASSERT_EQUAL(0, db_queue->migration_ranges[0].available);
    TEST_ASSERT_EQUAL(0, db_queue->migration_ranges[0].loaded);
    TEST_ASSERT_EQUAL(0, db_queue->migration_ranges[0].applied);
    TEST_ASSERT_EQUAL_STRING("", db_queue->migration_ranges[0].design);
    TEST_ASSERT_EQUAL(0, db_queue->migration_ranges[1].band);
    /* The globals are only rewritten by migration_ranges_sync_globals(). */
    TEST_ASSERT_EQUAL(2001, db_queue->latest_available_migration);

    free_queue(db_queue);
}

// ===== migration_ranges_clear_progress TESTS =====

void test_migration_ranges_clear_progress_null_queue(void) {
    migration_ranges_clear_progress(NULL);
    TEST_ASSERT_TRUE(true);
}

void test_migration_ranges_clear_progress_zeroes_watermarks(void) {
    DatabaseQueue* db_queue = make_ranged_queue(2);
    db_queue->migration_ranges[0].available = 1004;
    db_queue->migration_ranges[0].loaded = 1002;
    db_queue->migration_ranges[0].applied = 1001;
    db_queue->migration_ranges[1].available = 2004;
    db_queue->migration_ranges[1].loaded = 2002;
    db_queue->migration_ranges[1].applied = 2001;

    migration_ranges_clear_progress(db_queue);

    TEST_ASSERT_EQUAL(2, db_queue->migration_range_count);
    TEST_ASSERT_EQUAL(1, db_queue->migration_ranges[0].band);
    TEST_ASSERT_EQUAL(0, db_queue->migration_ranges[0].loaded);
    TEST_ASSERT_EQUAL(0, db_queue->migration_ranges[0].applied);
    TEST_ASSERT_EQUAL(1004, db_queue->migration_ranges[0].available);
    TEST_ASSERT_EQUAL(0, db_queue->migration_ranges[1].loaded);
    TEST_ASSERT_EQUAL(0, db_queue->migration_ranges[1].applied);
    TEST_ASSERT_EQUAL(2004, db_queue->migration_ranges[1].available);

    free_queue(db_queue);
}

// ===== migration_range_for_ref TESTS =====

void test_migration_range_for_ref_null_queue(void) {
    TEST_ASSERT_NULL(migration_range_for_ref(NULL, 1000));
}

void test_migration_range_for_ref_no_ranges(void) {
    DatabaseQueue* db_queue = make_queue();

    TEST_ASSERT_NULL(migration_range_for_ref(db_queue, 1000));

    free_queue(db_queue);
}

void test_migration_range_for_ref_match(void) {
    DatabaseQueue* db_queue = make_ranged_queue(2);
    db_queue->migration_ranges[1].design[0] = 'a';

    const MigrationRange* range = migration_range_for_ref(db_queue, 2500);

    TEST_ASSERT_NOT_NULL(range);
    TEST_ASSERT_EQUAL(2, range->band);
    TEST_ASSERT_EQUAL('a', range->design[0]);

    free_queue(db_queue);
}

void test_migration_range_for_ref_no_match(void) {
    DatabaseQueue* db_queue = make_ranged_queue(2);

    /* Band 3 is not present, and a negative ref maps to band -1. */
    TEST_ASSERT_NULL(migration_range_for_ref(db_queue, 3500));
    TEST_ASSERT_NULL(migration_range_for_ref(db_queue, -1500));

    free_queue(db_queue);
}

// ===== migration_ranges_note_query TESTS =====

void test_migration_ranges_note_query_null_queue(void) {
    migration_ranges_note_query(NULL, 1001, 1000);
    TEST_ASSERT_TRUE(true);
}

void test_migration_ranges_note_query_zero_ref(void) {
    DatabaseQueue* db_queue = make_ranged_queue(1);
    db_queue->migration_ranges[0].loaded = 1002;

    migration_ranges_note_query(db_queue, 0, 1000);

    TEST_ASSERT_EQUAL(1002, db_queue->migration_ranges[0].loaded);

    free_queue(db_queue);
}

void test_migration_ranges_note_query_negative_ref(void) {
    DatabaseQueue* db_queue = make_ranged_queue(1);
    db_queue->migration_ranges[0].loaded = 1002;

    migration_ranges_note_query(db_queue, -1005, 1000);

    TEST_ASSERT_EQUAL(1002, db_queue->migration_ranges[0].loaded);

    free_queue(db_queue);
}

void test_migration_ranges_note_query_other_band(void) {
    DatabaseQueue* db_queue = make_ranged_queue(1);
    db_queue->migration_ranges[0].loaded = 1002;
    db_queue->migration_ranges[0].applied = 1001;

    /* Band 2 has no range, so nothing is recorded. */
    migration_ranges_note_query(db_queue, 2005, 1000);
    migration_ranges_note_query(db_queue, 2005, 1003);

    TEST_ASSERT_EQUAL(1002, db_queue->migration_ranges[0].loaded);
    TEST_ASSERT_EQUAL(1001, db_queue->migration_ranges[0].applied);

    free_queue(db_queue);
}

void test_migration_ranges_note_query_load_type(void) {
    DatabaseQueue* db_queue = make_ranged_queue(1);
    db_queue->migration_ranges[0].loaded = 1002;

    migration_ranges_note_query(db_queue, 1004, 1000);

    TEST_ASSERT_EQUAL(1004, db_queue->migration_ranges[0].loaded);
    TEST_ASSERT_EQUAL(0, db_queue->migration_ranges[0].applied);

    free_queue(db_queue);
}

void test_migration_ranges_note_query_load_type_lower_ref(void) {
    DatabaseQueue* db_queue = make_ranged_queue(1);
    db_queue->migration_ranges[0].loaded = 1004;

    migration_ranges_note_query(db_queue, 1003, 1000);

    TEST_ASSERT_EQUAL(1004, db_queue->migration_ranges[0].loaded);

    free_queue(db_queue);
}

void test_migration_ranges_note_query_apply_type(void) {
    DatabaseQueue* db_queue = make_ranged_queue(1);
    db_queue->migration_ranges[0].applied = 1001;

    migration_ranges_note_query(db_queue, 1003, 1003);

    TEST_ASSERT_EQUAL(1003, db_queue->migration_ranges[0].applied);
    TEST_ASSERT_EQUAL(0, db_queue->migration_ranges[0].loaded);

    free_queue(db_queue);
}

void test_migration_ranges_note_query_apply_type_lower_ref(void) {
    DatabaseQueue* db_queue = make_ranged_queue(1);
    db_queue->migration_ranges[0].applied = 1003;

    migration_ranges_note_query(db_queue, 1001, 1003);

    TEST_ASSERT_EQUAL(1003, db_queue->migration_ranges[0].applied);

    free_queue(db_queue);
}

void test_migration_ranges_note_query_unknown_type(void) {
    DatabaseQueue* db_queue = make_ranged_queue(1);
    db_queue->migration_ranges[0].loaded = 1002;
    db_queue->migration_ranges[0].applied = 1001;

    migration_ranges_note_query(db_queue, 1004, 1001);

    TEST_ASSERT_EQUAL(1002, db_queue->migration_ranges[0].loaded);
    TEST_ASSERT_EQUAL(1001, db_queue->migration_ranges[0].applied);

    free_queue(db_queue);
}

// ===== migration_ranges_sync_globals TESTS =====

void test_migration_ranges_sync_globals_null_queue(void) {
    migration_ranges_sync_globals(NULL);
    TEST_ASSERT_TRUE(true);
}

void test_migration_ranges_sync_globals_takes_maximum(void) {
    DatabaseQueue* db_queue = make_ranged_queue(3);
    db_queue->migration_ranges[0].available = 1004;
    db_queue->migration_ranges[0].loaded = 1001;
    db_queue->migration_ranges[0].applied = 1000;
    db_queue->migration_ranges[1].available = 2005;
    db_queue->migration_ranges[1].loaded = 2001;
    db_queue->migration_ranges[1].applied = 2000;
    db_queue->migration_ranges[2].available = 3001;
    db_queue->migration_ranges[2].loaded = 3002;
    db_queue->migration_ranges[2].applied = 3003;

    migration_ranges_sync_globals(db_queue);

    TEST_ASSERT_EQUAL(3001, db_queue->latest_available_migration);
    TEST_ASSERT_EQUAL(3002, db_queue->latest_loaded_migration);
    TEST_ASSERT_EQUAL(3003, db_queue->latest_applied_migration);

    free_queue(db_queue);
}

// ===== migration_range_skip_through TESTS =====

void test_migration_range_skip_through_null_queue(void) {
    TEST_ASSERT_EQUAL(-1, migration_range_skip_through(NULL, 1000));
}

void test_migration_range_skip_through_no_match(void) {
    DatabaseQueue* db_queue = make_ranged_queue(1);

    TEST_ASSERT_EQUAL(-1, migration_range_skip_through(db_queue, 3500));

    free_queue(db_queue);
}

void test_migration_range_skip_through_loaded_only(void) {
    DatabaseQueue* db_queue = make_ranged_queue(1);
    db_queue->migration_ranges[0].loaded = 1004;

    TEST_ASSERT_EQUAL(1004, migration_range_skip_through(db_queue, 1500));

    free_queue(db_queue);
}

void test_migration_range_skip_through_applied_only(void) {
    DatabaseQueue* db_queue = make_ranged_queue(1);
    db_queue->migration_ranges[0].applied = 1007;

    TEST_ASSERT_EQUAL(1007, migration_range_skip_through(db_queue, 1500));

    free_queue(db_queue);
}

void test_migration_range_skip_through_applied_higher(void) {
    DatabaseQueue* db_queue = make_ranged_queue(1);
    db_queue->migration_ranges[0].loaded = 1002;
    db_queue->migration_ranges[0].applied = 1007;

    TEST_ASSERT_EQUAL(1007, migration_range_skip_through(db_queue, 1500));

    free_queue(db_queue);
}

// ===== migration_ranges_action TESTS =====

void test_migration_ranges_action_null_queue(void) {
    TEST_ASSERT_EQUAL(MIGRATION_ACTION_NONE, migration_ranges_action(NULL));
}

void test_migration_ranges_action_no_ranges(void) {
    DatabaseQueue* db_queue = make_queue();

    TEST_ASSERT_EQUAL(MIGRATION_ACTION_NONE, migration_ranges_action(db_queue));

    free_queue(db_queue);
}

void test_migration_ranges_action_nothing_to_do(void) {
    DatabaseQueue* db_queue = make_ranged_queue(1);
    db_queue->migration_ranges[0].available = 1004;
    db_queue->migration_ranges[0].loaded = 1004;
    db_queue->migration_ranges[0].applied = 1004;

    TEST_ASSERT_EQUAL(MIGRATION_ACTION_NONE, migration_ranges_action(db_queue));

    free_queue(db_queue);
}

void test_migration_ranges_action_available_equals_applied(void) {
    DatabaseQueue* db_queue = make_ranged_queue(1);
    db_queue->migration_ranges[0].available = 1000;
    db_queue->migration_ranges[0].loaded = 1004;
    db_queue->migration_ranges[0].applied = 1000;

    TEST_ASSERT_EQUAL(MIGRATION_ACTION_NONE, migration_ranges_action(db_queue));

    free_queue(db_queue);
}

void test_migration_ranges_action_need_load(void) {
    DatabaseQueue* db_queue = make_ranged_queue(1);
    db_queue->migration_ranges[0].available = 1004;
    db_queue->migration_ranges[0].loaded = 1002;
    db_queue->migration_ranges[0].applied = 1002;

    TEST_ASSERT_EQUAL(MIGRATION_ACTION_LOAD, migration_ranges_action(db_queue));

    free_queue(db_queue);
}

void test_migration_ranges_action_need_apply(void) {
    DatabaseQueue* db_queue = make_ranged_queue(1);
    db_queue->migration_ranges[0].available = 1004;
    db_queue->migration_ranges[0].loaded = 1004;
    db_queue->migration_ranges[0].applied = 1001;

    TEST_ASSERT_EQUAL(MIGRATION_ACTION_APPLY, migration_ranges_action(db_queue));

    free_queue(db_queue);
}

void test_migration_ranges_action_loaded_falls_back_to_applied(void) {
    DatabaseQueue* db_queue = make_ranged_queue(1);
    /* loaded == 0 falls back to applied, which already matches available. */
    db_queue->migration_ranges[0].available = 1001;
    db_queue->migration_ranges[0].loaded = 0;
    db_queue->migration_ranges[0].applied = 1001;

    TEST_ASSERT_EQUAL(MIGRATION_ACTION_NONE, migration_ranges_action(db_queue));

    free_queue(db_queue);
}

void test_migration_ranges_action_load_wins_over_apply(void) {
    DatabaseQueue* db_queue = make_ranged_queue(1);
    db_queue->migration_ranges[0].available = 1004;
    db_queue->migration_ranges[0].loaded = 1002;
    db_queue->migration_ranges[0].applied = 1001;

    TEST_ASSERT_EQUAL(MIGRATION_ACTION_LOAD, migration_ranges_action(db_queue));

    free_queue(db_queue);
}

// ===== migration_ranges_log TESTS =====

void test_migration_ranges_log_null_queue(void) {
    migration_ranges_log(NULL, "test-label", "check", LOG_LEVEL_DEBUG);
    TEST_ASSERT_EQUAL(0, mock_logging_get_call_count());
}

void test_migration_ranges_log_null_verb(void) {
    DatabaseQueue* db_queue = make_ranged_queue(1);

    migration_ranges_log(db_queue, "test-label", NULL, LOG_LEVEL_DEBUG);

    TEST_ASSERT_EQUAL(0, mock_logging_get_call_count());

    free_queue(db_queue);
}

void test_migration_ranges_log_reports_each_range(void) {
    DatabaseQueue* db_queue = make_ranged_queue(2);
    /* First range has no design name, second has one with applied ahead of loaded. */
    db_queue->migration_ranges[0].available = 1004;
    db_queue->migration_ranges[0].loaded = 1002;
    db_queue->migration_ranges[0].applied = 1001;
    db_queue->migration_ranges[1].band = 2;
    snprintf(db_queue->migration_ranges[1].design, sizeof(db_queue->migration_ranges[1].design),
             "%s", "argent");
    db_queue->migration_ranges[1].available = 2005;
    db_queue->migration_ranges[1].loaded = 2001;
    db_queue->migration_ranges[1].applied = 2003;

    mock_logging_reset_all();
    migration_ranges_log(db_queue, "test-label", "check", LOG_LEVEL_DEBUG);

    TEST_ASSERT_EQUAL(2, mock_logging_get_call_count());
    TEST_ASSERT_TRUE(mock_logging_message_contains("argent"));
    TEST_ASSERT_TRUE(mock_logging_message_contains("APPLY = 2003"));

    free_queue(db_queue);
}

int main(void) {
    UNITY_BEGIN();

    // migration_ref_band
    RUN_TEST(test_migration_ref_band_negative);
    RUN_TEST(test_migration_ref_band_zero);
    RUN_TEST(test_migration_ref_band_boundaries);
    RUN_TEST(test_migration_ref_band_large_values);

    // migration_ranges_reset
    RUN_TEST(test_migration_ranges_reset_null_queue);
    RUN_TEST(test_migration_ranges_reset_clears_ranges);

    // migration_ranges_clear_progress
    RUN_TEST(test_migration_ranges_clear_progress_null_queue);
    RUN_TEST(test_migration_ranges_clear_progress_zeroes_watermarks);

    // migration_range_for_ref
    RUN_TEST(test_migration_range_for_ref_null_queue);
    RUN_TEST(test_migration_range_for_ref_no_ranges);
    RUN_TEST(test_migration_range_for_ref_match);
    RUN_TEST(test_migration_range_for_ref_no_match);

    // migration_ranges_note_query
    RUN_TEST(test_migration_ranges_note_query_null_queue);
    RUN_TEST(test_migration_ranges_note_query_zero_ref);
    RUN_TEST(test_migration_ranges_note_query_negative_ref);
    RUN_TEST(test_migration_ranges_note_query_other_band);
    RUN_TEST(test_migration_ranges_note_query_load_type);
    RUN_TEST(test_migration_ranges_note_query_load_type_lower_ref);
    RUN_TEST(test_migration_ranges_note_query_apply_type);
    RUN_TEST(test_migration_ranges_note_query_apply_type_lower_ref);
    RUN_TEST(test_migration_ranges_note_query_unknown_type);

    // migration_ranges_sync_globals
    RUN_TEST(test_migration_ranges_sync_globals_null_queue);
    RUN_TEST(test_migration_ranges_sync_globals_takes_maximum);

    // migration_range_skip_through
    RUN_TEST(test_migration_range_skip_through_null_queue);
    RUN_TEST(test_migration_range_skip_through_no_match);
    RUN_TEST(test_migration_range_skip_through_loaded_only);
    RUN_TEST(test_migration_range_skip_through_applied_only);
    RUN_TEST(test_migration_range_skip_through_applied_higher);

    // migration_ranges_action
    RUN_TEST(test_migration_ranges_action_null_queue);
    RUN_TEST(test_migration_ranges_action_no_ranges);
    RUN_TEST(test_migration_ranges_action_nothing_to_do);
    RUN_TEST(test_migration_ranges_action_available_equals_applied);
    RUN_TEST(test_migration_ranges_action_need_load);
    RUN_TEST(test_migration_ranges_action_need_apply);
    RUN_TEST(test_migration_ranges_action_loaded_falls_back_to_applied);
    RUN_TEST(test_migration_ranges_action_load_wins_over_apply);

    // migration_ranges_log
    RUN_TEST(test_migration_ranges_log_null_queue);
    RUN_TEST(test_migration_ranges_log_null_verb);
    RUN_TEST(test_migration_ranges_log_reports_each_range);

    return UNITY_END();
}
