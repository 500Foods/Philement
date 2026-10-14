/*
 * Unity Test File: database_cache_test_error_paths
 *
 * Error-path and edge-case coverage for src/database/database_cache.c.
 *
 * The happy paths are covered by database_cache_test.c; this file drives the
 * allocation-failure, lock-failure, and guard branches that only show up when
 * the system misbehaves:
 *
 *   - query_cache_create() allocation / rwlock_init failures
 *   - query_cache_entry_create() string duplication failures
 *   - query_cache_add_entry() parameter and lock failures, including the
 *     array resize failure and the zero-capacity guard
 *   - lookup / clear / next-ref / stats read and write lock failures
 *   - the query_cache_next_ref_of_type() and query_cache_next_ref_between()
 *     selection rules (type filtering, bound handling, smallest match)
 *
 * Fault injection uses the shared mock allocator (mock_system) and the pthread
 * rwlock mock (mock_pthread); the log calls in the source are captured by the
 * logging mock so the tests can confirm the right error was reported.
 */

// Fault injection needs the shared allocator mock. CMake's per-test mock
// defines are not reliable for test objects, so the macro is set here exactly
// like the other database error-path tests do.
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>
#include <unity/mocks/mock_pthread.h>
#include <unity/mocks/mock_logging.h>

// Project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database_cache.h>

// Forward declarations for functions under test
QueryTableCache* query_cache_create(const char* dqm_label);
void query_cache_destroy(QueryTableCache* cache, const char* dqm_label);
void query_cache_clear(QueryTableCache* cache, const char* dqm_label);
bool query_cache_add_entry(QueryTableCache* cache, QueryCacheEntry* entry, const char* dqm_label);
QueryCacheEntry* query_cache_lookup(QueryTableCache* cache, int query_ref, const char* dqm_label);
QueryCacheEntry* query_cache_lookup_by_ref_and_type(QueryTableCache* cache, int query_ref, int query_type, const char* dqm_label);
long long query_cache_next_ref_of_type(QueryTableCache* cache, int query_type, long long after_ref, const char* dqm_label);
long long query_cache_next_ref_between(QueryTableCache* cache, int query_type, long long after_ref, long long max_ref, const char* dqm_label);
void query_cache_update_usage(QueryTableCache* cache, int query_ref, const char* dqm_label);
QueryCacheEntry* query_cache_entry_create(int query_ref, int query_type, const char* sql_template, const char* description, const char* queue_type, int timeout_seconds, const char* dqm_label);
void query_cache_entry_destroy(QueryCacheEntry* entry);
size_t query_cache_get_entry_count(QueryTableCache* cache);
void query_cache_get_stats(QueryTableCache* cache, char* buffer, size_t buffer_size);
bool database_cache_add_entry_locked(QueryTableCache* cache, QueryCacheEntry* entry, const char* dqm_label);
long long query_cache_next_ref_scan_locked(QueryTableCache* cache, int query_type, long long after_ref, long long max_ref);

// Test function prototypes
void test_query_cache_create_struct_allocation_failure(void);
void test_query_cache_create_rwlock_init_failure(void);
void test_query_cache_create_entries_allocation_failure(void);
void test_query_cache_create_reports_dqm_label(void);
void test_query_cache_destroy_null_cache(void);
void test_query_cache_destroy_with_entries(void);
void test_query_cache_entry_create_sql_strdup_failure(void);
void test_query_cache_entry_create_description_strdup_failure(void);
void test_query_cache_entry_destroy_null_entry(void);
void test_query_cache_add_entry_null_parameters(void);
void test_query_cache_add_entry_write_lock_failure(void);
void test_database_cache_add_entry_locked_resize_failure(void);
void test_database_cache_add_entry_locked_zero_capacity(void);
void test_query_cache_clear_null_cache(void);
void test_query_cache_clear_removes_entries(void);
void test_query_cache_clear_write_lock_failure(void);
void test_query_cache_lookup_read_lock_failure(void);
void test_query_cache_lookup_by_ref_and_type_null_cache(void);
void test_query_cache_lookup_by_ref_and_type_read_lock_failure(void);
void test_query_cache_lookup_by_ref_and_type_requires_type_match(void);
void test_query_cache_lookup_tracks_usage(void);
void test_query_cache_next_ref_of_type_null_cache(void);
void test_query_cache_next_ref_of_type_read_lock_failure(void);
void test_query_cache_next_ref_of_type_no_match(void);
void test_query_cache_next_ref_of_type_filters_type_and_old_refs(void);
void test_query_cache_next_ref_of_type_returns_smallest_greater_ref(void);
void test_query_cache_next_ref_between_null_cache(void);
void test_query_cache_next_ref_between_invalid_range(void);
void test_query_cache_next_ref_between_read_lock_failure(void);
void test_query_cache_next_ref_between_filters_by_bounds(void);
void test_query_cache_next_ref_between_no_match(void);
void test_query_cache_update_usage_null_and_missing(void);
void test_query_cache_get_entry_count_null_cache(void);
void test_query_cache_get_entry_count_read_lock_failure(void);
void test_query_cache_get_stats_null_cache(void);
void test_query_cache_get_stats_read_lock_failure(void);
void test_query_cache_get_stats_empty_cache(void);
void test_query_cache_get_stats_populated_cache(void);

// Test fixture
static QueryTableCache* test_cache = NULL;

// Build a cache entry with realistic values for the given ref and type
static QueryCacheEntry* make_entry(int query_ref, int query_type, const char* sql_template) {
    return query_cache_entry_create(query_ref, query_type, sql_template,
                                    "unit test entry", "fast", 30, "unittest");
}

// Add an entry, asserting the add succeeded (keeps the tests readable)
static void add_entry(int query_ref, int query_type, const char* sql_template) {
    QueryCacheEntry* entry = make_entry(query_ref, query_type, sql_template);
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_TRUE(query_cache_add_entry(test_cache, entry, "unittest"));
}

// Release the fixture so the next allocation is the one under test
static void release_fixture(void) {
    if (test_cache) {
        query_cache_destroy(test_cache, "unittest");
        test_cache = NULL;
    }
}

void setUp(void) {
    mock_logging_reset_all();
    mock_system_reset_all();
    mock_pthread_reset_all();

    test_cache = query_cache_create("unittest");
    TEST_ASSERT_NOT_NULL(test_cache);
}

void tearDown(void) {
    // Reset the fault injectors first so teardown itself cannot be affected by
    // a failure flag left set by the test that just ran.
    mock_logging_reset_all();
    mock_system_reset_all();
    mock_pthread_reset_all();

    if (test_cache) {
        query_cache_destroy(test_cache, "unittest");
        test_cache = NULL;
    }
}

// ---- query_cache_create failure paths -------------------------------------

// Cache structure allocation failure returns NULL and reports the error
void test_query_cache_create_struct_allocation_failure(void) {
    release_fixture();

    mock_system_set_malloc_failure(1); // fail the very first allocation

    QueryTableCache* cache = query_cache_create("unittest");

    TEST_ASSERT_NULL(cache);
    TEST_ASSERT_TRUE(mock_logging_message_contains("Failed to allocate memory for query cache"));

    mock_system_set_malloc_failure(0);
}

// rwlock initialization failure returns NULL without touching the lock
void test_query_cache_create_rwlock_init_failure(void) {
    release_fixture();

    mock_pthread_set_rwlock_init_failure(1);

    QueryTableCache* cache = query_cache_create("unittest");

    TEST_ASSERT_NULL(cache);
    TEST_ASSERT_TRUE(mock_logging_message_contains("Failed to initialize cache lock"));

    mock_pthread_set_rwlock_init_failure(0);
}

// Entries array allocation failure returns NULL after cleaning up the lock
void test_query_cache_create_entries_allocation_failure(void) {
    release_fixture();

    mock_system_set_malloc_failure(2); // struct succeeds, entries array fails

    QueryTableCache* cache = query_cache_create("unittest");

    TEST_ASSERT_NULL(cache);
    TEST_ASSERT_TRUE(mock_logging_message_contains("Failed to allocate memory for cache entries"));

    mock_system_set_malloc_failure(0);
}

// A supplied dqm_label is used as the log subsystem instead of the default
void test_query_cache_create_reports_dqm_label(void) {
    release_fixture();

    QueryTableCache* cache = query_cache_create("DQM-UNIT");
    TEST_ASSERT_NOT_NULL(cache);
    TEST_ASSERT_EQUAL_STRING("DQM-UNIT", mock_logging_get_last_subsystem());

    query_cache_destroy(cache, "DQM-UNIT");
}

// ---- query_cache_destroy / query_cache_clear ------------------------------

// Destroying a NULL cache is a no-op that logs nothing
void test_query_cache_destroy_null_cache(void) {
    mock_logging_reset_all();

    query_cache_destroy(NULL, "unittest");
    TEST_ASSERT_EQUAL(0, mock_logging_get_call_count());
}

// Destroying a populated cache frees every entry without leaking or crashing
void test_query_cache_destroy_with_entries(void) {
    add_entry(1, 999, "SELECT 1");
    add_entry(2, 999, "SELECT 2");
    add_entry(3, 999, "SELECT 3");
    TEST_ASSERT_EQUAL(3, query_cache_get_entry_count(test_cache));

    query_cache_destroy(test_cache, "unittest");
    test_cache = NULL; // ownership transferred to destroy
}

// ---- query_cache_entry_create / destroy -----------------------------------

// SQL template duplication failure frees the partial entry and returns NULL
void test_query_cache_entry_create_sql_strdup_failure(void) {
    mock_system_set_malloc_failure(2); // struct succeeds, sql strdup fails

    QueryCacheEntry* entry = make_entry(10, 999, "SELECT * FROM unit_test");

    TEST_ASSERT_NULL(entry);
    TEST_ASSERT_TRUE(mock_logging_message_contains("Failed to allocate memory for cache entry strings"));

    mock_system_set_malloc_failure(0);
}

// Description duplication failure is detected the same way as the template
void test_query_cache_entry_create_description_strdup_failure(void) {
    mock_system_set_malloc_failure(3); // struct + sql succeed, description fails

    QueryCacheEntry* entry = query_cache_entry_create(11, 999, "SELECT * FROM unit_test",
                                                      "unit test entry", "fast", 30, "unittest");

    TEST_ASSERT_NULL(entry);
    TEST_ASSERT_TRUE(mock_logging_message_contains("Failed to allocate memory for cache entry strings"));

    mock_system_set_malloc_failure(0);
}

// Destroying a NULL entry is a no-op that logs nothing
void test_query_cache_entry_destroy_null_entry(void) {
    mock_logging_reset_all();

    query_cache_entry_destroy(NULL);
    TEST_ASSERT_EQUAL(0, mock_logging_get_call_count());
}

// ---- query_cache_add_entry ------------------------------------------------

// NULL cache and NULL entry are rejected
void test_query_cache_add_entry_null_parameters(void) {
    QueryCacheEntry* entry = make_entry(20, 999, "SELECT 1");
    TEST_ASSERT_NOT_NULL(entry);

    TEST_ASSERT_FALSE(query_cache_add_entry(NULL, entry, "unittest"));
    TEST_ASSERT_FALSE(query_cache_add_entry(test_cache, NULL, "unittest"));
    TEST_ASSERT_EQUAL(0, query_cache_get_entry_count(test_cache));
    TEST_ASSERT_TRUE(mock_logging_message_contains("Invalid parameters for cache add entry"));

    query_cache_entry_destroy(entry);
}

// A failed write lock leaves the entry unowned by the cache
void test_query_cache_add_entry_write_lock_failure(void) {
    QueryCacheEntry* entry = make_entry(21, 999, "SELECT 1");
    TEST_ASSERT_NOT_NULL(entry);

    mock_pthread_set_rwlock_wrlock_failure(1);

    TEST_ASSERT_FALSE(query_cache_add_entry(test_cache, entry, "unittest"));
    TEST_ASSERT_EQUAL(0, query_cache_get_entry_count(test_cache));
    TEST_ASSERT_TRUE(mock_logging_message_contains("Failed to acquire write lock for cache"));

    mock_pthread_set_rwlock_wrlock_failure(0);

    // The cache is still usable afterwards
    TEST_ASSERT_TRUE(query_cache_add_entry(test_cache, entry, "unittest"));
    TEST_ASSERT_EQUAL(1, query_cache_get_entry_count(test_cache));
}

// A failed resize keeps the existing entries and rejects the new one
void test_database_cache_add_entry_locked_resize_failure(void) {
    const int initial_capacity = 256;

    for (int i = 0; i < initial_capacity; i++) {
        add_entry(i, 999, "SELECT 1");
    }
    TEST_ASSERT_EQUAL(initial_capacity, query_cache_get_entry_count(test_cache));

    QueryCacheEntry* extra = make_entry(initial_capacity, 999, "SELECT 1");
    TEST_ASSERT_NOT_NULL(extra);

    mock_system_set_realloc_failure(1); // fail the resize on the next add

    TEST_ASSERT_FALSE(query_cache_add_entry(test_cache, extra, "unittest"));
    TEST_ASSERT_EQUAL(initial_capacity, query_cache_get_entry_count(test_cache));
    TEST_ASSERT_TRUE(mock_logging_message_contains("Failed to resize cache entries array"));

    mock_system_set_realloc_failure(0);

    // Without the injection the cache grows normally
    TEST_ASSERT_TRUE(query_cache_add_entry(test_cache, extra, "unittest"));
    TEST_ASSERT_EQUAL(initial_capacity + 1, query_cache_get_entry_count(test_cache));
}

// A cache that never had its array allocated starts at the initial capacity
void test_database_cache_add_entry_locked_zero_capacity(void) {
    QueryTableCache cache = {0};
    QueryCacheEntry* entry = make_entry(22, 999, "SELECT 1");
    TEST_ASSERT_NOT_NULL(entry);

    TEST_ASSERT_TRUE(database_cache_add_entry_locked(&cache, entry, "unittest"));
    TEST_ASSERT_EQUAL(1, cache.entry_count);
    TEST_ASSERT_EQUAL(256, cache.capacity);
    TEST_ASSERT_EQUAL_PTR(entry, cache.entries[0]);

    free(cache.entries);
    query_cache_entry_destroy(entry);
}

// ---- query_cache_clear ----------------------------------------------------

// Clearing a NULL cache is a no-op that logs nothing
void test_query_cache_clear_null_cache(void) {
    mock_logging_reset_all();

    query_cache_clear(NULL, "unittest");
    TEST_ASSERT_EQUAL(0, mock_logging_get_call_count());
}

// Clearing removes every entry and leaves the cache reusable
void test_query_cache_clear_removes_entries(void) {
    add_entry(30, 999, "SELECT 1");
    add_entry(31, 999, "SELECT 2");
    TEST_ASSERT_EQUAL(2, query_cache_get_entry_count(test_cache));

    query_cache_clear(test_cache, "unittest");

    TEST_ASSERT_EQUAL(0, query_cache_get_entry_count(test_cache));
    TEST_ASSERT_NULL(query_cache_lookup(test_cache, 30, "unittest"));

    add_entry(32, 999, "SELECT 3");
    TEST_ASSERT_EQUAL(1, query_cache_get_entry_count(test_cache));
    TEST_ASSERT_TRUE(mock_logging_message_contains("Query cache cleared"));
}

// A failed write lock keeps every entry in place
void test_query_cache_clear_write_lock_failure(void) {
    add_entry(33, 999, "SELECT 1");

    mock_pthread_set_rwlock_wrlock_failure(1);

    query_cache_clear(test_cache, "unittest");

    TEST_ASSERT_EQUAL(1, query_cache_get_entry_count(test_cache));
    TEST_ASSERT_NOT_NULL(query_cache_lookup(test_cache, 33, "unittest"));
    TEST_ASSERT_TRUE(mock_logging_message_contains("Failed to acquire write lock for cache clear"));

    mock_pthread_set_rwlock_wrlock_failure(0);
}

// ---- lookup ---------------------------------------------------------------

// A failed read lock returns NULL instead of a stale entry
void test_query_cache_lookup_read_lock_failure(void) {
    add_entry(40, 999, "SELECT 1");

    mock_pthread_set_rwlock_rdlock_failure(1);

    TEST_ASSERT_NULL(query_cache_lookup(test_cache, 40, "unittest"));
    TEST_ASSERT_TRUE(mock_logging_message_contains("Failed to acquire read lock for cache lookup"));

    mock_pthread_set_rwlock_rdlock_failure(0);
}

// A NULL cache is handled by both lookup variants
void test_query_cache_lookup_by_ref_and_type_null_cache(void) {
    TEST_ASSERT_NULL(query_cache_lookup_by_ref_and_type(NULL, 41, 999, "unittest"));
}

// A failed read lock in the typed lookup returns NULL
void test_query_cache_lookup_by_ref_and_type_read_lock_failure(void) {
    add_entry(42, 999, "SELECT 1");

    mock_pthread_set_rwlock_rdlock_failure(1);

    TEST_ASSERT_NULL(query_cache_lookup_by_ref_and_type(test_cache, 42, 999, "unittest"));
    TEST_ASSERT_TRUE(mock_logging_message_contains("Failed to acquire read lock for cache lookup"));

    mock_pthread_set_rwlock_rdlock_failure(0);
}

// Entries sharing a ref are only returned when the type also matches
void test_query_cache_lookup_by_ref_and_type_requires_type_match(void) {
    add_entry(1001, 1000, "forward migration");
    add_entry(1001, 1001, "reverse migration");
    add_entry(1001, 1002, "diagram");

    QueryCacheEntry* forward = query_cache_lookup_by_ref_and_type(test_cache, 1001, 1000, "unittest");
    QueryCacheEntry* reverse = query_cache_lookup_by_ref_and_type(test_cache, 1001, 1001, "unittest");
    QueryCacheEntry* diagram = query_cache_lookup_by_ref_and_type(test_cache, 1001, 1002, "unittest");
    QueryCacheEntry* missing = query_cache_lookup_by_ref_and_type(test_cache, 1001, 1003, "unittest");

    TEST_ASSERT_NOT_NULL(forward);
    TEST_ASSERT_NOT_NULL(reverse);
    TEST_ASSERT_NOT_NULL(diagram);
    TEST_ASSERT_NULL(missing);
    TEST_ASSERT_EQUAL_STRING("forward migration", forward->sql_template);
    TEST_ASSERT_EQUAL_STRING("reverse migration", reverse->sql_template);
    TEST_ASSERT_EQUAL_STRING("diagram", diagram->sql_template);
}

// Every successful lookup bumps the usage counter
void test_query_cache_lookup_tracks_usage(void) {
    add_entry(43, 999, "SELECT 1");

    QueryCacheEntry* first = query_cache_lookup(test_cache, 43, "unittest");
    QueryCacheEntry* second = query_cache_lookup(test_cache, 43, "unittest");

    TEST_ASSERT_NOT_NULL(first);
    TEST_ASSERT_NOT_NULL(second);
    TEST_ASSERT_EQUAL(2, first->usage_count);
    TEST_ASSERT_EQUAL(2, second->usage_count);
}

// ---- next-ref lookups -----------------------------------------------------

// A NULL cache has no next ref
void test_query_cache_next_ref_of_type_null_cache(void) {
    TEST_ASSERT_EQUAL_INT64(0, query_cache_next_ref_of_type(NULL, 1000, 5, "unittest"));
}

// A failed read lock returns 0
void test_query_cache_next_ref_of_type_read_lock_failure(void) {
    add_entry(50, 1000, "SELECT 1");

    mock_pthread_set_rwlock_rdlock_failure(1);

    TEST_ASSERT_EQUAL_INT64(0, query_cache_next_ref_of_type(test_cache, 1000, 0, "unittest"));
    TEST_ASSERT_TRUE(mock_logging_message_contains("Failed to acquire read lock for next-ref lookup"));

    mock_pthread_set_rwlock_rdlock_failure(0);
}

// Nothing at or below the after-ref, and no matching type, yields 0
void test_query_cache_next_ref_of_type_no_match(void) {
    add_entry(10, 1000, "SELECT 1");
    add_entry(20, 999, "SELECT 2");

    TEST_ASSERT_EQUAL_INT64(0, query_cache_next_ref_of_type(test_cache, 1000, 20, "unittest"));
    TEST_ASSERT_EQUAL_INT64(0, query_cache_next_ref_of_type(test_cache, 999, 25, "unittest"));
}

// Entries of another type, and entries at or below after_ref, are skipped
void test_query_cache_next_ref_of_type_filters_type_and_old_refs(void) {
    add_entry(5, 1000, "SELECT 5");
    add_entry(10, 999, "SELECT 10");  // wrong type
    add_entry(15, 1000, "SELECT 15"); // at the after_ref
    add_entry(25, 1000, "SELECT 25");
    add_entry(35, 1000, "SELECT 35");

    TEST_ASSERT_EQUAL_INT64(25, query_cache_next_ref_of_type(test_cache, 1000, 15, "unittest"));
}

// The smallest ref strictly greater than after_ref wins, even when the entries
// were appended out of order
void test_query_cache_next_ref_of_type_returns_smallest_greater_ref(void) {
    add_entry(1000, 1000, "SELECT 1000");
    add_entry(1003, 1000, "SELECT 1003");
    add_entry(1002, 1000, "SELECT 1002");
    add_entry(1010, 1000, "SELECT 1010");

    TEST_ASSERT_EQUAL_INT64(1002, query_cache_next_ref_of_type(test_cache, 1000, 1001, "unittest"));
    TEST_ASSERT_EQUAL_INT64(1000, query_cache_next_ref_of_type(test_cache, 1000, 999, "unittest"));
    TEST_ASSERT_EQUAL_INT64(0, query_cache_next_ref_of_type(test_cache, 1000, 1010, "unittest"));
}

// A NULL cache and an empty range both return 0
void test_query_cache_next_ref_between_null_cache(void) {
    TEST_ASSERT_EQUAL_INT64(0, query_cache_next_ref_between(NULL, 1000, 5, 10, "unittest"));
}

// max_ref must be strictly greater than after_ref
void test_query_cache_next_ref_between_invalid_range(void) {
    add_entry(60, 1000, "SELECT 60");

    TEST_ASSERT_EQUAL_INT64(0, query_cache_next_ref_between(test_cache, 1000, 10, 10, "unittest"));
    TEST_ASSERT_EQUAL_INT64(0, query_cache_next_ref_between(test_cache, 1000, 11, 10, "unittest"));
}

// A failed read lock returns 0
void test_query_cache_next_ref_between_read_lock_failure(void) {
    add_entry(61, 1000, "SELECT 61");

    mock_pthread_set_rwlock_rdlock_failure(1);

    TEST_ASSERT_EQUAL_INT64(0, query_cache_next_ref_between(test_cache, 1000, 0, 100, "unittest"));
    TEST_ASSERT_TRUE(mock_logging_message_contains("Failed to acquire read lock for next-ref lookup"));

    mock_pthread_set_rwlock_rdlock_failure(0);
}

// Refs outside (after_ref, max_ref] and entries of other types are skipped
void test_query_cache_next_ref_between_filters_by_bounds(void) {
    add_entry(1004, 1000, "SELECT 1004");  // at after_ref, skipped
    add_entry(1005, 999, "SELECT 1005");   // wrong type, skipped
    add_entry(1006, 1000, "SELECT 1006");
    add_entry(1008, 1000, "SELECT 1008");
    add_entry(1107, 1000, "SELECT 1107");
    add_entry(1200, 1000, "SELECT 1200");  // above max_ref, skipped

    TEST_ASSERT_EQUAL_INT64(1006, query_cache_next_ref_between(test_cache, 1000, 1004, 1100, "unittest"));
    // The upper bound is inclusive: a ref exactly at max_ref is returned
    TEST_ASSERT_EQUAL_INT64(1107, query_cache_next_ref_between(test_cache, 1000, 1100, 1107, "unittest"));
    // A bound that only contains refs above max_ref yields 0
    TEST_ASSERT_EQUAL_INT64(0, query_cache_next_ref_between(test_cache, 1000, 1004, 1005, "unittest"));
}

// No entry inside the inclusive upper bound yields 0
void test_query_cache_next_ref_between_no_match(void) {
    add_entry(1004, 1000, "SELECT 1004");
    add_entry(1101, 1000, "SELECT 1101");

    TEST_ASSERT_EQUAL_INT64(0, query_cache_next_ref_between(test_cache, 1000, 1004, 1100, "unittest"));
    TEST_ASSERT_EQUAL_INT64(1004, query_cache_next_ref_between(test_cache, 1000, 1003, 1100, "unittest"));
    TEST_ASSERT_EQUAL_INT64(0, query_cache_next_ref_between(test_cache, 1000, 1004, 1004, "unittest"));
}

// ---- update_usage ---------------------------------------------------------

// NULL cache and unknown refs are handled without touching anything
void test_query_cache_update_usage_null_and_missing(void) {
    query_cache_update_usage(NULL, 70, "unittest");
    query_cache_update_usage(test_cache, 71, "unittest");

    TEST_ASSERT_EQUAL(0, query_cache_get_entry_count(test_cache));
    TEST_ASSERT_NULL(query_cache_lookup(test_cache, 71, "unittest"));
}

// ---- entry count ----------------------------------------------------------

// A NULL cache reports zero entries
void test_query_cache_get_entry_count_null_cache(void) {
    TEST_ASSERT_EQUAL(0, query_cache_get_entry_count(NULL));
}

// A failed read lock reports zero entries
void test_query_cache_get_entry_count_read_lock_failure(void) {
    add_entry(80, 999, "SELECT 1");

    mock_pthread_set_rwlock_rdlock_failure(1);

    TEST_ASSERT_EQUAL(0, query_cache_get_entry_count(test_cache));

    mock_pthread_set_rwlock_rdlock_failure(0);
    TEST_ASSERT_EQUAL(1, query_cache_get_entry_count(test_cache));
}

// ---- statistics -----------------------------------------------------------

// A NULL cache or a missing buffer leaves the buffer untouched
void test_query_cache_get_stats_null_cache(void) {
    char buffer[128] = "untouched";

    query_cache_get_stats(NULL, buffer, sizeof(buffer));
    query_cache_get_stats(test_cache, NULL, sizeof(buffer));
    query_cache_get_stats(test_cache, buffer, 0);

    TEST_ASSERT_EQUAL_STRING("untouched", buffer);
}

// A failed read lock reports the failure in the caller's buffer
void test_query_cache_get_stats_read_lock_failure(void) {
    add_entry(90, 999, "SELECT 1");

    char buffer[128] = {0};
    mock_pthread_set_rwlock_rdlock_failure(1);

    query_cache_get_stats(test_cache, buffer, sizeof(buffer));

    TEST_ASSERT_EQUAL_STRING("Failed to acquire cache lock", buffer);

    mock_pthread_set_rwlock_rdlock_failure(0);
}

// An empty cache still produces a well-formed stats line
void test_query_cache_get_stats_empty_cache(void) {
    char buffer[256] = {0};

    query_cache_get_stats(test_cache, buffer, sizeof(buffer));

    TEST_ASSERT_TRUE(strstr(buffer, "Cache entries: 0,") != NULL);
    TEST_ASSERT_TRUE(strstr(buffer, "Total usage: 0,") != NULL);
}

// Usage totals and entry counts reflect the lookups performed
void test_query_cache_get_stats_populated_cache(void) {
    add_entry(91, 999, "SELECT 1");
    add_entry(92, 999, "SELECT 2");

    query_cache_lookup(test_cache, 91, "unittest");
    query_cache_lookup(test_cache, 92, "unittest");
    query_cache_lookup(test_cache, 92, "unittest");

    char buffer[256] = {0};
    query_cache_get_stats(test_cache, buffer, sizeof(buffer));

    TEST_ASSERT_TRUE(strstr(buffer, "Cache entries: 2,") != NULL);
    TEST_ASSERT_TRUE(strstr(buffer, "Total usage: 3") != NULL);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_query_cache_create_struct_allocation_failure);
    RUN_TEST(test_query_cache_create_rwlock_init_failure);
    RUN_TEST(test_query_cache_create_entries_allocation_failure);
    RUN_TEST(test_query_cache_create_reports_dqm_label);
    RUN_TEST(test_query_cache_destroy_null_cache);
    RUN_TEST(test_query_cache_destroy_with_entries);
    RUN_TEST(test_query_cache_entry_create_sql_strdup_failure);
    RUN_TEST(test_query_cache_entry_create_description_strdup_failure);
    RUN_TEST(test_query_cache_entry_destroy_null_entry);
    RUN_TEST(test_query_cache_add_entry_null_parameters);
    RUN_TEST(test_query_cache_add_entry_write_lock_failure);
    RUN_TEST(test_database_cache_add_entry_locked_resize_failure);
    RUN_TEST(test_database_cache_add_entry_locked_zero_capacity);
    RUN_TEST(test_query_cache_clear_null_cache);
    RUN_TEST(test_query_cache_clear_removes_entries);
    RUN_TEST(test_query_cache_clear_write_lock_failure);
    RUN_TEST(test_query_cache_lookup_read_lock_failure);
    RUN_TEST(test_query_cache_lookup_by_ref_and_type_null_cache);
    RUN_TEST(test_query_cache_lookup_by_ref_and_type_read_lock_failure);
    RUN_TEST(test_query_cache_lookup_by_ref_and_type_requires_type_match);
    RUN_TEST(test_query_cache_lookup_tracks_usage);
    RUN_TEST(test_query_cache_next_ref_of_type_null_cache);
    RUN_TEST(test_query_cache_next_ref_of_type_read_lock_failure);
    RUN_TEST(test_query_cache_next_ref_of_type_no_match);
    RUN_TEST(test_query_cache_next_ref_of_type_filters_type_and_old_refs);
    RUN_TEST(test_query_cache_next_ref_of_type_returns_smallest_greater_ref);
    RUN_TEST(test_query_cache_next_ref_between_null_cache);
    RUN_TEST(test_query_cache_next_ref_between_invalid_range);
    RUN_TEST(test_query_cache_next_ref_between_read_lock_failure);
    RUN_TEST(test_query_cache_next_ref_between_filters_by_bounds);
    RUN_TEST(test_query_cache_next_ref_between_no_match);
    RUN_TEST(test_query_cache_update_usage_null_and_missing);
    RUN_TEST(test_query_cache_get_entry_count_null_cache);
    RUN_TEST(test_query_cache_get_entry_count_read_lock_failure);
    RUN_TEST(test_query_cache_get_stats_null_cache);
    RUN_TEST(test_query_cache_get_stats_read_lock_failure);
    RUN_TEST(test_query_cache_get_stats_empty_cache);
    RUN_TEST(test_query_cache_get_stats_populated_cache);

    return UNITY_END();
}
