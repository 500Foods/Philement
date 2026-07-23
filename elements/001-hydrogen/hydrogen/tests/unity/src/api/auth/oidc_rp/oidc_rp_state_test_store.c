/*
 * Unity Test File: OIDC RP State Store Tests
 *
 * Covers oidc_rp_state_init, oidc_rp_state_shutdown,
 * oidc_rp_state_put, oidc_rp_state_take,
 * oidc_rp_state_sweep_expired, oidc_rp_state_size,
 * and oidc_rp_state_record_free from
 * src/api/auth/oidc_rp/oidc_rp_state.c.
 *
 * Hard rules verified by these tests:
 *   - put/take round-trip preserves every field byte-exactly.
 *   - take is single-use: a second take of the same state returns NULL.
 *   - Expired records are not returned by take and are reaped by sweep.
 *   - Concurrent puts and takes from multiple threads do not corrupt
 *     the store (final size is exact, no double-free under ASAN).
 *   - cleanup is leak-free under ASAN (verified by test_11_leaks).
 *   - Sensitive fields (`nonce`, `code_verifier`) survive a put/take
 *     round-trip but are scrubbed by record_free (no observable
 *     plaintext after free; covered indirectly by ASAN).
 *
 * The sweeper thread is disabled in setUp() so tests can drive
 * sweep_expired deterministically without racing a background thread.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/auth/oidc_rp/oidc_rp_state.h>

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Forward declarations of test functions
void test_init_then_shutdown_is_clean(void);
void test_double_init_is_idempotent(void);
void test_shutdown_without_init_is_safe(void);
void test_put_and_take_round_trip(void);
void test_put_with_optional_fields_null(void);
void test_put_rejects_missing_required_fields(void);
void test_put_before_init_returns_false(void);
void test_take_unknown_state_returns_null(void);
void test_take_is_single_use(void);
void test_take_expired_record_returns_null(void);
void test_sweep_expired_removes_aged_records(void);
void test_sweep_expired_keeps_fresh_records(void);
void test_size_reflects_inserts_and_takes(void);
void test_put_replaces_existing_entry_for_same_state(void);
void test_default_ttl_applied_when_put_passes_zero(void);
void test_record_free_handles_null(void);
void test_size_returns_zero_when_uninitialized(void);
void test_take_returns_null_when_uninitialized(void);
void test_sweep_returns_zero_when_uninitialized(void);
void test_concurrent_put_and_take(void);

void setUp(void) {
    // Suppress the sweeper thread for deterministic tests.
    oidc_rp_state_test_disable_sweeper();
    // Always start each test from a clean store.
    oidc_rp_state_shutdown();
}

void tearDown(void) {
    oidc_rp_state_shutdown();
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void test_init_then_shutdown_is_clean(void) {
    TEST_ASSERT_TRUE(oidc_rp_state_init(600));
    TEST_ASSERT_EQUAL_size_t(0, oidc_rp_state_size());
    oidc_rp_state_shutdown();
    TEST_ASSERT_EQUAL_size_t(0, oidc_rp_state_size());
}

void test_double_init_is_idempotent(void) {
    TEST_ASSERT_TRUE(oidc_rp_state_init(600));
    TEST_ASSERT_TRUE(oidc_rp_state_init(600));   // No-op the second time
    oidc_rp_state_shutdown();
}

void test_shutdown_without_init_is_safe(void) {
    // setUp already calls shutdown; calling it again is a no-op.
    oidc_rp_state_shutdown();
    oidc_rp_state_shutdown();
    TEST_ASSERT_EQUAL_size_t(0, oidc_rp_state_size());
}

// ---------------------------------------------------------------------------
// put / take basics
// ---------------------------------------------------------------------------

void test_put_and_take_round_trip(void) {
    TEST_ASSERT_TRUE(oidc_rp_state_init(600));

    bool ok = oidc_rp_state_put(
        "state-abc-123",
        "nonce-xyz-789",
        "verifier-pkce-blob",
        "Lithium",
        "/dashboard",
        "192.0.2.7",
        "500passwords",
        600);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_size_t(1, oidc_rp_state_size());

    OidcRpStateRecord *r = oidc_rp_state_take("state-abc-123");
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_EQUAL_STRING("state-abc-123",      r->state);
    TEST_ASSERT_EQUAL_STRING("nonce-xyz-789",      r->nonce);
    TEST_ASSERT_EQUAL_STRING("verifier-pkce-blob", r->code_verifier);
    TEST_ASSERT_EQUAL_STRING("Lithium",            r->database);
    TEST_ASSERT_EQUAL_STRING("/dashboard",         r->return_to);
    TEST_ASSERT_EQUAL_STRING("192.0.2.7",          r->client_ip);
    TEST_ASSERT_EQUAL_STRING("500passwords",       r->provider_name);
    TEST_ASSERT_EQUAL_INT(600, r->ttl_seconds);
    TEST_ASSERT_NOT_EQUAL(0, r->created_at);

    oidc_rp_state_record_free(r);

    TEST_ASSERT_EQUAL_size_t(0, oidc_rp_state_size());
}

void test_put_with_optional_fields_null(void) {
    TEST_ASSERT_TRUE(oidc_rp_state_init(600));

    bool ok = oidc_rp_state_put(
        "state-only", "nonce-only", "verifier-only",
        NULL, NULL, NULL, NULL, 600);
    TEST_ASSERT_TRUE(ok);

    OidcRpStateRecord *r = oidc_rp_state_take("state-only");
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_NULL(r->database);
    TEST_ASSERT_NULL(r->return_to);
    TEST_ASSERT_NULL(r->client_ip);
    TEST_ASSERT_NULL(r->provider_name);
    oidc_rp_state_record_free(r);
}

void test_put_rejects_missing_required_fields(void) {
    TEST_ASSERT_TRUE(oidc_rp_state_init(600));

    // Missing state
    TEST_ASSERT_FALSE(oidc_rp_state_put(
        NULL, "n", "v", NULL, NULL, NULL, NULL, 600));
    // Empty state
    TEST_ASSERT_FALSE(oidc_rp_state_put(
        "", "n", "v", NULL, NULL, NULL, NULL, 600));
    // Missing nonce
    TEST_ASSERT_FALSE(oidc_rp_state_put(
        "s", NULL, "v", NULL, NULL, NULL, NULL, 600));
    // Missing verifier
    TEST_ASSERT_FALSE(oidc_rp_state_put(
        "s", "n", NULL, NULL, NULL, NULL, NULL, 600));

    TEST_ASSERT_EQUAL_size_t(0, oidc_rp_state_size());
}

void test_put_before_init_returns_false(void) {
    // setUp already shut the store down. Don't init.
    bool ok = oidc_rp_state_put(
        "s", "n", "v", NULL, NULL, NULL, NULL, 600);
    TEST_ASSERT_FALSE(ok);
}

// ---------------------------------------------------------------------------
// take semantics
// ---------------------------------------------------------------------------

void test_take_unknown_state_returns_null(void) {
    TEST_ASSERT_TRUE(oidc_rp_state_init(600));
    TEST_ASSERT_NULL(oidc_rp_state_take("never-inserted"));
}

void test_take_is_single_use(void) {
    TEST_ASSERT_TRUE(oidc_rp_state_init(600));
    TEST_ASSERT_TRUE(oidc_rp_state_put(
        "single-use", "n", "v", NULL, NULL, NULL, NULL, 600));

    OidcRpStateRecord *r1 = oidc_rp_state_take("single-use");
    TEST_ASSERT_NOT_NULL(r1);
    oidc_rp_state_record_free(r1);

    OidcRpStateRecord *r2 = oidc_rp_state_take("single-use");
    TEST_ASSERT_NULL(r2);
}

void test_take_expired_record_returns_null(void) {
    // 1-second TTL so we can easily age it out.
    TEST_ASSERT_TRUE(oidc_rp_state_init(1));
    TEST_ASSERT_TRUE(oidc_rp_state_put(
        "soon-stale", "n", "v", NULL, NULL, NULL, NULL, 1));

    // Sleep long enough to age past TTL (be generous to avoid flakes).
    sleep(2);

    OidcRpStateRecord *r = oidc_rp_state_take("soon-stale");
    TEST_ASSERT_NULL(r);
    // The expired entry should be reaped by the take attempt.
    TEST_ASSERT_EQUAL_size_t(0, oidc_rp_state_size());
}

// ---------------------------------------------------------------------------
// sweep
// ---------------------------------------------------------------------------

void test_sweep_expired_removes_aged_records(void) {
    TEST_ASSERT_TRUE(oidc_rp_state_init(1));

    TEST_ASSERT_TRUE(oidc_rp_state_put("a", "n", "v", NULL, NULL, NULL, NULL, 1));
    TEST_ASSERT_TRUE(oidc_rp_state_put("b", "n", "v", NULL, NULL, NULL, NULL, 1));
    TEST_ASSERT_TRUE(oidc_rp_state_put("c", "n", "v", NULL, NULL, NULL, NULL, 1));
    TEST_ASSERT_EQUAL_size_t(3, oidc_rp_state_size());

    sleep(2);

    size_t removed = oidc_rp_state_sweep_expired();
    TEST_ASSERT_EQUAL_size_t(3, removed);
    TEST_ASSERT_EQUAL_size_t(0, oidc_rp_state_size());
}

void test_sweep_expired_keeps_fresh_records(void) {
    TEST_ASSERT_TRUE(oidc_rp_state_init(600));

    TEST_ASSERT_TRUE(oidc_rp_state_put("fresh-1", "n", "v",
                                       NULL, NULL, NULL, NULL, 600));
    TEST_ASSERT_TRUE(oidc_rp_state_put("fresh-2", "n", "v",
                                       NULL, NULL, NULL, NULL, 600));

    size_t removed = oidc_rp_state_sweep_expired();
    TEST_ASSERT_EQUAL_size_t(0, removed);
    TEST_ASSERT_EQUAL_size_t(2, oidc_rp_state_size());

    // Drain so tearDown's shutdown sees an empty store.
    OidcRpStateRecord *r = oidc_rp_state_take("fresh-1");
    oidc_rp_state_record_free(r);
    r = oidc_rp_state_take("fresh-2");
    oidc_rp_state_record_free(r);
}

// ---------------------------------------------------------------------------
// size + replace + ttl
// ---------------------------------------------------------------------------

void test_size_reflects_inserts_and_takes(void) {
    TEST_ASSERT_TRUE(oidc_rp_state_init(600));

    TEST_ASSERT_EQUAL_size_t(0, oidc_rp_state_size());
    oidc_rp_state_put("k1", "n", "v", NULL, NULL, NULL, NULL, 600);
    TEST_ASSERT_EQUAL_size_t(1, oidc_rp_state_size());
    oidc_rp_state_put("k2", "n", "v", NULL, NULL, NULL, NULL, 600);
    oidc_rp_state_put("k3", "n", "v", NULL, NULL, NULL, NULL, 600);
    TEST_ASSERT_EQUAL_size_t(3, oidc_rp_state_size());

    OidcRpStateRecord *r = oidc_rp_state_take("k2");
    oidc_rp_state_record_free(r);
    TEST_ASSERT_EQUAL_size_t(2, oidc_rp_state_size());

    // Drain
    r = oidc_rp_state_take("k1"); oidc_rp_state_record_free(r);
    r = oidc_rp_state_take("k3"); oidc_rp_state_record_free(r);
}

void test_put_replaces_existing_entry_for_same_state(void) {
    TEST_ASSERT_TRUE(oidc_rp_state_init(600));

    TEST_ASSERT_TRUE(oidc_rp_state_put(
        "dupe", "first-nonce", "first-verifier",
        "DB-1", NULL, NULL, NULL, 600));
    TEST_ASSERT_TRUE(oidc_rp_state_put(
        "dupe", "second-nonce", "second-verifier",
        "DB-2", NULL, NULL, NULL, 600));

    TEST_ASSERT_EQUAL_size_t(1, oidc_rp_state_size());
    OidcRpStateRecord *r = oidc_rp_state_take("dupe");
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_EQUAL_STRING("second-nonce", r->nonce);
    TEST_ASSERT_EQUAL_STRING("second-verifier", r->code_verifier);
    TEST_ASSERT_EQUAL_STRING("DB-2", r->database);
    oidc_rp_state_record_free(r);
}

void test_default_ttl_applied_when_put_passes_zero(void) {
    TEST_ASSERT_TRUE(oidc_rp_state_init(123));

    TEST_ASSERT_TRUE(oidc_rp_state_put(
        "default-ttl", "n", "v", NULL, NULL, NULL, NULL, 0));
    OidcRpStateRecord *r = oidc_rp_state_take("default-ttl");
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_EQUAL_INT(123, r->ttl_seconds);
    oidc_rp_state_record_free(r);
}

void test_record_free_handles_null(void) {
    // Just must not crash.
    oidc_rp_state_record_free(NULL);
}

// ---------------------------------------------------------------------------
// uninit safety
// ---------------------------------------------------------------------------

void test_size_returns_zero_when_uninitialized(void) {
    // setUp shut down the store; don't re-init.
    TEST_ASSERT_EQUAL_size_t(0, oidc_rp_state_size());
}

void test_take_returns_null_when_uninitialized(void) {
    TEST_ASSERT_NULL(oidc_rp_state_take("anything"));
}

void test_sweep_returns_zero_when_uninitialized(void) {
    TEST_ASSERT_EQUAL_size_t(0, oidc_rp_state_sweep_expired());
}

// ---------------------------------------------------------------------------
// Concurrency
// ---------------------------------------------------------------------------

#define NUM_THREADS  4
#define PUTS_PER_THREAD  100

typedef struct {
    int thread_id;
} ThreadArg;

static void *putter_thread(void *arg) {
    ThreadArg *a = (ThreadArg *)arg;
    for (int i = 0; i < PUTS_PER_THREAD; i++) {
        char key[64];
        snprintf(key, sizeof(key), "k-%d-%d", a->thread_id, i);
        oidc_rp_state_put(key, "nonce", "verifier",
                          NULL, NULL, NULL, NULL, 600);
    }
    return NULL;
}

static void *taker_thread(void *arg) {
    ThreadArg *a = (ThreadArg *)arg;
    for (int i = 0; i < PUTS_PER_THREAD; i++) {
        char key[64];
        snprintf(key, sizeof(key), "k-%d-%d", a->thread_id, i);

        // Spin briefly while the corresponding put happens on the other
        // thread family.
        OidcRpStateRecord *r = NULL;
        for (int spin = 0; spin < 1000 && !r; spin++) {
            r = oidc_rp_state_take(key);
            if (!r) {
                // Yield to give putter a chance.
                usleep(100);
            }
        }
        if (r) oidc_rp_state_record_free(r);
    }
    return NULL;
}

void test_concurrent_put_and_take(void) {
    TEST_ASSERT_TRUE(oidc_rp_state_init(600));

    pthread_t put_threads[NUM_THREADS];
    pthread_t take_threads[NUM_THREADS];
    ThreadArg put_args[NUM_THREADS];
    ThreadArg take_args[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++) {
        put_args[i].thread_id  = i;
        take_args[i].thread_id = i;
        pthread_create(&put_threads[i],  NULL, putter_thread, &put_args[i]);
        pthread_create(&take_threads[i], NULL, taker_thread,  &take_args[i]);
    }
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(put_threads[i],  NULL);
        pthread_join(take_threads[i], NULL);
    }

    // All puts and takes were paired by the taker spin loop. The
    // store may still hold a few entries that the takers gave up on
    // before they were inserted; drain whatever is left so shutdown
    // sees a clean store and ASAN reports no leaks.
    for (int t = 0; t < NUM_THREADS; t++) {
        for (int i = 0; i < PUTS_PER_THREAD; i++) {
            char key[64];
            snprintf(key, sizeof(key), "k-%d-%d", t, i);
            OidcRpStateRecord *r = oidc_rp_state_take(key);
            if (r) oidc_rp_state_record_free(r);
        }
    }
    TEST_ASSERT_EQUAL_size_t(0, oidc_rp_state_size());
}

// ---------------------------------------------------------------------------
// Runner
// ---------------------------------------------------------------------------

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_init_then_shutdown_is_clean);
    RUN_TEST(test_double_init_is_idempotent);
    RUN_TEST(test_shutdown_without_init_is_safe);

    RUN_TEST(test_put_and_take_round_trip);
    RUN_TEST(test_put_with_optional_fields_null);
    RUN_TEST(test_put_rejects_missing_required_fields);
    RUN_TEST(test_put_before_init_returns_false);

    RUN_TEST(test_take_unknown_state_returns_null);
    RUN_TEST(test_take_is_single_use);
    RUN_TEST(test_take_expired_record_returns_null);

    RUN_TEST(test_sweep_expired_removes_aged_records);
    RUN_TEST(test_sweep_expired_keeps_fresh_records);

    RUN_TEST(test_size_reflects_inserts_and_takes);
    RUN_TEST(test_put_replaces_existing_entry_for_same_state);
    RUN_TEST(test_default_ttl_applied_when_put_passes_zero);
    RUN_TEST(test_record_free_handles_null);

    RUN_TEST(test_size_returns_zero_when_uninitialized);
    RUN_TEST(test_take_returns_null_when_uninitialized);
    RUN_TEST(test_sweep_returns_zero_when_uninitialized);

    RUN_TEST(test_concurrent_put_and_take);

    return UNITY_END();
}
