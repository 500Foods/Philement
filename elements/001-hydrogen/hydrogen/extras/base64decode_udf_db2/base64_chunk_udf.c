#include <string.h>
#include <stdlib.h>
#include "sqludf.h"

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

/* DB2SQL parameter style: VARCHAR(32672) -> VARCHAR(32672) */
void SQL_API_FN BASE64_DECODE_CHUNK(
    SQLUDF_VARCHAR *in_str,   SQLUDF_INTEGER *in_len,   SQLUDF_NULLIND *in_ind,
    SQLUDF_VARCHAR *out_str,  SQLUDF_INTEGER *out_len,  SQLUDF_NULLIND *out_ind,
    SQLUDF_TRAIL_ARGS
){
    if (*in_ind == -1){ *out_ind = -1; *out_len = 0; return; }
    *out_ind = 0;

    /* Out buffer capacity that Db2 actually provided */
    size_t outcap = (*out_len > 0) ? (size_t)(*out_len) : 0;
    if (outcap == 0){
        /* No space to write anything */
        *out_len = 0;
        return;
    }

    b64_init();

    /* Filter input to legal base64 chars */
    size_t inlen = (size_t)(*in_len);
    const unsigned char *src = (const unsigned char*)in_str;

    char *work = (char*)malloc(inlen + 4);
    if (!work){ strcpy(sqludf_sqlstate,"38MEM"); *out_ind=-1; *out_len=0; return; }

    size_t eff=0;
    for (size_t i=0;i<inlen;++i){
        unsigned char c = src[i];
        if (c=='=' || b64_idx[c]>=0) work[eff++] = (char)c;
    }

    /* Decode only full quartets; clamp so decoded bytes <= outcap */
    size_t full = (eff/4)*4;
    size_t dec_bytes = (full/4)*3;         /* exact max decoded for 'full' */
    if (dec_bytes > outcap){
        full      = (outcap/3)*4;          /* largest multiple-of-4 that fits */
        dec_bytes = (full/4)*3;
    }

    size_t wrote = 0;
    if (full){
        wrote = b64_decode_block(work, full, (unsigned char*)out_str);
        if (wrote == 0 && full>0){
            free(work); strcpy(sqludf_sqlstate,"38DEC"); *out_ind=-1; *out_len=0; return;
        }
    }

    free(work);
    *out_len = (SQLUDF_INTEGER)wrote;      /* tell Db2 how many bytes we set */
}
