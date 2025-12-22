#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sqludf.h>
#include <brotli/decode.h>

/* Base64 decode table */
static int b64_idx[256];
static int b64_inited = 0;

static void b64_init(void) {
    if (b64_inited) return;
    for (int i = 0; i < 256; ++i) b64_idx[i] = -1;
    for (int c = 'A'; c <= 'Z'; ++c) b64_idx[c] = c - 'A';
    for (int c = 'a'; c <= 'z'; ++c) b64_idx[c] = c - 'a' + 26;
    for (int c = '0'; c <= '9'; ++c) b64_idx[c] = c - '0' + 52;
    b64_idx[(int)'+'] = 62;
    b64_idx[(int)'/'] = 63;
    b64_inited = 1;
}

/* Decode base64 data - returns number of bytes written, or 0 on error */
static size_t b64_decode(const char *in, size_t inlen, unsigned char *out) {
    size_t o = 0;
    for (size_t i = 0; i + 3 < inlen; i += 4) {
        int a = b64_idx[(unsigned char)in[i]];
        int b = b64_idx[(unsigned char)in[i + 1]];
        int c = (in[i + 2] == '=') ? -1 : b64_idx[(unsigned char)in[i + 2]];
        int d = (in[i + 3] == '=') ? -1 : b64_idx[(unsigned char)in[i + 3]];
        if (a < 0 || b < 0) return 0;
        unsigned int triple = ((unsigned int)a << 18) | ((unsigned int)b << 12) | 
                             ((c < 0 ? 0 : (unsigned int)c) << 6) | (d < 0 ? 0 : (unsigned int)d);
        out[o++] = (triple >> 16) & 0xFF;
        if (c >= 0) {
            out[o++] = (triple >> 8) & 0xFF;
            if (d >= 0) out[o++] = triple & 0xFF;
        }
    }
    return o;
}

/* DB2 UDF for Brotli decompression
 * Input: BLOB containing compressed binary data (max 32KB compressed)
 * Output: CLOB(1M) containing decompressed text data
 * 
 * Note: We use BLOB for input (binary data from base64 decode) and
 * CLOB for output (decompressed text). BLOB preserves binary data
 * with embedded null bytes, while CLOB is appropriate for text output.
 */

/* Brotli decompression function
 * Input: BLOB containing compressed binary data
 * Output: CLOB(1M) containing decompressed text data
 */
#ifdef __cplusplus
extern "C"
#endif
void SQL_API_FN BROTLI_DECOMPRESS(
    const SQLUDF_BLOB *compressed,  /* Input parameter - binary compressed data */
    SQLUDF_CLOB *decompressed,      /* Output parameter - text data */
    const SQLUDF_NULLIND *in_ind,   /* Input null indicator */
    SQLUDF_NULLIND *out_ind,        /* Output null indicator */
    SQLUDF_TRAIL_ARGS
  ){
    /* Initialize SQLSTATE to success */
    strcpy(SQLUDF_STATE, "00000");
    
    if (*in_ind == -1) { 
        *out_ind = -1; 
        return; 
    }
    *out_ind = 0;
    
    /* Get input length from BLOB structure (handles binary data with null bytes) */
    size_t compressed_size = compressed->length;
    if (compressed_size == 0) {
        decompressed->length = 0;
        return;
    }
    
    /* Validate input size */
    if (compressed_size > 32672) {
        strcpy(SQLUDF_STATE, "22001");
        strcpy(SQLUDF_MSGTX, "Compressed data exceeds 32KB limit");
        *out_ind = -1;
        return;
    }
    
    /* Create Brotli decoder instance */
    BrotliDecoderState* decoder = BrotliDecoderCreateInstance(NULL, NULL, NULL);
    if (!decoder) {
        strcpy(SQLUDF_STATE, "UDF01");
        strcpy(SQLUDF_MSGTX, "Failed to create Brotli decoder");
        *out_ind = -1;
        return;
    }
    
    /* Allocate output buffer (10x compressed size as initial estimate) */
    size_t buffer_size = compressed_size * 10;
    if (buffer_size < 65536) {
        buffer_size = 65536;  /* Minimum 64KB */
    }
    if (buffer_size > 67108864) {
        buffer_size = 67108864;  /* Initial cap at 64MB */
    }
    
    uint8_t *output = (uint8_t *)malloc(buffer_size);
    if (!output) {
        strcpy(SQLUDF_STATE, "UDF02");
        strcpy(SQLUDF_MSGTX, "Memory allocation failed");
        BrotliDecoderDestroyInstance(decoder);
        *out_ind = -1;
        return;
    }
    
    /* Set up decompression parameters */
    const uint8_t *next_in = (const uint8_t *)compressed->data;
    size_t available_in = compressed_size;
    uint8_t *next_out = output;
    size_t available_out = buffer_size;
    size_t total_out = 0;
    
    /* Decompress the data */
    BrotliDecoderResult result;
    do {
        result = BrotliDecoderDecompressStream(
            decoder,
            &available_in, &next_in,
            &available_out, &next_out,
            &total_out
        );
        
        if (result == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT) {
            /* Need more output space */
            size_t current_position = (size_t)(next_out - output);
            size_t new_buffer_size = buffer_size * 2;
            /* Allow growth up to 512MB for decompressed CLOB output */
            if (new_buffer_size > 536870912) {
                new_buffer_size = 536870912;
            }
            
            if (new_buffer_size == buffer_size) {
                /* Already at max size and can't grow - data too large */
                strcpy(SQLUDF_STATE, "UDF04");
                strcpy(SQLUDF_MSGTX, "Decompressed data too large (>512MB)");
                free(output);
                BrotliDecoderDestroyInstance(decoder);
                *out_ind = -1;
                return;
            }
            
            uint8_t *new_output = (uint8_t *)realloc(output, new_buffer_size);
            if (!new_output) {
                strcpy(SQLUDF_STATE, "UDF02");
                strcpy(SQLUDF_MSGTX, "Memory allocation failed");
                free(output);
                BrotliDecoderDestroyInstance(decoder);
                *out_ind = -1;
                return;
            }
            
            output = new_output;
            buffer_size = new_buffer_size;
            next_out = output + current_position;
            available_out = buffer_size - current_position;
        } else if (result == BROTLI_DECODER_RESULT_ERROR) {
            strcpy(SQLUDF_STATE, "UDF03");
            strcpy(SQLUDF_MSGTX, "Brotli decompression error");
            free(output);
            BrotliDecoderDestroyInstance(decoder);
            *out_ind = -1;
            return;
        } else if (result == BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT) {
            /* Incomplete compressed data */
            strcpy(SQLUDF_STATE, "UDF05");
            strcpy(SQLUDF_MSGTX, "Incomplete compressed data");
            free(output);
            BrotliDecoderDestroyInstance(decoder);
            *out_ind = -1;
            return;
        }
    } while (result != BROTLI_DECODER_RESULT_SUCCESS);
    
    /* Set output */
    decompressed->length = total_out;
    memcpy(decompressed->data, output, total_out);
    
    free(output);
    BrotliDecoderDestroyInstance(decoder);
}

/* Combined Base64 decode + Brotli decompress function
 * This avoids passing binary data through SQL by doing both operations in C.
 * Input: CLOB containing base64-encoded compressed data
 * Output: CLOB containing decompressed text data
 */
#ifdef __cplusplus
extern "C"
#endif
void SQL_API_FN BASE64_BROTLI_DECOMPRESS(
    const SQLUDF_CLOB *encoded,     /* Input parameter - base64 text */
    SQLUDF_CLOB *decompressed,      /* Output parameter - decompressed text */
    const SQLUDF_NULLIND *in_ind,   /* Input null indicator */
    SQLUDF_NULLIND *out_ind,        /* Output null indicator */
    SQLUDF_TRAIL_ARGS
  ){
    /* Initialize SQLSTATE to success */
    strcpy(SQLUDF_STATE, "00000");
    
    if (*in_ind == -1) { 
        *out_ind = -1; 
        return; 
    }
    *out_ind = 0;
    
    /* Get input length */
    size_t encoded_len = encoded->length;
    if (encoded_len == 0) {
        decompressed->length = 0;
        return;
    }
    
    /* Initialize base64 decode table */
    b64_init();
    
    /* Filter input to valid base64 characters and decode */
    char *filtered = (char *)malloc(encoded_len + 4);
    if (!filtered) {
        strcpy(SQLUDF_STATE, "UDF02");
        strcpy(SQLUDF_MSGTX, "Memory allocation failed");
        *out_ind = -1;
        return;
    }
    
    size_t filtered_len = 0;
    for (size_t i = 0; i < encoded_len; ++i) {
        unsigned char c = (unsigned char)encoded->data[i];
        if (c == '=' || b64_idx[c] >= 0) {
            filtered[filtered_len++] = (char)c;
        }
    }
    
    /* Only decode complete 4-byte groups */
    size_t aligned = (filtered_len / 4) * 4;
    if (aligned == 0) {
        free(filtered);
        decompressed->length = 0;
        return;
    }
    
    /* Allocate buffer for decoded binary data (3/4 of base64 size) */
    size_t max_decoded = (aligned * 3) / 4 + 4;
    unsigned char *decoded = (unsigned char *)malloc(max_decoded);
    if (!decoded) {
        strcpy(SQLUDF_STATE, "UDF02");
        strcpy(SQLUDF_MSGTX, "Memory allocation failed");
        free(filtered);
        *out_ind = -1;
        return;
    }
    
    size_t decoded_len = b64_decode(filtered, aligned, decoded);
    free(filtered);
    
    if (decoded_len == 0) {
        strcpy(SQLUDF_STATE, "UDF06");
        strcpy(SQLUDF_MSGTX, "Invalid base64 input");
        free(decoded);
        *out_ind = -1;
        return;
    }
    
    /* Now decompress the binary data with Brotli */
    BrotliDecoderState* decoder = BrotliDecoderCreateInstance(NULL, NULL, NULL);
    if (!decoder) {
        strcpy(SQLUDF_STATE, "UDF01");
        strcpy(SQLUDF_MSGTX, "Failed to create Brotli decoder");
        free(decoded);
        *out_ind = -1;
        return;
    }
    
    /* Allocate output buffer */
    size_t buffer_size = decoded_len * 10;
    if (buffer_size < 65536) buffer_size = 65536;
    if (buffer_size > 67108864) buffer_size = 67108864;
    
    uint8_t *output = (uint8_t *)malloc(buffer_size);
    if (!output) {
        strcpy(SQLUDF_STATE, "UDF02");
        strcpy(SQLUDF_MSGTX, "Memory allocation failed");
        BrotliDecoderDestroyInstance(decoder);
        free(decoded);
        *out_ind = -1;
        return;
    }
    
    /* Decompress */
    const uint8_t *next_in = decoded;
    size_t available_in = decoded_len;
    uint8_t *next_out = output;
    size_t available_out = buffer_size;
    size_t total_out = 0;
    
    BrotliDecoderResult result;
    do {
        result = BrotliDecoderDecompressStream(
            decoder,
            &available_in, &next_in,
            &available_out, &next_out,
            &total_out
        );
        
        if (result == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT) {
            size_t current_position = (size_t)(next_out - output);
            size_t new_buffer_size = buffer_size * 2;
            if (new_buffer_size > 536870912) new_buffer_size = 536870912;
            
            if (new_buffer_size == buffer_size) {
                strcpy(SQLUDF_STATE, "UDF04");
                strcpy(SQLUDF_MSGTX, "Decompressed data too large");
                free(output);
                free(decoded);
                BrotliDecoderDestroyInstance(decoder);
                *out_ind = -1;
                return;
            }
            
            uint8_t *new_output = (uint8_t *)realloc(output, new_buffer_size);
            if (!new_output) {
                strcpy(SQLUDF_STATE, "UDF02");
                strcpy(SQLUDF_MSGTX, "Memory allocation failed");
                free(output);
                free(decoded);
                BrotliDecoderDestroyInstance(decoder);
                *out_ind = -1;
                return;
            }
            
            output = new_output;
            buffer_size = new_buffer_size;
            next_out = output + current_position;
            available_out = buffer_size - current_position;
        } else if (result == BROTLI_DECODER_RESULT_ERROR) {
            BrotliDecoderErrorCode err = BrotliDecoderGetErrorCode(decoder);
            (void)err;  /* Could log error code if needed */
            strcpy(SQLUDF_STATE, "UDF03");
            strcpy(SQLUDF_MSGTX, "Brotli decompression error");
            free(output);
            free(decoded);
            BrotliDecoderDestroyInstance(decoder);
            *out_ind = -1;
            return;
        } else if (result == BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT) {
            strcpy(SQLUDF_STATE, "UDF05");
            strcpy(SQLUDF_MSGTX, "Incomplete compressed data");
            free(output);
            free(decoded);
            BrotliDecoderDestroyInstance(decoder);
            *out_ind = -1;
            return;
        }
    } while (result != BROTLI_DECODER_RESULT_SUCCESS);
    
    /* Set output */
    decompressed->length = total_out;
    memcpy(decompressed->data, output, total_out);
    
    free(output);
    free(decoded);
    BrotliDecoderDestroyInstance(decoder);
}

/* Simple test UDF: returns "Helium Brotli" */
#ifdef __cplusplus
extern "C"
#endif
void SQL_API_FN HELIUM_BROTLI_CHECK(
    SQLUDF_VARCHAR *out_str,
    SQLUDF_NULLIND *out_ind,
    SQLUDF_TRAIL_ARGS
){
    /* Initialize SQLSTATE to success */
    strcpy(SQLUDF_STATE, "00000");

    /* Return the string "Helium Brotli" */
    strcpy(out_str, "Helium Brotli");
    *out_ind = 0;
}
