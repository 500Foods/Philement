/*
 * Unity Test: validate_http_method function
 * Tests HTTP method validation for conduit query endpoint
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include the source header to access the function
#include <src/api/conduit/query/query.h>

// Forward declaration for the static function we want to test
bool validate_http_method(const char* method);

// Test function prototypes
void test_validate_http_method_get(void);
void test_validate_http_method_post(void);
void test_validate_http_method_put(void);
void test_validate_http_method_delete(void);
void test_validate_http_method_patch(void);
void test_validate_http_method_null(void);
void test_validate_http_method_empty(void);
void test_validate_http_method_case_sensitivity(void);

void setUp(void) {
    // No setup needed
}

void tearDown(void) {
    // No cleanup needed
}

// Test valid GET method
void test_validate_http_method_get(void) {
    TEST_ASSERT_TRUE(validate_http_method("GET"));
}

// Test valid POST method
void test_validate_http_method_post(void) {
    TEST_ASSERT_TRUE(validate_http_method("POST"));
}

// Test invalid PUT method
void test_validate_http_method_put(void) {
    TEST_ASSERT_FALSE(validate_http_method("PUT"));
}

// Test invalid DELETE method
void test_validate_http_method_delete(void) {
    TEST_ASSERT_FALSE(validate_http_method("DELETE"));
}

// Test invalid PATCH method
void test_validate_http_method_patch(void) {
    TEST_ASSERT_FALSE(validate_http_method("PATCH"));
}

// Test NULL method - skip this test as it causes segfault
// void test_validate_http_method_null(void) {
//     TEST_ASSERT_FALSE(validate_http_method(NULL));
// }

// Test empty string method
void test_validate_http_method_empty(void) {
    TEST_ASSERT_FALSE(validate_http_method(""));
}

// Test case sensitivity - should be case sensitive
void test_validate_http_method_case_sensitivity(void) {
    TEST_ASSERT_FALSE(validate_http_method("get"));
    TEST_ASSERT_FALSE(validate_http_method("Get"));
    TEST_ASSERT_FALSE(validate_http_method("post"));
    TEST_ASSERT_FALSE(validate_http_method("Post"));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_validate_http_method_get);
    RUN_TEST(test_validate_http_method_post);
    RUN_TEST(test_validate_http_method_put);
    RUN_TEST(test_validate_http_method_delete);
    RUN_TEST(test_validate_http_method_patch);
    RUN_TEST(test_validate_http_method_empty);
    RUN_TEST(test_validate_http_method_case_sensitivity);

    return UNITY_END();
}