/*
 * Unity Test File: worker_pool_test_submit.c
 *
 * Phase 7 of the LUA_PLAN. Validates the submit path:
 *   - scripting_submit_job creates a scoreboard entry and enqueues
 *   - scripting_submit_job_with_source registers the source first
 *   - submit on a NULL pool returns NULL
 *   - submit with NULL script_name returns NULL
 *   - submit_job_with_source with NULL source returns NULL
 *   - multiple submits produce unique job_ids
 *
 * Workers are running but the tests don't wait for them to process -
 * we only check that the submission side has the right shape. The
 * execution path is exercised by worker_pool_test_execute.
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Standard includes
#include <pthread.h>
#include <string.h>

// Modules under test
#include <src/scripting/scripting.h>
#include <src/scripting/scoreboard.h>
#include <src/scripting/worker_pool.h>
#include <src/scripting/script_registry.h>
#include <src/threads/threads.h>

// Forward declarations (required for -Wmissing-prototypes)
void test_submit_job_creates_scoreboard_entry(void);
void test_submit_job_with_source_registers_source(void);
void test_submit_job_with_unknown_pool_returns_null(void);
void test_submit_job_with_null_name_returns_null(void);
void test_submit_with_source_null_source_returns_null(void);
void test_multiple_submits_produce_unique_ids(void);
void test_submit_with_source_replaces_prior_source(void);

void setUp(void) {
    // Reset the subsystem state cleanly. scripting_init_state is
    // idempotent on the scoreboard (it skips if already created).
    scripting_init_state();
}

void tearDown(void) {
    // Tear down workers first (drain, join, free), then the rest.
    scripting_workers_destroy();
    scripting_cleanup_state();
    scripting_system_shutdown = 0;
    scripting_orchestrator_state = NULL;
}

// A successful submit creates a PENDING entry on the scoreboard.
void test_submit_job_creates_scoreboard_entry(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(1));
    TEST_ASSERT_NOT_NULL(scripting_scoreboard);

    char* id = scripting_submit_job("greet", "{\"name\":\"world\"}");
    TEST_ASSERT_NOT_NULL(id);
    TEST_ASSERT_EQUAL_size_t(1, scoreboard_count(scripting_scoreboard));

    ScoreboardEntry* e = scoreboard_find(scripting_scoreboard, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL_STRING("greet", e->script_name);
    TEST_ASSERT_EQUAL_STRING("{\"name\":\"world\"}", e->params_json);
    TEST_ASSERT_EQUAL(SCOREBOARD_JOB_PENDING, e->status);
    scoreboard_entry_free(e);
    free(id);
}

// scripting_submit_job_with_source registers the source first.
void test_submit_job_with_source_registers_source(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(1));
    TEST_ASSERT_NOT_NULL(scripting_workers);

    char* id = scripting_submit_job_with_source("greet", "return 42", NULL);
    TEST_ASSERT_NOT_NULL(id);

    const char* src = script_registry_lookup(scripting_workers->registry,
                                             "greet");
    TEST_ASSERT_NOT_NULL(src);
    TEST_ASSERT_EQUAL_STRING("return 42", src);
    free(id);
}

// submit on a NULL pool returns NULL.
void test_submit_job_with_unknown_pool_returns_null(void) {
    // scripting_workers is NULL until init; submit should refuse.
    TEST_ASSERT_NULL(scripting_workers);
    char* id = scripting_submit_job("x", NULL);
    TEST_ASSERT_NULL(id);
}

// submit with NULL script_name returns NULL.
void test_submit_job_with_null_name_returns_null(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(1));
    char* id = scripting_submit_job(NULL, NULL);
    TEST_ASSERT_NULL(id);
}

// submit_with_source with NULL source returns NULL.
void test_submit_with_source_null_source_returns_null(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(1));
    char* id = scripting_submit_job_with_source("x", NULL, NULL);
    TEST_ASSERT_NULL(id);
}

// Multiple submits produce unique job_ids.
void test_multiple_submits_produce_unique_ids(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(1));
    char* id1 = scripting_submit_job("a", NULL);
    char* id2 = scripting_submit_job("b", NULL);
    char* id3 = scripting_submit_job("c", NULL);
    TEST_ASSERT_NOT_NULL(id1);
    TEST_ASSERT_NOT_NULL(id2);
    TEST_ASSERT_NOT_NULL(id3);
    TEST_ASSERT_TRUE(strcmp(id1, id2) != 0);
    TEST_ASSERT_TRUE(strcmp(id1, id3) != 0);
    TEST_ASSERT_TRUE(strcmp(id2, id3) != 0);
    TEST_ASSERT_EQUAL_size_t(3, scoreboard_count(scripting_scoreboard));
    free(id1);
    free(id2);
    free(id3);
}

// submit_with_source replaces any prior source for the same name.
void test_submit_with_source_replaces_prior_source(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(1));
    scripting_submit_job_with_source("a", "first", NULL);
    scripting_submit_job_with_source("a", "second", NULL);
    const char* src = script_registry_lookup(scripting_workers->registry, "a");
    TEST_ASSERT_NOT_NULL(src);
    TEST_ASSERT_EQUAL_STRING("second", src);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_submit_job_creates_scoreboard_entry);
    RUN_TEST(test_submit_job_with_source_registers_source);
    RUN_TEST(test_submit_job_with_unknown_pool_returns_null);
    RUN_TEST(test_submit_job_with_null_name_returns_null);
    RUN_TEST(test_submit_with_source_null_source_returns_null);
    RUN_TEST(test_multiple_submits_produce_unique_ids);
    RUN_TEST(test_submit_with_source_replaces_prior_source);

    return UNITY_END();
}
