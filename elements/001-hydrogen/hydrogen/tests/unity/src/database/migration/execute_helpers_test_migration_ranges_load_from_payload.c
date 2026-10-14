/*
 * Unity Test File: execute_helpers_test_migration_ranges_load_from_payload
 * Unit tests for migration_ranges_load_from_payload() in execute_helpers.c
 *
 * The function reads migration file names out of the global payload cache, so
 * these tests install a synthetic cache instead of loading a real payload.
 */

#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/dbqueue/dbqueue.h>
#include <src/payload/payload.h>
#include <src/payload/payload_cache.h>

// The payload cache is a plain global so tests can install a synthetic list
extern PayloadCache global_payload_cache;

// Forward declarations for functions being tested
bool migration_ranges_load_from_payload(DatabaseQueue* db_queue, const char* migrations_config,
                                        const char* dqm_label);

// Forward declarations for test functions
void test_migration_ranges_load_from_payload_null_queue(void);
void test_migration_ranges_load_from_payload_null_config(void);
void test_migration_ranges_load_from_payload_invalid_config(void);
void test_migration_ranges_load_from_payload_cache_unavailable(void);
void test_migration_ranges_load_from_payload_no_numbered_files(void);
void test_migration_ranges_load_from_payload_single_band(void);
void test_migration_ranges_load_from_payload_two_bands_sorted(void);
void test_migration_ranges_load_from_payload_skips_junk_names(void);
void test_migration_ranges_load_from_payload_restores_previous_progress(void);
void test_migration_ranges_load_from_payload_clamps_previous_count(void);
void test_migration_ranges_load_from_payload_too_many_ranges(void);

/*
 * Install a synthetic payload cache. File i gets a size of (i + 1) * 100 so
 * tests can tell which entry produced which slot.
 */
static void cache_install_files(const char** names, size_t count) {
    cleanup_payload_cache();
    initialize_payload_cache();

    global_payload_cache.is_initialized = true;
    global_payload_cache.is_available = true;
    global_payload_cache.capacity = count;
    global_payload_cache.num_files = count;
    global_payload_cache.files = calloc(count ? count : 1, sizeof(PayloadFile));
    TEST_ASSERT_NOT_NULL(global_payload_cache.files);

    for (size_t i = 0; i < count; i++) {
        global_payload_cache.files[i].name = strdup(names[i]);
        TEST_ASSERT_NOT_NULL(global_payload_cache.files[i].name);
        global_payload_cache.files[i].data = NULL;
        global_payload_cache.files[i].size = (i + 1) * 100;
        global_payload_cache.files[i].is_compressed = false;
    }
}

static DatabaseQueue* make_queue(void) {
    DatabaseQueue* db_queue = calloc(1, sizeof(DatabaseQueue));
    TEST_ASSERT_NOT_NULL(db_queue);
    return db_queue;
}

void setUp(void) {
    mock_system_reset_all();
    cleanup_payload_cache();
}

void tearDown(void) {
    cleanup_payload_cache();
    mock_system_reset_all();
}

// ===== migration_ranges_load_from_payload TESTS =====

void test_migration_ranges_load_from_payload_null_queue(void) {
    TEST_ASSERT_FALSE(migration_ranges_load_from_payload(NULL, "PAYLOAD:acuranzo", "test-label"));
}

void test_migration_ranges_load_from_payload_null_config(void) {
    DatabaseQueue* db_queue = make_queue();

    TEST_ASSERT_FALSE(migration_ranges_load_from_payload(db_queue, NULL, "test-label"));

    free(db_queue);
}

void test_migration_ranges_load_from_payload_invalid_config(void) {
    DatabaseQueue* db_queue = make_queue();
    db_queue->migration_range_count = 1;
    db_queue->migration_ranges[0].band = 1;

    /* Not a PAYLOAD spec: the design list cannot be built. */
    TEST_ASSERT_FALSE(migration_ranges_load_from_payload(db_queue, "/tmp/migrations", "test-label"));
    TEST_ASSERT_EQUAL(0, db_queue->migration_range_count);

    free(db_queue);
}

void test_migration_ranges_load_from_payload_cache_unavailable(void) {
    DatabaseQueue* db_queue = make_queue();

    /* Initialized but never loaded: get_payload_files_by_prefix reports failure. */
    cleanup_payload_cache();
    initialize_payload_cache();

    TEST_ASSERT_FALSE(migration_ranges_load_from_payload(db_queue, "PAYLOAD:acuranzo", "test-label"));
    TEST_ASSERT_EQUAL(0, db_queue->migration_range_count);

    free(db_queue);
}

void test_migration_ranges_load_from_payload_no_numbered_files(void) {
    DatabaseQueue* db_queue = make_queue();
    const char* names[] = {"acuranzo/readme.txt", "acuranzo/acuranzo_abc.lua"};

    cache_install_files(names, 2);

    TEST_ASSERT_FALSE(migration_ranges_load_from_payload(db_queue, "PAYLOAD:acuranzo", "test-label"));
    TEST_ASSERT_EQUAL(0, db_queue->migration_range_count);

    free(db_queue);
}

void test_migration_ranges_load_from_payload_single_band(void) {
    DatabaseQueue* db_queue = make_queue();
    const char* names[] = {"acuranzo/acuranzo_1001.lua", "acuranzo/acuranzo_1004.lua"};

    cache_install_files(names, 2);

    bool result = migration_ranges_load_from_payload(db_queue, "PAYLOAD:acuranzo", "test-label");

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, db_queue->migration_range_count);
    TEST_ASSERT_EQUAL(1, db_queue->migration_ranges[0].band);
    TEST_ASSERT_EQUAL_STRING("acuranzo", db_queue->migration_ranges[0].design);
    TEST_ASSERT_EQUAL(1004, db_queue->migration_ranges[0].available);
    TEST_ASSERT_EQUAL(0, db_queue->migration_ranges[0].loaded);
    TEST_ASSERT_EQUAL(0, db_queue->migration_ranges[0].applied);
    TEST_ASSERT_EQUAL(1004, db_queue->latest_available_migration);

    free(db_queue);
}

void test_migration_ranges_load_from_payload_two_bands_sorted(void) {
    DatabaseQueue* db_queue = make_queue();
    /* Band 2 shows up first, so the sort has to swap the two ranges. */
    const char* names[] = {
        "acuranzo/acuranzo_2003.lua",
        "acuranzo/acuranzo_1004.lua",
        "acuranzo/acuranzo_1001.lua"
    };

    cache_install_files(names, 3);

    bool result = migration_ranges_load_from_payload(db_queue, "PAYLOAD:acuranzo", "test-label");

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(2, db_queue->migration_range_count);
    TEST_ASSERT_EQUAL(1, db_queue->migration_ranges[0].band);
    TEST_ASSERT_EQUAL(1004, db_queue->migration_ranges[0].available);
    TEST_ASSERT_EQUAL(2, db_queue->migration_ranges[1].band);
    TEST_ASSERT_EQUAL(2003, db_queue->migration_ranges[1].available);
    TEST_ASSERT_EQUAL_STRING("acuranzo", db_queue->migration_ranges[1].design);
    TEST_ASSERT_EQUAL(2003, db_queue->latest_available_migration);

    free(db_queue);
}

void test_migration_ranges_load_from_payload_skips_junk_names(void) {
    DatabaseQueue* db_queue = make_queue();
    const char* names[] = {
        "acuranzo/readme.txt",         // wrong prefix
        "acuranzo/acuranzo_x.lua",     // not numeric
        "acuranzo/acuranzo_1002.br",   // no .lua suffix
        "acuranzo/acuranzo_.lua",      // zero length number
        "acuranzo/acuranzo_1234567.lua", // number too long
        "acuranzo/acuranzo_1001.lua"   // the only usable file
    };

    cache_install_files(names, 6);

    bool result = migration_ranges_load_from_payload(db_queue, "PAYLOAD:acuranzo", "test-label");

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, db_queue->migration_range_count);
    TEST_ASSERT_EQUAL(1, db_queue->migration_ranges[0].band);
    TEST_ASSERT_EQUAL(1001, db_queue->migration_ranges[0].available);

    free(db_queue);
}

void test_migration_ranges_load_from_payload_restores_previous_progress(void) {
    DatabaseQueue* db_queue = make_queue();
    const char* names[] = {"acuranzo/acuranzo_1001.lua", "acuranzo/acuranzo_1004.lua"};

    db_queue->migration_range_count = 1;
    db_queue->migration_ranges[0].band = 1;
    snprintf(db_queue->migration_ranges[0].design, sizeof(db_queue->migration_ranges[0].design),
             "%s", "old-design");
    db_queue->migration_ranges[0].available = 999;
    db_queue->migration_ranges[0].loaded = 1005;
    db_queue->migration_ranges[0].applied = 1007;

    cache_install_files(names, 2);

    bool result = migration_ranges_load_from_payload(db_queue, "PAYLOAD:acuranzo", "test-label");

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, db_queue->migration_range_count);
    TEST_ASSERT_EQUAL(1004, db_queue->migration_ranges[0].available);
    TEST_ASSERT_EQUAL(1005, db_queue->migration_ranges[0].loaded);
    TEST_ASSERT_EQUAL(1007, db_queue->migration_ranges[0].applied);
    TEST_ASSERT_EQUAL(1005, db_queue->latest_loaded_migration);
    TEST_ASSERT_EQUAL(1007, db_queue->latest_applied_migration);

    free(db_queue);
}

void test_migration_ranges_load_from_payload_clamps_previous_count(void) {
    DatabaseQueue* db_queue = make_queue();
    const char* names[] = {"acuranzo/acuranzo_1001.lua"};

    /* An out-of-range count must be clamped to the array capacity. */
    db_queue->migration_range_count = MIGRATION_RANGE_CAP + 5;
    for (size_t i = 0; i < MIGRATION_RANGE_CAP; i++) {
        db_queue->migration_ranges[i].band = (int)i + 1;
        db_queue->migration_ranges[i].loaded = 100;
        db_queue->migration_ranges[i].applied = 200;
    }

    cache_install_files(names, 1);

    bool result = migration_ranges_load_from_payload(db_queue, "PAYLOAD:acuranzo", "test-label");

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, db_queue->migration_range_count);
    TEST_ASSERT_EQUAL(1, db_queue->migration_ranges[0].band);
    TEST_ASSERT_EQUAL(1001, db_queue->migration_ranges[0].available);
    /* Band 1 matched a previous range, so the progress carries over. */
    TEST_ASSERT_EQUAL(100, db_queue->migration_ranges[0].loaded);
    TEST_ASSERT_EQUAL(200, db_queue->migration_ranges[0].applied);

    free(db_queue);
}

void test_migration_ranges_load_from_payload_too_many_ranges(void) {
    DatabaseQueue* db_queue = make_queue();
    const char* names[] = {
        "acuranzo/acuranzo_1000.lua",
        "acuranzo/acuranzo_2000.lua",
        "acuranzo/acuranzo_3000.lua",
        "acuranzo/acuranzo_4000.lua",
        "acuranzo/acuranzo_5000.lua",
        "acuranzo/acuranzo_6000.lua",
        "acuranzo/acuranzo_7000.lua",
        "acuranzo/acuranzo_8000.lua",
        "acuranzo/acuranzo_9000.lua"
    };

    cache_install_files(names, 9);

    /* Nine distinct bands exceed MIGRATION_RANGE_CAP. */
    TEST_ASSERT_FALSE(migration_ranges_load_from_payload(db_queue, "PAYLOAD:acuranzo", "test-label"));
    TEST_ASSERT_EQUAL(0, db_queue->migration_range_count);

    free(db_queue);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_migration_ranges_load_from_payload_null_queue);
    RUN_TEST(test_migration_ranges_load_from_payload_null_config);
    RUN_TEST(test_migration_ranges_load_from_payload_invalid_config);
    RUN_TEST(test_migration_ranges_load_from_payload_cache_unavailable);
    RUN_TEST(test_migration_ranges_load_from_payload_no_numbered_files);
    RUN_TEST(test_migration_ranges_load_from_payload_single_band);
    RUN_TEST(test_migration_ranges_load_from_payload_two_bands_sorted);
    RUN_TEST(test_migration_ranges_load_from_payload_skips_junk_names);
    RUN_TEST(test_migration_ranges_load_from_payload_restores_previous_progress);
    RUN_TEST(test_migration_ranges_load_from_payload_clamps_previous_count);
    RUN_TEST(test_migration_ranges_load_from_payload_too_many_ranges);

    return UNITY_END();
}
