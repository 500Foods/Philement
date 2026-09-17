/*
 * Unity tests for Firebase engine description helpers.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/firebase/firebase.h>

void test_firebase_engine_get_version(void);
void test_firebase_engine_is_available(void);
void test_firebase_engine_get_description(void);
void test_firebase_engine_test_functions(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_firebase_engine_get_version(void) {
    TEST_ASSERT_NOT_NULL(firebase_engine_get_version());
}

void test_firebase_engine_is_available(void) {
    TEST_ASSERT_TRUE(firebase_engine_is_available());
}

void test_firebase_engine_get_description(void) {
    TEST_ASSERT_NOT_NULL(strstr(firebase_engine_get_description(), "Firestore"));
}

void test_firebase_engine_test_functions(void) {
    firebase_engine_test_functions();
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_firebase_engine_get_version);
    RUN_TEST(test_firebase_engine_is_available);
    RUN_TEST(test_firebase_engine_get_description);
    RUN_TEST(test_firebase_engine_test_functions);
    return UNITY_END();
}
