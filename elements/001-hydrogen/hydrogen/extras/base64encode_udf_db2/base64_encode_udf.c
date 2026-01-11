#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sqludf.h>

/* Base64 encoding alphabet */
static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* Encode block of bytes to base64 */
static size_t b64_encode_block(const unsigned char* in, size_t inlen, char* out){
    size_t o = 0;
    for (size_t i = 0; i < inlen; i += 3){
        unsigned int octet_a = i < inlen ? in[i] : 0;
        unsigned int octet_b = i + 1 < inlen ? in[i + 1] : 0;
        unsigned int octet_c = i + 2 < inlen ? in[i + 2] : 0;
        
        unsigned int triple = (octet_a << 16) | (octet_b << 8) | octet_c;
        
        out[o++] = b64_table[(triple >> 18) & 0x3F];
        out[o++] = b64_table[(triple >> 12) & 0x3F];
        out[o++] = (i + 1 < inlen) ? b64_table[(triple >> 6) & 0x3F] : '=';
        out[o++] = (i + 2 < inlen) ? b64_table[triple & 0x3F] : '=';
    }
    return o;
}

/* DB2SQL parameter style: VARCHAR(32672) -> VARCHAR(32672)
   Parameter order: inputs, outputs, null indicators, trail args */
#ifdef __cplusplus
extern "C"
#endif
void SQL_API_FN BASE64_ENCODE_CHUNK(
    const SQLUDF_VARCHAR *in_str,    /* Input parameter */
    SQLUDF_VARCHAR *out_str,         /* Output parameter */
    const SQLUDF_NULLIND *in_ind,    /* Input null indicator */
    SQLUDF_NULLIND *out_ind,         /* Output null indicator */
    SQLUDF_TRAIL_ARGS
){
    /* Initialize SQLSTATE to success */
    strcpy(SQLUDF_STATE, "00000");
    
    if (*in_ind == -1){ *out_ind = -1; return; }
    *out_ind = 0;
    
    /* Initialize output */
    out_str[0] = '\0';
    
    /* Get input length - DB2 VARCHAR is null-terminated for DB2SQL parameter style */
    size_t inlen = strlen(in_str);
    if (inlen == 0) return;
    
    /* Sanity check: max input size for VARCHAR(32672) that produces
       base64 output fitting in VARCHAR(32672) is about 24500 bytes.
       For inputs > 24500, the output would exceed 32671 chars. */
    if (inlen > 24500){
        strcpy(SQLUDF_STATE, "UDF03");
        strcpy(SQLUDF_MSGTX, "Input exceeds 24500 byte limit");
        *out_ind = -1;
        return;
    }
    
    /* Encode the data */
    size_t wrote = b64_encode_block((const unsigned char*)in_str, inlen, out_str);
    
    /* Null-terminate the output */
    out_str[wrote] = '\0';
}

/* Binary version for handling binary data (BLOB input) */
#ifdef __cplusplus
extern "C"
#endif
void SQL_API_FN BASE64_ENCODE_CHUNK_BINARY(
    const SQLUDF_BLOB *in_blob,      /* Input parameter - binary data */
    SQLUDF_VARCHAR *out_str,         /* Output parameter */
    const SQLUDF_NULLIND *in_ind,    /* Input null indicator */
    SQLUDF_NULLIND *out_ind,         /* Output null indicator */
    SQLUDF_TRAIL_ARGS
){
    /* Initialize SQLSTATE to success */
    strcpy(SQLUDF_STATE, "00000");
    
    if (*in_ind == -1){ *out_ind = -1; return; }
    *out_ind = 0;
    
    /* Initialize output */
    out_str[0] = '\0';
    
    /* Get input length from BLOB */
    size_t inlen = in_blob->length;
    if (inlen == 0) return;
    
    /* Max input that produces output <= 32671 bytes is ~24500 bytes */
    if (inlen > 24500){
        strcpy(SQLUDF_STATE, "UDF03");
        strcpy(SQLUDF_MSGTX, "Input too large to encode");
        *out_ind = -1;
        return;
    }
    
    /* Encode the binary data */
    size_t wrote = b64_encode_block((const unsigned char*)in_blob->data, inlen, out_str);
    
    /* Null-terminate the output */
    out_str[wrote] = '\0';
}
