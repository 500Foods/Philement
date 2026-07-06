/*
 * Unity Test File: scoreboard_test_result.c
 *
 * Phase 25 of the LUA_PLAN. Tests scoreboard_update_result and
 * the result_type/result_location fields on ScoreboardEntry:
 *
 *   - A fresh entry's result fields are NULL.
 *   - scoreboard_update_result sets the fields (strdup's a copy).
 *   - Passing NULL or "" clears a field individually.
 *   - Multiple updates free the prior value (no leak; verified by
 *     repeated updates and entry destroy, not by ASAN here).
 *   - scoreboard_update_result does NOT change status and does
 *     NOT stamp started_at/finished_at.
 *   - scoreboard_update_result with a non-existent id returns
 *     false and does not leak the new string.
 *   - scoreboard_update_result with NULL scoreboard or NULL
 *     job_id returns false.
 *   - scoreboard_find returns a copy with strdup'd result fields
 *     that the caller owns and can free independently.
 *   - scoreboard_list preserves the result fields in copies.
 *   - scoreboard_destroy frees the result field strings of all
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
void test_result_initial_is_null(void);
void test_result_update_sets_fields(void);
void test_result_update_with_null_clears_type(void);
void test_result_update_with_empty_clears_type(void);
void test_result_update_with_null_clears_location(void);
void test_result_update_with_empty_clears_location(void);
void test_result_update_both_fields(void);
void test_result_update_unknown_id_returns_false(void);
void test_result_update_null_scoreboard_returns_false(void);
void test_result_update_null_job_id_returns_false(void);
void test_result_update_does_not_change_status(void);
void test_result_update_does_not_stamp_timestamps(void);
void test_result_find_returns_independent_copy(void);
void test_result_list_preserves_fields(void);
void test_result_destroy_frees_all_entries(void);

void setUp(void) {
}

void tearDown(void) {
}

// A fresh entry's result fields are NULL (the scoreboard's submit path
// memsets the entry to zero before filling it in).
void test_result_initial_is_null(void) {
    Scoreboard* sb = scoreboard_create();
    TEST_ASSERT_NOT_NULL(sb);
    char* id = scoreboard_submit(sb, "s", NULL);
    TEST_ASSERT_NOT_NULL(id);

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_NULL(e->result_type);
    TEST_ASSERT_NULL(e->result_location);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// Setting result_type and result_location populates the fields with strdup'd copies.
void test_result_update_sets_fields(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);

    TEST_ASSERT_TRUE(scoreboard_update_result(sb, id, "json", "/results/job_123.json"));

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_NOT_NULL(e->result_type);
    TEST_ASSERT_NOT_NULL(e->result_location);
    TEST_ASSERT_EQUAL_STRING("json", e->result_type);
    TEST_ASSERT_EQUAL_STRING("/results/job_123.json", e->result_location);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// Passing NULL for result_type clears the field.
void test_result_update_with_null_clears_type(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);

    scoreboard_update_result(sb, id, "json", "/results/1.json");
    TEST_ASSERT_TRUE(scoreboard_update_result(sb, id, NULL, "/results/1.json"));

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_NULL(e->result_type);
    TEST_ASSERT_NOT_NULL(e->result_location);
    TEST_ASSERT_EQUAL_STRING("/results/1.json", e->result_location);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// Passing an empty string for result_type also clears the field.
void test_result_update_with_empty_clears_type(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);

    scoreboard_update_result(sb, id, "json", "/results/1.json");
    TEST_ASSERT_TRUE(scoreboard_update_result(sb, id, "", "/results/1.json"));

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_NULL(e->result_type);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// Passing NULL for result_location clears the field.
void test_result_update_with_null_clears_location(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);

    scoreboard_update_result(sb, id, "json", "/results/1.json");
    TEST_ASSERT_TRUE(scoreboard_update_result(sb, id, "json", NULL));

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_NOT_NULL(e->result_type);
    TEST_ASSERT_NULL(e->result_location);
    TEST_ASSERT_EQUAL_STRING("json", e->result_type);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// Passing an empty string for result_location also clears the field.
void test_result_update_with_empty_clears_location(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);

    scoreboard_update_result(sb, id, "json", "/results/1.json");
    TEST_ASSERT_TRUE(scoreboard_update_result(sb, id, "json", ""));

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_NULL(e->result_location);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// Can set both fields at once.
void test_result_update_both_fields(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);

    TEST_ASSERT_TRUE(scoreboard_update_result(sb, id, "file", "/tmp/report.pdf"));

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL_STRING("file", e->result_type);
    TEST_ASSERT_EQUAL_STRING("/tmp/report.pdf", e->result_location);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// Unknown job_id: returns false. The new strings are freed by the
// implementation so no leak is visible to the test (real leak detection
// is ASAN's job, exercised in Test 11).
void test_result_update_unknown_id_returns_false(void) {
    Scoreboard* sb = scoreboard_create();
    TEST_ASSERT_FALSE(scoreboard_update_result(sb, "ZZZZZ", "json", "/x"));
    // The scoreboard should still be empty.
    TEST_ASSERT_EQUAL_size_t(0, scoreboard_count(sb));
    scoreboard_destroy(sb);
}

void test_result_update_null_scoreboard_returns_false(void) {
    TEST_ASSERT_FALSE(scoreboard_update_result(NULL, "AAAAA", "json", "/x"));
}

void test_result_update_null_job_id_returns_false(void) {
    Scoreboard* sb = scoreboard_create();
    TEST_ASSERT_FALSE(scoreboard_update_result(sb, NULL, "json", "/x"));
    scoreboard_destroy(sb);
}

// The data-only update must NOT change the entry's status. This is
// the same contract scoreboard_update_progress has; the two data
// writers (the hook and H.set_current_state) must be free to fire
// without changing the lifecycle.
void test_result_update_does_not_change_status(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);

    scoreboard_update_status(sb, id, SCOREBOARD_JOB_RUNNING);
    scoreboard_update_result(sb, id, "json", "/x");

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL_INT(SCOREBOARD_JOB_RUNNING, e->status);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

void test_result_update_does_not_stamp_timestamps(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);

    // Capture timestamps after the initial submit.
    ScoreboardEntry* before = scoreboard_find(sb, id);
    struct timespec started_before = before->started_at;
    struct timespec finished_before = before->finished_at;
    scoreboard_entry_free(before);

    scoreboard_update_result(sb, id, "json", "/x");

    ScoreboardEntry* after = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(after);
    TEST_ASSERT_EQUAL_INT(started_before.tv_sec, after->started_at.tv_sec);
    TEST_ASSERT_EQUAL_INT(finished_before.tv_sec, after->finished_at.tv_sec);
    scoreboard_entry_free(after);
    free(id);
    scoreboard_destroy(sb);
}

// The find path must strdup result_type/result_location on the copy, so the caller
// can free the copy without affecting the scoreboard's owned value.
void test_result_find_returns_independent_copy(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);
    scoreboard_update_result(sb, id, "db-ref", "SELECT * FROM jobs WHERE id = 123");

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_NOT_NULL(e->result_type);
    TEST_ASSERT_NOT_NULL(e->result_location);
    TEST_ASSERT_EQUAL_STRING("db-ref", e->result_type);
    TEST_ASSERT_EQUAL_STRING("SELECT * FROM jobs WHERE id = 123", e->result_location);

    // Free the copy and re-find. The scoreboard's value must be intact.
    scoreboard_entry_free(e);

    ScoreboardEntry* e2 = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e2);
    TEST_ASSERT_NOT_NULL(e2->result_type);
    TEST_ASSERT_EQUAL_STRING("db-ref", e2->result_type);
    scoreboard_entry_free(e2);

    free(id);
    scoreboard_destroy(sb);
}

// scoreboard_list must preserve the result fields in copies.
void test_result_list_preserves_fields(void) {
    Scoreboard* sb = scoreboard_create();
    char* id1 = scoreboard_submit(sb, "s1", NULL);
    char* id2 = scoreboard_submit(sb, "s2", NULL);
    scoreboard_update_result(sb, id1, "json", "/out/1.json");
    scoreboard_update_result(sb, id2, "file", "/out/2.pdf");
    free(id1);
    free(id2);

    ScoreboardEntry** list = NULL;
    size_t count = 0;
    TEST_ASSERT_TRUE(scoreboard_list(sb, &list, &count));
    TEST_ASSERT_EQUAL_size_t(2, count);

    // Find the entry for s1 and verify result fields.
    bool found1 = false;
    bool found2 = false;
    for (size_t i = 0; i < count; i++) {
        if (strcmp(list[i]->script_name, "s1") == 0) {
            found1 = true;
            TEST_ASSERT_NOT_NULL(list[i]->result_type);
            TEST_ASSERT_EQUAL_STRING("json", list[i]->result_type);
            TEST_ASSERT_NOT_NULL(list[i]->result_location);
            TEST_ASSERT_EQUAL_STRING("/out/1.json", list[i]->result_location);
        }
        if (strcmp(list[i]->script_name, "s2") == 0) {
            found2 = true;
            TEST_ASSERT_NOT_NULL(list[i]->result_type);
            TEST_ASSERT_EQUAL_STRING("file", list[i]->result_type);
            TEST_ASSERT_NOT_NULL(list[i]->result_location);
            TEST_ASSERT_EQUAL_STRING("/out/2.pdf", list[i]->result_location);
        }
    }
    TEST_ASSERT_TRUE(found1);
    TEST_ASSERT_TRUE(found2);

    scoreboard_list_free(list, count);
    scoreboard_destroy(sb);
}

// scoreboard_destroy frees the result field strings of every entry.
// This is the "lifecycle" guard: if the destroy path missed the new
// fields, the next scoreboard_create/destroy cycle would leak the
// accumulated strings (not a fatal leak, but visible in ASAN).
void test_result_destroy_frees_all_entries(void) {
    Scoreboard* sb = scoreboard_create();
    char* id1 = scoreboard_submit(sb, "s1", NULL);
    char* id2 = scoreboard_submit(sb, "s2", NULL);
    char* id3 = scoreboard_submit(sb, "s3", NULL);
    scoreboard_update_result(sb, id1, "json", "/out/1.json");
    scoreboard_update_result(sb, id2, "file", "/out/2.pdf");
    scoreboard_update_result(sb, id3, "db", "jobs:123");
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

    RUN_TEST(test_result_initial_is_null);
    RUN_TEST(test_result_update_sets_fields);
    RUN_TEST(test_result_update_with_null_clears_type);
    RUN_TEST(test_result_update_with_empty_clears_type);
    RUN_TEST(test_result_update_with_null_clears_location);
    RUN_TEST(test_result_update_with_empty_clears_location);
    RUN_TEST(test_result_update_both_fields);
    RUN_TEST(test_result_update_unknown_id_returns_false);
    RUN_TEST(test_result_update_null_scoreboard_returns_false);
    RUN_TEST(test_result_update_null_job_id_returns_false);
    RUN_TEST(test_result_update_does_not_change_status);
    RUN_TEST(test_result_update_does_not_stamp_timestamps);
    RUN_TEST(test_result_find_returns_independent_copy);
    RUN_TEST(test_result_list_preserves_fields);
    RUN_TEST(test_result_destroy_frees_all_entries);

    return UNITY_END();
}