/*
 * Unity tests for firebase_json_ingest().
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/firebase/fns_json.h>

void test_firebase_json_ingest_null(void);
void test_firebase_json_ingest_valid_passthrough(void);
void test_firebase_json_ingest_newline_in_string(void);
void test_firebase_json_ingest_tab_and_cr_in_string(void);
void test_firebase_json_ingest_schema_ref(void);
void test_firebase_json_ingest_garbage(void);
void test_firebase_json_ingest_already_escaped(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_firebase_json_ingest_null(void) {
    TEST_ASSERT_NULL(firebase_json_ingest(NULL));
    TEST_ASSERT_FALSE(firebase_json_is_valid(NULL));
    TEST_ASSERT_NULL(firebase_json_fixup_controls(NULL));
}

void test_firebase_json_ingest_valid_passthrough(void) {
    const char* original = " { \"a\" : 1, \"b\" : [true, false] }";
    char* ingested = firebase_json_ingest(original);
    TEST_ASSERT_NOT_NULL(ingested);
    TEST_ASSERT_EQUAL_STRING(original, ingested);
    free(ingested);
}

void test_firebase_json_ingest_newline_in_string(void) {
    const char* original = "{\"msg\":\"hello\nworld\"}";
    char* ingested = firebase_json_ingest(original);
    TEST_ASSERT_NOT_NULL(ingested);
    TEST_ASSERT_EQUAL_STRING("{\"msg\":\"hello\\nworld\"}", ingested);
    TEST_ASSERT_TRUE(firebase_json_is_valid(ingested));
    free(ingested);
}

void test_firebase_json_ingest_tab_and_cr_in_string(void) {
    const char* original = "{\"msg\":\"a\tb\rc\"}";
    char* ingested = firebase_json_ingest(original);
    TEST_ASSERT_NOT_NULL(ingested);
    TEST_ASSERT_EQUAL_STRING("{\"msg\":\"a\\tb\\rc\"}", ingested);
    free(ingested);
}

void test_firebase_json_ingest_schema_ref(void) {
    const char* schema =
        "{\"$schema\":\"http://json-schema.org/draft-07/schema#\","
        "\"$id\":\"https://example.com/item.json\","
        "\"properties\":{\"item\":{\"$ref\":\"#/definitions/item\"}}}";
    char* ingested = firebase_json_ingest(schema);
    TEST_ASSERT_NOT_NULL(ingested);
    TEST_ASSERT_EQUAL_STRING(schema, ingested);
    TEST_ASSERT_NOT_NULL(strstr(ingested, "\"$ref\""));
    TEST_ASSERT_NOT_NULL(strstr(ingested, "\"$id\""));
    TEST_ASSERT_NOT_NULL(strstr(ingested, "\"$schema\""));
    free(ingested);
}

void test_firebase_json_ingest_garbage(void) {
    TEST_ASSERT_NULL(firebase_json_ingest("{not json"));
    TEST_ASSERT_NULL(firebase_json_ingest(""));
    TEST_ASSERT_NULL(firebase_json_ingest("hello"));
}

void test_firebase_json_ingest_already_escaped(void) {
    const char* original = "{\"msg\":\"hello\\nworld\"}";
    char* ingested = firebase_json_ingest(original);
    TEST_ASSERT_NOT_NULL(ingested);
    TEST_ASSERT_EQUAL_STRING(original, ingested);
    free(ingested);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_firebase_json_ingest_null);
    RUN_TEST(test_firebase_json_ingest_valid_passthrough);
    RUN_TEST(test_firebase_json_ingest_newline_in_string);
    RUN_TEST(test_firebase_json_ingest_tab_and_cr_in_string);
    RUN_TEST(test_firebase_json_ingest_schema_ref);
    RUN_TEST(test_firebase_json_ingest_garbage);
    RUN_TEST(test_firebase_json_ingest_already_escaped);
    return UNITY_END();
}
