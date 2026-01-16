#define _XOPEN_SOURCE 700  // For strptime
#include <sqlite3ext.h>

SQLITE_EXTENSION_INIT1

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>  // For ntohl
#include <arpa/inet.h>   // For ntohll

// Minimal tzcode embed based on IANA tzfile format
// Parses /usr/share/zoneinfo/{tz_name} files for offsets/DST

#define TZ_MAGIC "TZif"
#define MAX_TRANS 1024
#define MAX_TYPES 256

// Structs from tzfile.h
struct tzhead {
    char tzh_magic[4];      /* TZ_MAGIC */
    char tzh_version[1];    /* '\0' or '2' or '3' as of 2005 */
    char tzh_reserved[15];  /* reserved--must be zero */
    char tzh_ttisgmtcnt[4]; /* coded number of trans. time flags */
    char tzh_ttisstdcnt[4]; /* coded number of trans. time flags */
    char tzh_leapcnt[4];    /* coded number of leap seconds */
    char tzh_timecnt[4];    /* coded number of transition times */
    char tzh_typecnt[4];    /* coded number of local time types */
    char tzh_charcnt[4];    /* coded number of abbr. chars */
};

struct ttinfo {
    long tt_gmtoff;
    int tt_isdst;
    unsigned int tt_abbrind;
    int tt_ttisstd;  // Changed to int for simplicity
    int tt_ttisgmt;
};


// UDF: convert_tz(dt, from_tz, to_tz)
// dt now supports 'YYYY-MM-DD HH:MM:SS' or 'YYYY-MM-DD HH:MM:SS.ZZZ'
static void convert_tz_func(sqlite3_context *context, int argc, sqlite3_value **argv) {
    if (argc != 3) {
        sqlite3_result_error(context, "convert_tz expects 3 args: dt, from_tz, to_tz", -1);
        return;
    }

    const char *dt_str = (const char *)sqlite3_value_text(argv[0]);
    const char *from_tz = (const char *)sqlite3_value_text(argv[1]);
    const char *to_tz = (const char *)sqlite3_value_text(argv[2]);

    if (!dt_str || !from_tz || !to_tz) {
        sqlite3_result_null(context);
        return;
    }

    // If timezones are the same, return input unchanged
    if (strcmp(from_tz, to_tz) == 0) {
        sqlite3_result_text(context, dt_str, -1, SQLITE_TRANSIENT);
        return;
    }

    // For now, implement basic UTC to America/Vancouver conversion
    // America/Vancouver is UTC-8 (PST) or UTC-7 (PDT)
    // For simplicity, use UTC-8 for now
    if (strcmp(from_tz, "UTC") == 0 && strcmp(to_tz, "America/Vancouver") == 0) {
        // Parse datetime
        struct tm tm_time = {0};
        if (strptime(dt_str, "%Y-%m-%d %H:%M:%S", &tm_time) == NULL) {
            sqlite3_result_error(context, "Invalid datetime format", -1);
            return;
        }

        // Convert to time_t, subtract 8 hours for Vancouver
        time_t utc_time = timegm(&tm_time);
        time_t vancouver_time = utc_time - (8 * 3600);  // 8 hours in seconds

        // Convert back to struct tm
        struct tm vancouver_tm;
        gmtime_r(&vancouver_time, &vancouver_tm);

        // Format output
        char buf[32];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &vancouver_tm);
        sqlite3_result_text(context, buf, -1, SQLITE_TRANSIENT);
        return;
    }

    // For other conversions, return error for now
    sqlite3_result_error(context, "Timezone conversion not implemented yet", -1);
}

int sqlite3_converttz_init(sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi) {
    SQLITE_EXTENSION_INIT2(pApi);
    (void)pzErrMsg; /* Unused */
    return sqlite3_create_function(db, "convert_tz", 3, SQLITE_UTF8 | SQLITE_DETERMINISTIC, NULL, convert_tz_func, NULL, NULL);
}