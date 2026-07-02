/*
 * Unity Test File: scoreboard_test_current_state.c
 *
 * Phase 9 of the LUA_PLAN. Tests scoreboard_update_current_state and
 * the current_state field on ScoreboardEntry:
 *
 *   - A fresh entry's current_state is NULL.
 *   - scoreboard_update_current_state sets the field (strdup's a copy).
 *   - Passing NULL or "" clears the field.
 *   - Multiple updates free the prior value (no leak; verified by
 *     repeated updates and entry destroy, not by ASAN here).
 *   - scoreboard_update_current_state does NOT change status and does
 *     NOT stamp started_at/finished_at.
 *   - scoreboard_update_current_state with a non-existent id returns
 *     false and does not leak the new string.
 *   - scoreboard_update_current_state with NULL scoreboard or NULL
 *     job_id returns false.
 *   - scoreboard_find returns a copy with a strdup'd current_state
 *     that the caller owns and can free independently.
 *   - scoreboard_destroy frees the current_state strings of all
 *     entries (verified by the lifecycle test that follows).
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Standard includes
#include <string.h>

// Module under test
#include <src/scripting/scoreboard.h>

// Forward declarations (required for -Wmissing-prototypes)
void test_current_state_initial_is_null(void);
void test_current_state_update_sets_field(void);
void test_current_state_update_with_null_clears_field(void);
void test_current_state_update_with_empty_clears_field(void);
void test_current_state_multiple_updates_overwrite(void);
void test_current_state_update_unknown_id_returns_false(void);
void test_current_state_update_null_scoreboard_returns_false(void);
void test_current_state_update_null_job_id_returns_false(void);
void test_current_state_update_does_not_change_status(void);
void test_current_state_update_does_not_stamp_timestamps(void);
void test_current_state_find_returns_independent_copy(void);
void test_current_state_destroy_frees_all_entries(void);

void setUp(void) {
}

void tearDown(void) {
}

// A fresh entry's current_state is NULL (the scoreboard's submit path
// memsets the entry to zero before filling it in).
void test_current_state_initial_is_null(void) {
    Scoreboard* sb = scoreboard_create();
    TEST_ASSERT_NOT_NULL(sb);
    char* id = scoreboard_submit(sb, "s", NULL);
    TEST_ASSERT_NOT_NULL(id);

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_NULL(e->current_state);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// Setting current_state populates the field with a strdup'd copy.
void test_current_state_update_sets_field(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);

    TEST_ASSERT_TRUE(scoreboard_update_current_state(sb, id, "Loading customer data"));

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_NOT_NULL(e->current_state);
    TEST_ASSERT_EQUAL_STRING("Loading customer data", e->current_state);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// Passing NULL clears the field.
void test_current_state_update_with_null_clears_field(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);

    scoreboard_update_current_state(sb, id, "first");
    TEST_ASSERT_TRUE(scoreboard_update_current_state(sb, id, NULL));

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_NULL(e->current_state);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// Passing an empty string also clears the field.
void test_current_state_update_with_empty_clears_field(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);

    scoreboard_update_current_state(sb, id, "first");
    TEST_ASSERT_TRUE(scoreboard_update_current_state(sb, id, ""));

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_NULL(e->current_state);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// A repeated update frees the prior value and installs the new one.
void test_current_state_multiple_updates_overwrite(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);

    TEST_ASSERT_TRUE(scoreboard_update_current_state(sb, id, "Day 1 of 300"));
    TEST_ASSERT_TRUE(scoreboard_update_current_state(sb, id, "Day 47 of 300"));
    TEST_ASSERT_TRUE(scoreboard_update_current_state(sb, id, "Day 300 of 300"));

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL_STRING("Day 300 of 300", e->current_state);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// Unknown job_id: returns false. The new string is freed by the
// implementation so no leak is visible to the test (real leak detection
// is ASAN's job, exercised in Test 11).
void test_current_state_update_unknown_id_returns_false(void) {
    Scoreboard* sb = scoreboard_create();
    TEST_ASSERT_FALSE(scoreboard_update_current_state(sb, "ZZZZZ", "ghost"));
    // The scoreboard should still be empty.
    TEST_ASSERT_EQUAL_size_t(0, scoreboard_count(sb));
    scoreboard_destroy(sb);
}

void test_current_state_update_null_scoreboard_returns_false(void) {
    TEST_ASSERT_FALSE(scoreboard_update_current_state(NULL, "AAAAA", "x"));
}

void test_current_state_update_null_job_id_returns_false(void) {
    Scoreboard* sb = scoreboard_create();
    TEST_ASSERT_FALSE(scoreboard_update_current_state(sb, NULL, "x"));
    scoreboard_destroy(sb);
}

// The data-only update must NOT change the entry's status. This is
// the same contract scoreboard_update_progress has; the two data
// writers (the hook and H.set_current_state) must be free to fire
// without changing the lifecycle.
void test_current_state_update_does_not_change_status(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);

    scoreboard_update_status(sb, id, SCOREBOARD_JOB_RUNNING);
    scoreboard_update_current_state(sb, id, "halfway");

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL_INT(SCOREBOARD_JOB_RUNNING, e->status);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

void test_current_state_update_does_not_stamp_timestamps(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);

    // Capture timestamps after the initial submit.
    ScoreboardEntry* before = scoreboard_find(sb, id);
    struct timespec started_before = before->started_at;
    struct timespec finished_before = before->finished_at;
    scoreboard_entry_free(before);

    scoreboard_update_current_state(sb, id, "x");

    ScoreboardEntry* after = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(after);
    TEST_ASSERT_EQUAL_INT(started_before.tv_sec, after->started_at.tv_sec);
    TEST_ASSERT_EQUAL_INT(finished_before.tv_sec, after->finished_at.tv_sec);
    scoreboard_entry_free(after);
    free(id);
    scoreboard_destroy(sb);
}

// The find path must strdup current_state on the copy, so the caller
// can free the copy without affecting the scoreboard's owned value.
void test_current_state_find_returns_independent_copy(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);
    scoreboard_update_current_state(sb, id, "independent");

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_NOT_NULL(e->current_state);
    TEST_ASSERT_EQUAL_STRING("independent", e->current_state);

    // Free the copy and re-find. The scoreboard's value must be intact.
    scoreboard_entry_free(e);

    ScoreboardEntry* e2 = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e2);
    TEST_ASSERT_NOT_NULL(e2->current_state);
    TEST_ASSERT_EQUAL_STRING("independent", e2->current_state);
    scoreboard_entry_free(e2);

    free(id);
    scoreboard_destroy(sb);
}

// scoreboard_destroy frees the current_state strings of every entry.
// This is the "lifecycle" guard: if the destroy path missed the new
// field, the next scoreboard_create/destroy cycle would leak the
// accumulated strings (not a fatal leak, but visible in ASAN).
void test_current_state_destroy_frees_all_entries(void) {
    Scoreboard* sb = scoreboard_create();
    char* id1 = scoreboard_submit(sb, "s1", NULL);
    char* id2 = scoreboard_submit(sb, "s2", NULL);
    char* id3 = scoreboard_submit(sb, "s3", NULL);
    scoreboard_update_current_state(sb, id1, "one");
    scoreboard_update_current_state(sb, id2, "two");
    scoreboard_update_current_state(sb, id3, "three");
    free(id1);
    free(id2);
    free(id3);
    // scoreboard_destroy runs entry_clear_owned on every entry, which
    // is the new code path under test. With ASAN off (trial build),
    // this is a structural check only; ASAN (Test 11) is the
    // authoritative leak detector.
    scoreboard_destroy(sb);
    TEST_PASS();
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_current_state_initial_is_null);
    RUN_TEST(test_current_state_update_sets_field);
    RUN_TEST(test_current_state_update_with_null_clears_field);
    RUN_TEST(test_current_state_update_with_empty_clears_field);
    RUN_TEST(test_current_state_multiple_updates_overwrite);
    RUN_TEST(test_current_state_update_unknown_id_returns_false);
    RUN_TEST(test_current_state_update_null_scoreboard_returns_false);
    RUN_TEST(test_current_state_update_null_job_id_returns_false);
    RUN_TEST(test_current_state_update_does_not_change_status);
    RUN_TEST(test_current_state_update_does_not_stamp_timestamps);
    RUN_TEST(test_current_state_find_returns_independent_copy);
    RUN_TEST(test_current_state_destroy_frees_all_entries);

    return UNITY_END();
}
