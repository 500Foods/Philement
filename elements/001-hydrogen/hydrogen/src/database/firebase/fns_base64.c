/*
 * Firebase in-process Base64. Alphabet is standard +/ with = padding.
 */

#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/utils/utils_crypto.h>
#include "fns_base64.h"

char* firebase_base64_encode(const unsigned char* data, size_t length) {
    if (!data) {
        return NULL;
    }
    if (length == 0) {
        char* empty = malloc(1);
        if (empty) {
            empty[0] = '\0';
        }
        return empty;
    }
    return utils_base64_encode(data, length);
}

unsigned char* firebase_base64_decode(const char* input, size_t* out_len) {
    if (!input || !out_len) {
        return NULL;
    }
    return utils_base64_decode(input, out_len);
}
