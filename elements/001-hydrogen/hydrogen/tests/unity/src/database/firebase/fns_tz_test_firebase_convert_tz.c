/*
 * Unity tests for firebase_convert_tz(). IANA via OS zoneinfo, DST-aware.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/firebase/fns_tz.h>

void test_firebase_convert_tz_null(void);
void test_firebase_convert_tz_unknown_zone(void);
void test_firebase_convert_tz_path_traversal(void);
void test_firebase_convert_tz_same_zone(void);
void test_firebase_convert_tz_utc_vancouver_winter(void);
void test_firebase_convert_tz_utc_vancouver_summer(void);
void test_firebase_convert_tz_invalid_datetime(void);
void test_firebase_tz_zone_exists_helpers(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_firebase_convert_tz_null(void) {
    TEST_ASSERT_NULL(firebase_convert_tz(NULL, "UTC", "UTC"));
    TEST_ASSERT_NULL(firebase_convert_tz("2024-01-01 12:00:00", NULL, "UTC"));
    TEST_ASSERT_NULL(firebase_convert_tz("2024-01-01 12:00:00", "UTC", NULL));
}

void test_firebase_convert_tz_unknown_zone(void) {
    TEST_ASSERT_NULL(firebase_convert_tz("2024-01-01 12:00:00", "UTC", "Not/AZone"));
    TEST_ASSERT_NULL(firebase_convert_tz("2024-01-01 12:00:00", "Not/AZone", "UTC"));
}

void test_firebase_convert_tz_path_traversal(void) {
    TEST_ASSERT_NULL(firebase_convert_tz("2024-01-01 12:00:00", "../etc/passwd", "UTC"));
}

void test_firebase_convert_tz_same_zone(void) {
    char* out = firebase_convert_tz("2024-01-01 12:00:00", "UTC", "UTC");
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_EQUAL_STRING("2024-01-01 12:00:00", out);
    free(out);
}

void test_firebase_convert_tz_utc_vancouver_winter(void) {
    char* out = firebase_convert_tz("2024-01-01 12:00:00", "UTC", "America/Vancouver");
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_EQUAL_STRING("2024-01-01 04:00:00", out);
    free(out);
}

void test_firebase_convert_tz_utc_vancouver_summer(void) {
    /* PDT is UTC-7. An 8-hour stub would wrongly yield 04:00:00. */
    char* out = firebase_convert_tz("2024-07-01 12:00:00", "UTC", "America/Vancouver");
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_EQUAL_STRING("2024-07-01 05:00:00", out);
    free(out);
}

void test_firebase_convert_tz_invalid_datetime(void) {
    TEST_ASSERT_NULL(firebase_convert_tz("not-a-date", "UTC", "UTC"));
    TEST_ASSERT_NULL(firebase_convert_tz("2024-13-01 12:00:00", "UTC", "UTC"));
}

void test_firebase_tz_zone_exists_helpers(void) {
    TEST_ASSERT_TRUE(firebase_tz_zone_exists("UTC"));
    TEST_ASSERT_TRUE(firebase_tz_zone_exists("America/Vancouver"));
    TEST_ASSERT_FALSE(firebase_tz_zone_exists("America"));
    TEST_ASSERT_FALSE(firebase_tz_zone_exists(NULL));
    TEST_ASSERT_FALSE(firebase_tz_zone_exists(""));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_firebase_convert_tz_null);
    RUN_TEST(test_firebase_convert_tz_unknown_zone);
    RUN_TEST(test_firebase_convert_tz_path_traversal);
    RUN_TEST(test_firebase_convert_tz_same_zone);
    RUN_TEST(test_firebase_convert_tz_utc_vancouver_winter);
    RUN_TEST(test_firebase_convert_tz_utc_vancouver_summer);
    RUN_TEST(test_firebase_convert_tz_invalid_datetime);
    RUN_TEST(test_firebase_tz_zone_exists_helpers);
    return UNITY_END();
}
