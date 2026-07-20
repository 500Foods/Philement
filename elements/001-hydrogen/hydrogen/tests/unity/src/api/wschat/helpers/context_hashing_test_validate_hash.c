/*
 * Unity Test File: chat_context_validate_hash
 * This file contains unit tests for chat_context_validate_hash()
 * in src/api/wschat/helpers/context_hashing.c
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/wschat/helpers/context_hashing.h>

void test_validate_hash_null(void);
void test_validate_hash_wrong_length(void);
void test_validate_hash_invalid_chars(void);
void test_validate_hash_valid(void);
void test_validate_hash_valid_with_dash_underscore(void);

void setUp(void) {
}

void tearDown(void) {
}

/* NULL hash is invalid */
void test_validate_hash_null(void) {
    TEST_ASSERT_FALSE(chat_context_validate_hash(NULL));
}

/* Wrong length (not 43) is invalid */
void test_validate_hash_wrong_length(void) {
    TEST_ASSERT_FALSE(chat_context_validate_hash("abc"));
    TEST_ASSERT_FALSE(chat_context_validate_hash("abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGH"));
}

/* Contains invalid characters */
void test_validate_hash_invalid_chars(void) {
    /* 43 chars but with a '+' which is not base64url */
    TEST_ASSERT_FALSE(chat_context_validate_hash("abcdefghijklmnopqrstuvwxyzABCDEF+GHIJKLMNO"));
    /* space inside */
    TEST_ASSERT_FALSE(chat_context_validate_hash("abcdefghijklmnopqrstuvwxyzABCDEFG HIJKLMN"));
}

/* A correctly formed 43-char base64url string is valid */
void test_validate_hash_valid(void) {
    /* 43 base64url chars */
    TEST_ASSERT_TRUE(chat_context_validate_hash("abcdefghijklmnopqrstuvwxyz0123456789ABCDEFG"));
}

/* Valid with the dash and underscore characters */
void test_validate_hash_valid_with_dash_underscore(void) {
    TEST_ASSERT_TRUE(chat_context_validate_hash("a-b_cdefghijklmnopqrstuvwxyz0123456789ABCDE"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_validate_hash_null);
    RUN_TEST(test_validate_hash_wrong_length);
    RUN_TEST(test_validate_hash_invalid_chars);
    RUN_TEST(test_validate_hash_valid);
    RUN_TEST(test_validate_hash_valid_with_dash_underscore);
    return UNITY_END();
}
