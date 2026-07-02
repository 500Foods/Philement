/*
 * Unity Test File: source_cache_test_put_get.c
 *
 * Phase 11g of the LUA_PLAN. Validates the SourceCache data structure:
 *
 *   - create/destroy lifecycle
 *   - put/get round-trip
 *   - composite key is (group_name, script_name)
 *   - replace-on-put
 *   - NULL safety on all public functions
 *   - concurrent puts from multiple threads
 *   - count is accurate
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Standard includes
#include <pthread.h>
#include <string.h>

// Module under test
#include <src/scripting/source_cache.h>

// Forward declarations (required for -Wmissing-prototypes)
void test_create_destroy(void);
void test_put_get_round_trip(void);
void test_composite_key(void);
void test_replace_on_put(void);
void test_null_safety(void);
void test_count(void);
void test_concurrent_puts(void);
void test_null_source_rejected(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_create_destroy(void) {
    SourceCache* cache = source_cache_create();
    TEST_ASSERT_NOT_NULL(cache);
    TEST_ASSERT_EQUAL(0, source_cache_count(cache));
    source_cache_destroy(cache);

    // Destroy NULL is safe.
    source_cache_destroy(NULL);
}

void test_put_get_round_trip(void) {
    SourceCache* cache = source_cache_create();
    TEST_ASSERT_TRUE(source_cache_put(cache, "Group", "Script", "local x = 1"));
    TEST_ASSERT_EQUAL(1, source_cache_count(cache));

    const char* source = source_cache_get(cache, "Group", "Script");
    TEST_ASSERT_NOT_NULL(source);
    TEST_ASSERT_EQUAL_STRING("local x = 1", source);

    source_cache_destroy(cache);
}

void test_composite_key(void) {
    SourceCache* cache = source_cache_create();
    TEST_ASSERT_TRUE(source_cache_put(cache, "A", "B", "source AB"));
    TEST_ASSERT_TRUE(source_cache_put(cache, "A", "C", "source AC"));
    TEST_ASSERT_TRUE(source_cache_put(cache, "B", "B", "source BB"));

    TEST_ASSERT_EQUAL_STRING("source AB", source_cache_get(cache, "A", "B"));
    TEST_ASSERT_EQUAL_STRING("source AC", source_cache_get(cache, "A", "C"));
    TEST_ASSERT_EQUAL_STRING("source BB", source_cache_get(cache, "B", "B"));
    TEST_ASSERT_NULL(source_cache_get(cache, "A", "D"));
    TEST_ASSERT_NULL(source_cache_get(cache, "C", "B"));

    source_cache_destroy(cache);
}

void test_replace_on_put(void) {
    SourceCache* cache = source_cache_create();
    TEST_ASSERT_TRUE(source_cache_put(cache, "G", "S", "v1"));
    TEST_ASSERT_TRUE(source_cache_put(cache, "G", "S", "v2"));
    TEST_ASSERT_EQUAL(1, source_cache_count(cache));
    TEST_ASSERT_EQUAL_STRING("v2", source_cache_get(cache, "G", "S"));

    source_cache_destroy(cache);
}

void test_null_safety(void) {
    SourceCache* cache = source_cache_create();

    TEST_ASSERT_FALSE(source_cache_put(NULL, "G", "S", "x"));
    TEST_ASSERT_FALSE(source_cache_put(cache, NULL, "S", "x"));
    TEST_ASSERT_FALSE(source_cache_put(cache, "G", NULL, "x"));
    TEST_ASSERT_FALSE(source_cache_put(cache, "G", "S", NULL));
    TEST_ASSERT_NULL(source_cache_get(NULL, "G", "S"));
    TEST_ASSERT_NULL(source_cache_get(cache, NULL, "S"));
    TEST_ASSERT_NULL(source_cache_get(cache, "G", NULL));
    TEST_ASSERT_EQUAL(0, source_cache_count(NULL));

    source_cache_destroy(cache);
}

void test_count(void) {
    SourceCache* cache = source_cache_create();
    TEST_ASSERT_EQUAL(0, source_cache_count(cache));
    source_cache_put(cache, "G1", "S1", "a");
    TEST_ASSERT_EQUAL(1, source_cache_count(cache));
    source_cache_put(cache, "G1", "S2", "b");
    TEST_ASSERT_EQUAL(2, source_cache_count(cache));
    source_cache_put(cache, "G1", "S1", "c");
    TEST_ASSERT_EQUAL(2, source_cache_count(cache));

    source_cache_destroy(cache);
}

// Concurrent put worker data.
typedef struct {
    SourceCache* cache;
    int          thread_index;
} concurrent_worker_arg_t;

static void* concurrent_worker(void* arg) {
    concurrent_worker_arg_t* w = (concurrent_worker_arg_t*)arg;
    char group[32];
    char script[32];
    char source[64];
    snprintf(group, sizeof(group), "group%d", w->thread_index);
    snprintf(script, sizeof(script), "script%d", w->thread_index);
    snprintf(source, sizeof(source), "source from thread %d", w->thread_index);
    for (int i = 0; i < 50; i++) {
        source_cache_put(w->cache, group, script, source);
    }
    return NULL;
}

void test_concurrent_puts(void) {
    SourceCache* cache = source_cache_create();
    pthread_t threads[4];
    concurrent_worker_arg_t args[4];

    for (int i = 0; i < 4; i++) {
        args[i].cache = cache;
        args[i].thread_index = i;
        pthread_create(&threads[i], NULL, concurrent_worker, &args[i]);
    }
    for (int i = 0; i < 4; i++) {
        pthread_join(threads[i], NULL);
    }

    TEST_ASSERT_EQUAL(4, source_cache_count(cache));
    for (int i = 0; i < 4; i++) {
        char group[32];
        char script[32];
        char expected[64];
        snprintf(group, sizeof(group), "group%d", i);
        snprintf(script, sizeof(script), "script%d", i);
        snprintf(expected, sizeof(expected), "source from thread %d", i);
        const char* source = source_cache_get(cache, group, script);
        TEST_ASSERT_NOT_NULL(source);
        TEST_ASSERT_EQUAL_STRING(expected, source);
    }

    source_cache_destroy(cache);
}

void test_null_source_rejected(void) {
    SourceCache* cache = source_cache_create();
    TEST_ASSERT_FALSE(source_cache_put(cache, "G", "S", NULL));
    TEST_ASSERT_EQUAL(0, source_cache_count(cache));
    source_cache_destroy(cache);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_create_destroy);
    RUN_TEST(test_put_get_round_trip);
    RUN_TEST(test_composite_key);
    RUN_TEST(test_replace_on_put);
    RUN_TEST(test_null_safety);
    RUN_TEST(test_count);
    RUN_TEST(test_concurrent_puts);
    RUN_TEST(test_null_source_rejected);

    return UNITY_END();
}
