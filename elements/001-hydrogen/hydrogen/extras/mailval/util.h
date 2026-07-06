/*
 * Small allocation / string helpers shared across mailval modules.
 */

#ifndef MV_UTIL_H
#define MV_UTIL_H

#include <stdbool.h>
#include <stddef.h>

/* x-prefixed allocators abort on failure (test fixture, fail-fast is fine). */
void* mv_xmalloc(size_t n);
void* mv_xcalloc(size_t n, size_t sz);
void* mv_xrealloc(void* p, size_t n);
char* mv_xstrdup(const char* s);

/* Trim leading/trailing whitespace in place; returns s. */
char* mv_trim(char* s);

/* Case-insensitive compare. */
bool mv_ieq(const char* a, const char* b);

/* Base64 encode (RFC 4648) of src into caller buffer; returns encoded length. */
int mv_base64_encode(const unsigned char* src, int len, char* out, int out_cap);

/* Base64 decode; returns decoded byte count or -1 on error. */
int mv_base64_decode(const char* src, unsigned char* out, int out_cap);

/* Current UTC time as an IMAP/internaldate style string. */
void mv_now_rfc3339(char* buf, size_t n);

/* Simple dynamic byte buffer. */
typedef struct {
    char* data;
    size_t len;
    size_t cap;
} mv_buf;

void mv_buf_init(mv_buf* b);
void mv_buf_free(mv_buf* b);
void mv_buf_append(mv_buf* b, const char* data, size_t n);
void mv_buf_append_str(mv_buf* b, const char* s);

#endif /* MV_UTIL_H */
