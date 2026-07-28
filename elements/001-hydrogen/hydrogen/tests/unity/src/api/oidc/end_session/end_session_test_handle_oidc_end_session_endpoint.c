/*
 * Unity Test File: handle_oidc_end_session_endpoint
 * Stub endpoint always returns not_implemented JSON.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/oidc/end_session/end_session.h>
#include <unity/mocks/mock_libmicrohttpd.h>

void test_end_session_returns_not_implemented(void);
void test_end_session_ignores_method_and_body(void);

static struct MHD_Connection *const FAKE = (struct MHD_Connection *)0xE5E5;

void setUp(void) {
    mock_mhd_reset_all();
    mock_mhd_set_queue_response_result(MHD_YES);
}

void tearDown(void) {
    mock_mhd_reset_all();
}

void test_end_session_returns_not_implemented(void) {
    size_t upload = 0;
    void *con_cls = NULL;
    enum MHD_Result ret = handle_oidc_end_session_endpoint(
        FAKE, "GET", NULL, &upload, &con_cls);
    TEST_ASSERT_EQUAL_INT(MHD_YES, ret);
}

void test_end_session_ignores_method_and_body(void) {
    size_t upload = 4;
    void *con_cls = (void *)0x1;
    enum MHD_Result ret = handle_oidc_end_session_endpoint(
        FAKE, "POST", "body", &upload, &con_cls);
    TEST_ASSERT_EQUAL_INT(MHD_YES, ret);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_end_session_returns_not_implemented);
    RUN_TEST(test_end_session_ignores_method_and_body);
    return UNITY_END();
}
