/*
 * Unity Test File: extract_token_request_params
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/oidc/oidc_service.h>
#include <src/api/oidc/token/token.h>
#include <unity/mocks/mock_libmicrohttpd.h>

void test_extract_null_output_params(void);
void test_extract_authorization_code_grant(void);
void test_extract_refresh_token_grant(void);
void test_extract_client_credentials_grant(void);
void test_extract_invalid_grant_type(void);
void test_extract_missing_grant_type(void);
void test_extract_no_body_uses_basic_auth(void);

static struct MHD_Connection *const FAKE = (struct MHD_Connection *)0xBEEF;

void setUp(void) {
    mock_mhd_reset_all();
}

void tearDown(void) {
    mock_mhd_reset_all();
}

void test_extract_null_output_params(void) {
    char *grant_type = NULL;
    TEST_ASSERT_FALSE(extract_token_request_params(FAKE, NULL, 0,
                                                   NULL, &grant_type, NULL, NULL, NULL, NULL, NULL));
}

void test_extract_authorization_code_grant(void) {
    char *grant_type = NULL;
    char *code = NULL;
    char *redirect_uri = NULL;
    char *client_id = NULL;
    char *client_secret = NULL;
    char *refresh_token = NULL;
    char *code_verifier = NULL;

    const char *body = "grant_type=authorization_code&code=abc123&redirect_uri=https%3A%2F%2Fapp.example%2Fcb&client_id=client1&client_secret=secret1&code_verifier=verifier1";

    TEST_ASSERT_TRUE(extract_token_request_params(FAKE, body, strlen(body),
                                                   &grant_type, &code, &redirect_uri,
                                                   &client_id, &client_secret,
                                                   &refresh_token, &code_verifier));
    TEST_ASSERT_EQUAL_STRING("authorization_code", grant_type);
    TEST_ASSERT_EQUAL_STRING("abc123", code);
    TEST_ASSERT_EQUAL_STRING("https://app.example/cb", redirect_uri);
    TEST_ASSERT_EQUAL_STRING("client1", client_id);
    TEST_ASSERT_EQUAL_STRING("secret1", client_secret);
    TEST_ASSERT_EQUAL_STRING("verifier1", code_verifier);

    free(grant_type);
    free(code);
    free(redirect_uri);
    free(client_id);
    free(client_secret);
    free(refresh_token);
    free(code_verifier);
}

void test_extract_refresh_token_grant(void) {
    char *grant_type = NULL;
    char *code = NULL;
    char *redirect_uri = NULL;
    char *client_id = NULL;
    char *client_secret = NULL;
    char *refresh_token = NULL;
    char *code_verifier = NULL;

    const char *body = "grant_type=refresh_token&refresh_token=ref123&client_id=client1&client_secret=secret1";

    TEST_ASSERT_TRUE(extract_token_request_params(FAKE, body, strlen(body),
                                                   &grant_type, &code, &redirect_uri,
                                                   &client_id, &client_secret,
                                                   &refresh_token, &code_verifier));
    TEST_ASSERT_EQUAL_STRING("refresh_token", grant_type);
    TEST_ASSERT_EQUAL_STRING("ref123", refresh_token);

    free(grant_type);
    free(code);
    free(redirect_uri);
    free(client_id);
    free(client_secret);
    free(refresh_token);
    free(code_verifier);
}

void test_extract_client_credentials_grant(void) {
    char *grant_type = NULL;
    char *code = NULL;
    char *redirect_uri = NULL;
    char *client_id = NULL;
    char *client_secret = NULL;
    char *refresh_token = NULL;
    char *code_verifier = NULL;

    const char *body = "grant_type=client_credentials&client_id=client1&client_secret=secret1";

    TEST_ASSERT_TRUE(extract_token_request_params(FAKE, body, strlen(body),
                                                   &grant_type, &code, &redirect_uri,
                                                   &client_id, &client_secret,
                                                   &refresh_token, &code_verifier));
    TEST_ASSERT_EQUAL_STRING("client_credentials", grant_type);

    free(grant_type);
    free(code);
    free(redirect_uri);
    free(client_id);
    free(client_secret);
    free(refresh_token);
    free(code_verifier);
}

void test_extract_invalid_grant_type(void) {
    char *grant_type = NULL;
    char *code = NULL;
    char *redirect_uri = NULL;
    char *client_id = NULL;
    char *client_secret = NULL;
    char *refresh_token = NULL;
    char *code_verifier = NULL;

    const char *body = "grant_type=invalid_grant&client_id=client1&client_secret=secret1";

    TEST_ASSERT_FALSE(extract_token_request_params(FAKE, body, strlen(body),
                                                    &grant_type, &code, &redirect_uri,
                                                    &client_id, &client_secret,
                                                    &refresh_token, &code_verifier));

    free(grant_type);
    free(code);
    free(redirect_uri);
    free(client_id);
    free(client_secret);
    free(refresh_token);
    free(code_verifier);
}

void test_extract_missing_grant_type(void) {
    char *grant_type = NULL;
    char *code = NULL;
    char *redirect_uri = NULL;
    char *client_id = NULL;
    char *client_secret = NULL;
    char *refresh_token = NULL;
    char *code_verifier = NULL;

    const char *body = "client_id=client1&client_secret=secret1";

    TEST_ASSERT_FALSE(extract_token_request_params(FAKE, body, strlen(body),
                                                    &grant_type, &code, &redirect_uri,
                                                    &client_id, &client_secret,
                                                    &refresh_token, &code_verifier));

    free(grant_type);
    free(code);
    free(redirect_uri);
    free(client_id);
    free(client_secret);
    free(refresh_token);
    free(code_verifier);
}

void test_extract_no_body_uses_basic_auth(void) {
    char *grant_type = NULL;
    char *code = NULL;
    char *redirect_uri = NULL;
    char *client_id = NULL;
    char *client_secret = NULL;
    char *refresh_token = NULL;
    char *code_verifier = NULL;

    mock_mhd_set_lookup_result("Basic Y2xpOnNlYw==");

    const char *body = "grant_type=client_credentials";

    TEST_ASSERT_TRUE(extract_token_request_params(FAKE, body, strlen(body),
                                                   &grant_type, &code, &redirect_uri,
                                                   &client_id, &client_secret,
                                                   &refresh_token, &code_verifier));
    TEST_ASSERT_EQUAL_STRING("cli", client_id);
    TEST_ASSERT_EQUAL_STRING("sec", client_secret);

    free(grant_type);
    free(code);
    free(redirect_uri);
    free(client_id);
    free(client_secret);
    free(refresh_token);
    free(code_verifier);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_extract_null_output_params);
    RUN_TEST(test_extract_authorization_code_grant);
    RUN_TEST(test_extract_refresh_token_grant);
    RUN_TEST(test_extract_client_credentials_grant);
    RUN_TEST(test_extract_invalid_grant_type);
    RUN_TEST(test_extract_missing_grant_type);
    RUN_TEST(test_extract_no_body_uses_basic_auth);
    return UNITY_END();
}
