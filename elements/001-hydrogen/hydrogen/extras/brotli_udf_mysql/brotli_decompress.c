/*
 * MySQL UDF for Brotli Decompression
 * 
 * This UDF decompresses Brotli-compressed data for use in the Helium migration system.
 * Leverages the Brotli decode library used elsewhere in the Hydrogen project.
 *
 * Build: See Makefile
 * Install: cp brotli_decompress.so /usr/lib/mysql/plugin/
 * Register: CREATE FUNCTION BROTLI_DECOMPRESS RETURNS STRING SONAME 'brotli_decompress.so';
 */

#include <mysql/mysql.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <brotli/decode.h>

/* MySQL UDF initialization function */
bool brotli_decompress_init(UDF_INIT *initid, UDF_ARGS *args, char *message) {
    if (args->arg_count != 1) {
        strcpy(message, "BROTLI_DECOMPRESS requires exactly 1 argument");
        return 1;
    }
    
    if (args->arg_type[0] != STRING_RESULT) {
        strcpy(message, "BROTLI_DECOMPRESS argument must be a string");
        return 1;
    }
    
    /* Allocate buffer for decompressed data (conservative estimate) */
    initid->max_length = 16777216; /* 16MB max output */
    initid->maybe_null = 1;
    
    return 0;
}

/* MySQL UDF main function */
char *brotli_decompress(UDF_INIT *initid, UDF_ARGS *args,
                       char *result, unsigned long *length,
                       char *is_null, char *error) {
    (void)initid; /* Unused */
    
    /* Handle NULL input */
    if (args->args[0] == NULL || args->lengths[0] == 0) {
        *is_null = 1;
        return NULL;
    }
    
    *is_null = 0;
    
    /* Get compressed data */
    const uint8_t *compressed_data = (const uint8_t *)args->args[0];
    size_t compressed_size = args->lengths[0];
    
    /* Create Brotli decoder */
    BrotliDecoderState* decoder = BrotliDecoderCreateInstance(NULL, NULL, NULL);
    if (!decoder) {
        *error = 1;
        return NULL;
    }
    
    /* Initial buffer size (4x compressed size) */
    size_t buffer_size = compressed_size * 4;
    if (buffer_size > 16777216) {
        buffer_size = 16777216; /* Limit to 16MB */
    }
    
    uint8_t *output = (uint8_t *)malloc(buffer_size);
    if (!output) {
        BrotliDecoderDestroyInstance(decoder);
        *error = 1;
        return NULL;
    }
    
    /* Decompress the data */
    const uint8_t *next_in = compressed_data;
    size_t available_in = compressed_size;
    uint8_t *next_out = output;
    size_t available_out = buffer_size;
    size_t total_out = 0;
    
    BrotliDecoderResult decode_result;
    do {
        decode_result = BrotliDecoderDecompressStream(
            decoder,
            &available_in, &next_in,
            &available_out, &next_out,
            &total_out
        );
        
        if (decode_result == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT) {
            /* Need more output space */
            size_t current_position = (size_t)(next_out - output);
            size_t new_buffer_size = buffer_size * 2;
            if (new_buffer_size > 16777216) {
                new_buffer_size = 16777216;
            }
            
            uint8_t *new_buffer = (uint8_t *)realloc(output, new_buffer_size);
            if (!new_buffer) {
                free(output);
                BrotliDecoderDestroyInstance(decoder);
                *error = 1;
                return NULL;
            }
            
            output = new_buffer;
            buffer_size = new_buffer_size;
            next_out = output + current_position;
            available_out = buffer_size - current_position;
        } else if (decode_result == BROTLI_DECODER_RESULT_ERROR) {
            free(output);
            BrotliDecoderDestroyInstance(decoder);
            *error = 1;
            return NULL;
        }
    } while (decode_result != BROTLI_DECODER_RESULT_SUCCESS);
    
    BrotliDecoderDestroyInstance(decoder);
    
    /* Copy to result buffer */
    if (total_out > 0) {
        memcpy(result, output, total_out);
        *length = total_out;
    } else {
        *length = 0;
    }
    
    free(output);
    return result;
}

/* MySQL UDF cleanup function */
void brotli_decompress_deinit(UDF_INIT *initid) {
    (void)initid; /* Unused */
}