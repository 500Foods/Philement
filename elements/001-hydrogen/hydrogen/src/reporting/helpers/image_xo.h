/*
 * XO Format Encoder — PDF XObject intermediate stream
 *
 * Encodes MagickWand pixel + optional alpha data into the XO binary
 * layout defined in docs/H/plans/IMAGE_PLAN.md, with zlib FlateDecode
 * compression on each data stream.
 */

#ifndef REPORTING_IMAGE_XO_H
#define REPORTING_IMAGE_XO_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// MagickWand is an opaque typedef in MagickWand.h; forward-declare to
// avoid forcing every includer to pull Magick headers.
typedef struct _MagickWand MagickWand;

/*
 * Encode wand image to XO binary (not base64).
 * Returns heap buffer (caller frees) or NULL on error.
 * *out_len receives binary length.
 */
unsigned char* encode_xo_binary(MagickWand* wand, size_t* out_len);

/*
 * Encode wand image to base64-encoded XO stream.
 * Returns heap string (caller frees) or NULL on error.
 * *out_len receives base64 string length (excluding NUL) if non-NULL.
 */
char* encode_xo_stream(MagickWand* wand, size_t* out_len);

// True if wand has a usable alpha channel.
bool xo_needs_alpha(MagickWand* wand);

// zlib FlateDecode compress (caller frees *out). Exposed for Unity tests.
bool xo_flate_compress(const unsigned char* src, size_t src_len,
                       unsigned char** out, size_t* out_len);

// Write big-endian uint32 (exposed for Unity tests).
void xo_write_be32(unsigned char* p, uint32_t value);

// Map Magick colorspace enum to XO color_space byte and channel count.
// cs is ColorspaceType from MagickCore; passed as int to avoid header coupling.
bool xo_colorspace_info_int(int cs, uint8_t* color_space, size_t* channels);

// Magick-native overload used by encode_xo_binary (declared after Magick include in .c).
// Prototype here uses void* to avoid Magick header dependency in this header.

/*
 * Build XO binary from raw pixel/alpha buffers (for unit tests without Magick).
 * pixel_data length must be width*height*channels.
 * alpha_data may be NULL if has_alpha is false; otherwise width*height bytes.
 * color_space: 0=Gray, 1=RGB, 2=CMYK.
 * Returns heap buffer (caller frees).
 */
unsigned char* encode_xo_from_buffers(uint32_t width, uint32_t height,
                                      uint8_t color_space, uint8_t has_alpha,
                                      const unsigned char* pixel_data, size_t pixel_len,
                                      const unsigned char* alpha_data, size_t alpha_len,
                                      size_t* out_len);

#endif /* REPORTING_IMAGE_XO_H */
