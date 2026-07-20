/*
 * Unity Test File: chat_context_parse_request_hashes
 * This file contains unit tests for chat_context_parse_request_hashes()
 * in src/api/wschat/helpers/context_hashing.c
 */

#include <src/hydrogen.h>
#include <unity.h>
#include <stdlib.h>
#include <string.h>

#include <src/api/wschat/helpers/context_hashing.h>

void test_parse_null_json(void);
void test_parse_no_field(void);
void test_parse_not_array(void);
void test_parse_empty_array(void);
void test_parse_too_many(void);
void test_parse_valid(void);
void test_parse_invalid_entries_skipped(void);
void test_parse_all_invalid_returns_null(void);
void test_parse_error_message_alloc(void);

void setUp(void) {
}

void tearDown(void) {
}

/* NULL request returns NULL and sets an error message */
void test_parse_null_json(void) {
    char* err = NULL;
    char** hashes = chat_context_parse_request_hashes(NULL, &err);
    TEST_ASSERT_NULL(hashes);
    TEST_ASSERT_NOT_NULL(err);
    TEST_ASSERT_EQUAL_STRING("Invalid request JSON", err);
    free(err);
}

/* No context_hashes field is treated as valid (NULL) */
void test_parse_no_field(void) {
    json_t* req = json_object();
    TEST_ASSERT_NOT_NULL(req);
    char* err = NULL;
    char** hashes = chat_context_parse_request_hashes(req, &err);
    TEST_ASSERT_NULL(hashes);
    TEST_ASSERT_NULL(err);
    json_decref(req);
}

/* context_hashes present but not an array -> error */
void test_parse_not_array(void) {
    json_t* req = json_object();
    json_object_set_new(req, "context_hashes", json_string("nope"));
    char* err = NULL;
    char** hashes = chat_context_parse_request_hashes(req, &err);
    TEST_ASSERT_NULL(hashes);
    TEST_ASSERT_NOT_NULL(err);
    TEST_ASSERT_EQUAL_STRING("context_hashes must be an array", err);
    free(err);
    json_decref(req);
}

/* Empty array -> NULL, no error message */
void test_parse_empty_array(void) {
    json_t* req = json_object();
    json_t* arr = json_array();
    json_object_set_new(req, "context_hashes", arr);
    char* err = NULL;
    char** hashes = chat_context_parse_request_hashes(req, &err);
    TEST_ASSERT_NULL(hashes);
    TEST_ASSERT_NULL(err);
    json_decref(req);
}

/* More than CHAT_CONTEXT_MAX_HASHES -> error */
void test_parse_too_many(void) {
    json_t* req = json_object();
    json_t* arr = json_array();
    for (int i = 0; i < CHAT_CONTEXT_MAX_HASHES + 1; i++) {
        json_array_append_new(arr, json_string("abcdefghijklmnopqrstuvwxyz0123456789ABCDEFG"));
    }
    json_object_set_new(req, "context_hashes", arr);
    char* err = NULL;
    char** hashes = chat_context_parse_request_hashes(req, &err);
    TEST_ASSERT_NULL(hashes);
    TEST_ASSERT_NOT_NULL(err);
    TEST_ASSERT_EQUAL_STRING("Too many context hashes (max 100)", err);
    free(err);
    json_decref(req);
}

/* All valid -> returns NULL-terminated array of hashes */
void test_parse_valid(void) {
    json_t* req = json_object();
    json_t* arr = json_array();
    json_array_append_new(arr, json_string("abcdefghijklmnopqrstuvwxyz0123456789ABCDEFG"));
    json_array_append_new(arr, json_string("a-b_cdefghijklmnopqrstuvwxyz0123456789ABCDE"));
    json_object_set_new(req, "context_hashes", arr);

    char* err = NULL;
    char** hashes = chat_context_parse_request_hashes(req, &err);
    TEST_ASSERT_NOT_NULL(hashes);
    TEST_ASSERT_NULL(err);

    size_t count = 0;
    while (hashes[count]) count++;
    TEST_ASSERT_EQUAL_size_t(2, count);
    TEST_ASSERT_EQUAL_STRING("abcdefghijklmnopqrstuvwxyz0123456789ABCDEFG", hashes[0]);
    TEST_ASSERT_EQUAL_STRING("a-b_cdefghijklmnopqrstuvwxyz0123456789ABCDE", hashes[1]);

    chat_context_free_hash_array(hashes, count);
    json_decref(req);
}

/* Non-string and invalid-format entries are skipped */
void test_parse_invalid_entries_skipped(void) {
    const char* valid = "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFG";
    json_t* req = json_object();
    json_t* arr = json_array();
    json_array_append_new(arr, json_integer(123));            /* not a string */
    json_array_append_new(arr, json_string("tooshort"));      /* invalid format */
    json_array_append_new(arr, json_string(valid));           /* valid */
    json_object_set_new(req, "context_hashes", arr);

    char* err = NULL;
    char** hashes = chat_context_parse_request_hashes(req, &err);
    TEST_ASSERT_NOT_NULL(hashes);
    TEST_ASSERT_NULL(err);

    size_t count = 0;
    while (hashes[count]) count++;
    TEST_ASSERT_EQUAL_size_t(1, count);
    TEST_ASSERT_EQUAL_STRING(valid, hashes[0]);

    chat_context_free_hash_array(hashes, count);
    json_decref(req);
}

/* All entries invalid -> NULL (no hashes) */
void test_parse_all_invalid_returns_null(void) {
    json_t* req = json_object();
    json_t* arr = json_array();
    json_array_append_new(arr, json_string("bad"));
    json_object_set_new(req, "context_hashes", arr);

    char* err = NULL;
    char** hashes = chat_context_parse_request_hashes(req, &err);
    TEST_ASSERT_NULL(hashes);
    TEST_ASSERT_NULL(err);
    json_decref(req);
}

/* error_message pointer is optional (NULL accepted) */
void test_parse_error_message_alloc(void) {
    json_t* req = json_object();
    json_object_set_new(req, "context_hashes", json_string("nope"));
    char** hashes = chat_context_parse_request_hashes(req, NULL);
    TEST_ASSERT_NULL(hashes);
    json_decref(req);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_parse_null_json);
    RUN_TEST(test_parse_no_field);
    RUN_TEST(test_parse_not_array);
    RUN_TEST(test_parse_empty_array);
    RUN_TEST(test_parse_too_many);
    RUN_TEST(test_parse_valid);
    RUN_TEST(test_parse_invalid_entries_skipped);
    RUN_TEST(test_parse_all_invalid_returns_null);
    RUN_TEST(test_parse_error_message_alloc);
    return UNITY_END();
}
