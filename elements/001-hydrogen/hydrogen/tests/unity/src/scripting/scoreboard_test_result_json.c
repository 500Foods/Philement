/*
 * Unity Test File: scoreboard_test_result_json.c
 *
 * LUA_CLIENT Phase 1: scoreboard_update_result_json and result_json field.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <stdlib.h>
#include <string.h>

#include <src/scripting/scoreboard.h>

void test_result_json_initial_null(void);
void test_result_json_update_sets(void);
void test_result_json_overwrite(void);
void test_result_json_clear_null(void);
void test_result_json_clear_empty(void);
void test_result_json_too_large_rejects(void);
void test_result_json_unknown_id(void);
void test_result_json_null_args(void);
void test_result_json_find_copy(void);
void test_result_json_does_not_change_status(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_result_json_initial_null(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);
    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_NULL(e->result_json);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

void test_result_json_update_sets(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);
    TEST_ASSERT_TRUE(scoreboard_update_result_json(sb, id, "{\"ok\":true}"));
    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL_STRING("{\"ok\":true}", e->result_json);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

void test_result_json_overwrite(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);
    scoreboard_update_result_json(sb, id, "{\"a\":1}");
    scoreboard_update_result_json(sb, id, "{\"b\":2}");
    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_EQUAL_STRING("{\"b\":2}", e->result_json);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

void test_result_json_clear_null(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);
    scoreboard_update_result_json(sb, id, "{\"x\":1}");
    TEST_ASSERT_TRUE(scoreboard_update_result_json(sb, id, NULL));
    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NULL(e->result_json);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

void test_result_json_clear_empty(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);
    scoreboard_update_result_json(sb, id, "{\"x\":1}");
    TEST_ASSERT_TRUE(scoreboard_update_result_json(sb, id, ""));
    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NULL(e->result_json);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

void test_result_json_too_large_rejects(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);
    size_t n = (size_t)SCOREBOARD_RESULT_JSON_MAX + 8;
    char* big = malloc(n + 1);
    TEST_ASSERT_NOT_NULL(big);
    memset(big, 'a', n);
    big[n] = '\0';
    TEST_ASSERT_FALSE(scoreboard_update_result_json(sb, id, big));
    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NULL(e->result_json);
    scoreboard_entry_free(e);
    free(big);
    free(id);
    scoreboard_destroy(sb);
}

void test_result_json_unknown_id(void) {
    Scoreboard* sb = scoreboard_create();
    TEST_ASSERT_FALSE(scoreboard_update_result_json(sb, "ZZZZZ", "{}"));
    scoreboard_destroy(sb);
}

void test_result_json_null_args(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);
    TEST_ASSERT_FALSE(scoreboard_update_result_json(NULL, id, "{}"));
    TEST_ASSERT_FALSE(scoreboard_update_result_json(sb, NULL, "{}"));
    free(id);
    scoreboard_destroy(sb);
}

void test_result_json_find_copy(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);
    scoreboard_update_result_json(sb, id, "{\"c\":3}");
    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_EQUAL_STRING("{\"c\":3}", e->result_json);
    free(e->result_json);
    e->result_json = NULL;
    ScoreboardEntry* e2 = scoreboard_find(sb, id);
    TEST_ASSERT_EQUAL_STRING("{\"c\":3}", e2->result_json);
    scoreboard_entry_free(e);
    scoreboard_entry_free(e2);
    free(id);
    scoreboard_destroy(sb);
}

void test_result_json_does_not_change_status(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);
    scoreboard_update_result_json(sb, id, "{}");
    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_EQUAL_INT(SCOREBOARD_JOB_PENDING, e->status);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_result_json_initial_null);
    RUN_TEST(test_result_json_update_sets);
    RUN_TEST(test_result_json_overwrite);
    RUN_TEST(test_result_json_clear_null);
    RUN_TEST(test_result_json_clear_empty);
    RUN_TEST(test_result_json_too_large_rejects);
    RUN_TEST(test_result_json_unknown_id);
    RUN_TEST(test_result_json_null_args);
    RUN_TEST(test_result_json_find_copy);
    RUN_TEST(test_result_json_does_not_change_status);
    return UNITY_END();
}
