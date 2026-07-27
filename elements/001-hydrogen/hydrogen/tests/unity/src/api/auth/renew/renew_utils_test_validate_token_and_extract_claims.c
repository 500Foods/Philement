/*
 * Unity Unit Tests for renew_utils.c - validate_token_and_extract_claims
 */

#include <src/hydrogen.h>
#include <unity.h>

#define USE_MOCK_AUTH_SERVICE_JWT
#include <unity/mocks/mock_auth_service_jwt.h>

#include <src/api/auth/renew/renew_utils.h>

void test_validate_token_and_extract_claims_invalid_token(void);
void test_validate_token_and_extract_claims_null_claims(void);
void test_validate_token_and_extract_claims_success(void);

void setUp(void) {
    mock_auth_service_jwt_reset_all();
}

void tearDown(void) {
    mock_auth_service_jwt_reset_all();
}

void test_validate_token_and_extract_claims_invalid_token(void) {
    jwt_validation_result_t mock = {0};
    mock.valid = false;
    mock.error = JWT_ERROR_EXPIRED;
    mock_auth_service_jwt_set_validation_result(mock);

    jwt_validation_result_t result = {0};
    bool success = validate_token_and_extract_claims("invalid.token", "testdb", &result);

    TEST_ASSERT_FALSE(success);
}

void test_validate_token_and_extract_claims_null_claims(void) {
    jwt_validation_result_t mock = {0};
    mock.valid = true;
    mock.claims = NULL;
    mock_auth_service_jwt_set_validation_result(mock);

    jwt_validation_result_t result = {0};
    bool success = validate_token_and_extract_claims("valid.token", "testdb", &result);

    TEST_ASSERT_FALSE(success);
}

void test_validate_token_and_extract_claims_success(void) {
    jwt_claims_t claims = {0};
    claims.user_id = 123;
    claims.database = (char*)"testdb";
    claims.username = (char*)"testuser";

    jwt_validation_result_t mock = {0};
    mock.valid = true;
    mock.error = JWT_ERROR_NONE;
    mock.claims = &claims;
    mock_auth_service_jwt_set_validation_result(mock);

    jwt_validation_result_t result = {0};
    bool success = validate_token_and_extract_claims("valid.token", "testdb", &result);

    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_TRUE(result.valid);
    TEST_ASSERT_NOT_NULL(result.claims);
    TEST_ASSERT_EQUAL(123, result.claims->user_id);
    TEST_ASSERT_EQUAL_STRING("testdb", result.claims->database);
    TEST_ASSERT_EQUAL_STRING("testuser", result.claims->username);

    mock_free_jwt_validation_result(&result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_validate_token_and_extract_claims_invalid_token);
    RUN_TEST(test_validate_token_and_extract_claims_null_claims);
    RUN_TEST(test_validate_token_and_extract_claims_success);

    return UNITY_END();
}
