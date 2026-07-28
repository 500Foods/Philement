/*
 * Unity Test File: handle_oidc_registration_endpoint
 * Stub endpoint always returns not_implemented JSON.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/oidc/registration/registration.h>
#include <unity/mocks/mock_libmicrohttpd.h>

void test_registration_returns_not_implemented(void);
void test_registration_ignores_method_and_body(void);

static struct MHD_Connection *const FAKE = (struct MHD_Connection *)0xB0B1;

void setUp(void) {
    mock_mhd_reset_all();
    mock_mhd_set_queue_response_result(MHD_YES);
}

void tearDown(void) {
    mock_mhd_reset_all();
}

void test_registration_returns_not_implemented(void) {
    size_t upload = 0;
    void *con_cls = NULL;
    enum MHD_Result ret = handle_oidc_registration_endpoint(
        FAKE, "POST", NULL, &upload, &con_cls);
    TEST_ASSERT_EQUAL_INT(MHD_YES, ret);
}

void test_registration_ignores_method_and_body(void) {
    size_t upload = 2;
    void *con_cls = (void *)0x2;
    enum MHD_Result ret = handle_oidc_registration_endpoint(
        FAKE, "GET", "{}", &upload, &con_cls);
    TEST_ASSERT_EQUAL_INT(MHD_YES, ret);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_registration_returns_not_implemented);
    RUN_TEST(test_registration_ignores_method_and_body);
    return UNITY_END();
}
