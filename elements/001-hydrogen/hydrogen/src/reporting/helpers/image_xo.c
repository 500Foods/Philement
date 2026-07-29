/*
 * XO Format Encoder Implementation
 */

#include <src/hydrogen.h>
#include <src/utils/utils_crypto.h>

#include <zlib.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
#include <MagickWand/MagickWand.h>
#pragma GCC diagnostic pop

#include "image_xo.h"

// XO header is 12 bytes (see IMAGE_PLAN.md)
#define XO_HEADER_SIZE 12

bool xo_colorspace_info(ColorspaceType cs, uint8_t* color_space, size_t* channels);

bool xo_needs_alpha(MagickWand* wand) {
    if (!wand) {
        return false;
    }
    return MagickGetImageAlphaChannel(wand) != MagickFalse;
}

// Compress src with zlib compress2 (default compression). Caller frees *out.
bool xo_flate_compress(const unsigned char* src, size_t src_len,
                       unsigned char** out, size_t* out_len) {
    if (!src || !out || !out_len || src_len == 0) {
        return false;
    }

    uLongf bound = compressBound((uLong)src_len);
    unsigned char* buf = malloc(bound);
    if (!buf) {
        return false;
    }

    uLongf dest_len = bound;
    int rc = compress2(buf, &dest_len, src, (uLong)src_len, Z_DEFAULT_COMPRESSION);
    if (rc != Z_OK) {
        free(buf);
        return false;
    }

    *out = buf;
    *out_len = (size_t)dest_len;
    return true;
}

void xo_write_be32(unsigned char* p, uint32_t value) {
    p[0] = (unsigned char)((value >> 24) & 0xFF);
    p[1] = (unsigned char)((value >> 16) & 0xFF);
    p[2] = (unsigned char)((value >> 8) & 0xFF);
    p[3] = (unsigned char)(value & 0xFF);
}

unsigned char* encode_xo_from_buffers(uint32_t width, uint32_t height,
                                      uint8_t color_space, uint8_t has_alpha,
                                      const unsigned char* pixel_data, size_t pixel_len,
                                      const unsigned char* alpha_data, size_t alpha_len,
                                      size_t* out_len) {
    if (!pixel_data || !out_len || width == 0 || height == 0 || pixel_len == 0) {
        return NULL;
    }
    if (has_alpha && (!alpha_data || alpha_len == 0)) {
        return NULL;
    }
    if (color_space > 2) {
        return NULL;
    }

    unsigned char* pix_comp = NULL;
    size_t pix_comp_len = 0;
    if (!xo_flate_compress(pixel_data, pixel_len, &pix_comp, &pix_comp_len)) {
        return NULL;
    }

    unsigned char* alpha_comp = NULL;
    size_t alpha_comp_len = 0;
    if (has_alpha) {
        if (!xo_flate_compress(alpha_data, alpha_len, &alpha_comp, &alpha_comp_len)) {
            free(pix_comp);
            return NULL;
        }
    }

    size_t total = XO_HEADER_SIZE + pix_comp_len + alpha_comp_len;
    unsigned char* out = malloc(total);
    if (!out) {
        free(pix_comp);
        free(alpha_comp);
        return NULL;
    }

    xo_write_be32(out, width);
    xo_write_be32(out + 4, height);
    out[8] = color_space;
    out[9] = has_alpha ? 1 : 0;
    out[10] = 0;
    out[11] = 0;
    memcpy(out + XO_HEADER_SIZE, pix_comp, pix_comp_len);
    if (has_alpha) {
        memcpy(out + XO_HEADER_SIZE + pix_comp_len, alpha_comp, alpha_comp_len);
    }

    free(pix_comp);
    free(alpha_comp);
    *out_len = total;
    return out;
}

// Map Magick colorspace to XO color_space byte and channel count.
bool xo_colorspace_info(ColorspaceType cs, uint8_t* color_space, size_t* channels) {
    return xo_colorspace_info_int((int)cs, color_space, channels);
}

bool xo_colorspace_info_int(int cs, uint8_t* color_space, size_t* channels) {
    if (!color_space || !channels) {
        return false;
    }

    if (cs == (int)GRAYColorspace || cs == (int)LinearGRAYColorspace) {
        *color_space = 0;
        *channels = 1;
        return true;
    }
    if (cs == (int)CMYKColorspace) {
        *color_space = 2;
        *channels = 4;
        return true;
    }

    // RGB family and everything else → DeviceRGB for PDF XObject
    *color_space = 1;
    *channels = 3;
    return true;
}

unsigned char* encode_xo_binary(MagickWand* wand, size_t* out_len) {
    if (!wand || !out_len) {
        return NULL;
    }

    size_t width = MagickGetImageWidth(wand);
    size_t height = MagickGetImageHeight(wand);
    if (width == 0 || height == 0) {
        return NULL;
    }

    ColorspaceType cs = MagickGetImageColorspace(wand);
    uint8_t color_space = 1;
    size_t channels = 3;
    xo_colorspace_info(cs, &color_space, &channels);

    // Normalize to a PDF-friendly colorspace for export
    if (color_space == 0) {
        MagickTransformImageColorspace(wand, GRAYColorspace);
    } else if (color_space == 2) {
        MagickTransformImageColorspace(wand, CMYKColorspace);
    } else {
        MagickTransformImageColorspace(wand, sRGBColorspace);
        color_space = 1;
        channels = 3;
    }

    bool has_alpha = xo_needs_alpha(wand);
    const char* map = "RGB";
    if (color_space == 0) {
        map = "I";
    } else if (color_space == 2) {
        map = "CMYK";
    }

    size_t pixel_len = width * height * channels;
    unsigned char* pixels = malloc(pixel_len);
    if (!pixels) {
        return NULL;
    }

    MagickBooleanType ok = MagickExportImagePixels(wand, 0, 0, width, height, map, CharPixel, pixels);
    if (ok == MagickFalse) {
        free(pixels);
        return NULL;
    }

    unsigned char* alpha = NULL;
    size_t alpha_len = 0;
    if (has_alpha) {
        alpha_len = width * height;
        alpha = malloc(alpha_len);
        if (!alpha) {
            free(pixels);
            return NULL;
        }
        ok = MagickExportImagePixels(wand, 0, 0, width, height, "A", CharPixel, alpha);
        if (ok == MagickFalse) {
            free(pixels);
            free(alpha);
            return NULL;
        }
    }

    unsigned char* result = encode_xo_from_buffers((uint32_t)width, (uint32_t)height,
                                                   color_space, has_alpha ? 1 : 0,
                                                   pixels, pixel_len,
                                                   alpha, alpha_len,
                                                   out_len);
    free(pixels);
    free(alpha);
    return result;
}

char* encode_xo_stream(MagickWand* wand, size_t* out_len) {
    size_t bin_len = 0;
    unsigned char* binary = encode_xo_binary(wand, &bin_len);
    if (!binary) {
        return NULL;
    }

    char* b64 = utils_base64_encode(binary, bin_len);
    free(binary);
    if (!b64) {
        return NULL;
    }
    if (out_len) {
        *out_len = strlen(b64);
    }
    return b64;
}
