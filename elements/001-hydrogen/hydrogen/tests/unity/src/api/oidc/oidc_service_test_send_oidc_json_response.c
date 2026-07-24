/*
 * Unity Test File: send_oidc_json_response / send_oauth_error
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/oidc/oidc_service.h>
#include <unity/mocks/mock_libmicrohttpd.h>

void test_send_json_ok(void);
void test_send_oauth_error_json(void);
void test_send_oauth_error_redirect(void);

static struct MHD_Connection *const FAKE = (struct MHD_Connection *)0xCAFE;

void setUp(void) {
    mock_mhd_reset_all();
    mock_mhd_set_queue_response_result(MHD_YES);
}

void tearDown(void) {
    mock_mhd_reset_all();
}

void test_send_json_ok(void) {
    TEST_ASSERT_EQUAL_INT(MHD_YES,
                          send_oidc_json_response(FAKE, "{\"ok\":true}", MHD_HTTP_OK));
}

void test_send_oauth_error_json(void) {
    TEST_ASSERT_EQUAL_INT(MHD_YES,
                          send_oauth_error(FAKE, "invalid_request", "bad", NULL, NULL));
}

void test_send_oauth_error_redirect(void) {
    TEST_ASSERT_EQUAL_INT(MHD_YES,
                          send_oauth_error(FAKE, "access_denied", "nope",
                                           "https://app.example/cb", "st1"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_send_json_ok);
    RUN_TEST(test_send_oauth_error_json);
    RUN_TEST(test_send_oauth_error_redirect);
    return UNITY_END();
}
