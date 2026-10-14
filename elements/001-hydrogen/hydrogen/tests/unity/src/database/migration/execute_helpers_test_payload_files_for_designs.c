/*
 * Unity Test File: execute_helpers_test_payload_files_for_designs
 * Unit tests for payload_files_for_designs() in execute_helpers.c
 *
 * The function reads file names out of the global payload cache, so these
 * tests install a synthetic cache instead of loading a real payload.
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
bool payload_files_for_designs(char** designs, size_t design_count, PayloadFile** files_out,
                               size_t* count_out, const char* dqm_label);

// Forward declarations for test functions
void test_payload_files_for_designs_single_design(void);
void test_payload_files_for_designs_two_designs(void);
void test_payload_files_for_designs_null_designs(void);
void test_payload_files_for_designs_null_files_out(void);
void test_payload_files_for_designs_null_count_out(void);
void test_payload_files_for_designs_zero_design_count(void);
void test_payload_files_for_designs_design_without_files(void);
void test_payload_files_for_designs_cache_unavailable(void);
void test_payload_files_for_designs_second_design_without_files(void);
void test_payload_files_for_designs_realloc_failure(void);

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

void setUp(void) {
    mock_system_reset_all();
    cleanup_payload_cache();
}

void tearDown(void) {
    cleanup_payload_cache();
    mock_system_reset_all();
}

// ===== payload_files_for_designs TESTS =====

void test_payload_files_for_designs_single_design(void) {
    const char* names[] = {"acuranzo/acuranzo_1001.lua", "acuranzo/acuranzo_1002.lua"};
    char* designs[] = {(char*)"acuranzo"};
    PayloadFile* files = NULL;
    size_t count = 99;

    cache_install_files(names, 2);

    bool result = payload_files_for_designs(designs, 1, &files, &count, "test-label");

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(2, count);
    TEST_ASSERT_NOT_NULL(files);
    TEST_ASSERT_EQUAL_STRING("acuranzo/acuranzo_1001.lua", files[0].name);
    TEST_ASSERT_EQUAL_STRING("acuranzo/acuranzo_1002.lua", files[1].name);
    TEST_ASSERT_EQUAL(100, files[0].size);
    TEST_ASSERT_EQUAL(200, files[1].size);

    free_payload_files(files, count);
}

void test_payload_files_for_designs_two_designs(void) {
    const char* names[] = {
        "acuranzo/acuranzo_1001.lua",
        "argent/argent_2001.lua",
        "argent/argent_2002.lua"
    };
    char* designs[] = {(char*)"acuranzo", (char*)"argent"};
    PayloadFile* files = NULL;
    size_t count = 0;

    cache_install_files(names, 3);

    bool result = payload_files_for_designs(designs, 2, &files, &count, "test-label");

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(3, count);
    TEST_ASSERT_NOT_NULL(files);
    TEST_ASSERT_EQUAL_STRING("acuranzo/acuranzo_1001.lua", files[0].name);
    TEST_ASSERT_EQUAL_STRING("argent/argent_2001.lua", files[1].name);
    TEST_ASSERT_EQUAL_STRING("argent/argent_2002.lua", files[2].name);

    free_payload_files(files, count);
}

void test_payload_files_for_designs_null_designs(void) {
    PayloadFile* files = (PayloadFile*)0x1;
    size_t count = 99;

    bool result = payload_files_for_designs(NULL, 1, &files, &count, "test-label");

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(files);
    TEST_ASSERT_EQUAL(0, count);
}

void test_payload_files_for_designs_null_files_out(void) {
    char* designs[] = {(char*)"acuranzo"};
    size_t count = 99;

    bool result = payload_files_for_designs(designs, 1, NULL, &count, "test-label");

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(0, count);
}

void test_payload_files_for_designs_null_count_out(void) {
    char* designs[] = {(char*)"acuranzo"};
    PayloadFile* files = NULL;

    bool result = payload_files_for_designs(designs, 1, &files, NULL, "test-label");

    TEST_ASSERT_FALSE(result);
}

void test_payload_files_for_designs_zero_design_count(void) {
    PayloadFile* files = (PayloadFile*)0x1;
    size_t count = 99;

    bool result = payload_files_for_designs(NULL, 0, &files, &count, "test-label");

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(files);
    TEST_ASSERT_EQUAL(0, count);
}

void test_payload_files_for_designs_design_without_files(void) {
    const char* names[] = {"terminal/app.js"};
    char* designs[] = {(char*)"acuranzo"};
    PayloadFile* files = (PayloadFile*)0x1;
    size_t count = 99;

    cache_install_files(names, 1);

    bool result = payload_files_for_designs(designs, 1, &files, &count, "test-label");

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(files);
    TEST_ASSERT_EQUAL(0, count);
}

void test_payload_files_for_designs_cache_unavailable(void) {
    char* designs[] = {(char*)"acuranzo"};
    PayloadFile* files = (PayloadFile*)0x1;
    size_t count = 99;

    /* Initialized but never loaded: get_payload_files_by_prefix reports failure. */
    cleanup_payload_cache();
    initialize_payload_cache();

    bool result = payload_files_for_designs(designs, 1, &files, &count, "test-label");

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(files);
    TEST_ASSERT_EQUAL(0, count);
}

void test_payload_files_for_designs_second_design_without_files(void) {
    const char* names[] = {"acuranzo/acuranzo_1001.lua"};
    char* designs[] = {(char*)"acuranzo", (char*)"argent"};
    PayloadFile* files = NULL;
    size_t count = 0;

    cache_install_files(names, 1);

    /* The first design succeeds, the second one empties and frees the result. */
    bool result = payload_files_for_designs(designs, 2, &files, &count, "test-label");

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(files);
    TEST_ASSERT_EQUAL(0, count);
}

void test_payload_files_for_designs_realloc_failure(void) {
    const char* names[] = {"acuranzo/acuranzo_1001.lua", "acuranzo/acuranzo_1002.lua"};
    char* designs[] = {(char*)"acuranzo"};
    PayloadFile* files = NULL;
    size_t count = 99;

    cache_install_files(names, 2);

    mock_system_set_realloc_failure(1);

    bool result = payload_files_for_designs(designs, 1, &files, &count, "test-label");

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(files);
    TEST_ASSERT_EQUAL(0, count);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_payload_files_for_designs_single_design);
    RUN_TEST(test_payload_files_for_designs_two_designs);
    RUN_TEST(test_payload_files_for_designs_null_designs);
    RUN_TEST(test_payload_files_for_designs_null_files_out);
    RUN_TEST(test_payload_files_for_designs_null_count_out);
    RUN_TEST(test_payload_files_for_designs_zero_design_count);
    RUN_TEST(test_payload_files_for_designs_design_without_files);
    RUN_TEST(test_payload_files_for_designs_cache_unavailable);
    RUN_TEST(test_payload_files_for_designs_second_design_without_files);
    RUN_TEST(test_payload_files_for_designs_realloc_failure);

    return UNITY_END();
}
