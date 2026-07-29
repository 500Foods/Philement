/*
 * Reporting Base64 Utilities
 *
 * Thin wrappers around utils_crypto base64 helpers with data: URI support
 * for image payloads used by the reporting image_scale endpoint.
 */

#ifndef REPORTING_BASE64_UTILS_H
#define REPORTING_BASE64_UTILS_H

#include <stddef.h>
#include <stdbool.h>

/*
 * Locate the base64 payload inside a data: URI or plain base64 string.
 * On success, *base64_start points into input at the first base64 character.
 * For "data:image/png;base64,XXXX" returns pointer to XXXX.
 * For plain base64 returns input itself.
 */
bool parse_data_uri(const char* input, const char** base64_start);

/*
 * Decode base64 image data. Strips an optional data: URI prefix first.
 * Returns heap buffer (caller frees) or NULL on error.
 * *out_len receives decoded byte count.
 */
unsigned char* reporting_base64_decode(const char* input, size_t* out_len);

/*
 * Encode binary data to standard base64 (with padding).
 * Returns heap string (caller frees) or NULL on error.
 */
char* reporting_base64_encode(const unsigned char* data, size_t len);

#endif /* REPORTING_BASE64_UTILS_H */
