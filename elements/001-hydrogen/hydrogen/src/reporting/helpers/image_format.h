/*
 * Image Format Mapping
 *
 * Maps API format strings to ImageMagick format names and MIME types.
 */

#ifndef REPORTING_IMAGE_FORMAT_H
#define REPORTING_IMAGE_FORMAT_H

#include <stddef.h>
#include <stdbool.h>

// Map API format (e.g. "jpg") to ImageMagick format name (e.g. "JPEG").
// Returns true on success and writes into magick_out.
bool format_to_imagemagick(const char* format, char* magick_out, size_t out_len);

// Return MIME type for an API format string, or NULL if unknown.
const char* format_to_mime(const char* format);

// Return true if format is in the comma-separated allowed list, or if
// allowed_formats is NULL/empty (all formats allowed).
bool format_is_supported(const char* format, const char* allowed_formats);

// Return true if format is the XO (PDF XObject) intermediate format.
bool format_is_xo(const char* format);

// Return true if the format string is one of the known built-in formats.
bool format_is_known(const char* format);

// Internal table lookup (non-static for Unity / no-static policy).
// Returns true if format is in the built-in table.
bool format_table_has(const char* format);

// Opaque-ish table entry accessors used by format mapping (no-static policy).
bool format_entry_lookup(const char* format, const char** magick_name, const char** mime_type);

#endif /* REPORTING_IMAGE_FORMAT_H */
