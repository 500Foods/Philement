/*
 * Core Image Scaling (MagickWand)
 */

#ifndef REPORTING_IMAGE_SCALE_CORE_H
#define REPORTING_IMAGE_SCALE_CORE_H

#include <stddef.h>
#include <stdbool.h>

typedef struct _MagickWand MagickWand;

// Convert width/height from units (px|pt) using dpi into pixel dimensions.
bool parse_dimensions(int width, int height, const char* units, int dpi,
                      int* out_width, int* out_height);

// Map algorithm name to MagickWand FilterType value.
// Returns filter enum as int; -1 if unknown (caller may default to Lanczos).
int get_scale_filter(const char* algorithm);

// Return filter name string for logging (never NULL for known algorithms).
const char* get_scale_filter_name(const char* algorithm);

/*
 * Core scale: read blob, scale to dimensions, set output format (unless xo).
 * On success returns MagickWand owned by caller (DestroyMagickWand).
 * On failure returns NULL and sets *error_msg to a heap string (caller frees)
 * when error_msg is non-NULL.
 *
 * Optional outputs (may be NULL):
 *   input_width / input_height — dimensions after read, before resize
 *   input_format_out — heap Magick format string (caller frees with free())
 */
MagickWand* scale_image_core(const unsigned char* data, size_t data_len,
                             int width, int height,
                             const char* units, int dpi,
                             const char* scale_algorithm,
                             const char* format,
                             char** error_msg,
                             size_t* input_width,
                             size_t* input_height,
                             char** input_format_out);

// Capture MagickWand exception into heap string (caller frees). May return NULL.
char* scale_image_core_exception(MagickWand* wand);

// Set *error_msg to a heap copy of msg (or NULL). Exposed for no-static policy.
void scale_image_core_set_error(char** error_msg, const char* msg);

// Keep only the first frame (GIF/animated WebP/multi-page). Documented limitation.
void scale_image_core_keep_first_frame(MagickWand* wand);

// True if output format is a web raster that should drop ICC profiles (jpg/png/webp/gif/ico).
bool scale_image_core_should_strip_profiles(const char* format);

// True if output format has no alpha support (JPEG and similar).
bool scale_image_core_format_lacks_alpha(const char* format);

// Flatten alpha onto white background when converting to opaque formats (e.g. JPEG).
// Returns false on Magick failure (wand may be left partially modified).
bool scale_image_core_flatten_for_opaque(MagickWand* wand, const char* format);

// Strip ICC/EXIF profiles from web output formats (size reduction). No-op for BMP/XO/SVG.
void scale_image_core_strip_profiles(MagickWand* wand, const char* format);

#endif /* REPORTING_IMAGE_SCALE_CORE_H */
