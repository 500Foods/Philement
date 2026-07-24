/*
 * Unity Test File: oidc_algorithm_to_string / oidc_algorithm_from_string
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/oidc/oidc_keys.h>

void test_algorithm_to_string_rs256(void);
void test_algorithm_to_string_rs384(void);
void test_algorithm_to_string_rs512(void);
void test_algorithm_to_string_es256(void);
void test_algorithm_to_string_es384(void);
void test_algorithm_to_string_es512(void);
void test_algorithm_to_string_default(void);

void test_algorithm_from_string_null(void);
void test_algorithm_from_string_rs256(void);
void test_algorithm_from_string_rs384(void);
void test_algorithm_from_string_rs512(void);
void test_algorithm_from_string_es256(void);
void test_algorithm_from_string_es384(void);
void test_algorithm_from_string_es512(void);
void test_algorithm_from_string_invalid(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_algorithm_to_string_rs256(void) {
    TEST_ASSERT_EQUAL_STRING("RS256", oidc_algorithm_to_string(KEY_ALG_RS256));
}

void test_algorithm_to_string_rs384(void) {
    TEST_ASSERT_EQUAL_STRING("RS384", oidc_algorithm_to_string(KEY_ALG_RS384));
}

void test_algorithm_to_string_rs512(void) {
    TEST_ASSERT_EQUAL_STRING("RS512", oidc_algorithm_to_string(KEY_ALG_RS512));
}

void test_algorithm_to_string_es256(void) {
    TEST_ASSERT_EQUAL_STRING("ES256", oidc_algorithm_to_string(KEY_ALG_ES256));
}

void test_algorithm_to_string_es384(void) {
    TEST_ASSERT_EQUAL_STRING("ES384", oidc_algorithm_to_string(KEY_ALG_ES384));
}

void test_algorithm_to_string_es512(void) {
    TEST_ASSERT_EQUAL_STRING("ES512", oidc_algorithm_to_string(KEY_ALG_ES512));
}

void test_algorithm_to_string_default(void) {
    TEST_ASSERT_EQUAL_STRING("RS256", oidc_algorithm_to_string((OIDCKeyAlgorithm)99));
}

void test_algorithm_from_string_null(void) {
    TEST_ASSERT_EQUAL_INT(KEY_ALG_RS256, oidc_algorithm_from_string(NULL));
}

void test_algorithm_from_string_rs256(void) {
    TEST_ASSERT_EQUAL_INT(KEY_ALG_RS256, oidc_algorithm_from_string("RS256"));
}

void test_algorithm_from_string_rs384(void) {
    TEST_ASSERT_EQUAL_INT(KEY_ALG_RS384, oidc_algorithm_from_string("RS384"));
}

void test_algorithm_from_string_rs512(void) {
    TEST_ASSERT_EQUAL_INT(KEY_ALG_RS512, oidc_algorithm_from_string("RS512"));
}

void test_algorithm_from_string_es256(void) {
    TEST_ASSERT_EQUAL_INT(KEY_ALG_ES256, oidc_algorithm_from_string("ES256"));
}

void test_algorithm_from_string_es384(void) {
    TEST_ASSERT_EQUAL_INT(KEY_ALG_ES384, oidc_algorithm_from_string("ES384"));
}

void test_algorithm_from_string_es512(void) {
    TEST_ASSERT_EQUAL_INT(KEY_ALG_ES512, oidc_algorithm_from_string("ES512"));
}

void test_algorithm_from_string_invalid(void) {
    TEST_ASSERT_EQUAL_INT(KEY_ALG_RS256, oidc_algorithm_from_string("invalid"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_algorithm_to_string_rs256);
    RUN_TEST(test_algorithm_to_string_rs384);
    RUN_TEST(test_algorithm_to_string_rs512);
    RUN_TEST(test_algorithm_to_string_es256);
    RUN_TEST(test_algorithm_to_string_es384);
    RUN_TEST(test_algorithm_to_string_es512);
    RUN_TEST(test_algorithm_to_string_default);
    RUN_TEST(test_algorithm_from_string_null);
    RUN_TEST(test_algorithm_from_string_rs256);
    RUN_TEST(test_algorithm_from_string_rs384);
    RUN_TEST(test_algorithm_from_string_rs512);
    RUN_TEST(test_algorithm_from_string_es256);
    RUN_TEST(test_algorithm_from_string_es384);
    RUN_TEST(test_algorithm_from_string_es512);
    RUN_TEST(test_algorithm_from_string_invalid);
    return UNITY_END();
}