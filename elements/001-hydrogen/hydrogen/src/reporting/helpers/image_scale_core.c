/*
 * Core Image Scaling Implementation
 */

#include <src/hydrogen.h>
#include <limits.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
#include <MagickWand/MagickWand.h>
#pragma GCC diagnostic pop

#include "image_format.h"
#include "image_scale_core.h"
#include <src/reporting/reporting_service.h>

char* scale_image_core_exception(MagickWand* wand) {
    if (!wand) {
        return NULL;
    }
    ExceptionType severity;
    char* description = MagickGetException(wand, &severity);
    if (!description) {
        return NULL;
    }
    char* copy = strdup(description);
    MagickRelinquishMemory(description);
    return copy;
}

bool parse_dimensions(int width, int height, const char* units, int dpi,
                      int* out_width, int* out_height) {
    if (!out_width || !out_height) {
        return false;
    }
    if (width <= 0 || height <= 0) {
        return false;
    }

    const char* u = units ? units : "px";
    if (strcasecmp(u, "px") == 0) {
        *out_width = width;
        *out_height = height;
        return true;
    }

    if (strcasecmp(u, "pt") == 0) {
        int use_dpi = dpi > 0 ? dpi : 72;
        // pixels = points * dpi / 72
        long w = ((long)width * (long)use_dpi + 36) / 72;
        long h = ((long)height * (long)use_dpi + 36) / 72;
        if (w <= 0 || h <= 0 || w > INT_MAX || h > INT_MAX) {
            return false;
        }
        *out_width = (int)w;
        *out_height = (int)h;
        return true;
    }

    return false;
}

int get_scale_filter(const char* algorithm) {
    if (!algorithm || algorithm[0] == '\0') {
        return (int)LanczosFilter;
    }
    if (strcasecmp(algorithm, "nearest") == 0 || strcasecmp(algorithm, "point") == 0) {
        return (int)PointFilter;
    }
    if (strcasecmp(algorithm, "bilinear") == 0 || strcasecmp(algorithm, "triangle") == 0) {
        return (int)TriangleFilter;
    }
    if (strcasecmp(algorithm, "bicubic") == 0 || strcasecmp(algorithm, "catrom") == 0) {
        return (int)CatromFilter;
    }
    if (strcasecmp(algorithm, "lanczos") == 0) {
        return (int)LanczosFilter;
    }
    if (strcasecmp(algorithm, "mitchell") == 0) {
        return (int)MitchellFilter;
    }
    return -1;
}

const char* get_scale_filter_name(const char* algorithm) {
    int f = get_scale_filter(algorithm);
    if (f < 0) {
        return "unknown";
    }
    if (f == (int)PointFilter) return "nearest";
    if (f == (int)TriangleFilter) return "bilinear";
    if (f == (int)CatromFilter) return "bicubic";
    if (f == (int)MitchellFilter) return "mitchell";
    return "lanczos";
}

void scale_image_core_set_error(char** error_msg, const char* msg) {
    if (error_msg) {
        *error_msg = msg ? strdup(msg) : NULL;
    }
}

void scale_image_core_keep_first_frame(MagickWand* wand) {
    if (!wand) {
        return;
    }
    // Multi-frame GIF/animated WebP/ICO: only the first frame is processed.
    size_t n = MagickGetNumberImages(wand);
    if (n <= 1) {
        return;
    }
    MagickResetIterator(wand);
    MagickNextImage(wand);
    MagickWand* first = MagickGetImage(wand);
    if (!first) {
        return;
    }
    ClearMagickWand(wand);
    MagickAddImage(wand, first);
    DestroyMagickWand(first);
    MagickResetIterator(wand);
    MagickNextImage(wand);
}

bool scale_image_core_should_strip_profiles(const char* format) {
    if (!format || format[0] == '\0' || format_is_xo(format)) {
        return false;
    }
    if (strcasecmp(format, "jpg") == 0 || strcasecmp(format, "jpeg") == 0 ||
        strcasecmp(format, "png") == 0 || strcasecmp(format, "webp") == 0 ||
        strcasecmp(format, "gif") == 0 || strcasecmp(format, "ico") == 0) {
        return true;
    }
    return false;
}

bool scale_image_core_format_lacks_alpha(const char* format) {
    if (!format) {
        return false;
    }
    return (strcasecmp(format, "jpg") == 0 || strcasecmp(format, "jpeg") == 0 ||
            strcasecmp(format, "bmp") == 0);
}

bool scale_image_core_flatten_for_opaque(MagickWand* wand, const char* format) {
    if (!wand || !scale_image_core_format_lacks_alpha(format)) {
        return true;
    }
    if (MagickGetImageAlphaChannel(wand) == MagickFalse) {
        return true;
    }
    // Composite onto white so JPEG/BMP have no transparency artifacts
    PixelWand* bg = NewPixelWand();
    if (!bg) {
        return false;
    }
    PixelSetColor(bg, "white");
    MagickSetImageBackgroundColor(wand, bg);
    MagickWand* flat = MagickMergeImageLayers(wand, FlattenLayer);
    DestroyPixelWand(bg);
    if (!flat) {
        return false;
    }
    ClearMagickWand(wand);
    MagickAddImage(wand, flat);
    DestroyMagickWand(flat);
    MagickResetIterator(wand);
    MagickNextImage(wand);
    MagickSetImageAlphaChannel(wand, OffAlphaChannel);
    return true;
}

void scale_image_core_strip_profiles(MagickWand* wand, const char* format) {
    if (!wand || !scale_image_core_should_strip_profiles(format)) {
        return;
    }
    // Drop ICC and other profiles for smaller web-format output
    MagickStripImage(wand);
}

MagickWand* scale_image_core(const unsigned char* data, size_t data_len,
                             int width, int height,
                             const char* units, int dpi,
                             const char* scale_algorithm,
                             const char* format,
                             char** error_msg,
                             size_t* input_width,
                             size_t* input_height,
                             char** input_format_out) {
    if (error_msg) {
        *error_msg = NULL;
    }
    if (input_width) {
        *input_width = 0;
    }
    if (input_height) {
        *input_height = 0;
    }
    if (input_format_out) {
        *input_format_out = NULL;
    }

    if (!data || data_len == 0) {
        scale_image_core_set_error(error_msg, "Empty image data");
        return NULL;
    }

    int px_w = 0;
    int px_h = 0;
    if (!parse_dimensions(width, height, units, dpi, &px_w, &px_h)) {
        scale_image_core_set_error(error_msg, "Invalid dimensions or units");
        return NULL;
    }

    int filter = get_scale_filter(scale_algorithm);
    if (filter < 0) {
        scale_image_core_set_error(error_msg, "Unknown scale_algorithm");
        return NULL;
    }

    // Ensure MagickWand is initialized (safe if launch already called init)
    if (!reporting_service_is_initialized()) {
        if (!reporting_service_init()) {
            scale_image_core_set_error(error_msg, "Failed to initialize MagickWand");
            return NULL;
        }
    }

    MagickWand* wand = NewMagickWand();
    if (!wand) {
        scale_image_core_set_error(error_msg, "Failed to create MagickWand");
        return NULL;
    }

    // For SVG inputs, density affects rasterization resolution (set before read)
    if (dpi > 0) {
        char density[32];
        snprintf(density, sizeof(density), "%d", dpi);
        MagickSetOption(wand, "density", density);
        MagickSetResolution(wand, (double)dpi, (double)dpi);
    }

    MagickBooleanType ok = MagickReadImageBlob(wand, data, data_len);
    if (ok == MagickFalse) {
        char* ex = scale_image_core_exception(wand);
        scale_image_core_set_error(error_msg, ex ? ex : "Failed to read image blob");
        free(ex);
        DestroyMagickWand(wand);
        return NULL;
    }

    // Animated GIF/WebP and multi-page: first frame only
    scale_image_core_keep_first_frame(wand);

    // Apply EXIF orientation before capturing input dimensions
    MagickAutoOrientImage(wand);

    if (input_width) {
        *input_width = MagickGetImageWidth(wand);
    }
    if (input_height) {
        *input_height = MagickGetImageHeight(wand);
    }
    if (input_format_out) {
        char* fmt = MagickGetImageFormat(wand);
        if (fmt) {
            *input_format_out = strdup(fmt);
            MagickRelinquishMemory(fmt);
        }
    }

    ok = MagickResizeImage(wand, (size_t)px_w, (size_t)px_h, (FilterType)filter);
    if (ok == MagickFalse) {
        char* ex = scale_image_core_exception(wand);
        scale_image_core_set_error(error_msg, ex ? ex : "Failed to resize image");
        free(ex);
        if (input_format_out && *input_format_out) {
            free(*input_format_out);
            *input_format_out = NULL;
        }
        DestroyMagickWand(wand);
        return NULL;
    }

    // Flatten alpha when encoding to JPEG/BMP (no transparency)
    if (!scale_image_core_flatten_for_opaque(wand, format)) {
        scale_image_core_set_error(error_msg, "Failed to flatten transparency for opaque format");
        if (input_format_out && *input_format_out) {
            free(*input_format_out);
            *input_format_out = NULL;
        }
        DestroyMagickWand(wand);
        return NULL;
    }

    // Set output format unless XO (handled by caller)
    if (format && !format_is_xo(format)) {
        char magick_fmt[32];
        if (!format_to_imagemagick(format, magick_fmt, sizeof(magick_fmt))) {
            scale_image_core_set_error(error_msg, "Unsupported output format");
            if (input_format_out && *input_format_out) {
                free(*input_format_out);
                *input_format_out = NULL;
            }
            DestroyMagickWand(wand);
            return NULL;
        }
        ok = MagickSetImageFormat(wand, magick_fmt);
        if (ok == MagickFalse) {
            char* ex = scale_image_core_exception(wand);
            scale_image_core_set_error(error_msg, ex ? ex : "Failed to set image format");
            free(ex);
            if (input_format_out && *input_format_out) {
                free(*input_format_out);
                *input_format_out = NULL;
            }
            DestroyMagickWand(wand);
            return NULL;
        }
    }

    // Strip ICC/profiles for web formats (not BMP/SVG/XO)
    scale_image_core_strip_profiles(wand, format);

    return wand;
}
