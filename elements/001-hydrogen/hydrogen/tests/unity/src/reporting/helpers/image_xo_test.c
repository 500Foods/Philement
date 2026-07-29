/*
 * Unity Test File: Reporting image_xo
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <zlib.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
#include <MagickWand/MagickWand.h>
#pragma GCC diagnostic pop
#include <src/reporting/helpers/image_xo.h>
#include <src/reporting/helpers/base64_utils.h>
#include <src/reporting/reporting_service.h>

void test_xo_write_be32(void);
void test_xo_flate_compress_roundtrip(void);
void test_encode_xo_from_buffers_rgb(void);
void test_encode_xo_from_buffers_with_alpha(void);
void test_encode_xo_from_buffers_invalid(void);
void test_xo_colorspace_info_int(void);
void test_xo_encode_stream_from_wand(void);
void test_xo_needs_alpha_null(void);

void setUp(void) {
    reporting_service_init();
}
void tearDown(void) {
    reporting_service_cleanup();
}

void test_xo_write_be32(void) {
    unsigned char buf[4] = {0};
    xo_write_be32(buf, 0x01020304);
    TEST_ASSERT_EQUAL_UINT8(0x01, buf[0]);
    TEST_ASSERT_EQUAL_UINT8(0x02, buf[1]);
    TEST_ASSERT_EQUAL_UINT8(0x03, buf[2]);
    TEST_ASSERT_EQUAL_UINT8(0x04, buf[3]);
}

void test_xo_flate_compress_roundtrip(void) {
    const unsigned char src[] = "hello xo flate";
    unsigned char *comp = NULL;
    size_t comp_len = 0;
    TEST_ASSERT_TRUE(xo_flate_compress(src, sizeof(src) - 1, &comp, &comp_len));
    TEST_ASSERT_NOT_NULL(comp);
    TEST_ASSERT_TRUE(comp_len > 0);

    unsigned char dest[64];
    uLongf dest_len = sizeof(dest);
    int rc = uncompress(dest, &dest_len, comp, (uLong)comp_len);
    TEST_ASSERT_EQUAL_INT(Z_OK, rc);
    TEST_ASSERT_EQUAL_size_t(sizeof(src) - 1, (size_t)dest_len);
    TEST_ASSERT_EQUAL_MEMORY(src, dest, sizeof(src) - 1);
    free(comp);
}

void test_encode_xo_from_buffers_rgb(void) {
    // 2x2 RGB solid red
    unsigned char pixels[2 * 2 * 3];
    for (int i = 0; i < 4; i++) {
        pixels[i * 3 + 0] = 255;
        pixels[i * 3 + 1] = 0;
        pixels[i * 3 + 2] = 0;
    }

    size_t out_len = 0;
    unsigned char *xo = encode_xo_from_buffers(2, 2, 1, 0, pixels, sizeof(pixels), NULL, 0, &out_len);
    TEST_ASSERT_NOT_NULL(xo);
    TEST_ASSERT_TRUE(out_len >= 12);

    // Header
    TEST_ASSERT_EQUAL_UINT8(0, xo[0]);
    TEST_ASSERT_EQUAL_UINT8(0, xo[1]);
    TEST_ASSERT_EQUAL_UINT8(0, xo[2]);
    TEST_ASSERT_EQUAL_UINT8(2, xo[3]); // width
    TEST_ASSERT_EQUAL_UINT8(0, xo[4]);
    TEST_ASSERT_EQUAL_UINT8(0, xo[5]);
    TEST_ASSERT_EQUAL_UINT8(0, xo[6]);
    TEST_ASSERT_EQUAL_UINT8(2, xo[7]); // height
    TEST_ASSERT_EQUAL_UINT8(1, xo[8]); // RGB
    TEST_ASSERT_EQUAL_UINT8(0, xo[9]); // no alpha
    TEST_ASSERT_EQUAL_UINT8(0, xo[10]);
    TEST_ASSERT_EQUAL_UINT8(0, xo[11]);

    free(xo);
}

void test_encode_xo_from_buffers_with_alpha(void) {
    unsigned char pixels[1 * 1 * 3] = {10, 20, 30};
    unsigned char alpha[1] = {200};
    size_t out_len = 0;
    unsigned char *xo = encode_xo_from_buffers(1, 1, 1, 1, pixels, sizeof(pixels),
                                               alpha, sizeof(alpha), &out_len);
    TEST_ASSERT_NOT_NULL(xo);
    TEST_ASSERT_EQUAL_UINT8(1, xo[9]);
    TEST_ASSERT_TRUE(out_len > 12);
    free(xo);
}

void test_encode_xo_from_buffers_invalid(void) {
    size_t out_len = 0;
    TEST_ASSERT_NULL(encode_xo_from_buffers(0, 1, 1, 0, (const unsigned char*)"x", 1, NULL, 0, &out_len));
    TEST_ASSERT_NULL(encode_xo_from_buffers(1, 1, 1, 1, (const unsigned char*)"x", 1, NULL, 0, &out_len));
    TEST_ASSERT_NULL(encode_xo_from_buffers(1, 1, 9, 0, (const unsigned char*)"x", 1, NULL, 0, &out_len));
}

void test_xo_colorspace_info_int(void) {
    uint8_t cs = 99;
    size_t ch = 0;
    TEST_ASSERT_FALSE(xo_colorspace_info_int(0, NULL, &ch));
    TEST_ASSERT_FALSE(xo_colorspace_info_int(0, &cs, NULL));
    TEST_ASSERT_TRUE(xo_colorspace_info_int((int)GRAYColorspace, &cs, &ch));
    TEST_ASSERT_EQUAL_UINT8(0, cs);
    TEST_ASSERT_EQUAL_size_t(1, ch);
    TEST_ASSERT_TRUE(xo_colorspace_info_int((int)CMYKColorspace, &cs, &ch));
    TEST_ASSERT_EQUAL_UINT8(2, cs);
    TEST_ASSERT_EQUAL_size_t(4, ch);
    TEST_ASSERT_TRUE(xo_colorspace_info_int((int)sRGBColorspace, &cs, &ch));
    TEST_ASSERT_EQUAL_UINT8(1, cs);
    TEST_ASSERT_EQUAL_size_t(3, ch);
}

void test_xo_encode_stream_from_wand(void) {
    MagickWand *wand = NewMagickWand();
    TEST_ASSERT_NOT_NULL(wand);
    PixelWand *pw = NewPixelWand();
    TEST_ASSERT_NOT_NULL(pw);
    PixelSetColor(pw, "red");
    MagickNewImage(wand, 2, 2, pw);
    DestroyPixelWand(pw);

    size_t b64_len = 0;
    char *b64 = encode_xo_stream(wand, &b64_len);
    TEST_ASSERT_NOT_NULL(b64);
    TEST_ASSERT_TRUE(b64_len > 0);

    size_t bin_len = 0;
    unsigned char *bin = reporting_base64_decode(b64, &bin_len);
    TEST_ASSERT_NOT_NULL(bin);
    TEST_ASSERT_TRUE(bin_len >= 12);
    TEST_ASSERT_EQUAL_UINT8(0, bin[0]);
    TEST_ASSERT_EQUAL_UINT8(2, bin[3]); /* width 2 */
    TEST_ASSERT_EQUAL_UINT8(2, bin[7]); /* height 2 */

    free(bin);
    free(b64);

    unsigned char *raw = encode_xo_binary(wand, &bin_len);
    TEST_ASSERT_NOT_NULL(raw);
    TEST_ASSERT_TRUE(bin_len >= 12);
    free(raw);

    TEST_ASSERT_FALSE(xo_needs_alpha(wand));
    DestroyMagickWand(wand);
}

void test_xo_needs_alpha_null(void) {
    TEST_ASSERT_FALSE(xo_needs_alpha(NULL));
    TEST_ASSERT_NULL(encode_xo_binary(NULL, NULL));
    size_t n = 0;
    TEST_ASSERT_NULL(encode_xo_stream(NULL, &n));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_xo_write_be32);
    RUN_TEST(test_xo_flate_compress_roundtrip);
    RUN_TEST(test_encode_xo_from_buffers_rgb);
    RUN_TEST(test_encode_xo_from_buffers_with_alpha);
    RUN_TEST(test_encode_xo_from_buffers_invalid);
    RUN_TEST(test_xo_colorspace_info_int);
    RUN_TEST(test_xo_encode_stream_from_wand);
    RUN_TEST(test_xo_needs_alpha_null);
    return UNITY_END();
}
