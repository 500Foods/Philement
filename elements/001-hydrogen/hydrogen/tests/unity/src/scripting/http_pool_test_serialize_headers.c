/*
 * Unity Test File: http_pool_test_serialize_headers.c
 *
 * Tests serialize_headers() directly. The happy-path pool/submit tests
 * only ever exercise it with a fixture response that carries no headers
 * (so it returns NULL at the headers_count == 0 guard). This file drives
 * the JSON-array building branch (json_array / json_pack /
 * json_array_append_new / json_dumps) with real header data, including
 * NULL name/value slots that must serialize as empty strings.
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Third-party includes
#include <jansson.h>

// Module under test
#include <src/scripting/http_pool.h>
#include <src/api/auth/oidc_rp/oidc_rp_http.h>

// Forward declarations
void test_serialize_headers_null_response(void);
void test_serialize_headers_empty(void);
void test_serialize_headers_single(void);
void test_serialize_headers_multiple_with_null_slots(void);

void setUp(void) {
}

void tearDown(void) {
}

// NULL response: serialize_headers must return NULL.
void test_serialize_headers_null_response(void) {
    char* out = serialize_headers(NULL);
    TEST_ASSERT_NULL(out);
}

// Non-NULL response but zero headers: serialize_headers returns NULL.
void test_serialize_headers_empty(void) {
    OidcRpHttpResponse resp = {0};
    resp.headers = NULL;
    resp.headers_count = 0;
    char* out = serialize_headers(&resp);
    TEST_ASSERT_NULL(out);
}

// A single header with both name and value set.
void test_serialize_headers_single(void) {
    OidcRpHttpHeader hdr;
    hdr.name = (char*)"Content-Type";
    hdr.value = (char*)"application/json";

    OidcRpHttpResponse resp = {0};
    resp.headers = &hdr;
    resp.headers_count = 1;

    char* out = serialize_headers(&resp);
    TEST_ASSERT_NOT_NULL(out);

    json_error_t err;
    json_t* arr = json_loads(out, 0, &err);
    TEST_ASSERT_NOT_NULL_MESSAGE(arr, err.text);
    TEST_ASSERT_TRUE(json_is_array(arr));
    TEST_ASSERT_EQUAL(1, json_array_size(arr));

    json_t* obj = json_array_get(arr, 0);
    TEST_ASSERT_EQUAL_STRING("Content-Type",
        json_string_value(json_object_get(obj, "name")));
    TEST_ASSERT_EQUAL_STRING("application/json",
        json_string_value(json_object_get(obj, "value")));

    json_decref(arr);
    free(out);
}

// Multiple headers, including a NULL name and a NULL value, to exercise
// the "name ? name : \"\"" and "value ? value : \"\"" fallbacks.
void test_serialize_headers_multiple_with_null_slots(void) {
    OidcRpHttpHeader hdrs[3];
    hdrs[0].name = (char*)"X-One";
    hdrs[0].value = (char*)"1";
    hdrs[1].name = NULL;     // "name ? name : \"\"" -> ""
    hdrs[1].value = (char*)"v2";
    hdrs[2].name = (char*)"X-Three";
    hdrs[2].value = NULL;    // "value ? value : \"\"" -> ""

    OidcRpHttpResponse resp = {0};
    resp.headers = hdrs;
    resp.headers_count = 3;

    char* out = serialize_headers(&resp);
    TEST_ASSERT_NOT_NULL(out);

    json_error_t err;
    json_t* arr = json_loads(out, 0, &err);
    TEST_ASSERT_NOT_NULL_MESSAGE(arr, err.text);
    TEST_ASSERT_EQUAL(3, json_array_size(arr));

    json_t* o0 = json_array_get(arr, 0);
    TEST_ASSERT_EQUAL_STRING("X-One",
        json_string_value(json_object_get(o0, "name")));
    TEST_ASSERT_EQUAL_STRING("1",
        json_string_value(json_object_get(o0, "value")));

    json_t* o1 = json_array_get(arr, 1);
    TEST_ASSERT_EQUAL_STRING("",
        json_string_value(json_object_get(o1, "name")));
    TEST_ASSERT_EQUAL_STRING("v2",
        json_string_value(json_object_get(o1, "value")));

    json_t* o2 = json_array_get(arr, 2);
    TEST_ASSERT_EQUAL_STRING("X-Three",
        json_string_value(json_object_get(o2, "name")));
    TEST_ASSERT_EQUAL_STRING("",
        json_string_value(json_object_get(o2, "value")));

    json_decref(arr);
    free(out);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_serialize_headers_null_response);
    RUN_TEST(test_serialize_headers_empty);
    RUN_TEST(test_serialize_headers_single);
    RUN_TEST(test_serialize_headers_multiple_with_null_slots);
    return UNITY_END();
}
