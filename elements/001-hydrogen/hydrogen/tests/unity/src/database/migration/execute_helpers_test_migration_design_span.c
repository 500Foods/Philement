/*
 * Unity Test File: execute_helpers_test_migration_design_span
 * Unit tests for migration_design_span() in execute_helpers.c
 *
 * The function reads migration file names out of the global payload cache, so
 * these tests install a synthetic cache instead of loading a real payload.
 */

#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/migration/migration.h>
#include <src/payload/payload.h>
#include <src/payload/payload_cache.h>

// The payload cache is a plain global so tests can install a synthetic list
extern PayloadCache global_payload_cache;

// Forward declarations for functions being tested
bool migration_design_span(const char* design, char** first_file_out, size_t* first_size_out,
                           long long* highest_out, const char* dqm_label);

// Forward declarations for test functions
void test_migration_design_span_null_design(void);
void test_migration_design_span_null_first_file_out(void);
void test_migration_design_span_null_first_size_out(void);
void test_migration_design_span_null_highest_out(void);
void test_migration_design_span_cache_unavailable(void);
void test_migration_design_span_empty_cache(void);
void test_migration_design_span_no_matching_files(void);
void test_migration_design_span_valid_files(void);
void test_migration_design_span_lower_file_replaces_first(void);
void test_migration_design_span_only_invalid_numbers(void);
void test_migration_design_span_strdup_failure_on_first(void);
void test_migration_design_span_strdup_failure_on_replacement(void);

/*
 * Install a synthetic payload cache. File i gets a size of (i + 1) * 100 so
 * tests can tell which entry produced first_size_out.
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

void setUp(void) {
    mock_system_reset_all();
    cleanup_payload_cache();
}

void tearDown(void) {
    cleanup_payload_cache();
    mock_system_reset_all();
}

// ===== migration_design_span TESTS =====

void test_migration_design_span_null_design(void) {
    char* first_file = (char*)0x1;
    size_t first_size = 7;
    long long highest = 7;

    bool result = migration_design_span(NULL, &first_file, &first_size, &highest, "test-label");

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(first_file);
    TEST_ASSERT_EQUAL(0, first_size);
    TEST_ASSERT_EQUAL(-1, highest);
}

void test_migration_design_span_null_first_file_out(void) {
    size_t first_size = 0;
    long long highest = 0;

    bool result = migration_design_span("acuranzo", NULL, &first_size, &highest, "test-label");

    TEST_ASSERT_FALSE(result);
}

void test_migration_design_span_null_first_size_out(void) {
    char* first_file = NULL;
    long long highest = 0;

    bool result = migration_design_span("acuranzo", &first_file, NULL, &highest, "test-label");

    TEST_ASSERT_FALSE(result);
}

void test_migration_design_span_null_highest_out(void) {
    char* first_file = NULL;
    size_t first_size = 0;

    bool result = migration_design_span("acuranzo", &first_file, &first_size, NULL, "test-label");

    TEST_ASSERT_FALSE(result);
}

void test_migration_design_span_cache_unavailable(void) {
    char* first_file = NULL;
    size_t first_size = 0;
    long long highest = 0;

    /* Initialized but never loaded: get_payload_files_by_prefix reports failure. */
    cleanup_payload_cache();
    initialize_payload_cache();

    bool result = migration_design_span("acuranzo", &first_file, &first_size, &highest, "test-label");

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(first_file);
}

void test_migration_design_span_empty_cache(void) {
    char* first_file = NULL;
    size_t first_size = 0;
    long long highest = 0;

    cache_install_files(NULL, 0);

    bool result = migration_design_span("acuranzo", &first_file, &first_size, &highest, "test-label");

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NULL(first_file);
    TEST_ASSERT_EQUAL(0, first_size);
    TEST_ASSERT_EQUAL(-1, highest);
}

void test_migration_design_span_no_matching_files(void) {
    char* first_file = NULL;
    size_t first_size = 0;
    long long highest = 0;
    const char* names[] = {"terminal/app.js", "acuranzo/readme.txt"};

    cache_install_files(names, 2);

    bool result = migration_design_span("acuranzo", &first_file, &first_size, &highest, "test-label");

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NULL(first_file);
    TEST_ASSERT_EQUAL(0, first_size);
    TEST_ASSERT_EQUAL(-1, highest);
}

void test_migration_design_span_valid_files(void) {
    char* first_file = NULL;
    size_t first_size = 0;
    long long highest = 0;
    const char* names[] = {
        "acuranzo/acuranzo_1001.lua",     // valid
        "argent/argent_2001.lua",         // wrong design prefix
        "acuranzo/acuranzo_1003.txt",     // no .lua suffix
        "acuranzo/acuranzo_abc.lua",      // number is not numeric
        "acuranzo/acuranzo_1004.lua",     // valid, highest
        "acuranzo/acuranzo_1234567.lua"   // number too long
    };

    cache_install_files(names, 6);

    bool result = migration_design_span("acuranzo", &first_file, &first_size, &highest, "test-label");

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(first_file);
    TEST_ASSERT_EQUAL_STRING("acuranzo/acuranzo_1001.lua", first_file);
    TEST_ASSERT_EQUAL(100, first_size);
    TEST_ASSERT_EQUAL(1004, highest);

    free(first_file);
}

void test_migration_design_span_lower_file_replaces_first(void) {
    char* first_file = NULL;
    size_t first_size = 0;
    long long highest = 0;
    /* Descending order so the lowest file replaces an earlier copy. */
    const char* names[] = {
        "acuranzo/acuranzo_1004.lua",
        "acuranzo/acuranzo_1001.lua",
        "acuranzo/acuranzo_1002.lua"
    };

    cache_install_files(names, 3);

    bool result = migration_design_span("acuranzo", &first_file, &first_size, &highest, "test-label");

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(first_file);
    TEST_ASSERT_EQUAL_STRING("acuranzo/acuranzo_1001.lua", first_file);
    TEST_ASSERT_EQUAL(200, first_size);
    TEST_ASSERT_EQUAL(1004, highest);

    free(first_file);
}

void test_migration_design_span_only_invalid_numbers(void) {
    char* first_file = NULL;
    size_t first_size = 0;
    long long highest = 0;
    const char* names[] = {
        "acuranzo/acuranzo_abc.lua",
        "acuranzo/acuranzo_1002.txt",
        "acuranzo/acuranzo_.lua"     // zero length number
    };

    cache_install_files(names, 3);

    bool result = migration_design_span("acuranzo", &first_file, &first_size, &highest, "test-label");

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NULL(first_file);
    TEST_ASSERT_EQUAL(0, first_size);
    TEST_ASSERT_EQUAL(-1, highest);
}

void test_migration_design_span_strdup_failure_on_first(void) {
    char* first_file = NULL;
    size_t first_size = 0;
    long long highest = 0;
    const char* names[] = {"acuranzo/acuranzo_1001.lua"};

    cache_install_files(names, 1);

    /* The payload cache copies with the real allocator, so the first mock
     * allocation inside migration_design_span is its own strdup. */
    mock_system_set_malloc_failure(1);

    bool result = migration_design_span("acuranzo", &first_file, &first_size, &highest, "test-label");

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(first_file);
}

void test_migration_design_span_strdup_failure_on_replacement(void) {
    char* first_file = NULL;
    size_t first_size = 0;
    long long highest = 0;
    const char* names[] = {
        "acuranzo/acuranzo_1004.lua",
        "acuranzo/acuranzo_1001.lua"
    };

    cache_install_files(names, 2);

    /* First strdup succeeds, the replacement for the lower file fails. */
    mock_system_set_malloc_failure(2);

    bool result = migration_design_span("acuranzo", &first_file, &first_size, &highest, "test-label");

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(first_file);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_migration_design_span_null_design);
    RUN_TEST(test_migration_design_span_null_first_file_out);
    RUN_TEST(test_migration_design_span_null_first_size_out);
    RUN_TEST(test_migration_design_span_null_highest_out);
    RUN_TEST(test_migration_design_span_cache_unavailable);
    RUN_TEST(test_migration_design_span_empty_cache);
    RUN_TEST(test_migration_design_span_no_matching_files);
    RUN_TEST(test_migration_design_span_valid_files);
    RUN_TEST(test_migration_design_span_lower_file_replaces_first);
    RUN_TEST(test_migration_design_span_only_invalid_numbers);
    RUN_TEST(test_migration_design_span_strdup_failure_on_first);
    RUN_TEST(test_migration_design_span_strdup_failure_on_replacement);

    return UNITY_END();
}
