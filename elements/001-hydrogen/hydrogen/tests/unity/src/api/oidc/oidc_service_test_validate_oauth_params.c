/*
 * Unity Test File: validate_oauth_params
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/oidc/oidc_service.h>

void test_validate_null_client_id(void);
void test_validate_null_redirect_uri(void);
void test_validate_null_response_type(void);
void test_validate_unsupported_response_type(void);
void test_validate_supported_response_types(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_validate_null_client_id(void) {
    char *error = NULL;
    char *error_description = NULL;
    TEST_ASSERT_FALSE(validate_oauth_params(NULL, "https://app.example/cb", "code", &error, &error_description));
    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_NOT_NULL(error_description);
    free(error);
    free(error_description);
}

void test_validate_null_redirect_uri(void) {
    char *error = NULL;
    char *error_description = NULL;
    TEST_ASSERT_FALSE(validate_oauth_params("client1", NULL, "code", &error, &error_description));
    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_NOT_NULL(error_description);
    free(error);
    free(error_description);
}

void test_validate_null_response_type(void) {
    char *error = NULL;
    char *error_description = NULL;
    TEST_ASSERT_FALSE(validate_oauth_params("client1", "https://app.example/cb", NULL, &error, &error_description));
    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_NOT_NULL(error_description);
    free(error);
    free(error_description);
}

void test_validate_unsupported_response_type(void) {
    char *error = NULL;
    char *error_description = NULL;
    TEST_ASSERT_FALSE(validate_oauth_params("client1", "https://app.example/cb", "invalid_type", &error, &error_description));
    TEST_ASSERT_EQUAL_STRING("unsupported_response_type", error);
    TEST_ASSERT_EQUAL_STRING("Unsupported response_type parameter", error_description);
    free(error);
    free(error_description);
}

void test_validate_supported_response_types(void) {
    char *error = NULL;
    char *error_description = NULL;

    const char *types[] = {
        "code",
        "token",
        "id_token",
        "code token",
        "code id_token",
        "token id_token",
        "code token id_token"
    };

    for (size_t i = 0; i < sizeof(types) / sizeof(types[0]); i++) {
        error = NULL;
        error_description = NULL;
        TEST_ASSERT_TRUE(validate_oauth_params("client1", "https://app.example/cb", types[i], &error, &error_description));
        TEST_ASSERT_NULL(error);
        TEST_ASSERT_NULL(error_description);
    }
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_validate_null_client_id);
    RUN_TEST(test_validate_null_redirect_uri);
    RUN_TEST(test_validate_null_response_type);
    RUN_TEST(test_validate_unsupported_response_type);
    RUN_TEST(test_validate_supported_response_types);
    return UNITY_END();
}
