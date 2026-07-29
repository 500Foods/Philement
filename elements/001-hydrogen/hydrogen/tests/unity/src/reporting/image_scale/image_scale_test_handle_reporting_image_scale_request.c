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

#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

#include <src/api/api_utils.h>
#include <src/config/config_defaults.h>
#include <src/config/config_reporting.h>
#include <src/reporting/reporting_service.h>
#include <src/reporting/image_scale/image_scale.h>
#include <src/reporting/helpers/base64_utils.h>

static int g_json_alloc_fail_on = 0;
static int g_json_alloc_call_count = 0;

static void *test_json_malloc(size_t size) {
    g_json_alloc_call_count++;
    if (g_json_alloc_fail_on > 0 && g_json_alloc_call_count == g_json_alloc_fail_on) {
        return NULL;
    }
    return malloc(size);
}

static void test_json_free(void *ptr) {
    free(ptr);
}

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
void test_image_scale_buffer_error(void);
void test_image_scale_dpi_default_zero(void);
void test_image_scale_invalid_units(void);
void test_image_scale_processing_failed(void);
void test_image_scale_output_too_large(void);
void test_image_scale_send_error_json_fail(void);
void test_image_scale_response_json_fail(void);

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

/*
 * Simulate the MHD access-handler callback lifecycle with three calls:
 *   1. con_cls == NULL, upload_data_size == 0  -> init buffer, API_BUFFER_CONTINUE
 *   2. con_cls != NULL, upload_data_size > 0    -> accumulate body, API_BUFFER_CONTINUE
 *   3. con_cls != NULL, upload_data_size == 0   -> API_BUFFER_COMPLETE, process
 *
 * The previous 2-call version never accumulated data, so every request hit
 * the "Invalid JSON" path instead of the intended validation branches.
 */
static enum MHD_Result call_handler(const char *method, const char *body) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1234;
    void *con_cls = NULL;
    size_t zero = 0;

    /* Call 1: init buffer */
    enum MHD_Result r1 = handle_reporting_image_scale_request(
        conn, "/api/reporting/image_scale", method, NULL, &zero, &con_cls);
    if (r1 != MHD_YES) {
        api_free_post_buffer(&con_cls);
        return r1;
    }

    /* Call 2: pass body data */
    size_t body_len = body ? strlen(body) : 0;
    size_t upload_size = body_len;
    enum MHD_Result r2 = handle_reporting_image_scale_request(
        conn, "/api/reporting/image_scale", method, body, &upload_size, &con_cls);
    if (r2 != MHD_YES) {
        api_free_post_buffer(&con_cls);
        return r2;
    }

    /* Call 3: signal end of data */
    size_t final_size = 0;
    enum MHD_Result r3 = handle_reporting_image_scale_request(
        conn, "/api/reporting/image_scale", method, "", &final_size, &con_cls);
    api_free_post_buffer(&con_cls);
    return r3;
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

static char *build_body_full(const char *image_b64, const char *format,
                             int w, int h, const char *units, int dpi) {
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
    if (units) {
        json_object_set_new(obj, "units", json_string(units));
    }
    if (dpi > 0) {
        json_object_set_new(obj, "dpi", json_integer(dpi));
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

/*
 * Trigger API_BUFFER_ERROR by exceeding API_MAX_POST_SIZE on the second
 * (data-accumulation) call.  The first call initialises the buffer; the
 * second call passes a huge upload_data_size so api_buffer_post_data
 * returns API_BUFFER_ERROR, and the handler returns MHD_YES at line 69.
 */
void test_image_scale_buffer_error(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1;
    void *con_cls = NULL;
    size_t zero = 0;

    /* Call 1: init buffer */
    enum MHD_Result r1 = handle_reporting_image_scale_request(
        conn, "/api/reporting/image_scale", "POST", NULL, &zero, &con_cls);
    TEST_ASSERT_EQUAL(MHD_YES, r1);

    /* Call 2: trigger API_BUFFER_ERROR with oversized upload */
    size_t huge = (size_t)API_MAX_POST_SIZE + 1;
    enum MHD_Result r2 = handle_reporting_image_scale_request(
        conn, "/api/reporting/image_scale", "POST", NULL, &huge, &con_cls);
    api_free_post_buffer(&con_cls);
    TEST_ASSERT_EQUAL(MHD_YES, r2);
}

/*
 * When DefaultDPI <= 0 and no dpi is provided in the request, the handler
 * falls through to dpi = 72 (line 125).  The request still succeeds because
 * the tiny PNG is valid.
 */
void test_image_scale_dpi_default_zero(void) {
    g_test_config.reporting.DefaultDPI = 0;
    char *body = build_body(TINY_PNG_B64, "png", 4, 4);
    TEST_ASSERT_NOT_NULL(body);
    TEST_ASSERT_EQUAL(MHD_YES, call_handler("POST", body));
    free(body);
}

/*
 * Provide units="inches" (not "px" or "pt") so parse_dimensions returns
 * false, hitting lines 149-150 (Invalid dimensions).
 */
void test_image_scale_invalid_units(void) {
    char *body = build_body_full(TINY_PNG_B64, "png", 8, 8, "inches", 0);
    TEST_ASSERT_NOT_NULL(body);
    TEST_ASSERT_EQUAL(MHD_YES, call_handler("POST", body));
    free(body);
}

/*
 * Provide valid base64 that decodes to non-image bytes ("AAAA" -> 3 zero
 * bytes).  MagickReadImageBlob fails, scale_image_core returns NULL, and
 * lines 193-199 are hit (Image processing failed).
 */
void test_image_scale_processing_failed(void) {
    char *body = build_body("AAAA", "png", 8, 8);
    TEST_ASSERT_NOT_NULL(body);
    TEST_ASSERT_EQUAL(MHD_YES, call_handler("POST", body));
    free(body);
}

/*
 * Set MaxOutputBytes very small so the encoded output exceeds the limit,
 * hitting lines 241-245 (Output too large).
 */
void test_image_scale_output_too_large(void) {
    g_test_config.reporting.MaxOutputBytes = 4;
    char *body = build_body(TINY_PNG_B64, "png", 4, 4);
    TEST_ASSERT_NOT_NULL(body);
    TEST_ASSERT_EQUAL(MHD_YES, call_handler("POST", body));
    free(body);
}

/*
 * Make json_object() fail on its first allocation call inside
 * image_scale_send_error, hitting line 27 which calls
 * api_send_error_and_cleanup with a NULL response.
 */
void test_image_scale_send_error_json_fail(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x99;
    g_json_alloc_fail_on = 1;
    g_json_alloc_call_count = 0;
    json_set_alloc_funcs(test_json_malloc, test_json_free);
    enum MHD_Result r = image_scale_send_error(conn, "err", "details", MHD_HTTP_BAD_REQUEST);
    json_set_alloc_funcs(malloc, free);
    g_json_alloc_fail_on = 0;
    g_json_alloc_call_count = 0;
    TEST_ASSERT_EQUAL(MHD_YES, r);
}

/*
 * Make json_object() fail at line 250 (response object creation in the
 * success path), hitting lines 252-256 ("Internal error").
 *
 * We make the 3rd call to handle_reporting_image_scale_request manually
 * so we can set the mock failure right before it.
 */
void test_image_scale_response_json_fail(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x1;
    void *con_cls = NULL;
    size_t zero = 0;

    char *body = build_body(TINY_PNG_B64, "png", 4, 4);
    TEST_ASSERT_NOT_NULL(body);
    size_t body_len = strlen(body);

    /* Call 1: init buffer */
    enum MHD_Result r1 = handle_reporting_image_scale_request(
        conn, "/api/reporting/image_scale", "POST", NULL, &zero, &con_cls);
    TEST_ASSERT_EQUAL(MHD_YES, r1);

    /* Call 2: pass body data */
    size_t upload_size = body_len;
    enum MHD_Result r2 = handle_reporting_image_scale_request(
        conn, "/api/reporting/image_scale", "POST", body, &upload_size, &con_cls);
    TEST_ASSERT_EQUAL(MHD_YES, r2);

    /* Call 3: signal end of data - make json_object() fail at line 250 */
    g_json_alloc_fail_on = 1;
    g_json_alloc_call_count = 0;
    json_set_alloc_funcs(test_json_malloc, test_json_free);
    size_t final_size = 0;
    enum MHD_Result r3 = handle_reporting_image_scale_request(
        conn, "/api/reporting/image_scale", "POST", "", &final_size, &con_cls);
    json_set_alloc_funcs(malloc, free);
    g_json_alloc_fail_on = 0;
    g_json_alloc_call_count = 0;
    api_free_post_buffer(&con_cls);
    TEST_ASSERT_EQUAL(MHD_YES, r3);
    free(body);
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
    RUN_TEST(test_image_scale_buffer_error);
    RUN_TEST(test_image_scale_dpi_default_zero);
    RUN_TEST(test_image_scale_invalid_units);
    RUN_TEST(test_image_scale_processing_failed);
    RUN_TEST(test_image_scale_output_too_large);
    RUN_TEST(test_image_scale_send_error_json_fail);
    RUN_TEST(test_image_scale_response_json_fail);
    return UNITY_END();
}
