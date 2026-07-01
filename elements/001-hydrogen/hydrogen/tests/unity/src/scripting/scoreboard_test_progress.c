/*
 * Unity Test File: scoreboard_test_progress.c
 *
 * Phase 8 of the LUA_PLAN. Tests the progress and per-job limit
 * additions to the scoreboard:
 *   - scoreboard_update_progress writes instruction_count and
 *     memory_used_kb; idempotent on unknown id returns false.
 *   - Per-job limits (instruction_hook_interval, memory_soft_limit_kb,
 *     memory_hard_limit_kb, enforce_limits) round-trip through
 *     scoreboard_find (so the worker can use the entry copy as the
 *     hook's stable contract).
 *   - scoreboard_submit_with_limits(NULL) is identical to
 *     scoreboard_submit (config defaults, enforce=true).
 *   - scoreboard_submit_with_limits with non-zero fields overrides.
 *   - Monotonic: instruction_count never goes backwards.
 *   - Memory is non-monotonic (GC can free).
 *   - The progress update does NOT change status or stamp timestamps.
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Standard includes
#include <string.h>

// Module under test
#include <src/scripting/scoreboard.h>

// Mock app_config so the submit path can read config defaults
// (InstructionHookInterval, MemorySoftLimitKB, MemoryHardLimitKB,
// MemorySampleEveryNHooks). A zeroed config means zero defaults;
// that exercises the "no config" path in the submit function.
static AppConfig mock_app_config_storage = {0};

// Forward declarations (required for -Wmissing-prototypes)
void test_progress_initial_fields_are_zero(void);
void test_progress_update_writes_values(void);
void test_progress_update_unknown_id_returns_false(void);
void test_progress_update_null_scoreboard_returns_false(void);
void test_progress_update_does_not_change_status(void);
void test_progress_update_does_not_stamp_timestamps(void);
void test_progress_instruction_count_is_monotonic(void);
void test_progress_memory_is_non_monotonic(void);
void test_progress_per_job_limits_round_trip_through_find(void);
void test_progress_submit_with_null_limits_uses_config(void);
void test_progress_submit_with_explicit_limits_overrides(void);
void test_progress_submit_with_limits_size_max_means_no_hard(void);

void setUp(void) {
    memset(&mock_app_config_storage, 0, sizeof(mock_app_config_storage));
    // Provide known config defaults so the "no limits" path is testable.
    mock_app_config_storage.scripting.InstructionHookInterval = 5000;
    mock_app_config_storage.scripting.MemorySoftLimitKB = 32768;
    mock_app_config_storage.scripting.MemoryHardLimitKB = 65536;
    app_config = &mock_app_config_storage;
}

void tearDown(void) {
    app_config = NULL;
}

// A fresh entry has zero progress and zero per-job limits (the
// default-zero memset on submit, before scoreboard_submit_with_limits
// fills the limits from config).
void test_progress_initial_fields_are_zero(void) {
    Scoreboard* sb = scoreboard_create();
    TEST_ASSERT_NOT_NULL(sb);
    char* id = scoreboard_submit(sb, "init_test", NULL);
    TEST_ASSERT_NOT_NULL(id);

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL_UINT64(0, e->instruction_count);
    TEST_ASSERT_EQUAL_UINT(0, (unsigned)e->memory_used_kb);
    // Per-job limits filled from config defaults.
    TEST_ASSERT_EQUAL_INT(5000, e->instruction_hook_interval);
    TEST_ASSERT_EQUAL_UINT(32768, (unsigned)e->memory_soft_limit_kb);
    TEST_ASSERT_EQUAL_UINT(65536, (unsigned)e->memory_hard_limit_kb);
    TEST_ASSERT_TRUE(e->enforce_limits);

    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// update_progress writes both fields and returns true on a real id.
void test_progress_update_writes_values(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "upd", NULL);

    bool ok = scoreboard_update_progress(sb, id, 12345, 678);
    TEST_ASSERT_TRUE(ok);

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL_UINT64(12345, e->instruction_count);
    TEST_ASSERT_EQUAL_UINT(678, (unsigned)e->memory_used_kb);

    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// Unknown id: returns false, no crash.
void test_progress_update_unknown_id_returns_false(void) {
    Scoreboard* sb = scoreboard_create();
    bool ok = scoreboard_update_progress(sb, "NOPE1", 1, 1);
    TEST_ASSERT_FALSE(ok);
    scoreboard_destroy(sb);
}

// NULL scoreboard: returns false, no crash.
void test_progress_update_null_scoreboard_returns_false(void) {
    bool ok = scoreboard_update_progress(NULL, "x", 1, 1);
    TEST_ASSERT_FALSE(ok);
}

// update_progress is a pure data update: status is unchanged.
void test_progress_update_does_not_change_status(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "status_unchanged", NULL);
    scoreboard_update_status(sb, id, SCOREBOARD_JOB_RUNNING);

    scoreboard_update_progress(sb, id, 999, 1);
    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_EQUAL(SCOREBOARD_JOB_RUNNING, e->status);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// update_progress must not stamp started_at or finished_at.
void test_progress_update_does_not_stamp_timestamps(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "ts_unchanged", NULL);
    scoreboard_update_status(sb, id, SCOREBOARD_JOB_RUNNING);

    ScoreboardEntry* before = scoreboard_find(sb, id);
    struct timespec s_before = before->started_at;
    struct timespec f_before = before->finished_at;
    scoreboard_entry_free(before);

    scoreboard_update_progress(sb, id, 42, 1);

    ScoreboardEntry* after = scoreboard_find(sb, id);
    TEST_ASSERT_EQUAL(s_before.tv_sec, after->started_at.tv_sec);
    TEST_ASSERT_EQUAL(s_before.tv_nsec, after->started_at.tv_nsec);
    TEST_ASSERT_EQUAL(f_before.tv_sec, after->finished_at.tv_sec);
    TEST_ASSERT_EQUAL(f_before.tv_nsec, after->finished_at.tv_nsec);
    scoreboard_entry_free(after);
    free(id);
    scoreboard_destroy(sb);
}

// Monotonic: a later smaller value must NOT reduce instruction_count.
void test_progress_instruction_count_is_monotonic(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "mono", NULL);
    scoreboard_update_progress(sb, id, 1000, 1);
    scoreboard_update_progress(sb, id, 500, 2);

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_EQUAL_UINT64(1000, e->instruction_count);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// Memory is non-monotonic: a smaller value must be recorded.
void test_progress_memory_is_non_monotonic(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "mem", NULL);
    scoreboard_update_progress(sb, id, 100, 200);
    scoreboard_update_progress(sb, id, 200, 50);

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_EQUAL_UINT64(200, e->instruction_count);
    TEST_ASSERT_EQUAL_UINT(50, (unsigned)e->memory_used_kb);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// Per-job limits from submit_with_limits round-trip through find.
void test_progress_per_job_limits_round_trip_through_find(void) {
    Scoreboard* sb = scoreboard_create();
    ScoreboardJobLimits limits = {0};
    limits.instruction_hook_interval = 250;
    limits.memory_soft_limit_kb = 4096;
    limits.memory_hard_limit_kb = 8192;
    limits.enforce_limits = false;
    char* id = scoreboard_submit_with_limits(sb, "limited", NULL, &limits);
    TEST_ASSERT_NOT_NULL(id);

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL_INT(250, e->instruction_hook_interval);
    TEST_ASSERT_EQUAL_UINT(4096, (unsigned)e->memory_soft_limit_kb);
    TEST_ASSERT_EQUAL_UINT(8192, (unsigned)e->memory_hard_limit_kb);
    TEST_ASSERT_FALSE(e->enforce_limits);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// NULL limits pointer: behaves like scoreboard_submit (config defaults,
// enforce=true).
void test_progress_submit_with_null_limits_uses_config(void) {
    Scoreboard* sb = scoreboard_create();
    char* id1 = scoreboard_submit(sb, "plain", NULL);
    char* id2 = scoreboard_submit_with_limits(sb, "plain2", NULL, NULL);
    TEST_ASSERT_NOT_NULL(id1);
    TEST_ASSERT_NOT_NULL(id2);

    ScoreboardEntry* e1 = scoreboard_find(sb, id1);
    ScoreboardEntry* e2 = scoreboard_find(sb, id2);
    TEST_ASSERT_EQUAL(e1->instruction_hook_interval, e2->instruction_hook_interval);
    TEST_ASSERT_EQUAL(e1->memory_soft_limit_kb, e2->memory_soft_limit_kb);
    TEST_ASSERT_EQUAL(e1->memory_hard_limit_kb, e2->memory_hard_limit_kb);
    TEST_ASSERT_EQUAL(e1->enforce_limits, e2->enforce_limits);
    TEST_ASSERT_TRUE(e2->enforce_limits);
    scoreboard_entry_free(e1);
    scoreboard_entry_free(e2);
    free(id1);
    free(id2);
    scoreboard_destroy(sb);
}

// Explicit non-zero limits override config defaults.
void test_progress_submit_with_explicit_limits_overrides(void) {
    Scoreboard* sb = scoreboard_create();
    ScoreboardJobLimits limits = {0};
    limits.instruction_hook_interval = 100;
    limits.memory_soft_limit_kb = 1;
    limits.memory_hard_limit_kb = 2;
    char* id = scoreboard_submit_with_limits(sb, "over", NULL, &limits);

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_EQUAL_INT(100, e->instruction_hook_interval);
    TEST_ASSERT_EQUAL_UINT(1, (unsigned)e->memory_soft_limit_kb);
    TEST_ASSERT_EQUAL_UINT(2, (unsigned)e->memory_hard_limit_kb);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// SIZE_MAX hard limit explicitly means "no limit" (set when config
// default is 0).
void test_progress_submit_with_limits_size_max_means_no_hard(void) {
    Scoreboard* sb = scoreboard_create();
    ScoreboardJobLimits limits = {0};
    limits.instruction_hook_interval = 100;
    limits.memory_soft_limit_kb = 1;
    limits.memory_hard_limit_kb = SIZE_MAX;
    char* id = scoreboard_submit_with_limits(sb, "no_hard", NULL, &limits);

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_TRUE(e->memory_hard_limit_kb == SIZE_MAX);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_progress_initial_fields_are_zero);
    RUN_TEST(test_progress_update_writes_values);
    RUN_TEST(test_progress_update_unknown_id_returns_false);
    RUN_TEST(test_progress_update_null_scoreboard_returns_false);
    RUN_TEST(test_progress_update_does_not_change_status);
    RUN_TEST(test_progress_update_does_not_stamp_timestamps);
    RUN_TEST(test_progress_instruction_count_is_monotonic);
    RUN_TEST(test_progress_memory_is_non_monotonic);
    RUN_TEST(test_progress_per_job_limits_round_trip_through_find);
    RUN_TEST(test_progress_submit_with_null_limits_uses_config);
    RUN_TEST(test_progress_submit_with_explicit_limits_overrides);
    RUN_TEST(test_progress_submit_with_limits_size_max_means_no_hard);

    return UNITY_END();
}
