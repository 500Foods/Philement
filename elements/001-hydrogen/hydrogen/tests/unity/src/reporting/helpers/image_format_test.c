/*
 * Unity Test File: Reporting image_format
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/reporting/helpers/image_format.h>

void test_format_to_imagemagick_common(void);
void test_format_to_mime(void);
void test_format_is_xo(void);
void test_format_is_known(void);
void test_format_is_supported_all(void);
void test_format_is_supported_list(void);
void test_format_to_imagemagick_null(void);
void test_format_to_imagemagick_fallback(void);
void test_format_table_has(void);
void test_format_all_known_mimes(void);

void setUp(void) {}
void tearDown(void) {}

void test_format_to_imagemagick_common(void) {
    char buf[32];
    TEST_ASSERT_TRUE(format_to_imagemagick("jpg", buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("JPEG", buf);
    TEST_ASSERT_TRUE(format_to_imagemagick("png", buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("PNG", buf);
    TEST_ASSERT_TRUE(format_to_imagemagick("webp", buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("WEBP", buf);
    TEST_ASSERT_TRUE(format_to_imagemagick("xo", buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("XO", buf);
}

void test_format_to_mime(void) {
    TEST_ASSERT_EQUAL_STRING("image/jpeg", format_to_mime("jpg"));
    TEST_ASSERT_EQUAL_STRING("image/png", format_to_mime("PNG"));
    TEST_ASSERT_EQUAL_STRING("application/x-xo", format_to_mime("xo"));
    TEST_ASSERT_EQUAL_STRING("application/octet-stream", format_to_mime("notarealformatxyz"));
}

void test_format_is_xo(void) {
    TEST_ASSERT_TRUE(format_is_xo("xo"));
    TEST_ASSERT_TRUE(format_is_xo("XO"));
    TEST_ASSERT_FALSE(format_is_xo("png"));
    TEST_ASSERT_FALSE(format_is_xo(NULL));
}

void test_format_is_known(void) {
    TEST_ASSERT_TRUE(format_is_known("bmp"));
    TEST_ASSERT_TRUE(format_is_known("svg"));
    TEST_ASSERT_FALSE(format_is_known("notarealformatxyz"));
    TEST_ASSERT_FALSE(format_is_known(NULL));
}

void test_format_is_supported_all(void) {
    TEST_ASSERT_TRUE(format_is_supported("png", NULL));
    TEST_ASSERT_TRUE(format_is_supported("anything", ""));
    TEST_ASSERT_FALSE(format_is_supported(NULL, NULL));
}

void test_format_is_supported_list(void) {
    TEST_ASSERT_TRUE(format_is_supported("png", "jpg,png,webp"));
    TEST_ASSERT_TRUE(format_is_supported("JPG", "jpg, png"));
    TEST_ASSERT_FALSE(format_is_supported("gif", "jpg,png"));
    TEST_ASSERT_TRUE(format_is_supported("xo", "jpg,png,bmp,svg,ico,webp,xo"));
}

void test_format_to_imagemagick_null(void) {
    char buf[8];
    TEST_ASSERT_FALSE(format_to_imagemagick(NULL, buf, sizeof(buf)));
    TEST_ASSERT_FALSE(format_to_imagemagick("png", NULL, 8));
    TEST_ASSERT_FALSE(format_to_imagemagick("png", buf, 0));
}

void test_format_to_imagemagick_fallback(void) {
    char buf[32];
    TEST_ASSERT_TRUE(format_to_imagemagick("customfmt", buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("CUSTOMFMT", buf);
}

void test_format_table_has(void) {
    TEST_ASSERT_TRUE(format_table_has("png"));
    TEST_ASSERT_TRUE(format_table_has("ICO"));
    TEST_ASSERT_FALSE(format_table_has("nope"));
    TEST_ASSERT_FALSE(format_table_has(NULL));
}

void test_format_all_known_mimes(void) {
    TEST_ASSERT_EQUAL_STRING("image/bmp", format_to_mime("bmp"));
    TEST_ASSERT_EQUAL_STRING("image/svg+xml", format_to_mime("svg"));
    TEST_ASSERT_EQUAL_STRING("image/x-icon", format_to_mime("ico"));
    TEST_ASSERT_EQUAL_STRING("image/webp", format_to_mime("webp"));
    TEST_ASSERT_EQUAL_STRING("image/gif", format_to_mime("gif"));
    TEST_ASSERT_EQUAL_STRING("image/tiff", format_to_mime("tiff"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_format_to_imagemagick_common);
    RUN_TEST(test_format_to_mime);
    RUN_TEST(test_format_is_xo);
    RUN_TEST(test_format_is_known);
    RUN_TEST(test_format_is_supported_all);
    RUN_TEST(test_format_is_supported_list);
    RUN_TEST(test_format_to_imagemagick_null);
    RUN_TEST(test_format_to_imagemagick_fallback);
    RUN_TEST(test_format_table_has);
    RUN_TEST(test_format_all_known_mimes);
    return UNITY_END();
}
