#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sqludf.h>
#include <brotli/decode.h>

/* DB2 UDF for Brotli decompression
 * Modeled after base64_chunk_udf.c
 * Processes data in chunks due to VARCHAR(32672) limit
 */

/* Chunk-based Brotli decompression function
 * Input: VARCHAR(32672) containing compressed data chunk
 * Output: VARCHAR(32672) containing decompressed data chunk
 */
#ifdef __cplusplus
extern "C"
#endif
void SQL_API_FN BROTLI_DECOMPRESS_CHUNK(
    const SQLUDF_VARCHAR *compressed_chunk,  /* Input parameter */
    SQLUDF_VARCHAR *decompressed_chunk,      /* Output parameter */
    const SQLUDF_NULLIND *in_ind,            /* Input null indicator */
    SQLUDF_NULLIND *out_ind,                 /* Output null indicator */
    SQLUDF_TRAIL_ARGS
){
    /* Initialize SQLSTATE to success */
    strcpy(SQLUDF_STATE, "00000");
    
    if (*in_ind == -1) { 
        *out_ind = -1; 
        return; 
    }
    *out_ind = 0;
    
    /* Initialize output */
    decompressed_chunk[0] = '\0';
    
    /* Get input length */
    size_t compressed_size = strlen(compressed_chunk);
    if (compressed_size == 0) {
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
    
    /* Allocate output buffer (4x compressed size as initial estimate) */
    size_t buffer_size = compressed_size * 4;
    if (buffer_size > 32672) {
        buffer_size = 32672;  /* Limit to VARCHAR max */
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
    const uint8_t *next_in = (const uint8_t *)compressed_chunk;
    size_t available_in = compressed_size;
    uint8_t *next_out = output;
    size_t available_out = buffer_size;
    size_t total_out = 0;
    
    /* Decompress the data */
    BrotliDecoderResult result = BrotliDecoderDecompressStream(
        decoder,
        &available_in, &next_in,
        &available_out, &next_out,
        &total_out
    );
    
    /* Handle decompression result */
    if (result == BROTLI_DECODER_RESULT_ERROR) {
        strcpy(SQLUDF_STATE, "UDF03");
        strcpy(SQLUDF_MSGTX, "Brotli decompression error");
        free(output);
        BrotliDecoderDestroyInstance(decoder);
        *out_ind = -1;
        return;
    }
    
    /* Copy decompressed data to output parameter */
    if (total_out > 0 && total_out < 32672) {
        memcpy(decompressed_chunk, output, total_out);
        decompressed_chunk[total_out] = '\0';
    } else if (total_out >= 32672) {
        /* Truncate if decompressed data exceeds VARCHAR limit */
        memcpy(decompressed_chunk, output, 32671);
        decompressed_chunk[32671] = '\0';
    }
    
    free(output);
    BrotliDecoderDestroyInstance(decoder);
}

/* Simple test UDF: returns "Helium-Brotli" */
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

    /* Return the string "Helium-Brotli" */
    strcpy(out_str, "Helium-Brotli");
    *out_ind = 0;
}
