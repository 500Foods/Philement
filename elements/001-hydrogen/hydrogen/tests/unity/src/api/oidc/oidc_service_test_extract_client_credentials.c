/*
 * Unity Test File: extract_client_credentials
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/oidc/oidc_service.h>
#include <unity/mocks/mock_libmicrohttpd.h>

void test_extract_nulls(void);
void test_extract_no_header(void);
void test_extract_basic_happy(void);
void test_extract_not_basic(void);
void test_extract_basic_extra_spaces(void);
void test_extract_basic_empty_after_spaces(void);
void test_extract_malloc_failure(void);
void test_extract_decode_failure(void);
void test_extract_pair_malloc_failure(void);
void test_extract_no_colon(void);
void test_extract_strdup_failure(void);

static struct MHD_Connection *const FAKE = (struct MHD_Connection *)0xBEEF;

void setUp(void) {
    mock_mhd_reset_all();
}

void tearDown(void) {
    mock_mhd_reset_all();
}

void test_extract_nulls(void) {
    char *id = NULL;
    char *sec = NULL;
    TEST_ASSERT_FALSE(extract_client_credentials(NULL, &id, &sec));
    TEST_ASSERT_FALSE(extract_client_credentials(FAKE, NULL, &sec));
    TEST_ASSERT_FALSE(extract_client_credentials(FAKE, &id, NULL));
}

void test_extract_no_header(void) {
    char *id = NULL;
    char *sec = NULL;
    mock_mhd_set_lookup_result(NULL);
    TEST_ASSERT_FALSE(extract_client_credentials(FAKE, &id, &sec));
}

void test_extract_basic_happy(void) {
    char *id = NULL;
    char *sec = NULL;
    /* "cli:sec" standard base64 */
    mock_mhd_set_lookup_result("Basic Y2xpOnNlYw==");
    TEST_ASSERT_TRUE(extract_client_credentials(FAKE, &id, &sec));
    TEST_ASSERT_EQUAL_STRING("cli", id);
    TEST_ASSERT_EQUAL_STRING("sec", sec);
    free(id);
    free(sec);
}

void test_extract_not_basic(void) {
    char *id = NULL;
    char *sec = NULL;
    mock_mhd_set_lookup_result("Bearer abc");
    TEST_ASSERT_FALSE(extract_client_credentials(FAKE, &id, &sec));
}

void test_extract_basic_extra_spaces(void) {
    char *id = NULL;
    char *sec = NULL;
    /* "Basic " followed by extra spaces then valid base64 */
    mock_mhd_set_lookup_result("Basic  Y2xpOnNlYw==");
    TEST_ASSERT_TRUE(extract_client_credentials(FAKE, &id, &sec));
    TEST_ASSERT_EQUAL_STRING("cli", id);
    TEST_ASSERT_EQUAL_STRING("sec", sec);
    free(id);
    free(sec);
}

void test_extract_basic_empty_after_spaces(void) {
    char *id = NULL;
    char *sec = NULL;
    /* "Basic " followed by only spaces */
    mock_mhd_set_lookup_result("Basic    ");
    TEST_ASSERT_FALSE(extract_client_credentials(FAKE, &id, &sec));
}

void test_extract_malloc_failure(void) {
    char *id = NULL;
    char *sec = NULL;
    mock_mhd_set_lookup_result("Basic Y2xpOnNlYw==");
    mock_system_set_malloc_failure(1);
    TEST_ASSERT_FALSE(extract_client_credentials(FAKE, &id, &sec));
    mock_system_reset_all();
}

void test_extract_decode_failure(void) {
    char *id = NULL;
    char *sec = NULL;
    /* Invalid base64 that causes EVP_DecodeBlock to return -1 */
    mock_mhd_set_lookup_result("Basic !!!invalid!!!");
    TEST_ASSERT_FALSE(extract_client_credentials(FAKE, &id, &sec));
}

void test_extract_pair_malloc_failure(void) {
    char *id = NULL;
    char *sec = NULL;
    mock_mhd_set_lookup_result("Basic Y2xpOnNlYw==");
    /* First malloc (decoded) succeeds, second malloc (pair) fails */
    mock_system_set_malloc_failure(2);
    TEST_ASSERT_FALSE(extract_client_credentials(FAKE, &id, &sec));
    mock_system_reset_all();
}

void test_extract_no_colon(void) {
    char *id = NULL;
    char *sec = NULL;
    /* "nocolon" in base64 is "bm9jb2xvbg==" */
    mock_mhd_set_lookup_result("Basic bm9jb2xvbg==");
    TEST_ASSERT_FALSE(extract_client_credentials(FAKE, &id, &sec));
}

void test_extract_strdup_failure(void) {
    char *id = NULL;
    char *sec = NULL;
    mock_mhd_set_lookup_result("Basic Y2xpOnNlYw==");
    /* First two mallocs succeed, first strdup fails */
    mock_system_set_malloc_failure(3);
    TEST_ASSERT_FALSE(extract_client_credentials(FAKE, &id, &sec));
    mock_system_reset_all();
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_extract_nulls);
    RUN_TEST(test_extract_no_header);
    RUN_TEST(test_extract_basic_happy);
    RUN_TEST(test_extract_not_basic);
    RUN_TEST(test_extract_basic_extra_spaces);
    RUN_TEST(test_extract_basic_empty_after_spaces);
    RUN_TEST(test_extract_malloc_failure);
    RUN_TEST(test_extract_decode_failure);
    RUN_TEST(test_extract_pair_malloc_failure);
    RUN_TEST(test_extract_no_colon);
    RUN_TEST(test_extract_strdup_failure);
    return UNITY_END();
}
