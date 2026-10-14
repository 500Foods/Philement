/*
 * Unity Test File: execute_helpers_test_migration_file_design
 * Unit tests for migration_file_design() in execute_helpers.c
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/migration/migration.h>

// Forward declarations for functions being tested
bool migration_file_design(const char* migration_file, char* design_out, size_t design_len);

// Forward declarations for test functions
void test_migration_file_design_valid_file(void);
void test_migration_file_design_single_char_design(void);
void test_migration_file_design_exact_buffer_size(void);
void test_migration_file_design_maximum_length(void);
void test_migration_file_design_null_migration_file(void);
void test_migration_file_design_null_design_out(void);
void test_migration_file_design_short_buffer(void);
void test_migration_file_design_no_slash(void);
void test_migration_file_design_leading_slash(void);
void test_migration_file_design_too_long_design(void);
void test_migration_file_design_buffer_too_small(void);
void test_migration_file_design_ignores_suffix(void);

void setUp(void) {
}

void tearDown(void) {
}

// ===== migration_file_design TESTS =====

void test_migration_file_design_valid_file(void) {
    char design[64] = {0};

    bool result = migration_file_design("acuranzo/acuranzo_1001.lua", design, sizeof(design));

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("acuranzo", design);
}

void test_migration_file_design_single_char_design(void) {
    char design[8] = {0};

    bool result = migration_file_design("a/a_1001.lua", design, sizeof(design));

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("a", design);
}

void test_migration_file_design_exact_buffer_size(void) {
    /* design_len of 9 exactly fits the 8 character name plus terminator. */
    char design[9] = {0};

    bool result = migration_file_design("acuranzo/x_1.lua", design, 9);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("acuranzo", design);
}

void test_migration_file_design_maximum_length(void) {
    /* A 32 character design name is the documented maximum. */
    char design[40] = {0};
    const char* migration_file = "abcdefghijklmnopqrstuvwxyz012345/abc_1001.lua";

    bool result = migration_file_design(migration_file, design, sizeof(design));

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("abcdefghijklmnopqrstuvwxyz012345", design);
    TEST_ASSERT_EQUAL(32, strlen(design));
}

void test_migration_file_design_null_migration_file(void) {
    char design[8] = {0};

    bool result = migration_file_design(NULL, design, sizeof(design));

    TEST_ASSERT_FALSE(result);
}

void test_migration_file_design_null_design_out(void) {
    bool result = migration_file_design("acuranzo/acuranzo_1001.lua", NULL, 16);

    TEST_ASSERT_FALSE(result);
}

void test_migration_file_design_short_buffer(void) {
    char design[8] = {0};

    /* design_len must be at least 2. */
    TEST_ASSERT_FALSE(migration_file_design("acuranzo/x_1.lua", design, 1));
    TEST_ASSERT_FALSE(migration_file_design("acuranzo/x_1.lua", design, 0));
}

void test_migration_file_design_no_slash(void) {
    char design[16] = {0};

    bool result = migration_file_design("acuranzo_1001.lua", design, sizeof(design));

    TEST_ASSERT_FALSE(result);
}

void test_migration_file_design_leading_slash(void) {
    char design[16] = {0};

    bool result = migration_file_design("/acuranzo_1001.lua", design, sizeof(design));

    TEST_ASSERT_FALSE(result);
}

void test_migration_file_design_too_long_design(void) {
    char design[64] = {0};
    const char* migration_file = "abcdefghijklmnopqrstuvwxyz0123456/abc_1001.lua";

    /* 33 characters before the slash exceeds the 32 character limit. */
    bool result = migration_file_design(migration_file, design, sizeof(design));

    TEST_ASSERT_FALSE(result);
}

void test_migration_file_design_buffer_too_small(void) {
    char design[16] = {0};

    /* "acuranzo" is 8 characters and needs a design_len of at least 9. */
    TEST_ASSERT_FALSE(migration_file_design("acuranzo/acuranzo_1001.lua", design, 8));
}

void test_migration_file_design_ignores_suffix(void) {
    char design[16] = {0};

    /* Only the first path component is copied, regardless of nesting. */
    bool result = migration_file_design("acuranzo/nested/dir/file.lua", design, sizeof(design));

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("acuranzo", design);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_migration_file_design_valid_file);
    RUN_TEST(test_migration_file_design_single_char_design);
    RUN_TEST(test_migration_file_design_exact_buffer_size);
    RUN_TEST(test_migration_file_design_maximum_length);
    RUN_TEST(test_migration_file_design_null_migration_file);
    RUN_TEST(test_migration_file_design_null_design_out);
    RUN_TEST(test_migration_file_design_short_buffer);
    RUN_TEST(test_migration_file_design_no_slash);
    RUN_TEST(test_migration_file_design_leading_slash);
    RUN_TEST(test_migration_file_design_too_long_design);
    RUN_TEST(test_migration_file_design_buffer_too_small);
    RUN_TEST(test_migration_file_design_ignores_suffix);

    return UNITY_END();
}
