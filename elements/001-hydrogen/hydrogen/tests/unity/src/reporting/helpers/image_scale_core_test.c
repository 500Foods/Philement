/*
 * Unity Test File: Reporting image_scale_core
 */

#include <src/hydrogen.h>
#include <unity.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
#include <MagickWand/MagickWand.h>
#pragma GCC diagnostic pop
#include <src/reporting/reporting_service.h>
#include <src/reporting/helpers/image_scale_core.h>
#include <src/reporting/helpers/base64_utils.h>

// Minimal 1x1 PNG (red pixel), base64
// Generated: convert -size 1x1 xc:red png:- | base64 -w0
static const char *TINY_PNG_B64 =
    "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8z8BQDwAEhQGAhKmMIQAAAABJRU5ErkJggg==";

void test_parse_dimensions_px(void);
void test_parse_dimensions_pt(void);
void test_parse_dimensions_invalid(void);
void test_get_scale_filter_all(void);
void test_get_scale_filter_name(void);
void test_scale_image_core_png(void);
void test_scale_image_core_bad_blob(void);
void test_scale_image_core_strip_and_opaque_helpers(void);
void test_scale_image_core_png_to_jpeg_flatten(void);
void test_scale_image_core_keep_first_frame(void);

void setUp(void) {
    reporting_service_init();
}

void tearDown(void) {
    reporting_service_cleanup();
}

void test_parse_dimensions_px(void) {
    int w = 0, h = 0;
    TEST_ASSERT_TRUE(parse_dimensions(200, 100, "px", 72, &w, &h));
    TEST_ASSERT_EQUAL_INT(200, w);
    TEST_ASSERT_EQUAL_INT(100, h);
    TEST_ASSERT_TRUE(parse_dimensions(10, 10, NULL, 72, &w, &h));
    TEST_ASSERT_EQUAL_INT(10, w);
}

void test_parse_dimensions_pt(void) {
    int w = 0, h = 0;
    // 2pt x 2pt at 300 DPI -> 2*300/72 = 8.333 -> 8 with rounding ((2*300+36)/72)=636/72=8
    TEST_ASSERT_TRUE(parse_dimensions(2, 2, "pt", 300, &w, &h));
    TEST_ASSERT_EQUAL_INT(8, w);
    TEST_ASSERT_EQUAL_INT(8, h);

    // 72pt at 72 DPI -> 72 px
    TEST_ASSERT_TRUE(parse_dimensions(72, 36, "pt", 72, &w, &h));
    TEST_ASSERT_EQUAL_INT(72, w);
    TEST_ASSERT_EQUAL_INT(36, h);
}

void test_parse_dimensions_invalid(void) {
    int w = 0, h = 0;
    TEST_ASSERT_FALSE(parse_dimensions(0, 10, "px", 72, &w, &h));
    TEST_ASSERT_FALSE(parse_dimensions(10, 10, "inches", 72, &w, &h));
    TEST_ASSERT_FALSE(parse_dimensions(10, 10, "px", 72, NULL, &h));
}

void test_get_scale_filter_all(void) {
    TEST_ASSERT_TRUE(get_scale_filter("nearest") >= 0);
    TEST_ASSERT_TRUE(get_scale_filter("bilinear") >= 0);
    TEST_ASSERT_TRUE(get_scale_filter("bicubic") >= 0);
    TEST_ASSERT_TRUE(get_scale_filter("lanczos") >= 0);
    TEST_ASSERT_TRUE(get_scale_filter("mitchell") >= 0);
    TEST_ASSERT_TRUE(get_scale_filter(NULL) >= 0);
    TEST_ASSERT_EQUAL_INT(-1, get_scale_filter("nope"));
}

void test_get_scale_filter_name(void) {
    TEST_ASSERT_EQUAL_STRING("nearest", get_scale_filter_name("nearest"));
    TEST_ASSERT_EQUAL_STRING("lanczos", get_scale_filter_name("lanczos"));
    TEST_ASSERT_EQUAL_STRING("unknown", get_scale_filter_name("nope"));
}

void test_scale_image_core_png(void) {
    size_t blob_len = 0;
    unsigned char *blob = reporting_base64_decode(TINY_PNG_B64, &blob_len);
    TEST_ASSERT_NOT_NULL(blob);
    TEST_ASSERT_TRUE(blob_len > 0);

    char *err = NULL;
    size_t in_w = 0, in_h = 0;
    char *in_fmt = NULL;
    MagickWand *wand = scale_image_core(blob, blob_len, 8, 8, "px", 72, "lanczos", "png",
                                        &err, &in_w, &in_h, &in_fmt);
    free(blob);

    TEST_ASSERT_NULL(err);
    TEST_ASSERT_NOT_NULL(wand);
    TEST_ASSERT_EQUAL_size_t(1, in_w);
    TEST_ASSERT_EQUAL_size_t(1, in_h);
    TEST_ASSERT_EQUAL_size_t(8, MagickGetImageWidth(wand));
    TEST_ASSERT_EQUAL_size_t(8, MagickGetImageHeight(wand));

    free(in_fmt);
    free(err);
    DestroyMagickWand(wand);
}

void test_scale_image_core_bad_blob(void) {
    const unsigned char junk[] = "not an image";
    char *err = NULL;
    MagickWand *wand = scale_image_core(junk, sizeof(junk) - 1, 10, 10, "px", 72,
                                        "lanczos", "png", &err, NULL, NULL, NULL);
    TEST_ASSERT_NULL(wand);
    TEST_ASSERT_NOT_NULL(err);
    free(err);
}

void test_scale_image_core_strip_and_opaque_helpers(void) {
    TEST_ASSERT_TRUE(scale_image_core_should_strip_profiles("png"));
    TEST_ASSERT_TRUE(scale_image_core_should_strip_profiles("jpg"));
    TEST_ASSERT_TRUE(scale_image_core_should_strip_profiles("webp"));
    TEST_ASSERT_FALSE(scale_image_core_should_strip_profiles("bmp"));
    TEST_ASSERT_FALSE(scale_image_core_should_strip_profiles("svg"));
    TEST_ASSERT_FALSE(scale_image_core_should_strip_profiles("xo"));
    TEST_ASSERT_FALSE(scale_image_core_should_strip_profiles(NULL));

    TEST_ASSERT_TRUE(scale_image_core_format_lacks_alpha("jpg"));
    TEST_ASSERT_TRUE(scale_image_core_format_lacks_alpha("jpeg"));
    TEST_ASSERT_TRUE(scale_image_core_format_lacks_alpha("bmp"));
    TEST_ASSERT_FALSE(scale_image_core_format_lacks_alpha("png"));
    TEST_ASSERT_FALSE(scale_image_core_format_lacks_alpha("webp"));
}

void test_scale_image_core_png_to_jpeg_flatten(void) {
    size_t blob_len = 0;
    unsigned char *blob = reporting_base64_decode(TINY_PNG_B64, &blob_len);
    TEST_ASSERT_NOT_NULL(blob);

    char *err = NULL;
    MagickWand *wand = scale_image_core(blob, blob_len, 4, 4, "px", 72, "nearest", "jpg",
                                        &err, NULL, NULL, NULL);
    free(blob);
    TEST_ASSERT_NULL(err);
    TEST_ASSERT_NOT_NULL(wand);
    TEST_ASSERT_EQUAL_size_t(4, MagickGetImageWidth(wand));
    TEST_ASSERT_EQUAL(MagickFalse, MagickGetImageAlphaChannel(wand));

    free(err);
    DestroyMagickWand(wand);
}

void test_scale_image_core_keep_first_frame(void) {
    MagickWand *wand = NewMagickWand();
    TEST_ASSERT_NOT_NULL(wand);

    PixelWand *pw = NewPixelWand();
    TEST_ASSERT_NOT_NULL(pw);
    PixelSetColor(pw, "red");
    MagickNewImage(wand, 2, 2, pw);
    PixelSetColor(pw, "blue");
    MagickNewImage(wand, 2, 2, pw);
    DestroyPixelWand(pw);

    TEST_ASSERT_EQUAL_size_t(2, MagickGetNumberImages(wand));
    scale_image_core_keep_first_frame(wand);
    TEST_ASSERT_EQUAL_size_t(1, MagickGetNumberImages(wand));

    DestroyMagickWand(wand);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_parse_dimensions_px);
    RUN_TEST(test_parse_dimensions_pt);
    RUN_TEST(test_parse_dimensions_invalid);
    RUN_TEST(test_get_scale_filter_all);
    RUN_TEST(test_get_scale_filter_name);
    RUN_TEST(test_scale_image_core_png);
    RUN_TEST(test_scale_image_core_bad_blob);
    RUN_TEST(test_scale_image_core_strip_and_opaque_helpers);
    RUN_TEST(test_scale_image_core_png_to_jpeg_flatten);
    RUN_TEST(test_scale_image_core_keep_first_frame);
    return UNITY_END();
}
