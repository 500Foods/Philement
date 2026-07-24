/*
 * Unity Test File: extract_client_credentials
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/oidc/oidc_service.h>
#include <unity/mocks/mock_libmicrohttpd.h>

#include <string.h>

void test_extract_nulls(void);
void test_extract_no_header(void);
void test_extract_basic_happy(void);
void test_extract_not_basic(void);

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

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_extract_nulls);
    RUN_TEST(test_extract_no_header);
    RUN_TEST(test_extract_basic_happy);
    RUN_TEST(test_extract_not_basic);
    return UNITY_END();
}
