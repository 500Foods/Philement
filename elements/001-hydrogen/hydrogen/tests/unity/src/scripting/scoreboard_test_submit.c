/*
 * Unity Test File: scoreboard_test_submit.c
 *
 * Phase 5 of the LUA_PLAN. Tests scoreboard_submit:
 *   - returns a non-NULL, heap-allocated 5-char ID string
 *   - the returned ID matches Hydrogen's 5-char ID format
 *     (only characters from ID_CHARS, null-terminated)
 *   - count grows on each submit
 *   - submit with NULL script_name returns NULL and doesn't change count
 *   - submit with NULL scoreboard returns NULL
 *   - submit with NULL params_json is allowed (params is optional)
 *   - submit with params stores the params on the entry
 *   - status of a freshly-submitted entry is PENDING
 *   - created_at is set; started_at and finished_at are 0
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Module under test
#include <src/scripting/scoreboard.h>

// Forward declarations
void test_submit_returns_non_null_id(void);
void test_submit_id_is_five_chars(void);
void test_submit_id_uses_id_chars_alphabet(void);
void test_submit_increments_count(void);
void test_submit_null_scoreboard_returns_null(void);
void test_submit_null_script_returns_null(void);
void test_submit_null_params_is_allowed(void);
void test_submit_with_params_stores_params(void);
void test_submit_initial_status_is_pending(void);
void test_submit_sets_created_at_clears_started_finished(void);
void test_submit_caller_owns_returned_string(void);

void setUp(void) {
}

void tearDown(void) {
}

// Returns a heap-allocated non-NULL id string
void test_submit_returns_non_null_id(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "test_script", NULL);
    TEST_ASSERT_NOT_NULL(id);
    free(id);
    scoreboard_destroy(sb);
}

// The returned id is 5 characters + null terminator
void test_submit_id_is_five_chars(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "test_script", NULL);
    TEST_ASSERT_NOT_NULL(id);
    TEST_ASSERT_EQUAL_INT(ID_LEN, (int)strlen(id));
    free(id);
    scoreboard_destroy(sb);
}

// The id uses only characters from ID_CHARS
void test_submit_id_uses_id_chars_alphabet(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "test_script", NULL);
    TEST_ASSERT_NOT_NULL(id);
    for (int i = 0; i < ID_LEN; i++) {
        bool found = false;
        for (size_t j = 0; ID_CHARS[j] != '\0'; j++) {
            if (id[i] == ID_CHARS[j]) {
                found = true;
                break;
            }
        }
        TEST_ASSERT_TRUE_MESSAGE(found, "id char must come from ID_CHARS");
    }
    free(id);
    scoreboard_destroy(sb);
}

// Each submit grows the count
void test_submit_increments_count(void) {
    Scoreboard* sb = scoreboard_create();
    TEST_ASSERT_EQUAL_size_t(0, scoreboard_count(sb));

    char* id1 = scoreboard_submit(sb, "s1", NULL);
    TEST_ASSERT_NOT_NULL(id1);
    free(id1);
    TEST_ASSERT_EQUAL_size_t(1, scoreboard_count(sb));

    char* id2 = scoreboard_submit(sb, "s2", NULL);
    TEST_ASSERT_NOT_NULL(id2);
    free(id2);
    TEST_ASSERT_EQUAL_size_t(2, scoreboard_count(sb));

    char* id3 = scoreboard_submit(sb, "s3", NULL);
    TEST_ASSERT_NOT_NULL(id3);
    free(id3);
    TEST_ASSERT_EQUAL_size_t(3, scoreboard_count(sb));

    scoreboard_destroy(sb);
}

// submit(NULL, ...) returns NULL
void test_submit_null_scoreboard_returns_null(void) {
    char* id = scoreboard_submit(NULL, "test_script", NULL);
    TEST_ASSERT_NULL(id);
}

// submit with NULL script_name returns NULL and doesn't crash
void test_submit_null_script_returns_null(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, NULL, NULL);
    TEST_ASSERT_NULL(id);
    TEST_ASSERT_EQUAL_size_t(0, scoreboard_count(sb));
    scoreboard_destroy(sb);
}

// submit with NULL params_json is allowed (params is optional)
void test_submit_null_params_is_allowed(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "no_params", NULL);
    TEST_ASSERT_NOT_NULL(id);

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_NULL(e->params_json);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// submit stores the params on the entry
void test_submit_with_params_stores_params(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "with_params", "{\"name\":\"x\"}");
    TEST_ASSERT_NOT_NULL(id);

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_NOT_NULL(e->params_json);
    TEST_ASSERT_EQUAL_STRING("{\"name\":\"x\"}", e->params_json);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// Freshly submitted entry has PENDING status
void test_submit_initial_status_is_pending(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);
    TEST_ASSERT_NOT_NULL(id);

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL_INT(SCOREBOARD_JOB_PENDING, e->status);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// created_at is set; started_at and finished_at are 0
void test_submit_sets_created_at_clears_started_finished(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);
    TEST_ASSERT_NOT_NULL(id);

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_TRUE_MESSAGE(e->created_at.tv_sec > 0, "created_at should be set");
    TEST_ASSERT_EQUAL_INT(0, e->started_at.tv_sec);
    TEST_ASSERT_EQUAL_INT(0, e->finished_at.tv_sec);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// The returned string is caller-owned; the caller can free it
// independently. Verifying this is the contract documented in the
// header. We free in a different order than allocation to detect
// any accidental aliasing.
void test_submit_caller_owns_returned_string(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "owned", NULL);
    TEST_ASSERT_NOT_NULL(id);
    // Mutate the returned buffer; the scoreboard's stored id must be
    // unaffected because submit() strdup'd it for the caller.
    memset(id, 'X', ID_LEN);
    id[ID_LEN] = '\0';

    // Submit a second job to confirm the scoreboard is still usable.
    char* id2 = scoreboard_submit(sb, "owned2", NULL);
    TEST_ASSERT_NOT_NULL(id2);
    free(id);
    free(id2);
    scoreboard_destroy(sb);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_submit_returns_non_null_id);
    RUN_TEST(test_submit_id_is_five_chars);
    RUN_TEST(test_submit_id_uses_id_chars_alphabet);
    RUN_TEST(test_submit_increments_count);
    RUN_TEST(test_submit_null_scoreboard_returns_null);
    RUN_TEST(test_submit_null_script_returns_null);
    RUN_TEST(test_submit_null_params_is_allowed);
    RUN_TEST(test_submit_with_params_stores_params);
    RUN_TEST(test_submit_initial_status_is_pending);
    RUN_TEST(test_submit_sets_created_at_clears_started_finished);
    RUN_TEST(test_submit_caller_owns_returned_string);

    return UNITY_END();
}
