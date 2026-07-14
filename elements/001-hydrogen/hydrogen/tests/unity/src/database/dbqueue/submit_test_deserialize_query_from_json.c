/*
 * Unity Test File: submit_test_deserialize_query_from_json
 *
 * Exercises deserialize_query_from_json() (submit.c) error and success
 * paths. The error branches (invalid JSON parse failure at line 86-87 and
 * missing required 'query_template' field at line 110-114) were uncovered
 * by both Unity and blackbox suites. A round-trip test is included to
 * confirm the happy path still behaves correctly.
 */

#include <src/hydrogen.h>
#include <unity.h>

// Standard includes
#include <stdlib.h>
#include <string.h>

// Project includes
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/database.h>

// Forward declarations (required for -Wmissing-prototypes)
void test_deserialize_query_from_json_null_input(void);
void test_deserialize_query_from_json_invalid_json(void);
void test_deserialize_query_from_json_missing_template(void);
void test_deserialize_query_from_json_valid_roundtrip(void);

void setUp(void) {
    // No setup required
}

void tearDown(void) {
    // No teardown required
}

void test_deserialize_query_from_json_null_input(void) {
    DatabaseQuery* q = deserialize_query_from_json(NULL);
    TEST_ASSERT_NULL(q);
}

// Covers the json_loads() failure path (lines 86-87).
void test_deserialize_query_from_json_invalid_json(void) {
    DatabaseQuery* q = deserialize_query_from_json("{not valid json");
    TEST_ASSERT_NULL(q);
}

// Covers the missing required 'query_template' field path (lines 110-114),
// including cleanup of a previously allocated query_id.
void test_deserialize_query_from_json_missing_template(void) {
    const char* json = "{\"query_id\":\"q1\"}";
    DatabaseQuery* q = deserialize_query_from_json(json);
    TEST_ASSERT_NULL(q);
}

void test_deserialize_query_from_json_valid_roundtrip(void) {
    DatabaseQuery in = {0};
    in.query_id = strdup("rid");
    in.query_template = strdup("SELECT * FROM t");
    in.parameter_json = strdup("{\"a\":1}");
    in.queue_type_hint = 1;
    in.submitted_at = 123;
    in.submitted_at_ns = 456;

    char* serialized = serialize_query_to_json(&in);
    TEST_ASSERT_NOT_NULL(serialized);

    DatabaseQuery* out = deserialize_query_from_json(serialized);
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_EQUAL_STRING("rid", out->query_id);
    TEST_ASSERT_EQUAL_STRING("SELECT * FROM t", out->query_template);
    TEST_ASSERT_EQUAL_STRING("{\"a\":1}", out->parameter_json);
    TEST_ASSERT_EQUAL(1, out->queue_type_hint);
    TEST_ASSERT_EQUAL(123, (int)out->submitted_at);
    TEST_ASSERT_EQUAL(456, (int)out->submitted_at_ns);

    free(out->query_id);
    free(out->query_template);
    free(out->parameter_json);
    free(out);

    free(serialized);
    free(in.query_id);
    free(in.query_template);
    free(in.parameter_json);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_deserialize_query_from_json_null_input);
    RUN_TEST(test_deserialize_query_from_json_invalid_json);
    RUN_TEST(test_deserialize_query_from_json_missing_template);
    RUN_TEST(test_deserialize_query_from_json_valid_roundtrip);

    return UNITY_END();
}
