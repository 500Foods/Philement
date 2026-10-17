/*
 * Unity Test File: execute_helpers_test_migration_payload_designs
 * Unit tests for migration_payload_designs() and migration_payload_designs_free()
 * in execute_helpers.c
 */

#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/migration/migration.h>

// Forward declarations for functions being tested
bool migration_payload_designs(const char* migrations_config, char*** designs_out,
                               size_t* count_out, const char* dqm_label);
void migration_payload_designs_free(char** designs, size_t count);

// Forward declarations for test functions
void test_migration_payload_designs_single_design(void);
void test_migration_payload_designs_acuranzo_and_argent(void);
void test_migration_payload_designs_acuranzo_and_gaius(void);
void test_migration_payload_designs_full_suite(void);
void test_migration_payload_designs_acuranzo_and_courses(void);
void test_migration_payload_designs_courses_without_acuranzo(void);
void test_migration_payload_designs_maximum_designs(void);
void test_migration_payload_designs_null_config(void);
void test_migration_payload_designs_not_payload_format(void);
void test_migration_payload_designs_empty_spec(void);
void test_migration_payload_designs_too_many_pluses(void);
void test_migration_payload_designs_calloc_failure(void);
void test_migration_payload_designs_name_malloc_failure(void);
void test_migration_payload_designs_null_designs_out(void);
void test_migration_payload_designs_null_count_out(void);
void test_migration_payload_designs_repeats_design(void);
void test_migration_payload_designs_trailing_plus(void);
void test_migration_payload_designs_leading_plus(void);
void test_migration_payload_designs_name_too_long(void);
void test_migration_payload_designs_uppercase_name(void);
void test_migration_payload_designs_invalid_character(void);
void test_migration_payload_designs_argent_without_acuranzo(void);
void test_migration_payload_designs_gaius_without_acuranzo(void);
void test_migration_payload_designs_free_null_list(void);
void test_migration_payload_designs_free_empty_list(void);
void test_migration_payload_designs_free_populated_list(void);

void setUp(void) {
    mock_system_reset_all();
}

void tearDown(void) {
    mock_system_reset_all();
}

// ===== migration_payload_designs TESTS =====

void test_migration_payload_designs_single_design(void) {
    char** designs = NULL;
    size_t count = 99;

    bool result = migration_payload_designs("PAYLOAD:acuranzo", &designs, &count, "test-label");

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, count);
    TEST_ASSERT_NOT_NULL(designs);
    TEST_ASSERT_NOT_NULL(designs[0]);
    TEST_ASSERT_EQUAL_STRING("acuranzo", designs[0]);

    migration_payload_designs_free(designs, count);
}

void test_migration_payload_designs_acuranzo_and_argent(void) {
    char** designs = NULL;
    size_t count = 0;

    bool result = migration_payload_designs("PAYLOAD:acuranzo+argent", &designs, &count, "test-label");

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(2, count);
    TEST_ASSERT_EQUAL_STRING("acuranzo", designs[0]);
    TEST_ASSERT_EQUAL_STRING("argent", designs[1]);

    migration_payload_designs_free(designs, count);
}

void test_migration_payload_designs_full_suite(void) {
    char** designs = NULL;
    size_t count = 0;

    bool result = migration_payload_designs("PAYLOAD:acuranzo+courses+argent",
                                            &designs, &count, "test-label");

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(3, count);
    TEST_ASSERT_EQUAL_STRING("acuranzo", designs[0]);
    TEST_ASSERT_EQUAL_STRING("courses", designs[1]);
    TEST_ASSERT_EQUAL_STRING("argent", designs[2]);

    migration_payload_designs_free(designs, count);
}

void test_migration_payload_designs_acuranzo_and_courses(void) {
    char** designs = NULL;
    size_t count = 0;

    bool result = migration_payload_designs("PAYLOAD:acuranzo+courses", &designs, &count, "test-label");

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(2, count);
    TEST_ASSERT_EQUAL_STRING("acuranzo", designs[0]);
    TEST_ASSERT_EQUAL_STRING("courses", designs[1]);

    migration_payload_designs_free(designs, count);
}

void test_migration_payload_designs_courses_without_acuranzo(void) {
    char** designs = NULL;
    size_t count = 99;

    bool result = migration_payload_designs("PAYLOAD:courses", &designs, &count, "test-label");

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(designs);
    TEST_ASSERT_EQUAL(0, count);
}

void test_migration_payload_designs_acuranzo_and_gaius(void) {
    char** designs = NULL;
    size_t count = 0;

    bool result = migration_payload_designs("PAYLOAD:acuranzo+gaius", &designs, &count, "test-label");

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(2, count);
    TEST_ASSERT_EQUAL_STRING("acuranzo", designs[0]);
    TEST_ASSERT_EQUAL_STRING("gaius", designs[1]);

    migration_payload_designs_free(designs, count);
}

void test_migration_payload_designs_maximum_designs(void) {
    char** designs = NULL;
    size_t count = 0;

    bool result = migration_payload_designs("PAYLOAD:a1+a2+a3+a4+a5+a6+a7+a8",
                                            &designs, &count, "test-label");

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(8, count);
    TEST_ASSERT_EQUAL_STRING("a8", designs[7]);

    migration_payload_designs_free(designs, count);
}

void test_migration_payload_designs_null_config(void) {
    char** designs = (char**)0x1;
    size_t count = 99;

    bool result = migration_payload_designs(NULL, &designs, &count, "test-label");

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(designs);
    TEST_ASSERT_EQUAL(0, count);
}

void test_migration_payload_designs_not_payload_format(void) {
    char** designs = NULL;
    size_t count = 99;

    bool result = migration_payload_designs("acuranzo+argent", &designs, &count, "test-label");

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(designs);
    TEST_ASSERT_EQUAL(0, count);
}

void test_migration_payload_designs_empty_spec(void) {
    char** designs = NULL;
    size_t count = 99;

    bool result = migration_payload_designs("PAYLOAD:", &designs, &count, "test-label");

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(designs);
    TEST_ASSERT_EQUAL(0, count);
}

void test_migration_payload_designs_too_many_pluses(void) {
    char** designs = NULL;
    size_t count = 99;

    bool result = migration_payload_designs("PAYLOAD:a+b+c+d+e+f+g+h+i",
                                            &designs, &count, "test-label");

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(designs);
    TEST_ASSERT_EQUAL(0, count);
}

void test_migration_payload_designs_calloc_failure(void) {
    char** designs = NULL;
    size_t count = 99;

    mock_system_set_malloc_failure(1);

    bool result = migration_payload_designs("PAYLOAD:acuranzo", &designs, &count, "test-label");

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(designs);
    TEST_ASSERT_EQUAL(0, count);
}

void test_migration_payload_designs_name_malloc_failure(void) {
    char** designs = NULL;
    size_t count = 99;

    /* First allocation is the designs array, second is the design name. */
    mock_system_set_malloc_failure(2);

    bool result = migration_payload_designs("PAYLOAD:acuranzo", &designs, &count, "test-label");

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(designs);
    TEST_ASSERT_EQUAL(0, count);
}

void test_migration_payload_designs_null_designs_out(void) {
    size_t count = 99;

    bool result = migration_payload_designs("PAYLOAD:acuranzo", NULL, &count, "test-label");

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(0, count);
}

void test_migration_payload_designs_null_count_out(void) {
    char** designs = NULL;

    bool result = migration_payload_designs("PAYLOAD:acuranzo", &designs, NULL, "test-label");

    TEST_ASSERT_FALSE(result);
}

void test_migration_payload_designs_repeats_design(void) {
    char** designs = NULL;
    size_t count = 99;

    bool result = migration_payload_designs("PAYLOAD:acuranzo+argent+acuranzo",
                                            &designs, &count, "test-label");

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(designs);
    TEST_ASSERT_EQUAL(0, count);
}

void test_migration_payload_designs_trailing_plus(void) {
    char** designs = NULL;
    size_t count = 99;

    bool result = migration_payload_designs("PAYLOAD:acuranzo+", &designs, &count, "test-label");

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(designs);
    TEST_ASSERT_EQUAL(0, count);
}

void test_migration_payload_designs_leading_plus(void) {
    char** designs = NULL;
    size_t count = 99;

    bool result = migration_payload_designs("PAYLOAD:+acuranzo", &designs, &count, "test-label");

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(designs);
    TEST_ASSERT_EQUAL(0, count);
}

void test_migration_payload_designs_name_too_long(void) {
    char** designs = NULL;
    size_t count = 99;

    bool result = migration_payload_designs("PAYLOAD:abcdefghijklmnopqrstuvwxyz0123456",
                                            &designs, &count, "test-label");

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(designs);
    TEST_ASSERT_EQUAL(0, count);
}

void test_migration_payload_designs_uppercase_name(void) {
    char** designs = NULL;
    size_t count = 99;

    bool result = migration_payload_designs("PAYLOAD:Acuranzo", &designs, &count, "test-label");

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(designs);
    TEST_ASSERT_EQUAL(0, count);
}

void test_migration_payload_designs_invalid_character(void) {
    char** designs = NULL;
    size_t count = 99;

    bool result = migration_payload_designs("PAYLOAD:acur-anzo", &designs, &count, "test-label");

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(designs);
    TEST_ASSERT_EQUAL(0, count);
}

void test_migration_payload_designs_argent_without_acuranzo(void) {
    char** designs = NULL;
    size_t count = 99;

    bool result = migration_payload_designs("PAYLOAD:argent", &designs, &count, "test-label");

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(designs);
    TEST_ASSERT_EQUAL(0, count);
}

void test_migration_payload_designs_gaius_without_acuranzo(void) {
    char** designs = NULL;
    size_t count = 99;

    bool result = migration_payload_designs("PAYLOAD:gaius", &designs, &count, "test-label");

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(designs);
    TEST_ASSERT_EQUAL(0, count);
}

void test_migration_payload_designs_free_null_list(void) {
    /* Must be a no-op rather than a crash. */
    migration_payload_designs_free(NULL, 4);
    TEST_ASSERT_TRUE(true);
}

void test_migration_payload_designs_free_empty_list(void) {
    char** designs = calloc(2, sizeof(char*));
    TEST_ASSERT_NOT_NULL(designs);

    migration_payload_designs_free(designs, 0);
    TEST_ASSERT_TRUE(true);
}

void test_migration_payload_designs_free_populated_list(void) {
    char** designs = calloc(3, sizeof(char*));
    TEST_ASSERT_NOT_NULL(designs);
    designs[0] = strdup("acuranzo");
    designs[1] = strdup("argent");
    designs[2] = strdup("gaius");

    migration_payload_designs_free(designs, 3);
    TEST_ASSERT_TRUE(true);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_migration_payload_designs_single_design);
    RUN_TEST(test_migration_payload_designs_acuranzo_and_argent);
    RUN_TEST(test_migration_payload_designs_acuranzo_and_gaius);
    RUN_TEST(test_migration_payload_designs_full_suite);
    RUN_TEST(test_migration_payload_designs_acuranzo_and_courses);
    RUN_TEST(test_migration_payload_designs_courses_without_acuranzo);
    RUN_TEST(test_migration_payload_designs_maximum_designs);
    RUN_TEST(test_migration_payload_designs_null_config);
    RUN_TEST(test_migration_payload_designs_not_payload_format);
    RUN_TEST(test_migration_payload_designs_empty_spec);
    RUN_TEST(test_migration_payload_designs_too_many_pluses);
    RUN_TEST(test_migration_payload_designs_calloc_failure);
    RUN_TEST(test_migration_payload_designs_name_malloc_failure);
    RUN_TEST(test_migration_payload_designs_null_designs_out);
    RUN_TEST(test_migration_payload_designs_null_count_out);
    RUN_TEST(test_migration_payload_designs_repeats_design);
    RUN_TEST(test_migration_payload_designs_trailing_plus);
    RUN_TEST(test_migration_payload_designs_leading_plus);
    RUN_TEST(test_migration_payload_designs_name_too_long);
    RUN_TEST(test_migration_payload_designs_uppercase_name);
    RUN_TEST(test_migration_payload_designs_invalid_character);
    RUN_TEST(test_migration_payload_designs_argent_without_acuranzo);
    RUN_TEST(test_migration_payload_designs_gaius_without_acuranzo);
    RUN_TEST(test_migration_payload_designs_free_null_list);
    RUN_TEST(test_migration_payload_designs_free_empty_list);
    RUN_TEST(test_migration_payload_designs_free_populated_list);

    return UNITY_END();
}
