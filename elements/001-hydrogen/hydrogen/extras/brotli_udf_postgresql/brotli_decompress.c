/*
 * PostgreSQL Extension for Brotli Decompression
 * 
 * This extension decompresses Brotli-compressed data for use in the Helium migration system.
 * Leverages the Brotli decode library used elsewhere in the Hydrogen project.
 *
 * Build: See Makefile
 * Install: make install
 * Enable: CREATE EXTENSION brotli_decompress;
 */

#include "postgres.h"
#include "fmgr.h"
#include "utils/builtins.h"
#include <string.h>
#include <brotli/decode.h>

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

/* Ensure VARDATA/VARSIZE/SET_VARSIZE macros are available */
#ifndef VARHDRSZ
#define VARHDRSZ ((int32) sizeof(int32))
#endif

#ifndef VARDATA
#define VARDATA(PTR) (((char *)(PTR)) + VARHDRSZ)
#endif

#ifndef VARSIZE
#define VARSIZE(PTR) (*((int32 *)(PTR)))
#endif

#ifndef SET_VARSIZE
#define SET_VARSIZE(PTR, len) (*((int32 *)(PTR)) = (len))
#endif

/* Function declaration */
PG_FUNCTION_INFO_V1(brotli_decompress);

/*
 * brotli_decompress
 *   Decompress Brotli-compressed bytea data and return as text
 *
 * Arguments:
 *   compressed - bytea containing Brotli-compressed data
 *
 * Returns:
 *   text - decompressed UTF-8 string
 */
Datum
brotli_decompress(PG_FUNCTION_ARGS)
{
    bytea *compressed;
    text *result;
    uint8_t *compressed_data;
    size_t compressed_size;
    BrotliDecoderState *decoder;
    uint8_t *output;
    size_t buffer_size;
    const uint8_t *next_in;
    size_t available_in;
    uint8_t *next_out;
    size_t available_out;
    size_t total_out = 0;
    BrotliDecoderResult decode_result;
    
    /* Handle NULL input */
    if (PG_ARGISNULL(0)) {
        PG_RETURN_NULL();
    }
    
    /* Get compressed data */
    compressed = PG_GETARG_BYTEA_P(0);
    compressed_data = (uint8_t *)VARDATA(compressed);
    compressed_size = VARSIZE(compressed) - VARHDRSZ;
    
    if (compressed_size == 0) {
        PG_RETURN_TEXT_P(cstring_to_text(""));
    }
    
    /* Create Brotli decoder */
    decoder = BrotliDecoderCreateInstance(NULL, NULL, NULL);
    if (!decoder) {
        ereport(ERROR,
                (errcode(ERRCODE_OUT_OF_MEMORY),
                 errmsg("Failed to create Brotli decoder")));
    }
    
    /* Initial buffer size (4x compressed size) */
    buffer_size = compressed_size * 4;
    if (buffer_size > 1073741824) { /* Limit to 1GB */
        buffer_size = 1073741824;
    }
    
    output = (uint8_t *)palloc(buffer_size);
    
    /* Set up decompression parameters */
    next_in = compressed_data;
    available_in = compressed_size;
    next_out = output;
    available_out = buffer_size;
    
    /* Decompress the data */
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
            if (new_buffer_size > 1073741824) {
                new_buffer_size = 1073741824;
            }
            
            output = (uint8_t *)repalloc(output, new_buffer_size);
            buffer_size = new_buffer_size;
            next_out = output + current_position;
            available_out = buffer_size - current_position;
        } else if (decode_result == BROTLI_DECODER_RESULT_ERROR) {
            BrotliDecoderErrorCode err_code = BrotliDecoderGetErrorCode(decoder);
            const char *err_str = BrotliDecoderErrorString(err_code);
            BrotliDecoderDestroyInstance(decoder);
            pfree(output);
            ereport(ERROR,
                    (errcode(ERRCODE_DATA_CORRUPTED),
                     errmsg("Brotli decompression error: %s", err_str)));
        }
    } while (decode_result != BROTLI_DECODER_RESULT_SUCCESS);
    
    BrotliDecoderDestroyInstance(decoder);
    
    /* Convert to PostgreSQL text type */
    result = (text *)palloc(total_out + VARHDRSZ);
    SET_VARSIZE(result, total_out + VARHDRSZ);
    memcpy(VARDATA(result), output, total_out);
    
    pfree(output);
    
    PG_RETURN_TEXT_P(result);
}