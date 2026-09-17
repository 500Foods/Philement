/*
 * Firebase in-process Brotli decompress. Engine never recompresses.
 * Fail closed if decompressed size exceeds FIREBASE_MAX_FIELD_BYTES.
 */

#include <src/hydrogen.h>
#include <src/database/database.h>
#include "types.h"
#include "fns_brotli.h"

#include <brotli/decode.h>

char* firebase_brotli_decompress(const unsigned char* data, size_t length, size_t* out_len) {
    if (!data) {
        return NULL;
    }
    if (length == 0) {
        char* empty = malloc(1);
        if (empty) {
            empty[0] = '\0';
        }
        if (out_len) {
            *out_len = 0;
        }
        return empty;
    }

    BrotliDecoderState* decoder = BrotliDecoderCreateInstance(NULL, NULL, NULL);
    if (!decoder) {
        return NULL;
    }

    size_t buffer_size = length * 4;
    if (buffer_size < 64) {
        buffer_size = 64;
    }
    if (buffer_size > FIREBASE_MAX_FIELD_BYTES) {
        buffer_size = FIREBASE_MAX_FIELD_BYTES;
    }

    uint8_t* output = malloc(buffer_size + 1);
    if (!output) {
        BrotliDecoderDestroyInstance(decoder);
        return NULL;
    }

    const uint8_t* next_in = data;
    size_t available_in = length;
    uint8_t* next_out = output;
    size_t available_out = buffer_size;
    size_t total_out = 0;
    BrotliDecoderResult result;
    bool failed = false;

    do {
        result = BrotliDecoderDecompressStream(decoder, &available_in, &next_in,
                                               &available_out, &next_out, &total_out);
        if (result == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT) {
            size_t current = (size_t)(next_out - output);
            if (buffer_size >= FIREBASE_MAX_FIELD_BYTES) {
                failed = true;
                break;
            }
            size_t new_size = buffer_size * 2;
            if (new_size > FIREBASE_MAX_FIELD_BYTES) {
                new_size = FIREBASE_MAX_FIELD_BYTES;
            }
            uint8_t* grown = realloc(output, new_size + 1);
            if (!grown) {
                failed = true;
                break;
            }
            output = grown;
            buffer_size = new_size;
            next_out = output + current;
            available_out = buffer_size - current;
        } else if (result != BROTLI_DECODER_RESULT_SUCCESS) {
            failed = true;
            break;
        }
    } while (result != BROTLI_DECODER_RESULT_SUCCESS);

    BrotliDecoderDestroyInstance(decoder);

    if (failed || result != BROTLI_DECODER_RESULT_SUCCESS || total_out > FIREBASE_MAX_FIELD_BYTES) {
        free(output);
        return NULL;
    }

    output[total_out] = '\0';
    if (out_len) {
        *out_len = total_out;
    }
    return (char*)output;
}
