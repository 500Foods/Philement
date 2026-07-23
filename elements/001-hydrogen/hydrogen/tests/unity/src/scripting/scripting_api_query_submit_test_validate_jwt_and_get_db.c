/*
 * Unity Test File: scripting_api_query_submit_test_validate_jwt_and_get_db
 *
 * Tests validate_jwt_and_get_db: empty/null token, invalid JWT, missing DB claim, success.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <string.h>
#include <stdlib.h>

#define USE_MOCK_AUTH_SERVICE_JWT
#include <unity/mocks/mock_auth_service_jwt.h>

#include <src/scripting/scripting_api_internal.h>
#include <src/api/auth/auth_service.h>

void test_validate_jwt_and_get_db_empty_token_returns_error(void);
void test_validate_jwt_and_get_db_null_token_returns_error(void);
void test_validate_jwt_and_get_db_invalid_jwt(void);
void test_validate_jwt_and_get_db_no_database_claim(void);
void test_validate_jwt_and_get_db_empty_database_claim(void);
void test_validate_jwt_and_get_db_success(void);

void setUp(void) {
    mock_auth_service_jwt_reset_all();
}

void tearDown(void) {
    mock_auth_service_jwt_reset_all();
}

void test_validate_jwt_and_get_db_empty_token_returns_error(void) {
    char* err_out = NULL;
    char* result = validate_jwt_and_get_db("", &err_out);
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_NOT_NULL(err_out);
    TEST_ASSERT_NOT_NULL(strstr(err_out, "empty token"));
    free(err_out);
}

void test_validate_jwt_and_get_db_null_token_returns_error(void) {
    char* err_out = NULL;
    char* result = validate_jwt_and_get_db(NULL, &err_out);
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_NOT_NULL(err_out);
    free(err_out);
}

void test_validate_jwt_and_get_db_invalid_jwt(void) {
    jwt_validation_result_t v = {0};
    v.valid = false;
    v.error = 7;
    mock_auth_service_jwt_set_validation_result(v);

    char* err_out = NULL;
    char* result = validate_jwt_and_get_db("tok", &err_out);
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_NOT_NULL(err_out);
    TEST_ASSERT_NOT_NULL(strstr(err_out, "JWT validation failed"));
    free(err_out);
}

void test_validate_jwt_and_get_db_no_database_claim(void) {
    jwt_claims_t claims = {0};
    claims.database = NULL;
    jwt_validation_result_t v = {0};
    v.valid = true;
    v.claims = &claims;
    mock_auth_service_jwt_set_validation_result(v);

    char* err_out = NULL;
    char* result = validate_jwt_and_get_db("tok", &err_out);
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_NOT_NULL(err_out);
    TEST_ASSERT_NOT_NULL(strstr(err_out, "no database claim"));
    free(err_out);
}

void test_validate_jwt_and_get_db_empty_database_claim(void) {
    jwt_claims_t claims = {0};
    claims.database = (char*)"";
    jwt_validation_result_t v = {0};
    v.valid = true;
    v.claims = &claims;
    mock_auth_service_jwt_set_validation_result(v);

    char* err_out = NULL;
    char* result = validate_jwt_and_get_db("tok", &err_out);
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_NOT_NULL(err_out);
    TEST_ASSERT_NOT_NULL(strstr(err_out, "no database claim"));
    free(err_out);
}

void test_validate_jwt_and_get_db_success(void) {
    jwt_claims_t claims = {0};
    claims.database = (char*)"prod-db";
    jwt_validation_result_t v = {0};
    v.valid = true;
    v.claims = &claims;
    mock_auth_service_jwt_set_validation_result(v);

    char* err_out = NULL;
    char* result = validate_jwt_and_get_db("tok", &err_out);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("prod-db", result);
    TEST_ASSERT_NULL(err_out);
    free(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_validate_jwt_and_get_db_empty_token_returns_error);
    RUN_TEST(test_validate_jwt_and_get_db_null_token_returns_error);
    RUN_TEST(test_validate_jwt_and_get_db_invalid_jwt);
    RUN_TEST(test_validate_jwt_and_get_db_no_database_claim);
    RUN_TEST(test_validate_jwt_and_get_db_empty_database_claim);
    RUN_TEST(test_validate_jwt_and_get_db_success);

    return UNITY_END();
}
