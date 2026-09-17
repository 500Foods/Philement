/*
 * Firebase in-process Brotli decompress (libbrotlidec).
 */

#ifndef DATABASE_ENGINE_FIREBASE_FNS_BROTLI_H
#define DATABASE_ENGINE_FIREBASE_FNS_BROTLI_H

#include <stddef.h>

char* firebase_brotli_decompress(const unsigned char* data, size_t length, size_t* out_len);

#endif /* DATABASE_ENGINE_FIREBASE_FNS_BROTLI_H */
