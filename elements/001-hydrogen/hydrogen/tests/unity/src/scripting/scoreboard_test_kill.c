/*
 * Unity Test File: scoreboard_test_kill.c
 *
 * Phase 10 of the LUA_PLAN. Tests the kill_requested and
 * max_runtime_seconds additions to the scoreboard:
 *   - Initial values: kill_requested=false, max_runtime_seconds
 *     resolved from config defaults.
 *   - scoreboard_request_kill sets the flag, idempotent, returns
 *     false on unknown id.
 *   - scoreboard_is_kill_requested returns the live flag value.
 *   - max_runtime_seconds round-trips through scoreboard_find
 *     (so the worker can use the entry copy as the hook's stable
 *     contract, just like the existing memory fields).
 *   - scoreboard_submit_with_limits(max_runtime_seconds=0) uses the
 *     config default; non-zero overrides; INT_MAX means no limit.
 *   - Setting kill_requested does NOT change status or stamp
 *     timestamps.
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Standard includes
#include <string.h>
#include <limits.h>

// Module under test
#include <src/scripting/scoreboard.h>

// Mock app_config so the submit path can read config defaults
// (DefaultMaxRuntime). Memory fields don't matter for these tests.
static AppConfig mock_app_config_storage = {0};

// Forward declarations (required for -Wmissing-prototypes)
void test_kill_initial_flag_is_false(void);
void test_kill_max_runtime_resolved_from_config(void);
void test_kill_request_kill_sets_flag(void);
void test_kill_request_kill_is_idempotent(void);
void test_kill_request_kill_unknown_id_returns_false(void);
void test_kill_request_kill_null_safety(void);
void test_kill_is_kill_requested_returns_live_value(void);
void test_kill_request_kill_does_not_change_status(void);
void test_kill_request_kill_does_not_stamp_timestamps(void);
void test_kill_max_runtime_overrides_config(void);
void test_kill_max_runtime_int_max_means_no_limit(void);
void test_kill_max_runtime_round_trips_through_find(void);

void setUp(void) {
    memset(&mock_app_config_storage, 0, sizeof(mock_app_config_storage));
    mock_app_config_storage.scripting.DefaultMaxRuntime = 60;  // 60s default for testing
    mock_app_config_storage.scripting.InstructionHookInterval = 5000;
    mock_app_config_storage.scripting.MemorySoftLimitKB = 32768;
    mock_app_config_storage.scripting.MemoryHardLimitKB = 65536;
    app_config = &mock_app_config_storage;
}

void tearDown(void) {
    app_config = NULL;
}

// A fresh entry has kill_requested=false and max_runtime_seconds
// resolved from the config default (60s in this suite).
void test_kill_initial_flag_is_false(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "init", NULL);
    TEST_ASSERT_NOT_NULL(id);

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_FALSE(e->kill_requested);
    TEST_ASSERT_EQUAL_INT(60, e->max_runtime_seconds);

    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// DefaultMaxRuntime is read from the config; with the suite's
// config set to 60, scoreboard_submit (which goes through
// scoreboard_submit_with_limits(NULL)) populates the entry with
// the configured value. This is a "covers the resolve path with
// a non-zero config" pin.
void test_kill_max_runtime_resolved_from_config(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "default_rt", NULL);

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    // Config DefaultMaxRuntime was 60 in setUp; verify the entry
    // picked it up.
    TEST_ASSERT_EQUAL_INT(60, e->max_runtime_seconds);
    TEST_ASSERT_FALSE(e->kill_requested);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// scoreboard_request_kill sets the flag and returns true.
void test_kill_request_kill_sets_flag(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "kill", NULL);
    TEST_ASSERT_NOT_NULL(id);

    bool ok = scoreboard_request_kill(sb, id);
    TEST_ASSERT_TRUE(ok);

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_TRUE(e->kill_requested);

    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// Idempotent: setting twice is a no-op and still returns true.
void test_kill_request_kill_is_idempotent(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "idem", NULL);
    TEST_ASSERT_NOT_NULL(id);

    TEST_ASSERT_TRUE(scoreboard_request_kill(sb, id));
    TEST_ASSERT_TRUE(scoreboard_request_kill(sb, id));
    TEST_ASSERT_TRUE(scoreboard_request_kill(sb, id));

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_TRUE(e->kill_requested);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// Unknown id: returns false, no crash.
void test_kill_request_kill_unknown_id_returns_false(void) {
    Scoreboard* sb = scoreboard_create();
    bool ok = scoreboard_request_kill(sb, "NOPE1");
    TEST_ASSERT_FALSE(ok);
    scoreboard_destroy(sb);
}

// NULL safety: NULL scoreboard or NULL id returns false, no crash.
void test_kill_request_kill_null_safety(void) {
    TEST_ASSERT_FALSE(scoreboard_request_kill(NULL, "x"));
    Scoreboard* sb = scoreboard_create();
    TEST_ASSERT_FALSE(scoreboard_request_kill(sb, NULL));
    scoreboard_destroy(sb);
}

// scoreboard_is_kill_requested reflects the live flag.
void test_kill_is_kill_requested_returns_live_value(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "live", NULL);

    bool out = true;
    TEST_ASSERT_TRUE(scoreboard_is_kill_requested(sb, id, &out));
    TEST_ASSERT_FALSE(out);

    TEST_ASSERT_TRUE(scoreboard_request_kill(sb, id));

    out = false;
    TEST_ASSERT_TRUE(scoreboard_is_kill_requested(sb, id, &out));
    TEST_ASSERT_TRUE(out);

    // Unknown id: returns false, *out untouched (we don't pre-set it).
    out = true;
    TEST_ASSERT_FALSE(scoreboard_is_kill_requested(sb, "NOPE1", &out));
    TEST_ASSERT_FALSE(out);

    free(id);
    scoreboard_destroy(sb);
}

// Setting kill_requested does NOT change status (PENDING stays PENDING).
void test_kill_request_kill_does_not_change_status(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "status", NULL);

    TEST_ASSERT_TRUE(scoreboard_request_kill(sb, id));

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_EQUAL(SCOREBOARD_JOB_PENDING, e->status);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// Setting kill_requested does NOT stamp started_at or finished_at.
void test_kill_request_kill_does_not_stamp_timestamps(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "ts", NULL);

    ScoreboardEntry* before = scoreboard_find(sb, id);
    struct timespec s_before = before->started_at;
    struct timespec f_before = before->finished_at;
    scoreboard_entry_free(before);

    TEST_ASSERT_TRUE(scoreboard_request_kill(sb, id));

    ScoreboardEntry* after = scoreboard_find(sb, id);
    TEST_ASSERT_EQUAL(s_before.tv_sec, after->started_at.tv_sec);
    TEST_ASSERT_EQUAL(s_before.tv_nsec, after->started_at.tv_nsec);
    TEST_ASSERT_EQUAL(f_before.tv_sec, after->finished_at.tv_sec);
    TEST_ASSERT_EQUAL(f_before.tv_nsec, after->finished_at.tv_nsec);
    scoreboard_entry_free(after);
    free(id);
    scoreboard_destroy(sb);
}

// A non-zero max_runtime_seconds override replaces the config default.
void test_kill_max_runtime_overrides_config(void) {
    Scoreboard* sb = scoreboard_create();
    ScoreboardJobLimits limits = {0};
    limits.instruction_hook_interval = 100;
    limits.max_runtime_seconds = 5;
    char* id = scoreboard_submit_with_limits(sb, "tight", NULL, &limits);

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_EQUAL_INT(5, e->max_runtime_seconds);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// INT_MAX explicitly means "no limit" (set when caller wants to opt out).
void test_kill_max_runtime_int_max_means_no_limit(void) {
    Scoreboard* sb = scoreboard_create();
    ScoreboardJobLimits limits = {0};
    limits.instruction_hook_interval = 100;
    limits.max_runtime_seconds = INT_MAX;
    char* id = scoreboard_submit_with_limits(sb, "norlim", NULL, &limits);

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_EQUAL_INT(INT_MAX, e->max_runtime_seconds);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// max_runtime_seconds round-trips through scoreboard_find (entry copy
// includes it), so the worker can read the value from the entry copy
// after find() without re-reading the scoreboard.
void test_kill_max_runtime_round_trips_through_find(void) {
    Scoreboard* sb = scoreboard_create();
    ScoreboardJobLimits limits = {0};
    limits.instruction_hook_interval = 200;
    limits.max_runtime_seconds = 17;
    limits.enforce_limits = true;
    char* id = scoreboard_submit_with_limits(sb, "round", NULL, &limits);

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_EQUAL_INT(17, e->max_runtime_seconds);
    TEST_ASSERT_TRUE(e->enforce_limits);
    TEST_ASSERT_FALSE(e->kill_requested);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_kill_initial_flag_is_false);
    RUN_TEST(test_kill_max_runtime_resolved_from_config);
    RUN_TEST(test_kill_request_kill_sets_flag);
    RUN_TEST(test_kill_request_kill_is_idempotent);
    RUN_TEST(test_kill_request_kill_unknown_id_returns_false);
    RUN_TEST(test_kill_request_kill_null_safety);
    RUN_TEST(test_kill_is_kill_requested_returns_live_value);
    RUN_TEST(test_kill_request_kill_does_not_change_status);
    RUN_TEST(test_kill_request_kill_does_not_stamp_timestamps);
    RUN_TEST(test_kill_max_runtime_overrides_config);
    RUN_TEST(test_kill_max_runtime_int_max_means_no_limit);
    RUN_TEST(test_kill_max_runtime_round_trips_through_find);

    return UNITY_END();
}
