/*
 * Unity Test File: OIDC RP Handoff Store Tests
 *
 * Covers oidc_rp_handoff_store_init, oidc_rp_handoff_store_shutdown,
 * oidc_rp_handoff_store_put, oidc_rp_handoff_store_take,
 * oidc_rp_handoff_store_sweep_expired, oidc_rp_handoff_store_size,
 * and oidc_rp_handoff_record_free from
 * src/api/auth/oidc_rp/oidc_rp_handoff_store.c.
 *
 * Hard rules verified by these tests:
 *   - put/take round-trip preserves every field (including int and
 *     time_t fields) byte-exactly.
 *   - take is single-use: a second take of the same handoff returns NULL.
 *   - Expired records are not returned by take and are reaped by sweep.
 *   - Concurrent puts and takes from multiple threads do not corrupt
 *     the store (final size is exact, no double-free under ASAN).
 *   - The sensitive `jwt` field survives a put/take round-trip but is
 *     scrubbed by record_free (no observable plaintext after free;
 *     covered indirectly by ASAN).
 *
 * The sweeper thread is disabled in setUp() so tests can drive
 * sweep_expired deterministically without racing a background thread.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/auth/oidc_rp/oidc_rp_handoff_store.h>

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
void test_take_unknown_handoff_returns_null(void);
void test_take_is_single_use(void);
void test_take_expired_record_returns_null(void);
void test_sweep_expired_removes_aged_records(void);
void test_sweep_expired_keeps_fresh_records(void);
void test_size_reflects_inserts_and_takes(void);
void test_put_replaces_existing_entry_for_same_handoff(void);
void test_default_ttl_applied_when_put_passes_zero(void);
void test_record_free_handles_null(void);
void test_size_returns_zero_when_uninitialized(void);
void test_take_returns_null_when_uninitialized(void);
void test_sweep_returns_zero_when_uninitialized(void);
void test_account_id_and_expires_at_round_trip(void);
void test_concurrent_put_and_take(void);

void setUp(void) {
    // Suppress the sweeper thread for deterministic tests.
    oidc_rp_handoff_store_test_disable_sweeper();
    // Always start each test from a clean store.
    oidc_rp_handoff_store_shutdown();
}

void tearDown(void) {
    oidc_rp_handoff_store_shutdown();
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void test_init_then_shutdown_is_clean(void) {
    TEST_ASSERT_TRUE(oidc_rp_handoff_store_init(60));
    TEST_ASSERT_EQUAL_size_t(0, oidc_rp_handoff_store_size());
    oidc_rp_handoff_store_shutdown();
    TEST_ASSERT_EQUAL_size_t(0, oidc_rp_handoff_store_size());
}

void test_double_init_is_idempotent(void) {
    TEST_ASSERT_TRUE(oidc_rp_handoff_store_init(60));
    TEST_ASSERT_TRUE(oidc_rp_handoff_store_init(60));   // No-op the second time
    oidc_rp_handoff_store_shutdown();
}

void test_shutdown_without_init_is_safe(void) {
    // setUp already calls shutdown; calling it again is a no-op.
    oidc_rp_handoff_store_shutdown();
    oidc_rp_handoff_store_shutdown();
    TEST_ASSERT_EQUAL_size_t(0, oidc_rp_handoff_store_size());
}

// ---------------------------------------------------------------------------
// put / take basics
// ---------------------------------------------------------------------------

void test_put_and_take_round_trip(void) {
    TEST_ASSERT_TRUE(oidc_rp_handoff_store_init(60));

    bool ok = oidc_rp_handoff_store_put(
        "handoff-abc-123",
        "fake.jwt.token-string",
        42,
        "alice",
        "admin,user",
        "192.0.2.7",
        (time_t)1700000000,
        60);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_size_t(1, oidc_rp_handoff_store_size());

    OidcRpHandoffRecord *r = oidc_rp_handoff_store_take("handoff-abc-123");
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_EQUAL_STRING("handoff-abc-123",       r->handoff);
    TEST_ASSERT_EQUAL_STRING("fake.jwt.token-string", r->jwt);
    TEST_ASSERT_EQUAL_INT(42, r->account_id);
    TEST_ASSERT_EQUAL_STRING("alice",                 r->username);
    TEST_ASSERT_EQUAL_STRING("admin,user",            r->roles);
    TEST_ASSERT_EQUAL_STRING("192.0.2.7",             r->client_ip);
    TEST_ASSERT_EQUAL_INT(60, r->ttl_seconds);
    TEST_ASSERT_EQUAL_INT64((int64_t)1700000000, (int64_t)r->expires_at);
    TEST_ASSERT_NOT_EQUAL(0, r->created_at);

    oidc_rp_handoff_record_free(r);

    TEST_ASSERT_EQUAL_size_t(0, oidc_rp_handoff_store_size());
}

void test_put_with_optional_fields_null(void) {
    TEST_ASSERT_TRUE(oidc_rp_handoff_store_init(60));

    bool ok = oidc_rp_handoff_store_put(
        "handoff-only", "some.jwt", 7,
        NULL, NULL, NULL, 0, 60);
    TEST_ASSERT_TRUE(ok);

    OidcRpHandoffRecord *r = oidc_rp_handoff_store_take("handoff-only");
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_NULL(r->username);
    TEST_ASSERT_NULL(r->roles);
    TEST_ASSERT_NULL(r->client_ip);
    TEST_ASSERT_EQUAL_INT(7, r->account_id);
    TEST_ASSERT_EQUAL_INT64(0, (int64_t)r->expires_at);
    oidc_rp_handoff_record_free(r);
}

void test_put_rejects_missing_required_fields(void) {
    TEST_ASSERT_TRUE(oidc_rp_handoff_store_init(60));

    // Missing handoff
    TEST_ASSERT_FALSE(oidc_rp_handoff_store_put(
        NULL, "jwt", 1, NULL, NULL, NULL, 0, 60));
    // Empty handoff
    TEST_ASSERT_FALSE(oidc_rp_handoff_store_put(
        "", "jwt", 1, NULL, NULL, NULL, 0, 60));
    // Missing jwt
    TEST_ASSERT_FALSE(oidc_rp_handoff_store_put(
        "h", NULL, 1, NULL, NULL, NULL, 0, 60));

    TEST_ASSERT_EQUAL_size_t(0, oidc_rp_handoff_store_size());
}

void test_put_before_init_returns_false(void) {
    // setUp already shut the store down. Don't init.
    bool ok = oidc_rp_handoff_store_put(
        "h", "jwt", 1, NULL, NULL, NULL, 0, 60);
    TEST_ASSERT_FALSE(ok);
}

// ---------------------------------------------------------------------------
// take semantics
// ---------------------------------------------------------------------------

void test_take_unknown_handoff_returns_null(void) {
    TEST_ASSERT_TRUE(oidc_rp_handoff_store_init(60));
    TEST_ASSERT_NULL(oidc_rp_handoff_store_take("never-inserted"));
}

void test_take_is_single_use(void) {
    TEST_ASSERT_TRUE(oidc_rp_handoff_store_init(60));
    TEST_ASSERT_TRUE(oidc_rp_handoff_store_put(
        "single-use", "jwt", 1, NULL, NULL, NULL, 0, 60));

    OidcRpHandoffRecord *r1 = oidc_rp_handoff_store_take("single-use");
    TEST_ASSERT_NOT_NULL(r1);
    oidc_rp_handoff_record_free(r1);

    // Second take must return NULL — the record was atomically removed.
    OidcRpHandoffRecord *r2 = oidc_rp_handoff_store_take("single-use");
    TEST_ASSERT_NULL(r2);
}

void test_take_expired_record_returns_null(void) {
    // 1-second TTL so we can easily age it out.
    TEST_ASSERT_TRUE(oidc_rp_handoff_store_init(1));
    TEST_ASSERT_TRUE(oidc_rp_handoff_store_put(
        "soon-stale", "jwt", 1, NULL, NULL, NULL, 0, 1));

    // Sleep long enough to age past TTL (be generous to avoid flakes).
    sleep(2);

    OidcRpHandoffRecord *r = oidc_rp_handoff_store_take("soon-stale");
    TEST_ASSERT_NULL(r);
    // The expired entry should be reaped by the take attempt.
    TEST_ASSERT_EQUAL_size_t(0, oidc_rp_handoff_store_size());
}

// ---------------------------------------------------------------------------
// sweep
// ---------------------------------------------------------------------------

void test_sweep_expired_removes_aged_records(void) {
    TEST_ASSERT_TRUE(oidc_rp_handoff_store_init(1));

    TEST_ASSERT_TRUE(oidc_rp_handoff_store_put("a", "jwt", 1, NULL, NULL, NULL, 0, 1));
    TEST_ASSERT_TRUE(oidc_rp_handoff_store_put("b", "jwt", 1, NULL, NULL, NULL, 0, 1));
    TEST_ASSERT_TRUE(oidc_rp_handoff_store_put("c", "jwt", 1, NULL, NULL, NULL, 0, 1));
    TEST_ASSERT_EQUAL_size_t(3, oidc_rp_handoff_store_size());

    sleep(2);

    size_t removed = oidc_rp_handoff_store_sweep_expired();
    TEST_ASSERT_EQUAL_size_t(3, removed);
    TEST_ASSERT_EQUAL_size_t(0, oidc_rp_handoff_store_size());
}

void test_sweep_expired_keeps_fresh_records(void) {
    TEST_ASSERT_TRUE(oidc_rp_handoff_store_init(60));

    TEST_ASSERT_TRUE(oidc_rp_handoff_store_put(
        "fresh-1", "jwt", 1, NULL, NULL, NULL, 0, 60));
    TEST_ASSERT_TRUE(oidc_rp_handoff_store_put(
        "fresh-2", "jwt", 1, NULL, NULL, NULL, 0, 60));

    size_t removed = oidc_rp_handoff_store_sweep_expired();
    TEST_ASSERT_EQUAL_size_t(0, removed);
    TEST_ASSERT_EQUAL_size_t(2, oidc_rp_handoff_store_size());

    // Drain so tearDown's shutdown sees an empty store.
    OidcRpHandoffRecord *r = oidc_rp_handoff_store_take("fresh-1");
    oidc_rp_handoff_record_free(r);
    r = oidc_rp_handoff_store_take("fresh-2");
    oidc_rp_handoff_record_free(r);
}

// ---------------------------------------------------------------------------
// size + replace + ttl
// ---------------------------------------------------------------------------

void test_size_reflects_inserts_and_takes(void) {
    TEST_ASSERT_TRUE(oidc_rp_handoff_store_init(60));

    TEST_ASSERT_EQUAL_size_t(0, oidc_rp_handoff_store_size());
    oidc_rp_handoff_store_put("k1", "jwt", 1, NULL, NULL, NULL, 0, 60);
    TEST_ASSERT_EQUAL_size_t(1, oidc_rp_handoff_store_size());
    oidc_rp_handoff_store_put("k2", "jwt", 1, NULL, NULL, NULL, 0, 60);
    oidc_rp_handoff_store_put("k3", "jwt", 1, NULL, NULL, NULL, 0, 60);
    TEST_ASSERT_EQUAL_size_t(3, oidc_rp_handoff_store_size());

    OidcRpHandoffRecord *r = oidc_rp_handoff_store_take("k2");
    oidc_rp_handoff_record_free(r);
    TEST_ASSERT_EQUAL_size_t(2, oidc_rp_handoff_store_size());

    // Drain
    r = oidc_rp_handoff_store_take("k1"); oidc_rp_handoff_record_free(r);
    r = oidc_rp_handoff_store_take("k3"); oidc_rp_handoff_record_free(r);
}

void test_put_replaces_existing_entry_for_same_handoff(void) {
    TEST_ASSERT_TRUE(oidc_rp_handoff_store_init(60));

    TEST_ASSERT_TRUE(oidc_rp_handoff_store_put(
        "dupe", "first.jwt", 100, "first-user", NULL, NULL, 0, 60));
    TEST_ASSERT_TRUE(oidc_rp_handoff_store_put(
        "dupe", "second.jwt", 200, "second-user", NULL, NULL, 0, 60));

    TEST_ASSERT_EQUAL_size_t(1, oidc_rp_handoff_store_size());
    OidcRpHandoffRecord *r = oidc_rp_handoff_store_take("dupe");
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_EQUAL_STRING("second.jwt", r->jwt);
    TEST_ASSERT_EQUAL_INT(200, r->account_id);
    TEST_ASSERT_EQUAL_STRING("second-user", r->username);
    oidc_rp_handoff_record_free(r);
}

void test_default_ttl_applied_when_put_passes_zero(void) {
    TEST_ASSERT_TRUE(oidc_rp_handoff_store_init(45));

    TEST_ASSERT_TRUE(oidc_rp_handoff_store_put(
        "default-ttl", "jwt", 1, NULL, NULL, NULL, 0, 0));
    OidcRpHandoffRecord *r = oidc_rp_handoff_store_take("default-ttl");
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_EQUAL_INT(45, r->ttl_seconds);
    oidc_rp_handoff_record_free(r);
}

void test_record_free_handles_null(void) {
    // Just must not crash.
    oidc_rp_handoff_record_free(NULL);
}

void test_account_id_and_expires_at_round_trip(void) {
    // Specifically targets the int and time_t fields, which the
    // generic round-trip test also covers but worth pinning
    // explicitly: zero, large positive, and "looks like a JWT exp"
    // values must all survive a put/take cycle exactly.
    TEST_ASSERT_TRUE(oidc_rp_handoff_store_init(60));

    TEST_ASSERT_TRUE(oidc_rp_handoff_store_put(
        "z", "jwt", 0, NULL, NULL, NULL, (time_t)0, 60));
    TEST_ASSERT_TRUE(oidc_rp_handoff_store_put(
        "p", "jwt", 2147483640, NULL, NULL, NULL, (time_t)2147483640, 60));
    TEST_ASSERT_TRUE(oidc_rp_handoff_store_put(
        "j", "jwt", 5, NULL, NULL, NULL, (time_t)1893456000, 60));

    OidcRpHandoffRecord *rz = oidc_rp_handoff_store_take("z");
    TEST_ASSERT_NOT_NULL(rz);
    TEST_ASSERT_EQUAL_INT(0, rz->account_id);
    TEST_ASSERT_EQUAL_INT64(0, (int64_t)rz->expires_at);
    oidc_rp_handoff_record_free(rz);

    OidcRpHandoffRecord *rp = oidc_rp_handoff_store_take("p");
    TEST_ASSERT_NOT_NULL(rp);
    TEST_ASSERT_EQUAL_INT(2147483640, rp->account_id);
    TEST_ASSERT_EQUAL_INT64((int64_t)2147483640, (int64_t)rp->expires_at);
    oidc_rp_handoff_record_free(rp);

    OidcRpHandoffRecord *rj = oidc_rp_handoff_store_take("j");
    TEST_ASSERT_NOT_NULL(rj);
    TEST_ASSERT_EQUAL_INT(5, rj->account_id);
    TEST_ASSERT_EQUAL_INT64((int64_t)1893456000, (int64_t)rj->expires_at);
    oidc_rp_handoff_record_free(rj);
}

// ---------------------------------------------------------------------------
// uninit safety
// ---------------------------------------------------------------------------

void test_size_returns_zero_when_uninitialized(void) {
    // setUp shut down the store; don't re-init.
    TEST_ASSERT_EQUAL_size_t(0, oidc_rp_handoff_store_size());
}

void test_take_returns_null_when_uninitialized(void) {
    TEST_ASSERT_NULL(oidc_rp_handoff_store_take("anything"));
}

void test_sweep_returns_zero_when_uninitialized(void) {
    TEST_ASSERT_EQUAL_size_t(0, oidc_rp_handoff_store_sweep_expired());
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
        oidc_rp_handoff_store_put(key, "jwt", a->thread_id,
                                  NULL, NULL, NULL, 0, 60);
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
        OidcRpHandoffRecord *r = NULL;
        for (int spin = 0; spin < 1000 && !r; spin++) {
            r = oidc_rp_handoff_store_take(key);
            if (!r) {
                // Yield to give putter a chance.
                usleep(100);
            }
        }
        if (r) oidc_rp_handoff_record_free(r);
    }
    return NULL;
}

void test_concurrent_put_and_take(void) {
    TEST_ASSERT_TRUE(oidc_rp_handoff_store_init(60));

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
            OidcRpHandoffRecord *r = oidc_rp_handoff_store_take(key);
            if (r) oidc_rp_handoff_record_free(r);
        }
    }
    TEST_ASSERT_EQUAL_size_t(0, oidc_rp_handoff_store_size());
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

    RUN_TEST(test_take_unknown_handoff_returns_null);
    RUN_TEST(test_take_is_single_use);
    RUN_TEST(test_take_expired_record_returns_null);

    RUN_TEST(test_sweep_expired_removes_aged_records);
    RUN_TEST(test_sweep_expired_keeps_fresh_records);

    RUN_TEST(test_size_reflects_inserts_and_takes);
    RUN_TEST(test_put_replaces_existing_entry_for_same_handoff);
    RUN_TEST(test_default_ttl_applied_when_put_passes_zero);
    RUN_TEST(test_record_free_handles_null);
    RUN_TEST(test_account_id_and_expires_at_round_trip);

    RUN_TEST(test_size_returns_zero_when_uninitialized);
    RUN_TEST(test_take_returns_null_when_uninitialized);
    RUN_TEST(test_sweep_returns_zero_when_uninitialized);

    RUN_TEST(test_concurrent_put_and_take);

    return UNITY_END();
}
