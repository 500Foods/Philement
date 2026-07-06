#include "util.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <time.h>

void* mv_xmalloc(size_t n) {
    void* p = malloc(n ? n : 1);
    if (!p) { fprintf(stderr, "mailval: out of memory\n"); exit(1); }
    return p;
}

void* mv_xcalloc(size_t n, size_t sz) {
    void* p = calloc(n ? n : 1, sz ? sz : 1);
    if (!p) { fprintf(stderr, "mailval: out of memory\n"); exit(1); }
    return p;
}

void* mv_xrealloc(void* p, size_t n) {
    void* q = realloc(p, n ? n : 1);
    if (!q) { fprintf(stderr, "mailval: out of memory\n"); exit(1); }
    return q;
}

char* mv_xstrdup(const char* s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char* d = mv_xmalloc(n);
    memcpy(d, s, n);
    return d;
}

char* mv_trim(char* s) {
    if (!s) return s;
    while (*s && isspace((unsigned char)*s)) s++;
    char* end = s + strlen(s);
    while (end > s && isspace((unsigned char)*(end - 1))) end--;
    *end = '\0';
    return s;
}

bool mv_ieq(const char* a, const char* b) {
    if (!a || !b) return false;
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return false;
        a++; b++;
    }
    return *a == *b;
}

static const char MV_B64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int mv_base64_encode(const unsigned char* src, int len, char* out, int out_cap) {
    int i, o = 0;
    for (i = 0; i < len; i += 3) {
        unsigned int v = ((unsigned int)src[i]) << 16;
        if (i + 1 < len) v |= ((unsigned int)src[i + 1]) << 8;
        if (i + 2 < len) v |= ((unsigned int)src[i + 2]);
        int n = (len - i) < 3 ? (len - i) : 3;
        if (o + 4 >= out_cap) return -1;
        out[o++] = MV_B64[(v >> 18) & 0x3F];
        out[o++] = MV_B64[(v >> 12) & 0x3F];
        out[o++] = (n > 1) ? MV_B64[(v >> 6) & 0x3F] : '=';
        out[o++] = (n > 2) ? MV_B64[v & 0x3F] : '=';
    }
    if (o < out_cap) out[o] = '\0';
    return o;
}

int mv_base64_decode(const char* src, unsigned char* out, int out_cap) {
    unsigned char tbl[256];
    int i;
    for (i = 0; i < 256; i++) tbl[i] = 0xFF;
    for (i = 0; i < 64; i++) tbl[(unsigned char)MV_B64[i]] = (unsigned char)i;
    int o = 0;
    for (i = 0; src[i]; i += 4) {
        unsigned int v = 0;
        int pad = 0;
        for (int j = 0; j < 4; j++) {
            unsigned char c = (unsigned char)src[i + j];
            if (c == '=') { pad++; continue; }
            if (tbl[c] == 0xFF) return -1;
            v = (v << 6) | tbl[c];
        }
        if (o + 3 - pad > out_cap) return -1;
        if (pad < 3) out[o++] = (v >> 16) & 0xFF;
        if (pad < 2) out[o++] = (v >> 8) & 0xFF;
        if (pad < 1) out[o++] = v & 0xFF;
    }
    return o;
}

void mv_now_rfc3339(char* buf, size_t n) {
    time_t t = time(NULL);
    struct tm tm;
#if defined(_WIN32)
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    strftime(buf, n, "%Y-%m-%dT%H:%M:%SZ", &tm);
}

void mv_buf_init(mv_buf* b) {
    b->data = NULL; b->len = 0; b->cap = 0;
}

void mv_buf_free(mv_buf* b) {
    free(b->data);
    b->data = NULL; b->len = 0; b->cap = 0;
}

void mv_buf_append(mv_buf* b, const char* data, size_t n) {
    if (b->len + n + 1 > b->cap) {
        size_t nc = (b->cap ? b->cap * 2 : 256);
        while (nc < b->len + n + 1) nc *= 2;
        b->data = mv_xrealloc(b->data, nc);
        b->cap = nc;
    }
    memcpy(b->data + b->len, data, n);
    b->len += n;
    b->data[b->len] = '\0';
}

void mv_buf_append_str(mv_buf* b, const char* s) {
    mv_buf_append(b, s, strlen(s));
}
