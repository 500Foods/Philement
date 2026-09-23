/*
 * Firebird query results: XSQLDA buffers, column JSON, blobs, and temporal values.
 */

#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/database/database_params.h>

#include "types.h"
#include "connection.h"
#include "transaction.h"
#include "utils.h"
#include "query_internal.h"

#include <ctype.h>

fb_xsqlda_min* firebird_alloc_sqlda(short cols) {
    size_t bytes = sizeof(fb_xsqlda_min) + (size_t)(cols > 0 ? cols - 1 : 0) * sizeof(fb_xsqlvar_min);
    fb_xsqlda_min* sqlda = calloc(1, bytes);
    if (!sqlda) {
        return NULL;
    }
    sqlda->version = 1;
    sqlda->sqln = cols;
    sqlda->sqld = 0;
    return sqlda;
}

void firebird_free_sqlda_buffers(fb_xsqlda_min* sqlda) {
    if (!sqlda) {
        return;
    }
    for (short i = 0; i < sqlda->sqln && i < sqlda->sqld; i++) {
        free(sqlda->sqlvar[i].sqldata);
        sqlda->sqlvar[i].sqldata = NULL;
        free(sqlda->sqlvar[i].sqlind);
        sqlda->sqlvar[i].sqlind = NULL;
    }
}

bool firebird_bind_sqlda_buffers(fb_xsqlda_min* sqlda) {
    if (!sqlda) {
        return false;
    }
    for (short i = 0; i < sqlda->sqld; i++) {
        short typ = (short)(sqlda->sqlvar[i].sqltype & ~1);
        short len = sqlda->sqlvar[i].sqllen;
        if (typ == FB_SQL_VARYING) {
            len = (short)(len + 2);
        } else if (typ == FB_SQL_BLOB) {
            len = (short)sizeof(fb_quad_t);
        }
        if (len < 1) {
            len = 64;
        }
        sqlda->sqlvar[i].sqldata = calloc(1, (size_t)len + 1);
        sqlda->sqlvar[i].sqlind = calloc(1, sizeof(short));
        if (!sqlda->sqlvar[i].sqldata || !sqlda->sqlvar[i].sqlind) {
            return false;
        }
    }
    return true;
}

bool firebird_json_buffer_append(char** buf, size_t* size, size_t* cap, const char* piece) {
    size_t piece_len = strlen(piece);
    if (*size + piece_len + 1 > *cap) {
        size_t new_cap = (*cap < 1024) ? 1024 : (*cap * 2);
        while (new_cap < *size + piece_len + 1) {
            new_cap *= 2;
        }
        char* nbuf = realloc(*buf, new_cap);
        if (!nbuf) {
            return false;
        }
        *buf = nbuf;
        *cap = new_cap;
    }
    memcpy(*buf + *size, piece, piece_len);
    *size += piece_len;
    (*buf)[*size] = '\0';
    return true;
}

bool firebird_json_append_escaped(char** buf, size_t* size, size_t* cap, const char* text) {
    if (!text) {
        return firebird_json_buffer_append(buf, size, cap, "null");
    }
    if (!firebird_json_buffer_append(buf, size, cap, "\"")) {
        return false;
    }
    for (const unsigned char* p = (const unsigned char*)text; *p; p++) {
        char tmp[8];
        if (*p == '"' || *p == '\\') {
            tmp[0] = '\\';
            tmp[1] = (char)*p;
            tmp[2] = '\0';
            if (!firebird_json_buffer_append(buf, size, cap, tmp)) {
                return false;
            }
        } else if (*p < 0x20 || *p >= 0x80) {
            snprintf(tmp, sizeof(tmp), "\\u%04x", *p);
            if (!firebird_json_buffer_append(buf, size, cap, tmp)) {
                return false;
            }
        } else {
            tmp[0] = (char)*p;
            tmp[1] = '\0';
            if (!firebird_json_buffer_append(buf, size, cap, tmp)) {
                return false;
            }
        }
    }
    return firebird_json_buffer_append(buf, size, cap, "\"");
}

void firebird_column_label_copy(const fb_xsqlvar_min* var, char* buf, size_t buflen) {
    if (!buf || buflen == 0) {
        return;
    }
    buf[0] = '\0';
    if (!var) {
        return;
    }
    const char* src = NULL;
    short len = 0;
    if (var->aliasname_length > 0) {
        src = var->aliasname;
        len = var->aliasname_length;
    } else if (var->sqlname_length > 0) {
        src = var->sqlname;
        len = var->sqlname_length;
    }
    if (!src || len <= 0) {
        snprintf(buf, buflen, "col");
        return;
    }
    if ((size_t)len >= buflen) {
        len = (short)(buflen - 1);
    }
    memcpy(buf, src, (size_t)len);
    buf[len] = '\0';
    /* Auth/QueryRefs expect lowercase keys (valid_until, system_id, …). */
    for (short i = 0; i < len; i++) {
        if (buf[i] >= 'A' && buf[i] <= 'Z') {
            buf[i] = (char)(buf[i] - 'A' + 'a');
        }
    }
}


char* firebird_read_blob_text(FirebirdConnection* fb_conn, const fb_quad_t* blob_id) {
    if (!fb_conn || !fb_conn->db_handle || !fb_conn->tr_handle || !blob_id) {
        return NULL;
    }
    if (!isc_open_blob2_ptr || !isc_get_segment_ptr || !isc_close_blob_ptr) {
        return NULL;
    }

    fb_status_t status[FB_STATUS_LENGTH];
    void* blob_handle = NULL;
    fb_quad_t id = *blob_id;

    memset(status, 0, sizeof(status));
    fb_status_t rc = isc_open_blob2_ptr(
        status, &fb_conn->db_handle, &fb_conn->tr_handle,
        &blob_handle, &id, 0, NULL);
    if ((rc != FB_SQL_SUCCESS && rc != FB_SQL_SUCCESS_INFO) || !blob_handle) {
        return NULL;
    }

    size_t cap = 4096;
    size_t len = 0;
    char* buf = malloc(cap);
    if (!buf) {
        memset(status, 0, sizeof(status));
        (void)isc_close_blob_ptr(status, &blob_handle);
        return NULL;
    }

    for (;;) {
        char segment[1024];
        unsigned short got = 0;
        memset(status, 0, sizeof(status));
        rc = isc_get_segment_ptr(status, &blob_handle, &got, (unsigned short)sizeof(segment), segment);
        fb_status_t code = status[1];
        if (got > 0) {
            if (len + got + 1 > cap) {
                size_t ncap = cap * 2;
                while (ncap < len + got + 1) {
                    ncap *= 2;
                }
                char* nbuf = realloc(buf, ncap);
                if (!nbuf) {
                    free(buf);
                    memset(status, 0, sizeof(status));
                    (void)isc_close_blob_ptr(status, &blob_handle);
                    return NULL;
                }
                buf = nbuf;
                cap = ncap;
            }
            memcpy(buf + len, segment, got);
            len += got;
        }
        if (code == FB_ISC_SEGSTR_EOF || rc == FB_ISC_SEGSTR_EOF) {
            break;
        }
        if (code != 0 && code != FB_ISC_SEGMENT && rc != FB_SQL_SUCCESS && rc != FB_SQL_SUCCESS_INFO && rc != FB_ISC_SEGMENT) {
            free(buf);
            memset(status, 0, sizeof(status));
            (void)isc_close_blob_ptr(status, &blob_handle);
            return NULL;
        }
        if (got == 0 && code == 0 && rc == 0) {
            break;
        }
    }

    memset(status, 0, sizeof(status));
    (void)isc_close_blob_ptr(status, &blob_handle);
    buf[len] = '\0';
    return buf;
}


/* ISC_TIMESTAMP is date(int32) + time(uint32); TZ variants prefix the same 8 bytes. */
typedef struct {
    int date_days;
    unsigned int time_ticks;
} fb_isc_timestamp;

/* Format DATE/TIME/TIMESTAMP(/TZ) as JSON string via libfbclient isc_decode_*. */
bool firebird_append_temporal_json(char** buf, size_t* size, size_t* cap,
                                          short typ, const char* sqldata, short sqllen) {
    if (!sqldata || sqllen < 4) {
        return firebird_json_buffer_append(buf, size, cap, "null");
    }
    struct tm tm_out;
    memset(&tm_out, 0, sizeof(tm_out));
    char out[64];

    if (typ == FB_SQL_TYPE_DATE) {
        if (!isc_decode_sql_date_ptr) {
            return firebird_json_buffer_append(buf, size, cap, "null");
        }
        isc_decode_sql_date_ptr(sqldata, &tm_out);
        snprintf(out, sizeof(out), "%04d-%02d-%02d",
                 tm_out.tm_year + 1900, tm_out.tm_mon + 1, tm_out.tm_mday);
        return firebird_json_append_escaped(buf, size, cap, out);
    }
    if (typ == FB_SQL_TYPE_TIME || typ == FB_SQL_TIME_TZ || typ == FB_SQL_TIME_TZ_EX) {
        if (!isc_decode_sql_time_ptr) {
            return firebird_json_buffer_append(buf, size, cap, "null");
        }
        isc_decode_sql_time_ptr(sqldata, &tm_out);
        snprintf(out, sizeof(out), "%02d:%02d:%02d",
                 tm_out.tm_hour, tm_out.tm_min, tm_out.tm_sec);
        return firebird_json_append_escaped(buf, size, cap, out);
    }
    if (typ == FB_SQL_TIMESTAMP || typ == FB_SQL_TIMESTAMP_TZ || typ == FB_SQL_TIMESTAMP_TZ_EX) {
        if (sqllen < (short)sizeof(fb_isc_timestamp) || !isc_decode_timestamp_ptr) {
            return firebird_json_buffer_append(buf, size, cap, "null");
        }
        /* TZ types: leading ISC_TIMESTAMP (8 bytes); ignore zone trailer. */
        fb_isc_timestamp ts;
        memcpy(&ts, sqldata, sizeof(ts));
        isc_decode_timestamp_ptr(&ts, &tm_out);
        /* ISC_TIME is 1/10000 sec. Keep milliseconds when present so a
         * timestamp of .123 matches the other engines, and a whole-second
         * datetime stays YYYY-MM-DD HH:MM:SS. */
        {
            unsigned int frac = ts.time_ticks % 10000u;
            int ms = (int)(frac / 10u);
            if (ms > 0) {
                snprintf(out, sizeof(out), "%04d-%02d-%02d %02d:%02d:%02d.%03d",
                         tm_out.tm_year + 1900, tm_out.tm_mon + 1, tm_out.tm_mday,
                         tm_out.tm_hour, tm_out.tm_min, tm_out.tm_sec, ms);
            } else {
                snprintf(out, sizeof(out), "%04d-%02d-%02d %02d:%02d:%02d",
                         tm_out.tm_year + 1900, tm_out.tm_mon + 1, tm_out.tm_mday,
                         tm_out.tm_hour, tm_out.tm_min, tm_out.tm_sec);
            }
        }
        return firebird_json_append_escaped(buf, size, cap, out);
    }
    return firebird_json_buffer_append(buf, size, cap, "null");
}

bool firebird_append_cell_json(char** buf, size_t* size, size_t* cap, FirebirdConnection* fb_conn, const fb_xsqlvar_min* var) {
    if (var->sqlind && *var->sqlind < 0) {
        return firebird_json_buffer_append(buf, size, cap, "null");
    }
    short typ = (short)(var->sqltype & ~1);
    char numbuf[64];
    if (typ == FB_SQL_LONG && var->sqldata) {
        int v = 0;
        memcpy(&v, var->sqldata, sizeof(int) <= (size_t)var->sqllen ? sizeof(int) : (size_t)var->sqllen);
        snprintf(numbuf, sizeof(numbuf), "%d", v);
        return firebird_json_buffer_append(buf, size, cap, numbuf);
    }
    if (typ == FB_SQL_SHORT && var->sqldata) {
        short v = 0;
        memcpy(&v, var->sqldata, sizeof(short));
        snprintf(numbuf, sizeof(numbuf), "%d", (int)v);
        return firebird_json_buffer_append(buf, size, cap, numbuf);
    }
    if (typ == FB_SQL_INT64 && var->sqldata) {
        long long v = 0;
        memcpy(&v, var->sqldata, sizeof(long long) <= (size_t)var->sqllen ? sizeof(long long) : (size_t)var->sqllen);
        snprintf(numbuf, sizeof(numbuf), "%lld", v);
        return firebird_json_buffer_append(buf, size, cap, numbuf);
    }
    if ((typ == FB_SQL_FLOAT || typ == FB_SQL_DOUBLE) && var->sqldata) {
        double v = 0.0;
        if (typ == FB_SQL_FLOAT) {
            float f = 0.0f;
            memcpy(&f, var->sqldata, sizeof(float));
            v = (double)f;
        } else {
            memcpy(&v, var->sqldata, sizeof(double));
        }
        /* DOUBLE uses %.17g so 3.14159 matches the other engines' JSON number. */
        snprintf(numbuf, sizeof(numbuf), typ == FB_SQL_DOUBLE ? "%.17g" : "%.15g", v);
        return firebird_json_buffer_append(buf, size, cap, numbuf);
    }
    if (typ == FB_SQL_VARYING && var->sqldata) {
        unsigned short vlen = 0;
        memcpy(&vlen, var->sqldata, sizeof(unsigned short));
        char* tmp = malloc((size_t)vlen + 1);
        if (!tmp) {
            return false;
        }
        memcpy(tmp, var->sqldata + 2, vlen);
        tmp[vlen] = '\0';
        bool ok = firebird_json_append_escaped(buf, size, cap, tmp);
        free(tmp);
        return ok;
    }
    if (typ == FB_SQL_TEXT && var->sqldata) {
        char* tmp = malloc((size_t)var->sqllen + 1);
        if (!tmp) {
            return false;
        }
        memcpy(tmp, var->sqldata, (size_t)var->sqllen);
        tmp[var->sqllen] = '\0';
        for (int i = var->sqllen - 1; i >= 0 && tmp[i] == ' '; i--) {
            tmp[i] = '\0';
        }
        bool ok = firebird_json_append_escaped(buf, size, cap, tmp);
        free(tmp);
        return ok;
    }
    if (typ == FB_SQL_TIMESTAMP || typ == FB_SQL_TYPE_DATE || typ == FB_SQL_TYPE_TIME ||
        typ == FB_SQL_TIMESTAMP_TZ || typ == FB_SQL_TIME_TZ ||
        typ == FB_SQL_TIMESTAMP_TZ_EX || typ == FB_SQL_TIME_TZ_EX) {
        return firebird_append_temporal_json(buf, size, cap, typ, var->sqldata, var->sqllen);
    }
    if (typ == FB_SQL_BLOB && var->sqldata) {
        fb_quad_t id;
        memcpy(&id, var->sqldata, sizeof(id));
        char* text = firebird_read_blob_text(fb_conn, &id);
        if (!text) {
            return firebird_json_buffer_append(buf, size, cap, "null");
        }
        bool ok = firebird_json_append_escaped(buf, size, cap, text);
        free(text);
        return ok;
    }
    /* Fallback: do not treat opaque sqldata as a C string (blob ids are binary). */
    return firebird_json_buffer_append(buf, size, cap, "null");
}
