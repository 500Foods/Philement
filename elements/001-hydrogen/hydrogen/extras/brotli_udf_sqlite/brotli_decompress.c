/*
 * SQLite Loadable Extension for Brotli Decompression
 * 
 * This extension decompresses Brotli-compressed data for use in the Helium migration system.
 * Leverages the Brotli decode library used elsewhere in the Hydrogen project.
 *
 * Build: See Makefile
 * Load: SELECT load_extension('brotli_decompress');
 */

#include <sqlite3ext.h>

SQLITE_EXTENSION_INIT1

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <brotli/decode.h>

/*
 * brotli_decompress(compressed_blob)
 *   Decompress Brotli-compressed blob and return as text
 */
static void brotli_decompress_func(
    sqlite3_context *context,
    int argc,
    sqlite3_value **argv
){
    const uint8_t *compressed_data;
    size_t compressed_size;
    BrotliDecoderState *decoder;
    uint8_t *output;
    size_t buffer_size;
    const uint8_t *next_in;
    size_t available_in;
    uint8_t *next_out;
    size_t available_out;
    size_t total_out = 0;
    BrotliDecoderResult result;
    
    /* Validate argument count */
    if (argc != 1) {
        sqlite3_result_error(context, "BROTLI_DECOMPRESS requires exactly 1 argument", -1);
        return;
    }
    
    /* Handle NULL input */
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
        sqlite3_result_null(context);
        return;
    }
    
    /* Get compressed data */
    compressed_data = (const uint8_t *)sqlite3_value_blob(argv[0]);
    compressed_size = (size_t)sqlite3_value_bytes(argv[0]);
    
    if (compressed_size == 0) {
        sqlite3_result_text(context, "", 0, SQLITE_STATIC);
        return;
    }
    
    /* Create Brotli decoder */
    decoder = BrotliDecoderCreateInstance(NULL, NULL, NULL);
    if (!decoder) {
        sqlite3_result_error(context, "Failed to create Brotli decoder", -1);
        return;
    }
    
    /* Initial buffer size (4x compressed size) */
    buffer_size = compressed_size * 4;
    if (buffer_size > 268435456) { /* Limit to 256MB */
        buffer_size = 268435456;
    }
    
    output = (uint8_t *)sqlite3_malloc((int)buffer_size);
    if (!output) {
        BrotliDecoderDestroyInstance(decoder);
        sqlite3_result_error(context, "Memory allocation failed", -1);
        return;
    }
    
    /* Set up decompression parameters */
    next_in = compressed_data;
    available_in = compressed_size;
    next_out = output;
    available_out = buffer_size;
    
    /* Decompress the data */
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
            if (new_buffer_size > 268435456) {
                new_buffer_size = 268435456;
            }
            
            uint8_t *new_buffer = (uint8_t *)sqlite3_realloc(output, (int)new_buffer_size);
            if (!new_buffer) {
                sqlite3_free(output);
                BrotliDecoderDestroyInstance(decoder);
                sqlite3_result_error(context, "Memory reallocation failed", -1);
                return;
            }
            
            output = new_buffer;
            buffer_size = new_buffer_size;
            next_out = output + current_position;
            available_out = buffer_size - current_position;
        } else if (result == BROTLI_DECODER_RESULT_ERROR) {
            BrotliDecoderErrorCode err_code = BrotliDecoderGetErrorCode(decoder);
            const char *err_str = BrotliDecoderErrorString(err_code);
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg), "Brotli decompression error: %s", err_str);
            
            sqlite3_free(output);
            BrotliDecoderDestroyInstance(decoder);
            sqlite3_result_error(context, error_msg, -1);
            return;
        }
    } while (result != BROTLI_DECODER_RESULT_SUCCESS);
    
    BrotliDecoderDestroyInstance(decoder);
    
    /* Return decompressed text */
    sqlite3_result_text(context, (const char *)output, (int)total_out, sqlite3_free);
}

/*
 * Extension initialization function
 */
#ifdef _WIN32
__declspec(dllexport)
#endif
int sqlite3_brotlidecompress_init(
    sqlite3 *db,
    char **pzErrMsg,
    const sqlite3_api_routines *pApi
){
    int rc = SQLITE_OK;
    SQLITE_EXTENSION_INIT2(pApi);
    
    (void)pzErrMsg; /* Unused */
    
    /* Register the brotli_decompress function */
    rc = sqlite3_create_function(
        db,
        "BROTLI_DECOMPRESS",  /* Function name */
        1,                     /* Number of arguments */
        SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_INNOCUOUS,
        NULL,                  /* User data */
        brotli_decompress_func,  /* Implementation */
        NULL,                  /* xStep (not aggregate) */
        NULL                   /* xFinal (not aggregate) */
    );
    
    return rc;
}