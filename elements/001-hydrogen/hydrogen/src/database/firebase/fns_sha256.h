/*
 * Firebase in-process SHA-256 then standard Base64.
 * FB_SHA256_B64(a, b) = base64(SHA256(utf8(a) || utf8(b))).
 */

#ifndef DATABASE_ENGINE_FIREBASE_FNS_SHA256_H
#define DATABASE_ENGINE_FIREBASE_FNS_SHA256_H

char* firebase_sha256_b64(const char* a, const char* b);

#endif /* DATABASE_ENGINE_FIREBASE_FNS_SHA256_H */
