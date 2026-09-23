/*
 * Firebird query helpers shared by query.c, query_result.c, and query_bind.c.
 * XSQLDA layouts mirror ibase.h without including it in the build.
 */

#ifndef DATABASE_ENGINE_FIREBIRD_QUERY_INTERNAL_H
#define DATABASE_ENGINE_FIREBIRD_QUERY_INTERNAL_H

#include "query.h"
#include "types.h"

/* Fetch EOF from isc_dsql_fetch (same numeric value as historical SQLCODE 100). */
#define FB_FETCH_EOF 100

/*
 * Minimal XSQLDA / XSQLVAR layouts (ibase.h not included in the build).
 * Sized for describe with up to FB_SQLDA_INIT_COLS columns, then grown.
 */
#define FB_SQLDA_INIT_COLS 20
#define FB_SQL_LONG 496
#define FB_SQL_SHORT 500
#define FB_SQL_INT64 580
#define FB_SQL_FLOAT 482
#define FB_SQL_DOUBLE 480
#define FB_SQL_VARYING 448
#define FB_SQL_TEXT 452
#define FB_SQL_BLOB 520
#define FB_SQL_TIMESTAMP 510
#define FB_SQL_TYPE_TIME 560
#define FB_SQL_TYPE_DATE 570
#define FB_SQL_TIMESTAMP_TZ 32754
#define FB_SQL_TIME_TZ 32756
#define FB_SQL_TIMESTAMP_TZ_EX 32748
#define FB_SQL_TIME_TZ_EX 32750
#define FB_SQL_BOOLEAN 32764
/* fb_tzid_gmt from Firebird TimeZones.h. Zone id on a TIMESTAMP WITH TIME ZONE. */
#define FB_TZID_GMT 65535

typedef struct {
    short sqltype;
    short sqlscale;
    short sqlsubtype;
    short sqllen;
    char* sqldata;
    short* sqlind;
    short sqlname_length;
    char  sqlname[32];
    short relname_length;
    char  relname[32];
    short ownname_length;
    char  ownname[32];
    short aliasname_length;
    char  aliasname[32];
} fb_xsqlvar_min;

typedef struct {
    short version;
    char  sqldaid[8];
    int   sqldabc;
    short sqln;
    short sqld;
    fb_xsqlvar_min sqlvar[1];
} fb_xsqlda_min;

bool firebird_sql_expects_rows(const char* sql);
short firebird_statement_type(void* stmt_handle, const char* desig);
int firebird_rows_affected(void* stmt_handle);

fb_xsqlda_min* firebird_alloc_sqlda(short cols);
void firebird_free_sqlda_buffers(fb_xsqlda_min* sqlda);
bool firebird_bind_sqlda_buffers(fb_xsqlda_min* sqlda);
bool firebird_json_buffer_append(char** buf, size_t* size, size_t* cap, const char* piece);
bool firebird_json_append_escaped(char** buf, size_t* size, size_t* cap, const char* text);
void firebird_column_label_copy(const fb_xsqlvar_min* var, char* buf, size_t buflen);
bool firebird_append_cell_json(char** buf, size_t* size, size_t* cap, FirebirdConnection* fb_conn, const fb_xsqlvar_min* var);
const char* firebird_param_text_value(const TypedParameter* param);
unsigned int firebird_fraction_ticks(const char* frac);
int firebird_parse_clock(const char* text, void* tm_out, unsigned int* frac_ticks);
bool firebird_bind_temporal(fb_xsqlvar_min* var, const char* text);
bool firebird_fill_input_var(fb_xsqlvar_min* var, const TypedParameter* param);
char* firebird_rewrite_engine_sql(const char* sql, bool* oom);
bool firebird_build_input_sqlda(void** stmt_handle, TypedParameter** ordered_params, size_t ordered_count,
                                fb_xsqlda_min** in_sqlda_out, const char* desig);
char* firebird_read_blob_text(FirebirdConnection* fb_conn, const fb_quad_t* blob_id);

#endif /* DATABASE_ENGINE_FIREBIRD_QUERY_INTERNAL_H */
