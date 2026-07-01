/*
 * Unity Test File: scoreboard_test_find.c
 *
 * Phase 5 of the LUA_PLAN. Tests scoreboard_find:
 *   - find returns a heap copy of the entry
 *   - find on unknown id returns NULL
 *   - find with NULL scoreboard returns NULL
 *   - find with NULL job_id returns NULL
 *   - the returned copy is independent of subsequent submit/update
 *     (realloc + status change do not invalidate the copy)
 *   - the returned copy has its own ownership of script_name and
 *     params_json (caller can free without affecting the scoreboard)
 *   - scoreboard_entry_free(NULL) is safe
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Standard includes
#include <string.h>

// Module under test
#include <src/scripting/scoreboard.h>

// Forward declarations
void test_find_returns_copy_of_submitted_entry(void);
void test_find_unknown_id_returns_null(void);
void test_find_null_scoreboard_returns_null(void);
void test_find_null_id_returns_null(void);
void test_find_copy_survives_subsequent_submit(void);
void test_find_copy_survives_status_update(void);
void test_find_copy_owns_its_strings(void);
void test_entry_free_null_is_safe(void);
void test_entry_free_releases_owned_strings(void);

void setUp(void) {
}

void tearDown(void) {
}

// find on a known id returns a copy with matching fields
void test_find_returns_copy_of_submitted_entry(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "lookup_script", "{\"k\":1}");
    TEST_ASSERT_NOT_NULL(id);

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL_STRING(id, e->job_id);
    TEST_ASSERT_EQUAL_STRING("lookup_script", e->script_name);
    TEST_ASSERT_EQUAL_STRING("{\"k\":1}", e->params_json);
    TEST_ASSERT_EQUAL_INT(SCOREBOARD_JOB_PENDING, e->status);

    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// find on an id that was never submitted returns NULL
void test_find_unknown_id_returns_null(void) {
    Scoreboard* sb = scoreboard_create();
    ScoreboardEntry* e = scoreboard_find(sb, "ZZZZZ");
    TEST_ASSERT_NULL(e);
    scoreboard_destroy(sb);
}

// find(NULL, ...) is safe and returns NULL
void test_find_null_scoreboard_returns_null(void) {
    ScoreboardEntry* e = scoreboard_find(NULL, "AAAAA");
    TEST_ASSERT_NULL(e);
}

// find(sb, NULL) is safe and returns NULL
void test_find_null_id_returns_null(void) {
    Scoreboard* sb = scoreboard_create();
    ScoreboardEntry* e = scoreboard_find(sb, NULL);
    TEST_ASSERT_NULL(e);
    scoreboard_destroy(sb);
}

// A found copy is independent of later submits (the realloc that
// grows the array cannot have moved the caller's copy because the
// copy was already strdup'd).
void test_find_copy_survives_subsequent_submit(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "stable", "{\"a\":1}");
    TEST_ASSERT_NOT_NULL(id);

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);

    // Force several reallocs to exercise the grow path.
    for (int i = 0; i < 100; i++) {
        char* tmp = scoreboard_submit(sb, "grow", NULL);
        free(tmp);
    }

    // The original copy's contents must still be intact.
    TEST_ASSERT_EQUAL_STRING(id, e->job_id);
    TEST_ASSERT_EQUAL_STRING("stable", e->script_name);
    TEST_ASSERT_EQUAL_STRING("{\"a\":1}", e->params_json);

    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// A found copy is independent of later status updates on the entry.
void test_find_copy_survives_status_update(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "stale", NULL);
    TEST_ASSERT_NOT_NULL(id);

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL_INT(SCOREBOARD_JOB_PENDING, e->status);

    // Move the underlying entry to RUNNING after we have the copy.
    bool ok = scoreboard_update_status(sb, id, SCOREBOARD_JOB_RUNNING);
    TEST_ASSERT_TRUE(ok);

    // The original copy's status is still PENDING - it was a value
    // copy, not a reference to the live entry.
    TEST_ASSERT_EQUAL_INT(SCOREBOARD_JOB_PENDING, e->status);

    // But the live entry really did move.
    ScoreboardEntry* e2 = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e2);
    TEST_ASSERT_EQUAL_INT(SCOREBOARD_JOB_RUNNING, e2->status);
    scoreboard_entry_free(e2);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// The returned copy owns its script_name and params_json; freeing the
// copy does not corrupt the scoreboard's ability to find the entry
// again, and the next find returns intact values.
void test_find_copy_owns_its_strings(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "owned", "{\"x\":\"y\"}");
    TEST_ASSERT_NOT_NULL(id);

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    // Mutate the copy's strings.
    strcpy(e->script_name, "MUTATED");
    strcpy(e->params_json, "{}");
    scoreboard_entry_free(e);

    // Re-find the same id; the original "owned" / "{\"x\":\"y\"}"
    // must still be there.
    ScoreboardEntry* e2 = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e2);
    TEST_ASSERT_EQUAL_STRING("owned", e2->script_name);
    TEST_ASSERT_EQUAL_STRING("{\"x\":\"y\"}", e2->params_json);
    scoreboard_entry_free(e2);

    free(id);
    scoreboard_destroy(sb);
}

// scoreboard_entry_free(NULL) is safe
void test_entry_free_null_is_safe(void) {
    scoreboard_entry_free(NULL);
    TEST_ASSERT_TRUE(1);
}

// After entry_free, freeing the script_name/params_json pointers the
// entry held would be a double-free. We can't test the double-free
// directly, but we can verify the helper survives a populated entry
// and that destroying the scoreboard afterwards works (i.e. there
// was no aliasing).
void test_entry_free_releases_owned_strings(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "release", "{\"a\":1}");
    TEST_ASSERT_NOT_NULL(id);

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    scoreboard_entry_free(e);

    // The scoreboard is still functional after we freed the copy.
    char* id2 = scoreboard_submit(sb, "after_free", NULL);
    TEST_ASSERT_NOT_NULL(id2);
    TEST_ASSERT_EQUAL_size_t(2, scoreboard_count(sb));
    free(id);
    free(id2);
    scoreboard_destroy(sb);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_find_returns_copy_of_submitted_entry);
    RUN_TEST(test_find_unknown_id_returns_null);
    RUN_TEST(test_find_null_scoreboard_returns_null);
    RUN_TEST(test_find_null_id_returns_null);
    RUN_TEST(test_find_copy_survives_subsequent_submit);
    RUN_TEST(test_find_copy_survives_status_update);
    RUN_TEST(test_find_copy_owns_its_strings);
    RUN_TEST(test_entry_free_null_is_safe);
    RUN_TEST(test_entry_free_releases_owned_strings);

    return UNITY_END();
}
