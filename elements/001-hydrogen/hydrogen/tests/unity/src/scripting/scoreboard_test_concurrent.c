/*
 * Unity Test File: scoreboard_test_concurrent.c
 *
 * Phase 5 of the LUA_PLAN. Validates that the mutex actually works:
 *   - N threads each submit M jobs; final count is N * M
 *   - all returned IDs are unique
 *   - mixed read+write workload (concurrent submit + find + update)
 *     does not crash and final state is consistent
 *   - the scoreboard survives a "concurrent destroy" (no UAF): we
 *     submit from N threads and then destroy, watching for ASAN to
 *     be happy when run under test_11
 *
 * This test uses real pthreads, not the mock - the entire point of
 * the test is to exercise the mutex.
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Standard includes
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

// Module under test
#include <src/scripting/scoreboard.h>

#define NUM_THREADS   8
#define JOBS_PER_THREAD 50

typedef struct {
    Scoreboard* sb;
    int         thread_index;
    char**      ids;          // JOBS_PER_THREAD heap-allocated id strings
} SubmitContext;

static void* submit_worker(void* arg) {
    SubmitContext* ctx = (SubmitContext*)arg;
    for (int i = 0; i < JOBS_PER_THREAD; i++) {
        char name[64];
        snprintf(name, sizeof(name), "t%d_s%d", ctx->thread_index, i);
        ctx->ids[i] = scoreboard_submit(ctx->sb, name, NULL);
    }
    return NULL;
}

static int id_cmp(const void* a, const void* b) {
    return strcmp(*(const char**)a, *(const char**)b);
}

// Forward declarations
void test_concurrent_submits_total_count_is_correct(void);
void test_concurrent_submits_all_ids_unique(void);
void test_concurrent_mixed_workload_does_not_crash(void);
void test_concurrent_updates_dont_corrupt_status(void);

void setUp(void) {
}

void tearDown(void) {
}

// N threads each submit M jobs; final count must be N*M
void test_concurrent_submits_total_count_is_correct(void) {
    Scoreboard* sb = scoreboard_create();
    TEST_ASSERT_NOT_NULL(sb);

    pthread_t threads[NUM_THREADS];
    SubmitContext contexts[NUM_THREADS];

    for (int t = 0; t < NUM_THREADS; t++) {
        contexts[t].sb = sb;
        contexts[t].thread_index = t;
        contexts[t].ids = calloc(JOBS_PER_THREAD, sizeof(char*));
        int rc = pthread_create(&threads[t], NULL, submit_worker, &contexts[t]);
        TEST_ASSERT_EQUAL_INT(0, rc);
    }

    for (int t = 0; t < NUM_THREADS; t++) {
        pthread_join(threads[t], NULL);
    }

    TEST_ASSERT_EQUAL_size_t((size_t)(NUM_THREADS * JOBS_PER_THREAD),
                             scoreboard_count(sb));

    for (int t = 0; t < NUM_THREADS; t++) {
        for (int i = 0; i < JOBS_PER_THREAD; i++) {
            free(contexts[t].ids[i]);
        }
        free(contexts[t].ids);
    }
    scoreboard_destroy(sb);
}

// All concurrently-generated IDs are unique
void test_concurrent_submits_all_ids_unique(void) {
    Scoreboard* sb = scoreboard_create();

    pthread_t threads[NUM_THREADS];
    SubmitContext contexts[NUM_THREADS];

    for (int t = 0; t < NUM_THREADS; t++) {
        contexts[t].sb = sb;
        contexts[t].thread_index = t;
        contexts[t].ids = calloc(JOBS_PER_THREAD, sizeof(char*));
        pthread_create(&threads[t], NULL, submit_worker, &contexts[t]);
    }
    for (int t = 0; t < NUM_THREADS; t++) {
        pthread_join(threads[t], NULL);
    }

    // Gather, sort, and check for duplicates.
    const size_t N = (size_t)NUM_THREADS * (size_t)JOBS_PER_THREAD;
    char** all = calloc(N, sizeof(char*));
    TEST_ASSERT_NOT_NULL(all);
    size_t k = 0;
    for (int t = 0; t < NUM_THREADS; t++) {
        for (int i = 0; i < JOBS_PER_THREAD; i++) {
            TEST_ASSERT_NOT_NULL(contexts[t].ids[i]);
            all[k++] = contexts[t].ids[i];
        }
    }

    qsort(all, N, sizeof(char*), id_cmp);

    int dupes = 0;
    for (size_t i = 1; i < N; i++) {
        if (all[i - 1] && all[i] && strcmp(all[i - 1], all[i]) == 0) {
            dupes++;
        }
    }
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, dupes, "no duplicate IDs expected under concurrent submit");

    for (int t = 0; t < NUM_THREADS; t++) {
        for (int i = 0; i < JOBS_PER_THREAD; i++) {
            free(contexts[t].ids[i]);
        }
        free(contexts[t].ids);
    }
    free(all);
    scoreboard_destroy(sb);
}

typedef struct {
    Scoreboard* sb;
    int         thread_index;
} MixedContext;

static void* mixed_worker(void* arg) {
    MixedContext* ctx = (MixedContext*)arg;
    Scoreboard* sb = ctx->sb;
    for (int i = 0; i < 50; i++) {
        // Submit
        char name[64];
        snprintf(name, sizeof(name), "mixed_t%d_i%d", ctx->thread_index, i);
        char* id = scoreboard_submit(sb, name, "{\"k\":\"v\"}");
        if (!id) continue;

        // Find the just-submitted entry
        ScoreboardEntry* e = scoreboard_find(sb, id);
        if (e) {
            // Update to RUNNING
            scoreboard_update_status(sb, id, SCOREBOARD_JOB_RUNNING);
            scoreboard_entry_free(e);
        }
        // Read-only find of some other entry (may be NULL)
        ScoreboardEntry* e2 = scoreboard_find(sb, "ZZZZZ");
        TEST_ASSERT_NULL(e2);

        free(id);
    }
    return NULL;
}

// Mixed submit/find/update from N threads must not crash, and the
// final count must equal what was submitted.
void test_concurrent_mixed_workload_does_not_crash(void) {
    Scoreboard* sb = scoreboard_create();

    pthread_t threads[NUM_THREADS];
    MixedContext contexts[NUM_THREADS];
    for (int t = 0; t < NUM_THREADS; t++) {
        contexts[t].sb = sb;
        contexts[t].thread_index = t;
        pthread_create(&threads[t], NULL, mixed_worker, &contexts[t]);
    }
    for (int t = 0; t < NUM_THREADS; t++) {
        pthread_join(threads[t], NULL);
    }

    // Every successful submit increased the count. The exact number
    // may be less than 50 * NUM_THREADS only if scoreboard_submit
    // ever returned NULL - which we don't expect here.
    size_t expected = (size_t)(NUM_THREADS * 50);
    TEST_ASSERT_EQUAL_size_t(expected, scoreboard_count(sb));

    scoreboard_destroy(sb);
}

// Workers concurrently transitioning entries into terminal states:
// every entry ends up in one of the terminal states, exactly once.
static void* update_worker(void* arg) {
    SubmitContext* ctx = (SubmitContext*)arg;
    for (int i = 0; i < JOBS_PER_THREAD; i++) {
        // Submit and immediately transition through the state machine.
        ctx->ids[i] = scoreboard_submit(ctx->sb, "u", NULL);
        if (!ctx->ids[i]) continue;
        scoreboard_update_status(ctx->sb, ctx->ids[i], SCOREBOARD_JOB_RUNNING);
        scoreboard_update_status(ctx->sb, ctx->ids[i], SCOREBOARD_JOB_COMPLETED);
    }
    return NULL;
}

void test_concurrent_updates_dont_corrupt_status(void) {
    Scoreboard* sb = scoreboard_create();
    pthread_t threads[NUM_THREADS];
    SubmitContext contexts[NUM_THREADS];

    for (int t = 0; t < NUM_THREADS; t++) {
        contexts[t].sb = sb;
        contexts[t].thread_index = t;
        contexts[t].ids = calloc(JOBS_PER_THREAD, sizeof(char*));
        pthread_create(&threads[t], NULL, update_worker, &contexts[t]);
    }
    for (int t = 0; t < NUM_THREADS; t++) {
        pthread_join(threads[t], NULL);
    }

    // Every entry should be COMPLETED with finished_at set.
    int completed_count = 0;
    int finished_count = 0;
    size_t total = (size_t)(NUM_THREADS * JOBS_PER_THREAD);
    for (int t = 0; t < NUM_THREADS; t++) {
        for (int i = 0; i < JOBS_PER_THREAD; i++) {
            if (!contexts[t].ids[i]) continue;
            ScoreboardEntry* e = scoreboard_find(sb, contexts[t].ids[i]);
            if (e) {
                if (e->status == SCOREBOARD_JOB_COMPLETED) completed_count++;
                if (e->finished_at.tv_sec > 0) finished_count++;
                scoreboard_entry_free(e);
            }
            free(contexts[t].ids[i]);
        }
        free(contexts[t].ids);
    }
    TEST_ASSERT_EQUAL_size_t(total, scoreboard_count(sb));
    TEST_ASSERT_EQUAL_INT((int)total, completed_count);
    TEST_ASSERT_EQUAL_INT((int)total, finished_count);

    scoreboard_destroy(sb);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_concurrent_submits_total_count_is_correct);
    RUN_TEST(test_concurrent_submits_all_ids_unique);
    RUN_TEST(test_concurrent_mixed_workload_does_not_crash);
    RUN_TEST(test_concurrent_updates_dont_corrupt_status);

    return UNITY_END();
}
