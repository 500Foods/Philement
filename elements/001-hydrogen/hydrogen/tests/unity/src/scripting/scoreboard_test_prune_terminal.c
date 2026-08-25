/*
 * Unity Test File: scoreboard_test_prune_terminal.c
 *
 * Tests scoreboard_prune_terminal:
 *   - NULL scoreboard returns 0 (no crash)
 *   - empty scoreboard returns 0
 *   - all-terminal scoreboard is emptied, count returns number pruned
 *   - only terminal entries are removed; PENDING/RUNNING preserved
 *   - non-terminal entries preserve their relative order
 *   - mixed PENDING/RUNNING/COMPLETED/FAILED/KILLED: prunes only terminal
 *   - prune twice is idempotent (second call returns 0)
 *   - after prune, submitted jobs reuse compacted slots
 *   - owned strings freed by prune (destroy is clean)
 *   - result_json / params_json owned strings are freed on prune
 *
 * Leak detection is authoritative under ASAN (test_11); here we verify
 * structural correctness.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <string.h>

#include <src/scripting/scoreboard.h>

#define TEST_RESULT_JSON_MAX 512

void setUp(void) {
}

void tearDown(void) {
}

void test_prune_null_scoreboard_returns_zero(void);
void test_prune_empty_scoreboard_returns_zero(void);
void test_prune_all_completed_returns_count_and_empties(void);
void test_prune_all_failed_returns_count_and_empties(void);
void test_prune_all_killed_returns_count_and_empties(void);
void test_prune_preserves_pending_and_running(void);
void test_prune_mixed_states_prunes_only_terminal(void);
void test_prune_preserves_relative_order_of_non_terminal(void);
void test_prune_then_submit_reuses_slots(void);
void test_prune_idempotent_second_call_returns_zero(void);
void test_prune_frees_owned_strings(void);
void test_prune_frees_result_json(void);
void test_prune_frees_params_json(void);

void test_prune_null_scoreboard_returns_zero(void) {
    TEST_ASSERT_EQUAL_size_t(0, scoreboard_prune_terminal(NULL));
}

void test_prune_empty_scoreboard_returns_zero(void) {
    Scoreboard* sb = scoreboard_create();
    TEST_ASSERT_NOT_NULL(sb);
    TEST_ASSERT_EQUAL_size_t(0, scoreboard_prune_terminal(sb));
    TEST_ASSERT_EQUAL_size_t(0, scoreboard_count(sb));
    scoreboard_destroy(sb);
}

void test_prune_all_completed_returns_count_and_empties(void) {
    Scoreboard* sb = scoreboard_create();
    char* id1 = scoreboard_submit(sb, "s1", NULL);
    char* id2 = scoreboard_submit(sb, "s2", NULL);
    char* id3 = scoreboard_submit(sb, "s3", NULL);
    TEST_ASSERT_NOT_NULL(id1);
    TEST_ASSERT_NOT_NULL(id2);
    TEST_ASSERT_NOT_NULL(id3);

    scoreboard_update_status(sb, id1, SCOREBOARD_JOB_COMPLETED);
    scoreboard_update_status(sb, id2, SCOREBOARD_JOB_COMPLETED);
    scoreboard_update_status(sb, id3, SCOREBOARD_JOB_COMPLETED);

    size_t pruned = scoreboard_prune_terminal(sb);
    TEST_ASSERT_EQUAL_size_t(3, pruned);
    TEST_ASSERT_EQUAL_size_t(0, scoreboard_count(sb));

    free(id1);
    free(id2);
    free(id3);
    scoreboard_destroy(sb);
}

void test_prune_all_failed_returns_count_and_empties(void) {
    Scoreboard* sb = scoreboard_create();
    char* id1 = scoreboard_submit(sb, "s1", NULL);
    char* id2 = scoreboard_submit(sb, "s2", NULL);
    TEST_ASSERT_NOT_NULL(id1);
    TEST_ASSERT_NOT_NULL(id2);

    scoreboard_update_status(sb, id1, SCOREBOARD_JOB_FAILED);
    scoreboard_update_status(sb, id2, SCOREBOARD_JOB_FAILED);

    size_t pruned = scoreboard_prune_terminal(sb);
    TEST_ASSERT_EQUAL_size_t(2, pruned);
    TEST_ASSERT_EQUAL_size_t(0, scoreboard_count(sb));

    free(id1);
    free(id2);
    scoreboard_destroy(sb);
}

void test_prune_all_killed_returns_count_and_empties(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);
    TEST_ASSERT_NOT_NULL(id);

    scoreboard_update_status(sb, id, SCOREBOARD_JOB_KILLED);

    size_t pruned = scoreboard_prune_terminal(sb);
    TEST_ASSERT_EQUAL_size_t(1, pruned);
    TEST_ASSERT_EQUAL_size_t(0, scoreboard_count(sb));

    free(id);
    scoreboard_destroy(sb);
}

void test_prune_preserves_pending_and_running(void) {
    Scoreboard* sb = scoreboard_create();
    char* id_pending = scoreboard_submit(sb, "pending_job", NULL);
    char* id_running = scoreboard_submit(sb, "running_job", NULL);
    char* id_completed = scoreboard_submit(sb, "completed_job", NULL);
    TEST_ASSERT_NOT_NULL(id_pending);
    TEST_ASSERT_NOT_NULL(id_running);
    TEST_ASSERT_NOT_NULL(id_completed);

    scoreboard_update_status(sb, id_running, SCOREBOARD_JOB_RUNNING);
    scoreboard_update_status(sb, id_completed, SCOREBOARD_JOB_COMPLETED);

    size_t pruned = scoreboard_prune_terminal(sb);
    TEST_ASSERT_EQUAL_size_t(1, pruned);
    TEST_ASSERT_EQUAL_size_t(2, scoreboard_count(sb));

    /* The two surviving entries should be findable. */
    ScoreboardEntry* e1 = scoreboard_find(sb, id_pending);
    TEST_ASSERT_NOT_NULL(e1);
    TEST_ASSERT_EQUAL_INT(SCOREBOARD_JOB_PENDING, e1->status);
    scoreboard_entry_free(e1);

    ScoreboardEntry* e2 = scoreboard_find(sb, id_running);
    TEST_ASSERT_NOT_NULL(e2);
    TEST_ASSERT_EQUAL_INT(SCOREBOARD_JOB_RUNNING, e2->status);
    scoreboard_entry_free(e2);

    /* The pruned entry must be gone. */
    ScoreboardEntry* e3 = scoreboard_find(sb, id_completed);
    TEST_ASSERT_NULL(e3);

    free(id_pending);
    free(id_running);
    free(id_completed);
    scoreboard_destroy(sb);
}

void test_prune_mixed_states_prunes_only_terminal(void) {
    Scoreboard* sb = scoreboard_create();
    /* Interleave terminal and non-terminal entries. */
    char* ids[6];
    for (int i = 0; i < 6; i++) {
        char name[32];
        snprintf(name, sizeof(name), "job_%d", i);
        ids[i] = scoreboard_submit(sb, name, NULL);
        TEST_ASSERT_NOT_NULL(ids[i]);
    }

    /* 0: PENDING (keep), 1: COMPLETED (prune), 2: RUNNING (keep) */
    scoreboard_update_status(sb, ids[1], SCOREBOARD_JOB_COMPLETED);
    scoreboard_update_status(sb, ids[2], SCOREBOARD_JOB_RUNNING);
    /* 3: FAILED (prune), 4: PENDING (keep), 5: KILLED (prune) */
    scoreboard_update_status(sb, ids[3], SCOREBOARD_JOB_FAILED);
    scoreboard_update_status(sb, ids[5], SCOREBOARD_JOB_KILLED);

    size_t pruned = scoreboard_prune_terminal(sb);
    TEST_ASSERT_EQUAL_size_t(3, pruned);
    TEST_ASSERT_EQUAL_size_t(3, scoreboard_count(sb));

    /* Survivors: ids[0], ids[2], ids[4] */
    TEST_ASSERT_NOT_NULL(scoreboard_find(sb, ids[0]));
    TEST_ASSERT_NOT_NULL(scoreboard_find(sb, ids[2]));
    TEST_ASSERT_NOT_NULL(scoreboard_find(sb, ids[4]));
    TEST_ASSERT_NULL(scoreboard_find(sb, ids[1]));
    TEST_ASSERT_NULL(scoreboard_find(sb, ids[3]));
    TEST_ASSERT_NULL(scoreboard_find(sb, ids[5]));

    for (int i = 0; i < 6; i++) {
        scoreboard_entry_free(scoreboard_find(sb, ids[i]));
        free(ids[i]);
    }
    scoreboard_destroy(sb);
}

void test_prune_preserves_relative_order_of_non_terminal(void) {
    Scoreboard* sb = scoreboard_create();
    char* id_a = scoreboard_submit(sb, "a", NULL); /* keep */
    char* id_b = scoreboard_submit(sb, "b", NULL); /* prune */
    char* id_c = scoreboard_submit(sb, "c", NULL); /* keep */
    char* id_d = scoreboard_submit(sb, "d", NULL); /* prune */
    char* id_e = scoreboard_submit(sb, "e", NULL); /* keep */
    TEST_ASSERT_NOT_NULL(id_a);
    TEST_ASSERT_NOT_NULL(id_b);
    TEST_ASSERT_NOT_NULL(id_c);
    TEST_ASSERT_NOT_NULL(id_d);
    TEST_ASSERT_NOT_NULL(id_e);

    scoreboard_update_status(sb, id_b, SCOREBOARD_JOB_COMPLETED);
    scoreboard_update_status(sb, id_d, SCOREBOARD_JOB_FAILED);

    size_t pruned = scoreboard_prune_terminal(sb);
    TEST_ASSERT_EQUAL_size_t(2, pruned);
    TEST_ASSERT_EQUAL_size_t(3, scoreboard_count(sb));

    /* Snapshot and verify order is preserved: a, c, e */
    ScoreboardEntry** list = NULL;
    size_t count = 0;
    TEST_ASSERT_TRUE(scoreboard_list(sb, &list, &count));
    TEST_ASSERT_EQUAL_size_t(3, count);
    TEST_ASSERT_EQUAL_STRING("a", list[0]->script_name);
    TEST_ASSERT_EQUAL_STRING("c", list[1]->script_name);
    TEST_ASSERT_EQUAL_STRING("e", list[2]->script_name);
    scoreboard_list_free(list, count);

    free(id_a);
    free(id_b);
    free(id_c);
    free(id_d);
    free(id_e);
    scoreboard_destroy(sb);
}

void test_prune_then_submit_reuses_slots(void) {
    Scoreboard* sb = scoreboard_create();
    char* id1 = scoreboard_submit(sb, "s1", NULL);
    char* id2 = scoreboard_submit(sb, "s2", NULL);
    TEST_ASSERT_NOT_NULL(id1);
    TEST_ASSERT_NOT_NULL(id2);

    scoreboard_update_status(sb, id1, SCOREBOARD_JOB_COMPLETED);
    scoreboard_update_status(sb, id2, SCOREBOARD_JOB_FAILED);

    size_t pruned = scoreboard_prune_terminal(sb);
    TEST_ASSERT_EQUAL_size_t(2, pruned);
    TEST_ASSERT_EQUAL_size_t(0, scoreboard_count(sb));

    /* After full prune, a new submit should work normally. */
    char* id3 = scoreboard_submit(sb, "s3", NULL);
    TEST_ASSERT_NOT_NULL(id3);
    TEST_ASSERT_EQUAL_size_t(1, scoreboard_count(sb));

    ScoreboardEntry* e = scoreboard_find(sb, id3);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL_STRING("s3", e->script_name);
    scoreboard_entry_free(e);

    free(id1);
    free(id2);
    free(id3);
    scoreboard_destroy(sb);
}

void test_prune_idempotent_second_call_returns_zero(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);
    TEST_ASSERT_NOT_NULL(id);

    scoreboard_update_status(sb, id, SCOREBOARD_JOB_COMPLETED);

    size_t first = scoreboard_prune_terminal(sb);
    TEST_ASSERT_EQUAL_size_t(1, first);

    size_t second = scoreboard_prune_terminal(sb);
    TEST_ASSERT_EQUAL_size_t(0, second);

    free(id);
    scoreboard_destroy(sb);
}

void test_prune_frees_owned_strings(void) {
    Scoreboard* sb = scoreboard_create();
    /* Submit with params_json to exercise entry_clear_owned path. */
    char* id = scoreboard_submit(sb, "s", "{\"key\":\"value\"}");
    TEST_ASSERT_NOT_NULL(id);

    scoreboard_update_status(sb, id, SCOREBOARD_JOB_COMPLETED);

    size_t pruned = scoreboard_prune_terminal(sb);
    TEST_ASSERT_EQUAL_size_t(1, pruned);

    /* destroy must not crash — the owned params_json was freed by prune. */
    free(id);
    scoreboard_destroy(sb);
}

void test_prune_frees_result_json(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);
    TEST_ASSERT_NOT_NULL(id);

    char big_json[TEST_RESULT_JSON_MAX];
    memset(big_json, 'x', sizeof(big_json) - 1);
    big_json[sizeof(big_json) - 1] = '\0';
    TEST_ASSERT_TRUE(scoreboard_update_result_json(sb, id, big_json));

    scoreboard_update_status(sb, id, SCOREBOARD_JOB_COMPLETED);

    size_t pruned = scoreboard_prune_terminal(sb);
    TEST_ASSERT_EQUAL_size_t(1, pruned);

    /* destroy must not crash — result_json was freed by prune. */
    free(id);
    scoreboard_destroy(sb);
}

void test_prune_frees_params_json(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", "{\"param\":\"data\"}");
    TEST_ASSERT_NOT_NULL(id);

    scoreboard_update_status(sb, id, SCOREBOARD_JOB_FAILED);

    size_t pruned = scoreboard_prune_terminal(sb);
    TEST_ASSERT_EQUAL_size_t(1, pruned);
    TEST_ASSERT_EQUAL_size_t(0, scoreboard_count(sb));

    /* destroy must not crash — params_json was freed by prune. */
    free(id);
    scoreboard_destroy(sb);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_prune_null_scoreboard_returns_zero);
    RUN_TEST(test_prune_empty_scoreboard_returns_zero);
    RUN_TEST(test_prune_all_completed_returns_count_and_empties);
    RUN_TEST(test_prune_all_failed_returns_count_and_empties);
    RUN_TEST(test_prune_all_killed_returns_count_and_empties);
    RUN_TEST(test_prune_preserves_pending_and_running);
    RUN_TEST(test_prune_mixed_states_prunes_only_terminal);
    RUN_TEST(test_prune_preserves_relative_order_of_non_terminal);
    RUN_TEST(test_prune_then_submit_reuses_slots);
    RUN_TEST(test_prune_idempotent_second_call_returns_zero);
    RUN_TEST(test_prune_frees_owned_strings);
    RUN_TEST(test_prune_frees_result_json);
    RUN_TEST(test_prune_frees_params_json);

    return UNITY_END();
}
