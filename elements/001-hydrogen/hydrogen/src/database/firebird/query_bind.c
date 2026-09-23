/*
 * Firebird query inputs: typed parameter binding and DATEADD(- ?) rewrite.
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
#include <time.h>

/* String payload for text-ish TypedParameter values (NULL if none). */
const char* firebird_param_text_value(const TypedParameter* param) {
    if (!param || param->is_null) {
        return NULL;
    }
    switch (param->type) {
        case PARAM_TYPE_STRING:
            return param->value.string_value ? param->value.string_value : "";
        case PARAM_TYPE_TEXT:
            return param->value.text_value ? param->value.text_value : "";
        case PARAM_TYPE_DATE:
            return param->value.date_value ? param->value.date_value : "";
        case PARAM_TYPE_TIME:
            return param->value.time_value ? param->value.time_value : "";
        case PARAM_TYPE_DATETIME:
            return param->value.datetime_value ? param->value.datetime_value : "";
        case PARAM_TYPE_TIMESTAMP:
            return param->value.timestamp_value ? param->value.timestamp_value : "";
        case PARAM_TYPE_INTEGER:
        case PARAM_TYPE_BOOLEAN:
        case PARAM_TYPE_FLOAT:
            /* Numeric types use typed union fields in firebird_fill_input_var. */
            return NULL;
    }
    return NULL;
}

/* Hundred-microsecond ticks (ISC_TIME units) from a fractional-second digit string. */
unsigned int firebird_fraction_ticks(const char* frac) {
    char pad[5] = {'0', '0', '0', '0', '\0'};
    int n = 0;
    if (!frac) {
        return 0;
    }
    while (n < 4 && frac[n] >= '0' && frac[n] <= '9') {
        pad[n] = frac[n];
        n++;
    }
    return (unsigned int)strtoul(pad, NULL, 10);
}

/*
 * Returns 1 for a date, 2 for a time, 3 for a date and time, 0 if unparseable.
 * tm_out is a struct tm. frac_ticks is the sub-second part in ISC_TIME units.
 */
int firebird_parse_clock(const char* text, void* tm_out, unsigned int* frac_ticks) {
    struct tm* tm_in = (struct tm*)tm_out;
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;
    char frac[16];
    int count;

    if (!text || !tm_in || !frac_ticks) {
        return 0;
    }
    memset(tm_in, 0, sizeof(*tm_in));
    *frac_ticks = 0;
    frac[0] = '\0';

    count = sscanf(text, "%d-%d-%d %d:%d:%d.%15[0-9]",
                   &year, &month, &day, &hour, &minute, &second, frac);
    if (count < 6) {
        frac[0] = '\0';
        count = sscanf(text, "%d-%d-%dT%d:%d:%d.%15[0-9]",
                       &year, &month, &day, &hour, &minute, &second, frac);
    }
    if (count >= 6) {
        if (year < 1 || month < 1 || month > 12 || day < 1 || day > 31 ||
            hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 60) {
            return 0;
        }
        tm_in->tm_year = year - 1900;
        tm_in->tm_mon = month - 1;
        tm_in->tm_mday = day;
        tm_in->tm_hour = hour;
        tm_in->tm_min = minute;
        tm_in->tm_sec = second;
        if (frac[0]) {
            *frac_ticks = firebird_fraction_ticks(frac);
        }
        return 3;
    }

    count = sscanf(text, "%d-%d-%d", &year, &month, &day);
    if (count == 3) {
        if (year < 1 || month < 1 || month > 12 || day < 1 || day > 31) {
            return 0;
        }
        tm_in->tm_year = year - 1900;
        tm_in->tm_mon = month - 1;
        tm_in->tm_mday = day;
        return 1;
    }

    frac[0] = '\0';
    count = sscanf(text, "%d:%d:%d.%15[0-9]", &hour, &minute, &second, frac);
    if (count >= 3) {
        if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 60) {
            return 0;
        }
        tm_in->tm_hour = hour;
        tm_in->tm_min = minute;
        tm_in->tm_sec = second;
        if (frac[0]) {
            *frac_ticks = firebird_fraction_ticks(frac);
        }
        return 2;
    }
    return 0;
}

/* Write a DATE / TIME / TIMESTAMP(/TZ) input from text. Buffers are already sized. */
bool firebird_bind_temporal(fb_xsqlvar_min* var, const char* text) {
    struct tm tm_in;
    unsigned int frac = 0;
    int kind;
    short typ;
    int date_days = 0;
    unsigned int time_ticks = 0;

    if (!var || !var->sqldata || !text) {
        return false;
    }
    kind = firebird_parse_clock(text, &tm_in, &frac);
    if (!kind) {
        return false;
    }
    typ = (short)(var->sqltype & ~1);

    if (typ == FB_SQL_TYPE_DATE) {
        if (kind == 2 || !isc_encode_sql_date_ptr || var->sqllen < 4) {
            return false;
        }
        isc_encode_sql_date_ptr(&tm_in, &date_days);
        memcpy(var->sqldata, &date_days, 4);
        return true;
    }

    if (typ == FB_SQL_TYPE_TIME || typ == FB_SQL_TIME_TZ || typ == FB_SQL_TIME_TZ_EX) {
        if (kind == 1 || !isc_encode_sql_time_ptr || var->sqllen < 4) {
            return false;
        }
        isc_encode_sql_time_ptr(&tm_in, &time_ticks);
        time_ticks += frac;
        memcpy(var->sqldata, &time_ticks, 4);
        if (typ != FB_SQL_TYPE_TIME) {
            unsigned short zone = (unsigned short)FB_TZID_GMT;
            if (var->sqllen < 6) {
                return false;
            }
            memcpy(var->sqldata + 4, &zone, 2);
        }
        return true;
    }

    if (typ == FB_SQL_TIMESTAMP || typ == FB_SQL_TIMESTAMP_TZ || typ == FB_SQL_TIMESTAMP_TZ_EX) {
        char tsbuf[8];
        if (kind == 2 || !isc_encode_timestamp_ptr || var->sqllen < 8) {
            return false;
        }
        memset(tsbuf, 0, sizeof(tsbuf));
        isc_encode_timestamp_ptr(&tm_in, tsbuf);
        memcpy(&time_ticks, tsbuf + 4, 4);
        time_ticks += frac;
        memcpy(tsbuf + 4, &time_ticks, 4);
        memcpy(var->sqldata, tsbuf, 8);
        if (typ != FB_SQL_TIMESTAMP) {
            unsigned short zone = (unsigned short)FB_TZID_GMT;
            if (var->sqllen < 10) {
                return false;
            }
            memcpy(var->sqldata + 8, &zone, 2);
        }
        return true;
    }
    return false;
}

/* Fill one input XSQLVAR from a TypedParameter (buffers already allocated). */
bool firebird_fill_input_var(fb_xsqlvar_min* var, const TypedParameter* param) {
    if (!var || !param || !var->sqldata || !var->sqlind) {
        return false;
    }
    if (param->is_null) {
        *var->sqlind = -1;
        return true;
    }
    *var->sqlind = 0;

    short typ = (short)(var->sqltype & ~1);
    short len = var->sqllen;
    if (len < 1) {
        len = 1;
    }

    if (typ == FB_SQL_VARYING || typ == FB_SQL_TEXT) {
        char numbuf[64];
        const char* s = firebird_param_text_value(param);
        if (!s) {
            if (param->type == PARAM_TYPE_INTEGER) {
                snprintf(numbuf, sizeof(numbuf), "%lld", param->value.int_value);
                s = numbuf;
            } else if (param->type == PARAM_TYPE_BOOLEAN) {
                snprintf(numbuf, sizeof(numbuf), "%d", param->value.bool_value ? 1 : 0);
                s = numbuf;
            } else if (param->type == PARAM_TYPE_FLOAT) {
                snprintf(numbuf, sizeof(numbuf), "%.17g", param->value.float_value);
                s = numbuf;
            } else {
                s = "";
            }
        }
        size_t slen = strlen(s);
        if (typ == FB_SQL_VARYING) {
            short use = (short)(slen > (size_t)len ? (size_t)len : slen);
            memcpy(var->sqldata, &use, sizeof(short));
            memcpy(var->sqldata + sizeof(short), s, (size_t)use);
        } else {
            short use = (short)(slen > (size_t)len ? (size_t)len : slen);
            memcpy(var->sqldata, s, (size_t)use);
            if (use < len) {
                memset(var->sqldata + use, ' ', (size_t)(len - use));
            }
        }
        return true;
    }

    if (typ == FB_SQL_LONG) {
        int v;
        if (param->type == PARAM_TYPE_INTEGER) {
            v = (int)param->value.int_value;
        } else if (param->type == PARAM_TYPE_BOOLEAN) {
            v = param->value.bool_value ? 1 : 0;
        } else {
            const char* s = firebird_param_text_value(param);
            v = s ? (int)strtol(s, NULL, 10) : 0;
        }
        memcpy(var->sqldata, &v, sizeof(int));
        return true;
    }

    if (typ == FB_SQL_SHORT) {
        short v;
        if (param->type == PARAM_TYPE_INTEGER) {
            v = (short)param->value.int_value;
        } else if (param->type == PARAM_TYPE_BOOLEAN) {
            v = param->value.bool_value ? 1 : 0;
        } else {
            const char* s = firebird_param_text_value(param);
            v = s ? (short)strtol(s, NULL, 10) : 0;
        }
        memcpy(var->sqldata, &v, sizeof(short));
        return true;
    }

    if (typ == FB_SQL_INT64) {
        long long v;
        if (param->type == PARAM_TYPE_INTEGER) {
            v = param->value.int_value;
        } else if (param->type == PARAM_TYPE_BOOLEAN) {
            v = param->value.bool_value ? 1 : 0;
        } else {
            const char* s = firebird_param_text_value(param);
            v = s ? strtoll(s, NULL, 10) : 0;
        }
        memcpy(var->sqldata, &v, sizeof(long long));
        return true;
    }

    if (typ == FB_SQL_DOUBLE) {
        double v;
        if (param->type == PARAM_TYPE_FLOAT) {
            v = param->value.float_value;
        } else if (param->type == PARAM_TYPE_INTEGER) {
            v = (double)param->value.int_value;
        } else {
            const char* s = firebird_param_text_value(param);
            v = s ? strtod(s, NULL) : 0.0;
        }
        memcpy(var->sqldata, &v, sizeof(double));
        return true;
    }

    if (typ == FB_SQL_FLOAT) {
        float v;
        if (param->type == PARAM_TYPE_FLOAT) {
            v = (float)param->value.float_value;
        } else if (param->type == PARAM_TYPE_INTEGER) {
            v = (float)param->value.int_value;
        } else {
            const char* s = firebird_param_text_value(param);
            v = s ? (float)strtod(s, NULL) : 0.0f;
        }
        memcpy(var->sqldata, &v, sizeof(float));
        return true;
    }

    if (typ == FB_SQL_BOOLEAN) {
        unsigned char bit = 0;
        if (var->sqllen < 1) {
            return false;
        }
        if (param->type == PARAM_TYPE_BOOLEAN) {
            bit = param->value.bool_value ? 1 : 0;
        } else if (param->type == PARAM_TYPE_INTEGER) {
            bit = param->value.int_value ? 1 : 0;
        } else {
            const char* s = firebird_param_text_value(param);
            if (s && (s[0] == '1' || s[0] == 't' || s[0] == 'T' || s[0] == 'y' || s[0] == 'Y')) {
                bit = 1;
            }
        }
        var->sqldata[0] = (char)bit;
        return true;
    }

    if (typ == FB_SQL_TYPE_DATE || typ == FB_SQL_TYPE_TIME ||
        typ == FB_SQL_TIME_TZ || typ == FB_SQL_TIME_TZ_EX ||
        typ == FB_SQL_TIMESTAMP || typ == FB_SQL_TIMESTAMP_TZ ||
        typ == FB_SQL_TIMESTAMP_TZ_EX) {
        const char* s = firebird_param_text_value(param);
        if (!s) {
            return false;
        }
        return firebird_bind_temporal(var, s);
    }

    /* Unsupported input sqltype: leave zeroed buffer (non-null). */
    return true;
}

/*
 * describe_bind + allocate buffers + fill from ordered TypedParameters.
 * On success, *in_sqlda is owned by caller (free buffers then struct).
 */
bool firebird_build_input_sqlda(void** stmt_handle,
                                TypedParameter** ordered_params,
                                size_t ordered_count,
                                fb_xsqlda_min** in_sqlda_out,
                                const char* desig) {
    if (!stmt_handle || !in_sqlda_out || ordered_count == 0) {
        return false;
    }
    *in_sqlda_out = NULL;
    if (!isc_dsql_describe_bind_ptr) {
        log_this(desig, "Firebird isc_dsql_describe_bind unavailable", LOG_LEVEL_ERROR, 0);
        return false;
    }

    fb_xsqlda_min* in_sqlda = firebird_alloc_sqlda(FB_SQLDA_INIT_COLS);
    if (!in_sqlda) {
        return false;
    }

    fb_status_t status[FB_STATUS_LENGTH];
    memset(status, 0, sizeof(status));
    fb_status_t rc = isc_dsql_describe_bind_ptr(status, stmt_handle, 1, in_sqlda);
    if (rc != FB_SQL_SUCCESS && rc != FB_SQL_SUCCESS_INFO) {
        firebird_status_to_error(status, desig);
        free(in_sqlda);
        return false;
    }

    for (short ci = 0; ci < in_sqlda->sqln; ci++) {
        in_sqlda->sqlvar[ci].sqldata = NULL;
        in_sqlda->sqlvar[ci].sqlind = NULL;
    }

    if (in_sqlda->sqld > in_sqlda->sqln) {
        short need = in_sqlda->sqld;
        if (need > 512) {
            free(in_sqlda);
            return false;
        }
        free(in_sqlda);
        in_sqlda = firebird_alloc_sqlda(need);
        if (!in_sqlda) {
            return false;
        }
        memset(status, 0, sizeof(status));
        rc = isc_dsql_describe_bind_ptr(status, stmt_handle, 1, in_sqlda);
        if (rc != FB_SQL_SUCCESS && rc != FB_SQL_SUCCESS_INFO) {
            firebird_status_to_error(status, desig);
            free(in_sqlda);
            return false;
        }
        for (short ci = 0; ci < in_sqlda->sqln; ci++) {
            in_sqlda->sqlvar[ci].sqldata = NULL;
            in_sqlda->sqlvar[ci].sqlind = NULL;
        }
    }

    if ((size_t)in_sqlda->sqld != ordered_count) {
        log_this(desig, "Firebird input bind count mismatch: sqld=%d ordered=%zu", LOG_LEVEL_ERROR, 2,
                 (int)in_sqlda->sqld, ordered_count);
        free(in_sqlda);
        return false;
    }

    if (in_sqlda->sqld > 0) {
        if (!firebird_bind_sqlda_buffers(in_sqlda)) {
            firebird_free_sqlda_buffers(in_sqlda);
            free(in_sqlda);
            return false;
        }
        for (short i = 0; i < in_sqlda->sqld; i++) {
            if (!firebird_fill_input_var(&in_sqlda->sqlvar[i], ordered_params[i])) {
                firebird_free_sqlda_buffers(in_sqlda);
                free(in_sqlda);
                return false;
            }
        }
    }

    *in_sqlda_out = in_sqlda;
    return true;
}

char* firebird_rewrite_dateadd_params(const char* sql, bool* oom) {
    static const char* units[] = {
        "MINUTE", "SECOND", "HOUR", "DAY", "WEEK", "MONTH", "YEAR", NULL
    };
    const char* p;
    const char* copy_from;
    size_t sql_len;
    char* out = NULL;
    size_t out_len = 0;
    size_t out_cap = 0;
    int changed = 0;

    if (oom) {
        *oom = false;
    }
    if (!sql) {
        return NULL;
    }
    sql_len = strlen(sql);
    copy_from = sql;
    p = sql;

    while (*p) {
        const char* q;
        const char* unit_pos = NULL;
        size_t unit_len = 0;
        char sign;
        const char* expr_start;
        const char* expr_end;
        int matched = 0;

        if ((p[0] == 'D' || p[0] == 'd') &&
            (p[1] == 'A' || p[1] == 'a') &&
            (p[2] == 'T' || p[2] == 't') &&
            (p[3] == 'E' || p[3] == 'e') &&
            (p[4] == 'A' || p[4] == 'a') &&
            (p[5] == 'D' || p[5] == 'd') &&
            (p[6] == 'D' || p[6] == 'd')) {
            q = p + 7;
            while (*q == ' ' || *q == '\t' || *q == '\n' || *q == '\r') {
                q++;
            }
            if (*q == '(') {
                q++;
                while (*q == ' ' || *q == '\t' || *q == '\n' || *q == '\r') {
                    q++;
                }
                if (*q == '+' || *q == '-') {
                    sign = *q;
                    q++;
                    while (*q == ' ' || *q == '\t' || *q == '\n' || *q == '\r') {
                        q++;
                    }
                    if (*q == '?') {
                        q++;
                        while (*q == ' ' || *q == '\t' || *q == '\n' || *q == '\r') {
                            q++;
                        }
                        for (int i = 0; units[i]; i++) {
                            size_t n = strlen(units[i]);
                            if (strncasecmp(q, units[i], n) == 0) {
                                unsigned char c = (unsigned char)q[n];
                                if (!c || (!isalnum(c) && c != '_')) {
                                    unit_pos = q;
                                    unit_len = n;
                                    break;
                                }
                            }
                        }
                        if (unit_pos) {
                            q = unit_pos + unit_len;
                            while (*q == ' ' || *q == '\t' || *q == '\n' || *q == '\r') {
                                q++;
                            }
                            if ((q[0] == 'T' || q[0] == 't') &&
                                (q[1] == 'O' || q[1] == 'o')) {
                                unsigned char c = (unsigned char)q[2];
                                if (!c || (!isalnum(c) && c != '_')) {
                                    q += 2;
                                    while (*q == ' ' || *q == '\t' || *q == '\n' || *q == '\r') {
                                        q++;
                                    }
                                    expr_start = q;
                                    int depth = 1;
                                    expr_end = expr_start;
                                    while (*expr_end && depth > 0) {
                                        if (*expr_end == '(') {
                                            depth++;
                                        } else if (*expr_end == ')') {
                                            depth--;
                                            if (depth == 0) {
                                                break;
                                            }
                                        }
                                        expr_end++;
                                    }
                                    if (depth == 0 && *expr_end == ')') {
                                        matched = 1;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        if (!matched) {
            p++;
            continue;
        }

        /* Ensure output buffer and copy prefix up to DATEADD. */
        {
            size_t need_prefix = (size_t)(p - copy_from);
            size_t need_emit = 8 + unit_len + 4 + 1 + 4 + (size_t)(expr_end - expr_start) + 1;
            size_t need = out_len + need_prefix + need_emit + 1;
            if (need > out_cap) {
                size_t nc = out_cap ? out_cap * 2 : (sql_len + 64);
                char* nd;
                while (nc < need) {
                    nc *= 2;
                }
                nd = realloc(out, nc);
                if (!nd) {
                    free(out);
                    if (oom) {
                        *oom = true;
                    }
                    return NULL;
                }
                out = nd;
                out_cap = nc;
            }
            if (need_prefix) {
                memcpy(out + out_len, copy_from, need_prefix);
                out_len += need_prefix;
            }
            /* DATEADD(UNIT, 0 SIGN ?, expr) */
            memcpy(out + out_len, "DATEADD(", 8);
            out_len += 8;
            memcpy(out + out_len, unit_pos, unit_len);
            out_len += unit_len;
            memcpy(out + out_len, ", 0 ", 4);
            out_len += 4;
            out[out_len++] = sign;
            memcpy(out + out_len, " ?, ", 4);
            out_len += 4;
            if (expr_end > expr_start) {
                memcpy(out + out_len, expr_start, (size_t)(expr_end - expr_start));
                out_len += (size_t)(expr_end - expr_start);
            }
            out[out_len++] = ')';
            out[out_len] = '\0';
        }

        changed = 1;
        p = expr_end + 1;
        copy_from = p;
    }

    if (!changed) {
        free(out);
        return NULL;
    }

    /* Append trailing suffix after last rewrite. */
    {
        size_t need_suffix = (size_t)((sql + sql_len) - copy_from);
        size_t need = out_len + need_suffix + 1;
        if (need > out_cap) {
            char* nd = realloc(out, need);
            if (!nd) {
                free(out);
                if (oom) {
                    *oom = true;
                }
                return NULL;
            }
            out = nd;
        }
        if (need_suffix) {
            memcpy(out + out_len, copy_from, need_suffix);
            out_len += need_suffix;
        }
        out[out_len] = '\0';
    }
    return out;
}

/*
 * Already-loaded QueryRefs: Firebird has CHAR_LENGTH, not LENGTH().
 * Dialect 3 rejects an untyped divisor (`numbers / ?`).
 * FLOAT is binary32; CAST(? AS FLOAT) becomes DOUBLE PRECISION so 3.14159
 * matches the other engines. LIMIT n [OFFSET m] becomes FETCH FIRST, which
 * Firebird accepts and LIMIT does not. Returns NULL when sql needs no change.
 */
char* firebird_rewrite_engine_sql(const char* sql, bool* oom) {
    size_t len;
    size_t cap;
    size_t out_len = 0;
    char* out;
    size_t i = 0;
    int changed = 0;
    int state = 0; /* 0 code, 1 line comment, 2 block comment, 3 string */

    if (oom) {
        *oom = false;
    }
    if (!sql) {
        return NULL;
    }
    len = strlen(sql);
    cap = len + 64;
    out = malloc(cap);
    if (!out) {
        if (oom) {
            *oom = true;
        }
        return NULL;
    }

#define FB_REWRITE_GROW(extra) \
    do { \
        if (out_len + (size_t)(extra) + 1 > cap) { \
            size_t nc = cap * 2 + (size_t)(extra); \
            char* nd = realloc(out, nc); \
            if (!nd) { \
                free(out); \
                if (oom) { \
                    *oom = true; \
                } \
                return NULL; \
            } \
            out = nd; \
            cap = nc; \
        } \
    } while (0)

    while (sql[i]) {
        unsigned char c = (unsigned char)sql[i];
        if (state == 1) {
            FB_REWRITE_GROW(1);
            out[out_len++] = (char)c;
            if (c == '\n') {
                state = 0;
            }
            i++;
            continue;
        }
        if (state == 2) {
            if (c == '*' && sql[i + 1] == '/') {
                FB_REWRITE_GROW(2);
                out[out_len++] = '*';
                out[out_len++] = '/';
                i += 2;
                state = 0;
            } else {
                FB_REWRITE_GROW(1);
                out[out_len++] = (char)c;
                i++;
            }
            continue;
        }
        if (state == 3) {
            if (c == '\'' && sql[i + 1] == '\'') {
                FB_REWRITE_GROW(2);
                out[out_len++] = '\'';
                out[out_len++] = '\'';
                i += 2;
                continue;
            }
            FB_REWRITE_GROW(1);
            out[out_len++] = (char)c;
            if (c == '\'') {
                state = 0;
            }
            i++;
            continue;
        }

        if (c == '-' && sql[i + 1] == '-') {
            FB_REWRITE_GROW(2);
            out[out_len++] = '-';
            out[out_len++] = '-';
            i += 2;
            state = 1;
            continue;
        }
        if (c == '/' && sql[i + 1] == '*') {
            FB_REWRITE_GROW(2);
            out[out_len++] = '/';
            out[out_len++] = '*';
            i += 2;
            state = 2;
            continue;
        }
        if (c == '\'') {
            FB_REWRITE_GROW(1);
            out[out_len++] = '\'';
            i++;
            state = 3;
            continue;
        }

        if ((c == 'L' || c == 'l') && strncasecmp(sql + i, "LENGTH", 6) == 0) {
            int boundary = (i == 0) ||
                (!isalnum((unsigned char)sql[i - 1]) && sql[i - 1] != '_');
            if (boundary) {
                size_t j = i + 6;
                while (sql[j] == ' ' || sql[j] == '\t' || sql[j] == '\n' || sql[j] == '\r') {
                    j++;
                }
                if (sql[j] == '(') {
                    FB_REWRITE_GROW(11);
                    memcpy(out + out_len, "CHAR_LENGTH", 11);
                    out_len += 11;
                    i += 6;
                    changed = 1;
                    continue;
                }
            }
        }

        if ((c == 'A' || c == 'a') && strncasecmp(sql + i, "AS FLOAT", 8) == 0) {
            int boundary = (i == 0) ||
                (!isalnum((unsigned char)sql[i - 1]) && sql[i - 1] != '_');
            unsigned char next = (unsigned char)sql[i + 8];
            if (boundary && (next == '\0' || (!isalnum(next) && next != '_'))) {
                const char* repl = "AS DOUBLE PRECISION";
                size_t repl_len = strlen(repl);
                FB_REWRITE_GROW(repl_len);
                memcpy(out + out_len, repl, repl_len);
                out_len += repl_len;
                i += 8;
                changed = 1;
                continue;
            }
        }

        /* LIMIT n [OFFSET m] is rejected (token LIMIT). FETCH FIRST is the same limit. */
        if ((c == 'L' || c == 'l') && strncasecmp(sql + i, "LIMIT", 5) == 0) {
            int boundary = (i == 0) ||
                (!isalnum((unsigned char)sql[i - 1]) && sql[i - 1] != '_');
            unsigned char next = (unsigned char)sql[i + 5];
            if (boundary && (next == '\0' || (!isalnum(next) && next != '_'))) {
                size_t j = i + 5;
                size_t num_at;
                size_t num_len;
                char count_txt[10];
                while (sql[j] == ' ' || sql[j] == '\t' || sql[j] == '\n' || sql[j] == '\r') {
                    j++;
                }
                num_at = j;
                while (sql[j] >= '0' && sql[j] <= '9') {
                    j++;
                }
                num_len = j - num_at;
                if (num_len > 0 && num_len < sizeof(count_txt)) {
                    size_t k = j;
                    size_t off_at = 0;
                    size_t off_len = 0;
                    char off_txt[10];
                    char repl[96];
                    int repl_len;
                    while (sql[k] == ' ' || sql[k] == '\t' || sql[k] == '\n' || sql[k] == '\r') {
                        k++;
                    }
                    if (strncasecmp(sql + k, "OFFSET", 6) == 0) {
                        unsigned char off_next = (unsigned char)sql[k + 6];
                        if (off_next == '\0' || (!isalnum(off_next) && off_next != '_')) {
                            k += 6;
                            while (sql[k] == ' ' || sql[k] == '\t' || sql[k] == '\n' || sql[k] == '\r') {
                                k++;
                            }
                            off_at = k;
                            while (sql[k] >= '0' && sql[k] <= '9') {
                                k++;
                            }
                            off_len = k - off_at;
                            if (off_len > 0 && off_len < sizeof(off_txt)) {
                                j = k;
                            } else {
                                off_len = 0;
                            }
                        }
                    }
                    memcpy(count_txt, sql + num_at, num_len);
                    count_txt[num_len] = '\0';
                    if (off_len > 0) {
                        memcpy(off_txt, sql + off_at, off_len);
                        off_txt[off_len] = '\0';
                        repl_len = snprintf(repl, sizeof(repl),
                            "OFFSET %s ROWS FETCH FIRST %s ROWS ONLY", off_txt, count_txt);
                    } else {
                        repl_len = snprintf(repl, sizeof(repl),
                            "FETCH FIRST %s ROWS ONLY", count_txt);
                    }
                    if (repl_len > 0 && (size_t)repl_len < sizeof(repl)) {
                        FB_REWRITE_GROW(repl_len);
                        memcpy(out + out_len, repl, (size_t)repl_len);
                        out_len += (size_t)repl_len;
                        i = j;
                        changed = 1;
                        continue;
                    }
                }
            }
        }

        if (c == '/' && sql[i + 1] != '/' && sql[i + 1] != '*') {
            size_t j = i + 1;
            while (sql[j] == ' ' || sql[j] == '\t' || sql[j] == '\n' || sql[j] == '\r') {
                j++;
            }
            if (sql[j] == '?') {
                const char* repl = "/ CAST(? AS INTEGER)";
                size_t repl_len = strlen(repl);
                FB_REWRITE_GROW(repl_len);
                memcpy(out + out_len, repl, repl_len);
                out_len += repl_len;
                i = j + 1;
                changed = 1;
                continue;
            }
        }

        FB_REWRITE_GROW(1);
        out[out_len++] = (char)c;
        i++;
    }

#undef FB_REWRITE_GROW

    out[out_len] = '\0';
    if (!changed) {
        free(out);
        return NULL;
    }
    return out;
}
