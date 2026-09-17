/*
 * Firebase in-process Base64 (RFC 4648 +/ with padding).
 */

#ifndef DATABASE_ENGINE_FIREBASE_FNS_BASE64_H
#define DATABASE_ENGINE_FIREBASE_FNS_BASE64_H

#include <stddef.h>

char* firebase_base64_encode(const unsigned char* data, size_t length);
unsigned char* firebase_base64_decode(const char* input, size_t* out_len);

#endif /* DATABASE_ENGINE_FIREBASE_FNS_BASE64_H */
