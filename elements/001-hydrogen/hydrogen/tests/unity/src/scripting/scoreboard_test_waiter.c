/*
 * Unity Test File: scoreboard_test_waiter.c
 *
 * Phase 12 of the LUA_PLAN. Tests the scoreboard-side waiter
 * primitive:
 *   - has_waiter, waiter_handle, result_ref default to false/NULL
 *     on a fresh entry.
 *   - scoreboard_attach_waiter sets all three fields; idempotent
 *     (second attach with different pointers is a no-op, first
 *     writer wins).
 *   - scoreboard_attach_waiter rejects "tag only" attaches (both
 *     pointers NULL) and unknown id.
 *   - scoreboard_get_waiter returns the attached values; safe with
 *     NULL output pointers.
 *   - scoreboard_clear_waiter resets to initial state.
 *   - scoreboard_find copy-on-find preserves the waiter fields.
 *   - scoreboard_list copy preserves the waiter fields.
 *
 * Phase 12 is the scoreboard primitive only. The actual wake-up
 * (H_Handle signal, condvar broadcast) lands in Phase 13; the
 * worker only logs a "would signal waiter" marker for now (see
 * worker_pool.c). The end-to-end "worker fires the waiter hook on
 * completion" assertion is in worker_pool_test_waiter.c.
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Standard includes
#include <string.h>

// Module under test
#include <src/scripting/scoreboard.h>

// Forward declarations (required for -Wmissing-prototypes)
void test_waiter_initial_values_are_default(void);
void test_waiter_attach_sets_all_three_fields(void);
void test_waiter_attach_is_idempotent_first_writer_wins(void);
void test_waiter_attach_with_only_handle_succeeds(void);
void test_waiter_attach_with_only_result_succeeds(void);
void test_waiter_attach_with_both_null_returns_false(void);
void test_waiter_attach_unknown_id_returns_false(void);
void test_waiter_attach_null_safety(void);
void test_waiter_get_returns_attached_values(void);
void test_waiter_get_with_null_outputs_is_safe(void);
void test_waiter_get_unknown_id_returns_false(void);
void test_waiter_clear_resets_to_initial(void);
void test_waiter_clear_unknown_id_returns_false(void);
void test_waiter_clear_idempotent(void);
void test_waiter_find_preserves_fields(void);
void test_waiter_list_preserves_fields(void);

void setUp(void) {
}

void tearDown(void) {
}

// A fresh entry's waiter fields are all default (false / NULL).
// The scoreboard's submit path memsets the entry to zero before
// filling it in, so the new fields are zero-initialised.
void test_waiter_initial_values_are_default(void) {
    Scoreboard* sb = scoreboard_create();
    TEST_ASSERT_NOT_NULL(sb);
    char* id = scoreboard_submit(sb, "s", NULL);
    TEST_ASSERT_NOT_NULL(id);

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_FALSE(e->has_waiter);
    TEST_ASSERT_NULL(e->waiter_handle);
    TEST_ASSERT_NULL(e->result_ref);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// attach sets all three fields. The pointers are stored verbatim;
// the scoreboard does not dereference them in Phase 12.
void test_waiter_attach_sets_all_three_fields(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);

    int sentinel_a = 0;
    int sentinel_b = 0;
    TEST_ASSERT_TRUE(scoreboard_attach_waiter(sb, id, &sentinel_a, &sentinel_b));

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_TRUE(e->has_waiter);
    TEST_ASSERT_EQUAL_PTR(&sentinel_a, e->waiter_handle);
    TEST_ASSERT_EQUAL_PTR(&sentinel_b, e->result_ref);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// A second attach with different pointers does NOT overwrite the
// first. First writer wins; this matches the submitter's typical
// pattern ("attach if not already attached") and prevents a racing
// attach from clobbering a waiter that is already in place.
void test_waiter_attach_is_idempotent_first_writer_wins(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);

    int first_handle = 1;
    int first_result = 2;
    int second_handle = 3;
    int second_result = 4;

    TEST_ASSERT_TRUE(scoreboard_attach_waiter(sb, id,
                                              &first_handle, &first_result));
    TEST_ASSERT_TRUE(scoreboard_attach_waiter(sb, id,
                                              &second_handle, &second_result));

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_TRUE(e->has_waiter);
    TEST_ASSERT_EQUAL_PTR(&first_handle, e->waiter_handle);
    TEST_ASSERT_EQUAL_PTR(&first_result, e->result_ref);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// A handle with no result_ref is a legitimate attach (the waiter
// only cares about completion, not the result).
void test_waiter_attach_with_only_handle_succeeds(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);

    int sentinel = 0;
    TEST_ASSERT_TRUE(scoreboard_attach_waiter(sb, id, &sentinel, NULL));

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_TRUE(e->has_waiter);
    TEST_ASSERT_EQUAL_PTR(&sentinel, e->waiter_handle);
    TEST_ASSERT_NULL(e->result_ref);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// A result_ref with no handle is a legitimate attach (the result
// storage is enough; the completion signal is fanned out by some
// other mechanism).
void test_waiter_attach_with_only_result_succeeds(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);

    int sentinel = 0;
    TEST_ASSERT_TRUE(scoreboard_attach_waiter(sb, id, NULL, &sentinel));

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_TRUE(e->has_waiter);
    TEST_ASSERT_NULL(e->waiter_handle);
    TEST_ASSERT_EQUAL_PTR(&sentinel, e->result_ref);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// "Tag only" attaches (no handle, no result) are not useful and
// would surprise the worker. Reject and return false.
void test_waiter_attach_with_both_null_returns_false(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);

    TEST_ASSERT_FALSE(scoreboard_attach_waiter(sb, id, NULL, NULL));

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_FALSE(e->has_waiter);
    TEST_ASSERT_NULL(e->waiter_handle);
    TEST_ASSERT_NULL(e->result_ref);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// Unknown id returns false and does not change any state (the
// entry the caller asked about does not exist).
void test_waiter_attach_unknown_id_returns_false(void) {
    Scoreboard* sb = scoreboard_create();
    int sentinel = 0;
    TEST_ASSERT_FALSE(scoreboard_attach_waiter(sb, "ZZZZZ",
                                               &sentinel, &sentinel));
    TEST_ASSERT_EQUAL_UINT(0, scoreboard_count(sb));
    scoreboard_destroy(sb);
}

// NULL scoreboard or NULL job_id are safe no-ops.
void test_waiter_attach_null_safety(void) {
    int sentinel = 0;
    TEST_ASSERT_FALSE(scoreboard_attach_waiter(NULL, "ABCDE",
                                               &sentinel, &sentinel));
    Scoreboard* sb = scoreboard_create();
    TEST_ASSERT_FALSE(scoreboard_attach_waiter(sb, NULL,
                                               &sentinel, &sentinel));
    scoreboard_destroy(sb);
}

// get returns the attached values. The returned pointers are
// owned by the scoreboard; the caller does not free them.
void test_waiter_get_returns_attached_values(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);

    int sentinel_a = 0;
    int sentinel_b = 0;
    TEST_ASSERT_TRUE(scoreboard_attach_waiter(sb, id,
                                              &sentinel_a, &sentinel_b));

    bool has_waiter = false;
    void* handle = NULL;
    void* result = NULL;
    TEST_ASSERT_TRUE(scoreboard_get_waiter(sb, id,
                                           &has_waiter, &handle, &result));
    TEST_ASSERT_TRUE(has_waiter);
    TEST_ASSERT_EQUAL_PTR(&sentinel_a, handle);
    TEST_ASSERT_EQUAL_PTR(&sentinel_b, result);

    free(id);
    scoreboard_destroy(sb);
}

// get is safe when the optional output pointers are NULL.
void test_waiter_get_with_null_outputs_is_safe(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);

    bool has_waiter = false;
    TEST_ASSERT_TRUE(scoreboard_get_waiter(sb, id,
                                           &has_waiter, NULL, NULL));
    TEST_ASSERT_FALSE(has_waiter);

    free(id);
    scoreboard_destroy(sb);
}

// Unknown id returns false and leaves the out_has_waiter as false.
void test_waiter_get_unknown_id_returns_false(void) {
    Scoreboard* sb = scoreboard_create();
    bool has_waiter = true;   // deliberately non-default to verify reset
    void* handle = (void*)0xDEADBEEF;
    void* result = (void*)0xCAFEBABE;
    TEST_ASSERT_FALSE(scoreboard_get_waiter(sb, "ZZZZZ",
                                           &has_waiter, &handle, &result));
    TEST_ASSERT_FALSE(has_waiter);
    TEST_ASSERT_NULL(handle);
    TEST_ASSERT_NULL(result);
    scoreboard_destroy(sb);
}

// clear resets to initial state. The same id can then be
// re-attached with fresh pointers.
void test_waiter_clear_resets_to_initial(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);

    int sentinel = 0;
    TEST_ASSERT_TRUE(scoreboard_attach_waiter(sb, id, &sentinel, &sentinel));
    TEST_ASSERT_TRUE(scoreboard_clear_waiter(sb, id));

    bool has_waiter = true;
    void* handle = NULL;
    void* result = NULL;
    TEST_ASSERT_TRUE(scoreboard_get_waiter(sb, id,
                                           &has_waiter, &handle, &result));
    TEST_ASSERT_FALSE(has_waiter);
    TEST_ASSERT_NULL(handle);
    TEST_ASSERT_NULL(result);

    free(id);
    scoreboard_destroy(sb);
}

// Unknown id returns false; no state changes.
void test_waiter_clear_unknown_id_returns_false(void) {
    Scoreboard* sb = scoreboard_create();
    TEST_ASSERT_FALSE(scoreboard_clear_waiter(sb, "ZZZZZ"));
    TEST_ASSERT_EQUAL_UINT(0, scoreboard_count(sb));
    scoreboard_destroy(sb);
}

// clear on an entry that has no waiter is a no-op that still
// returns true (the entry was found; nothing to clear).
void test_waiter_clear_idempotent(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);

    TEST_ASSERT_TRUE(scoreboard_clear_waiter(sb, id));
    TEST_ASSERT_TRUE(scoreboard_clear_waiter(sb, id));

    free(id);
    scoreboard_destroy(sb);
}

// scoreboard_find returns a copy whose waiter fields reflect the
// scoreboard's state. The copy's pointers may differ from the
// scoreboard's (POD fields are not strdup'd because they are
// not owned), but the has_waiter flag and the pointer values
// round-trip.
void test_waiter_find_preserves_fields(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);

    int sentinel_a = 0;
    int sentinel_b = 0;
    TEST_ASSERT_TRUE(scoreboard_attach_waiter(sb, id,
                                              &sentinel_a, &sentinel_b));

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_TRUE(e->has_waiter);
    TEST_ASSERT_EQUAL_PTR(&sentinel_a, e->waiter_handle);
    TEST_ASSERT_EQUAL_PTR(&sentinel_b, e->result_ref);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// scoreboard_list preserves the waiter fields on every copy. This
// is the round-trip that H.scoreboard.list (Phase 11) relies on.
void test_waiter_list_preserves_fields(void) {
    Scoreboard* sb = scoreboard_create();
    char* id1 = scoreboard_submit(sb, "a", NULL);
    char* id2 = scoreboard_submit(sb, "b", NULL);

    int sentinel = 0;
    TEST_ASSERT_TRUE(scoreboard_attach_waiter(sb, id1, &sentinel, NULL));
    // id2 intentionally has no waiter attached.

    ScoreboardEntry** list = NULL;
    size_t count = 0;
    TEST_ASSERT_TRUE(scoreboard_list(sb, &list, &count));
    TEST_ASSERT_EQUAL_UINT(2, count);

    bool found_attached = false;
    bool found_no_waiter = false;
    for (size_t i = 0; i < count; i++) {
        if (list[i]->has_waiter) {
            TEST_ASSERT_EQUAL_PTR(&sentinel, list[i]->waiter_handle);
            found_attached = true;
        } else {
            TEST_ASSERT_NULL(list[i]->waiter_handle);
            TEST_ASSERT_NULL(list[i]->result_ref);
            found_no_waiter = true;
        }
    }
    TEST_ASSERT_TRUE(found_attached);
    TEST_ASSERT_TRUE(found_no_waiter);

    scoreboard_list_free(list, count);
    free(id1);
    free(id2);
    scoreboard_destroy(sb);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_waiter_initial_values_are_default);
    RUN_TEST(test_waiter_attach_sets_all_three_fields);
    RUN_TEST(test_waiter_attach_is_idempotent_first_writer_wins);
    RUN_TEST(test_waiter_attach_with_only_handle_succeeds);
    RUN_TEST(test_waiter_attach_with_only_result_succeeds);
    RUN_TEST(test_waiter_attach_with_both_null_returns_false);
    RUN_TEST(test_waiter_attach_unknown_id_returns_false);
    RUN_TEST(test_waiter_attach_null_safety);
    RUN_TEST(test_waiter_get_returns_attached_values);
    RUN_TEST(test_waiter_get_with_null_outputs_is_safe);
    RUN_TEST(test_waiter_get_unknown_id_returns_false);
    RUN_TEST(test_waiter_clear_resets_to_initial);
    RUN_TEST(test_waiter_clear_unknown_id_returns_false);
    RUN_TEST(test_waiter_clear_idempotent);
    RUN_TEST(test_waiter_find_preserves_fields);
    RUN_TEST(test_waiter_list_preserves_fields);

    return UNITY_END();
}
