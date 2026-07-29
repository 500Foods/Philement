/*
 * Image Format Mapping Implementation
 */

#include <src/hydrogen.h>

#include "image_format.h"

typedef struct {
    const char* api_name;
    const char* magick_name;
    const char* mime_type;
} FormatEntry;

bool format_entry_lookup(const char* format, const char** magick_name, const char** mime_type);
bool format_table_has(const char* format);
bool format_to_imagemagick(const char* format, char* magick_out, size_t out_len);
const char* format_to_mime(const char* format);
bool format_is_xo(const char* format);
bool format_is_known(const char* format);
bool format_is_supported(const char* format, const char* allowed_formats);

const FormatEntry format_table[] = {
    {"jpg",  "JPEG", "image/jpeg"},
    {"jpeg", "JPEG", "image/jpeg"},
    {"png",  "PNG",  "image/png"},
    {"bmp",  "BMP",  "image/bmp"},
    {"svg",  "SVG",  "image/svg+xml"},
    {"ico",  "ICO",  "image/x-icon"},
    {"webp", "WEBP", "image/webp"},
    {"gif",  "GIF",  "image/gif"},
    {"tiff", "TIFF", "image/tiff"},
    {"tif",  "TIFF", "image/tiff"},
    {"ppm",  "PPM",  "image/x-portable-pixmap"},
    {"tga",  "TGA",  "image/x-tga"},
    {"xo",   "XO",   "application/x-xo"},
    {NULL,   NULL,   NULL}
};

bool format_entry_lookup(const char* format, const char** magick_name, const char** mime_type) {
    if (!format || format[0] == '\0') {
        return false;
    }

    for (const FormatEntry* e = format_table; e->api_name != NULL; e++) {
        if (strcasecmp(format, e->api_name) == 0) {
            if (magick_name) {
                *magick_name = e->magick_name;
            }
            if (mime_type) {
                *mime_type = e->mime_type;
            }
            return true;
        }
    }
    return false;
}

bool format_table_has(const char* format) {
    return format_entry_lookup(format, NULL, NULL);
}

bool format_to_imagemagick(const char* format, char* magick_out, size_t out_len) {
    if (!format || !magick_out || out_len == 0) {
        return false;
    }

    if (format_is_xo(format)) {
        // XO is not an ImageMagick coder; callers handle it separately
        snprintf(magick_out, out_len, "XO");
        return true;
    }

    const char* magick_name = NULL;
    if (format_entry_lookup(format, &magick_name, NULL)) {
        snprintf(magick_out, out_len, "%s", magick_name);
        return true;
    }

    // Fallback: uppercase the API name and let ImageMagick try it
    size_t i = 0;
    for (; format[i] != '\0' && i + 1 < out_len; i++) {
        char c = format[i];
        if (c >= 'a' && c <= 'z') {
            c = (char)(c - 'a' + 'A');
        }
        magick_out[i] = c;
    }
    magick_out[i] = '\0';
    return i > 0;
}

const char* format_to_mime(const char* format) {
    const char* mime = NULL;
    if (format_entry_lookup(format, NULL, &mime)) {
        return mime;
    }
    return "application/octet-stream";
}

bool format_is_xo(const char* format) {
    return format && strcasecmp(format, "xo") == 0;
}

bool format_is_known(const char* format) {
    return format_entry_lookup(format, NULL, NULL);
}

bool format_is_supported(const char* format, const char* allowed_formats) {
    if (!format || format[0] == '\0') {
        return false;
    }

    if (!allowed_formats || allowed_formats[0] == '\0') {
        return true;
    }

    // Tokenize comma-separated list
    size_t flen = strlen(format);
    const char* p = allowed_formats;
    while (*p) {
        while (*p == ' ' || *p == '\t' || *p == ',') {
            p++;
        }
        if (*p == '\0') {
            break;
        }
        const char* start = p;
        while (*p && *p != ',' && *p != ' ' && *p != '\t') {
            p++;
        }
        size_t tlen = (size_t)(p - start);
        if (tlen == flen && strncasecmp(start, format, flen) == 0) {
            return true;
        }
    }
    return false;
}
