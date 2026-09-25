/*
 * Unity Test File: Firebird Engine Metadata Tests
 * Tests firebird_engine_get_version(), firebird_engine_is_available(),
 * and firebird_engine_get_description() from src/database/firebird/firebird.c
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/firebird/interface.h>

/* Forward declarations for engine functions */
const char* firebird_engine_get_version(void);
bool firebird_engine_is_available(void);
const char* firebird_engine_get_description(void);
void firebird_engine_test_functions(void);

/* Function prototypes for test functions */
void test_firebird_engine_get_version_returns_nonnull(void);
void test_firebird_engine_get_version_nonempty(void);
void test_firebird_engine_get_version_contains_firebird(void);
void test_firebird_engine_is_available_returns_bool(void);
void test_firebird_engine_is_available_in_mock_mode(void);
void test_firebird_engine_get_description_returns_nonnull(void);
void test_firebird_engine_get_description_contains_firebird(void);
void test_firebird_engine_test_functions_runs_without_crash(void);

void setUp(void) {
    /* No setup needed for these pure functions */
}

void tearDown(void) {
    /* No cleanup needed for these pure functions */
}

void test_firebird_engine_get_version_returns_nonnull(void) {
    const char* version = firebird_engine_get_version();
    TEST_ASSERT_NOT_NULL(version);
}

void test_firebird_engine_get_version_nonempty(void) {
    const char* version = firebird_engine_get_version();
    TEST_ASSERT_TRUE(strlen(version) > 0);
}

void test_firebird_engine_get_version_contains_firebird(void) {
    const char* version = firebird_engine_get_version();
    TEST_ASSERT_NOT_NULL(strstr(version, "Firebird"));
}

void test_firebird_engine_is_available_returns_bool(void) {
    /* Call the function to ensure it executes without error */
    bool available = firebird_engine_is_available();
    (void)available;
    TEST_ASSERT_TRUE(true);
}

void test_firebird_engine_is_available_in_mock_mode(void) {
    /* In Unity test builds, USE_MOCK_LIBFBC is defined, so is_available always returns true */
    bool available = firebird_engine_is_available();
    TEST_ASSERT_TRUE(available);
}

void test_firebird_engine_get_description_returns_nonnull(void) {
    const char* description = firebird_engine_get_description();
    TEST_ASSERT_NOT_NULL(description);
}

void test_firebird_engine_get_description_contains_firebird(void) {
    const char* description = firebird_engine_get_description();
    TEST_ASSERT_NOT_NULL(strstr(description, "Firebird"));
}

void test_firebird_engine_test_functions_runs_without_crash(void) {
    /* This covers the firebird_engine_test_functions wrapper */
    firebird_engine_test_functions();
    TEST_ASSERT_TRUE(true);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_firebird_engine_get_version_returns_nonnull);
    RUN_TEST(test_firebird_engine_get_version_nonempty);
    RUN_TEST(test_firebird_engine_get_version_contains_firebird);
    RUN_TEST(test_firebird_engine_is_available_returns_bool);
    RUN_TEST(test_firebird_engine_is_available_in_mock_mode);
    RUN_TEST(test_firebird_engine_get_description_returns_nonnull);
    RUN_TEST(test_firebird_engine_get_description_contains_firebird);
    RUN_TEST(test_firebird_engine_test_functions_runs_without_crash);

    return UNITY_END();
}
