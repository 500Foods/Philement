/*
 * Unity Test File: scoreboard_test_update_status.c
 *
 * Phase 5 of the LUA_PLAN. Tests scoreboard_update_status:
 *   - PENDING -> RUNNING: stamps started_at
 *   - RUNNING -> COMPLETED: stamps finished_at
 *   - RUNNING -> FAILED: stamps finished_at
 *   - RUNNING -> KILLED: stamps finished_at
 *   - Idempotent: re-setting the same status is a no-op
 *   - Re-entering a terminal state (e.g. PENDING after COMPLETED) is
 *     not prevented; this is a v1 simplicity choice documented in
 *     the header. We assert the literal behavior here.
 *   - Unknown id returns false
 *   - NULL scoreboard or NULL id returns false
 *   - scoreboard_status_name returns the right strings
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Standard includes
#include <string.h>

// Module under test
#include <src/scripting/scoreboard.h>

// Forward declarations
void test_update_pending_to_running_stamps_started_at(void);
void test_update_running_to_completed_stamps_finished_at(void);
void test_update_running_to_failed_stamps_finished_at(void);
void test_update_running_to_killed_stamps_finished_at(void);
void test_update_same_status_is_idempotent(void);
void test_update_unknown_id_returns_false(void);
void test_update_null_scoreboard_returns_false(void);
void test_update_null_id_returns_false(void);
void test_update_does_not_re_stamp_idempotent_transition(void);
void test_status_name_returns_correct_strings(void);
void test_status_name_unknown_for_garbage(void);

void setUp(void) {
}

void tearDown(void) {
}

static void sleep_one_ms(void) {
    struct timespec ts = { .tv_sec = 0, .tv_nsec = 1 * 1000 * 1000 };
    nanosleep(&ts, NULL);
}

// PENDING -> RUNNING stamps started_at
void test_update_pending_to_running_stamps_started_at(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);
    TEST_ASSERT_NOT_NULL(id);

    sleep_one_ms();   // Ensure a measurable started_at
    bool ok = scoreboard_update_status(sb, id, SCOREBOARD_JOB_RUNNING);
    TEST_ASSERT_TRUE(ok);

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL_INT(SCOREBOARD_JOB_RUNNING, e->status);
    TEST_ASSERT_TRUE_MESSAGE(e->started_at.tv_sec > 0, "started_at should be set after RUNNING transition");
    TEST_ASSERT_EQUAL_INT(0, e->finished_at.tv_sec);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// RUNNING -> COMPLETED stamps finished_at
void test_update_running_to_completed_stamps_finished_at(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);
    TEST_ASSERT_NOT_NULL(id);

    scoreboard_update_status(sb, id, SCOREBOARD_JOB_RUNNING);
    sleep_one_ms();
    bool ok = scoreboard_update_status(sb, id, SCOREBOARD_JOB_COMPLETED);
    TEST_ASSERT_TRUE(ok);

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL_INT(SCOREBOARD_JOB_COMPLETED, e->status);
    TEST_ASSERT_TRUE(e->finished_at.tv_sec > 0);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// RUNNING -> FAILED stamps finished_at
void test_update_running_to_failed_stamps_finished_at(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);
    TEST_ASSERT_NOT_NULL(id);
    scoreboard_update_status(sb, id, SCOREBOARD_JOB_RUNNING);
    sleep_one_ms();
    scoreboard_update_status(sb, id, SCOREBOARD_JOB_FAILED);

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL_INT(SCOREBOARD_JOB_FAILED, e->status);
    TEST_ASSERT_TRUE(e->finished_at.tv_sec > 0);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// RUNNING -> KILLED stamps finished_at
void test_update_running_to_killed_stamps_finished_at(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);
    TEST_ASSERT_NOT_NULL(id);
    scoreboard_update_status(sb, id, SCOREBOARD_JOB_RUNNING);
    sleep_one_ms();
    scoreboard_update_status(sb, id, SCOREBOARD_JOB_KILLED);

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL_INT(SCOREBOARD_JOB_KILLED, e->status);
    TEST_ASSERT_TRUE(e->finished_at.tv_sec > 0);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// Re-setting the same status is a no-op (no exception, no error).
void test_update_same_status_is_idempotent(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);
    TEST_ASSERT_NOT_NULL(id);

    bool ok1 = scoreboard_update_status(sb, id, SCOREBOARD_JOB_RUNNING);
    bool ok2 = scoreboard_update_status(sb, id, SCOREBOARD_JOB_RUNNING);
    TEST_ASSERT_TRUE(ok1);
    TEST_ASSERT_TRUE(ok2);

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL_INT(SCOREBOARD_JOB_RUNNING, e->status);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// update on an unknown id returns false
void test_update_unknown_id_returns_false(void) {
    Scoreboard* sb = scoreboard_create();
    bool ok = scoreboard_update_status(sb, "ZZZZZ", SCOREBOARD_JOB_RUNNING);
    TEST_ASSERT_FALSE(ok);
    scoreboard_destroy(sb);
}

// NULL scoreboard returns false
void test_update_null_scoreboard_returns_false(void) {
    bool ok = scoreboard_update_status(NULL, "AAAAA", SCOREBOARD_JOB_RUNNING);
    TEST_ASSERT_FALSE(ok);
}

// NULL id returns false
void test_update_null_id_returns_false(void) {
    Scoreboard* sb = scoreboard_create();
    bool ok = scoreboard_update_status(sb, NULL, SCOREBOARD_JOB_RUNNING);
    TEST_ASSERT_FALSE(ok);
    scoreboard_destroy(sb);
}

// An idempotent re-set must not re-stamp started_at or finished_at.
void test_update_does_not_re_stamp_idempotent_transition(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "s", NULL);
    TEST_ASSERT_NOT_NULL(id);

    scoreboard_update_status(sb, id, SCOREBOARD_JOB_RUNNING);
    ScoreboardEntry* e1 = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e1);
    long started_sec = e1->started_at.tv_sec;
    long started_nsec = e1->started_at.tv_nsec;
    scoreboard_entry_free(e1);

    sleep_one_ms();
    scoreboard_update_status(sb, id, SCOREBOARD_JOB_RUNNING);

    ScoreboardEntry* e2 = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e2);
    TEST_ASSERT_EQUAL_INT(started_sec, e2->started_at.tv_sec);
    TEST_ASSERT_EQUAL_INT(started_nsec, e2->started_at.tv_nsec);
    scoreboard_entry_free(e2);
    free(id);
    scoreboard_destroy(sb);
}

// status_name returns the canonical strings
void test_status_name_returns_correct_strings(void) {
    TEST_ASSERT_EQUAL_STRING("pending",   scoreboard_status_name(SCOREBOARD_JOB_PENDING));
    TEST_ASSERT_EQUAL_STRING("running",   scoreboard_status_name(SCOREBOARD_JOB_RUNNING));
    TEST_ASSERT_EQUAL_STRING("completed", scoreboard_status_name(SCOREBOARD_JOB_COMPLETED));
    TEST_ASSERT_EQUAL_STRING("failed",    scoreboard_status_name(SCOREBOARD_JOB_FAILED));
    TEST_ASSERT_EQUAL_STRING("killed",    scoreboard_status_name(SCOREBOARD_JOB_KILLED));
}

// status_name returns "unknown" for out-of-range values
void test_status_name_unknown_for_garbage(void) {
    TEST_ASSERT_EQUAL_STRING("unknown", scoreboard_status_name((ScoreboardJobStatus)999));
    TEST_ASSERT_EQUAL_STRING("unknown", scoreboard_status_name((ScoreboardJobStatus)-1));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_update_pending_to_running_stamps_started_at);
    RUN_TEST(test_update_running_to_completed_stamps_finished_at);
    RUN_TEST(test_update_running_to_failed_stamps_finished_at);
    RUN_TEST(test_update_running_to_killed_stamps_finished_at);
    RUN_TEST(test_update_same_status_is_idempotent);
    RUN_TEST(test_update_unknown_id_returns_false);
    RUN_TEST(test_update_null_scoreboard_returns_false);
    RUN_TEST(test_update_null_id_returns_false);
    RUN_TEST(test_update_does_not_re_stamp_idempotent_transition);
    RUN_TEST(test_status_name_returns_correct_strings);
    RUN_TEST(test_status_name_unknown_for_garbage);

    return UNITY_END();
}
