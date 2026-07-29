/*
 * Unity Test File: handle_reporting_image_scale_request
 *
 * Exercises parameter validation, disabled subsystem, method checks, and a
 * successful PNG scale path using real MagickWand on a tiny PNG.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <jansson.h>
#include <stdlib.h>
#include <string.h>

/* USE_MOCK_LIBMICROHTTPD and USE_MOCK_SYSTEM are defined globally by CMake */
#include <unity/mocks/mock_libmicrohttpd.h>

#include <src/api/api_utils.h>
#include <src/config/config_defaults.h>
#include <src/config/config_reporting.h>
#include <src/reporting/reporting_service.h>
#include <src/reporting/image_scale/image_scale.h>

/* Minimal 1x1 PNG (red), base64 */
static const char *TINY_PNG_B64 =
    "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8z8BQDwAEhQGAhKmMIQAAAABJRU5ErkJggg==";

static AppConfig g_test_config;
static AppConfig *g_saved_app_config = NULL;

void test_image_scale_null_params(void);
void test_image_scale_disabled(void);
void test_image_scale_wrong_method(void);
void test_image_scale_missing_image(void);
void test_image_scale_missing_format(void);
void test_image_scale_missing_dims(void);
void test_image_scale_unsupported_format(void);
void test_image_scale_invalid_base64(void);
void test_image_scale_invalid_json(void);
void test_image_scale_dims_too_large(void);
void test_image_scale_input_too_large(void);
void test_image_scale_success_png(void);
void test_image_scale_success_xo(void);
void test_image_scale_send_error_helper(void);

static void setup_reporting_config(bool enabled) {
    memset(&g_test_config, 0, sizeof(g_test_config));
    initialize_config_defaults_reporting(&g_test_config);
    g_test_config.reporting.Enabled = enabled;
    g_test_config.reporting.MaxImageSize = 8192;
    g_test_config.reporting.MaxInputBytes = 1024 * 1024;
    g_test_config.reporting.MaxOutputBytes = 1024 * 1024;
    g_test_config.reporting.DefaultDPI = 72;
    if (g_test_config.reporting.AllowedFormats) {
        free(g_test_config.reporting.AllowedFormats);
    }
    g_test_config.reporting.AllowedFormats = strdup("jpg,jpeg,png,bmp,webp,xo");
    app_config = &g_test_config;
}

static enum MHD_Result call_handler(const char *method, const char *body) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1234;
    size_t body_len = body ? strlen(body) : 0;
    void *con_cls = NULL;

    size_t upload_size = body_len;
    enum MHD_Result r1 = handle_reporting_image_scale_request(
        conn, "/api/reporting/image_scale", method, body, &upload_size, &con_cls);
    if (r1 != MHD_YES) {
        api_free_post_buffer(&con_cls);
        return r1;
    }

    size_t final_size = 0;
    enum MHD_Result r2 = handle_reporting_image_scale_request(
        conn, "/api/reporting/image_scale", method, "", &final_size, &con_cls);
    api_free_post_buffer(&con_cls);
    return r2;
}

static char *build_body(const char *image_b64, const char *format, int w, int h) {
    json_t *obj = json_object();
    if (image_b64) {
        json_object_set_new(obj, "image", json_string(image_b64));
    }
    if (format) {
        json_object_set_new(obj, "format", json_string(format));
    }
    if (w > 0) {
        json_object_set_new(obj, "width", json_integer(w));
    }
    if (h > 0) {
        json_object_set_new(obj, "height", json_integer(h));
    }
    char *s = json_dumps(obj, JSON_COMPACT);
    json_decref(obj);
    return s;
}

void setUp(void) {
    mock_mhd_reset_all();
    mock_mhd_set_queue_response_result(MHD_YES);

    g_saved_app_config = app_config;
    setup_reporting_config(true);
    reporting_service_init();
}

void tearDown(void) {
    reporting_service_cleanup();
    if (g_test_config.reporting.AllowedFormats) {
        free(g_test_config.reporting.AllowedFormats);
        g_test_config.reporting.AllowedFormats = NULL;
    }
    cleanup_reporting_config(&g_test_config.reporting);
    memset(&g_test_config, 0, sizeof(g_test_config));
    app_config = g_saved_app_config;
    mock_mhd_reset_all();
}

void test_image_scale_null_params(void) {
    size_t sz = 0;
    void *cls = NULL;
    TEST_ASSERT_EQUAL(MHD_NO, handle_reporting_image_scale_request(
        NULL, "/x", "POST", "", &sz, &cls));
    TEST_ASSERT_EQUAL(MHD_NO, handle_reporting_image_scale_request(
        (struct MHD_Connection *)0x1, "/x", NULL, "", &sz, &cls));
    TEST_ASSERT_EQUAL(MHD_NO, handle_reporting_image_scale_request(
        (struct MHD_Connection *)0x1, "/x", "POST", "", NULL, &cls));
    TEST_ASSERT_EQUAL(MHD_NO, handle_reporting_image_scale_request(
        (struct MHD_Connection *)0x1, "/x", "POST", "", &sz, NULL));
}

void test_image_scale_disabled(void) {
    g_test_config.reporting.Enabled = false;
    enum MHD_Result r = call_handler("POST", "{\"image\":\"x\",\"format\":\"png\",\"width\":1,\"height\":1}");
    TEST_ASSERT_EQUAL(MHD_YES, r);
}

void test_image_scale_wrong_method(void) {
    size_t sz = 0;
    void *cls = NULL;
    enum MHD_Result r = handle_reporting_image_scale_request(
        (struct MHD_Connection *)0x1, "/api/reporting/image_scale", "GET",
        NULL, &sz, &cls);
    api_free_post_buffer(&cls);
    TEST_ASSERT_EQUAL(MHD_YES, r);
}

void test_image_scale_missing_image(void) {
    char *body = build_body(NULL, "png", 8, 8);
    TEST_ASSERT_NOT_NULL(body);
    TEST_ASSERT_EQUAL(MHD_YES, call_handler("POST", body));
    free(body);
}

void test_image_scale_missing_format(void) {
    char *body = build_body(TINY_PNG_B64, NULL, 8, 8);
    TEST_ASSERT_NOT_NULL(body);
    TEST_ASSERT_EQUAL(MHD_YES, call_handler("POST", body));
    free(body);
}

void test_image_scale_missing_dims(void) {
    char *body = build_body(TINY_PNG_B64, "png", 0, 0);
    TEST_ASSERT_NOT_NULL(body);
    /* width/height omitted entirely */
    free(body);
    body = strdup("{\"image\":\"abc\",\"format\":\"png\"}");
    TEST_ASSERT_EQUAL(MHD_YES, call_handler("POST", body));
    free(body);
}

void test_image_scale_unsupported_format(void) {
    char *body = build_body(TINY_PNG_B64, "gif", 8, 8);
    TEST_ASSERT_NOT_NULL(body);
    TEST_ASSERT_EQUAL(MHD_YES, call_handler("POST", body));
    free(body);
}

void test_image_scale_invalid_base64(void) {
    char *body = build_body("!!!!", "png", 8, 8);
    TEST_ASSERT_NOT_NULL(body);
    TEST_ASSERT_EQUAL(MHD_YES, call_handler("POST", body));
    free(body);
}

void test_image_scale_invalid_json(void) {
    TEST_ASSERT_EQUAL(MHD_YES, call_handler("POST", "{not json"));
}

void test_image_scale_dims_too_large(void) {
    g_test_config.reporting.MaxImageSize = 10;
    char *body = build_body(TINY_PNG_B64, "png", 64, 64);
    TEST_ASSERT_NOT_NULL(body);
    TEST_ASSERT_EQUAL(MHD_YES, call_handler("POST", body));
    free(body);
}

void test_image_scale_input_too_large(void) {
    g_test_config.reporting.MaxInputBytes = 4;
    char *body = build_body(TINY_PNG_B64, "png", 8, 8);
    TEST_ASSERT_NOT_NULL(body);
    TEST_ASSERT_EQUAL(MHD_YES, call_handler("POST", body));
    free(body);
}

void test_image_scale_success_png(void) {
    char *body = build_body(TINY_PNG_B64, "png", 4, 4);
    TEST_ASSERT_NOT_NULL(body);
    TEST_ASSERT_EQUAL(MHD_YES, call_handler("POST", body));
    free(body);
}

void test_image_scale_success_xo(void) {
    char *body = build_body(TINY_PNG_B64, "xo", 4, 4);
    TEST_ASSERT_NOT_NULL(body);
    TEST_ASSERT_EQUAL(MHD_YES, call_handler("POST", body));
    free(body);
}

void test_image_scale_send_error_helper(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x99;
    enum MHD_Result r = image_scale_send_error(conn, "err", "details", MHD_HTTP_BAD_REQUEST);
    TEST_ASSERT_EQUAL(MHD_YES, r);
    r = image_scale_send_error(conn, NULL, NULL, MHD_HTTP_INTERNAL_SERVER_ERROR);
    TEST_ASSERT_EQUAL(MHD_YES, r);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_image_scale_null_params);
    RUN_TEST(test_image_scale_disabled);
    RUN_TEST(test_image_scale_wrong_method);
    RUN_TEST(test_image_scale_missing_image);
    RUN_TEST(test_image_scale_missing_format);
    RUN_TEST(test_image_scale_missing_dims);
    RUN_TEST(test_image_scale_unsupported_format);
    RUN_TEST(test_image_scale_invalid_base64);
    RUN_TEST(test_image_scale_invalid_json);
    RUN_TEST(test_image_scale_dims_too_large);
    RUN_TEST(test_image_scale_input_too_large);
    RUN_TEST(test_image_scale_success_png);
    RUN_TEST(test_image_scale_success_xo);
    RUN_TEST(test_image_scale_send_error_helper);
    return UNITY_END();
}
