#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sqludf.h>

/* Base64 map */
static int b64_idx[256];
static void b64_init(void){
    static int inited = 0;
    if (inited) return;
    for (int i=0;i<256;++i) b64_idx[i] = -1;
    for (int c='A'; c<='Z'; ++c) b64_idx[c] = c - 'A';
    for (int c='a'; c<='z'; ++c) b64_idx[c] = c - 'a' + 26;
    for (int c='0'; c<='9'; ++c) b64_idx[c] = c - '0' + 52;
    b64_idx[(int)'+'] = 62; b64_idx[(int)'/'] = 63;
    inited = 1;
}

static size_t b64_decode_block(const char* in, size_t inlen, unsigned char* out){
    size_t o = 0;
    for (size_t i=0;i<inlen;i+=4){
        int a=b64_idx[(unsigned char)in[i]];
        int b=b64_idx[(unsigned char)in[i+1]];
        int c=(in[i+2]=='=')?-1:b64_idx[(unsigned char)in[i+2]];
        int d=(in[i+3]=='=')?-1:b64_idx[(unsigned char)in[i+3]];
        if (a<0 || b<0) return 0;
        unsigned int triple=(a<<18)|(b<<12)|((c<0?0:c)<<6)|(d<0?0:d);
        out[o++]=(triple>>16)&0xFF;
        if (c>=0){
            out[o++]=(triple>>8)&0xFF;
            if (d>=0) out[o++]=triple&0xFF;
        }
    }
    return o;
}

/* DB2SQL parameter style: VARCHAR(32672) -> VARCHAR(32672)
   Parameter order: inputs, outputs, null indicators, trail args */
#ifdef __cplusplus
extern "C"
#endif
void SQL_API_FN BASE64_DECODE_CHUNK(
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
    
    b64_init();

    /* Get input length and filter to legal base64 chars */
    size_t inlen = strlen(in_str);
    if (inlen == 0) return;
    
    const unsigned char *src = (const unsigned char*)in_str;
    char *work = (char*)malloc(inlen + 4);
    if (!work){
        strcpy(SQLUDF_STATE, "UDF02");
        strcpy(SQLUDF_MSGTX, "Memory allocation failed");
        *out_ind=-1; return;
    }

    size_t eff=0;
    for (size_t i=0;i<inlen;++i){
        unsigned char c = src[i];
        if (c=='=' || b64_idx[c]>=0) work[eff++] = (char)c;
    }

    /* Decode only full quartets */
    size_t full = (eff/4)*4;
    size_t wrote = 0;
    if (full > 0){
        wrote = b64_decode_block(work, full, (unsigned char*)out_str);
        if (wrote == 0){
            strcpy(SQLUDF_STATE, "UDF01");
            strcpy(SQLUDF_MSGTX, "Invalid base64 input");
            free(work); *out_ind=-1; return;
        }
    }

    /* Null-terminate the output */
    out_str[wrote] = '\0';
    free(work);
}

/* Copy of official DB2 ScalarUDF sample for testing */
#ifdef __cplusplus
extern "C"
#endif
void SQL_API_FN ScalarUDF(const SQLUDF_CHAR *inJob,
                          const SQLUDF_DOUBLE *inSalary,
                          SQLUDF_DOUBLE *outNewSalary,
                          const SQLUDF_SMALLINT *jobNullInd,
                          const SQLUDF_SMALLINT *salaryNullInd,
                          SQLUDF_SMALLINT *newSalaryNullInd,
                          SQLUDF_TRAIL_ARGS)
{
  if (*jobNullInd == -1 || *salaryNullInd == -1)
  {
    *newSalaryNullInd = -1;
  }
  else
  {
    if (strcmp(inJob, "Mgr  ") == 0)
    {
      *outNewSalary = *inSalary * 1.20;
    }
    else if (strcmp(inJob, "Sales") == 0)
    {
      *outNewSalary = *inSalary * 1.10;
    }
    else /* it is clerk */
    {
      *outNewSalary = *inSalary * 1.05;
    }
    *newSalaryNullInd = 0;
  }
}

/* Simple test UDF: returns "Hydrogen" */
#ifdef __cplusplus
extern "C"
#endif
void SQL_API_FN HYDROGEN_CHECK(
    SQLUDF_VARCHAR *out_str,
    SQLUDF_NULLIND *out_ind,
    SQLUDF_TRAIL_ARGS
){
    /* Initialize SQLSTATE to success */
    strcpy(SQLUDF_STATE, "00000");

    /* Return the string "Hydrogen" */
    strcpy(out_str, "Hydrogen");
    *out_ind = 0;
}

/* DB2SQL parameter style: VARCHAR(32672) -> BLOB(32672)
   Parameter order: inputs, outputs, null indicators, trail args
   Returns BLOB for binary data (preserves null bytes) */
#ifdef __cplusplus
extern "C"
#endif
void SQL_API_FN BASE64_DECODE_CHUNK_BINARY(
    const SQLUDF_VARCHAR *in_str,    /* Input parameter */
    SQLUDF_BLOB *out_blob,           /* Output parameter - binary data */
    const SQLUDF_NULLIND *in_ind,    /* Input null indicator */
    SQLUDF_NULLIND *out_ind,         /* Output null indicator */
    SQLUDF_TRAIL_ARGS
){
    /* Initialize SQLSTATE to success */
    strcpy(SQLUDF_STATE, "00000");
    
    if (*in_ind == -1){ *out_ind = -1; return; }
    *out_ind = 0;
    
    /* Initialize output */
    out_blob->length = 0;
    
    b64_init();

    /* Get input length and filter to legal base64 chars */
    size_t inlen = strlen(in_str);
    if (inlen == 0) return;
    
    const unsigned char *src = (const unsigned char*)in_str;
    char *work = (char*)malloc(inlen + 4);
    if (!work){
        strcpy(SQLUDF_STATE, "UDF02");
        strcpy(SQLUDF_MSGTX, "Memory allocation failed");
        *out_ind=-1; return;
    }

    size_t eff=0;
    for (size_t i=0;i<inlen;++i){
        unsigned char c = src[i];
        if (c=='=' || b64_idx[c]>=0) work[eff++] = (char)c;
    }

    /* Decode only full quartets */
    size_t full = (eff/4)*4;
    size_t wrote = 0;
    if (full > 0){
        wrote = b64_decode_block(work, full, (unsigned char*)out_blob->data);
        if (wrote == 0){
            strcpy(SQLUDF_STATE, "UDF01");
            strcpy(SQLUDF_MSGTX, "Invalid base64 input");
            free(work); *out_ind=-1; return;
        }
    }

    /* Set output length (binary data may contain null bytes) */
    out_blob->length = wrote;
    free(work);
}
