/*
 * Firebase NOW / CONVERT_TZ. Clock is injectable for Unity.
 * CONVERT_TZ uses OS /usr/share/zoneinfo (DST-aware). Do not copy the
 * SQLite extra's fixed UTC→America/Vancouver 8-hour stub.
 */

#include <src/hydrogen.h>
#include <src/database/database.h>
#include "types.h"
#include "fns_tz.h"

#include <ctype.h>
#include <pthread.h>
#include <sys/stat.h>

static time_t g_now_override;
static bool g_now_override_set = false;
static pthread_mutex_t g_tz_lock = PTHREAD_MUTEX_INITIALIZER;

void firebase_now_test_set_unix(time_t unix_seconds) {
    g_now_override = unix_seconds;
    g_now_override_set = true;
}

void firebase_now_test_clear(void) {
    g_now_override_set = false;
}

time_t firebase_now_unix(void) {
    if (g_now_override_set) {
        return g_now_override;
    }
    return time(NULL);
}

char* firebase_now(void) {
    time_t now = firebase_now_unix();
    struct tm utc;
    if (!gmtime_r(&now, &utc)) {
        return NULL;
    }
    return firebase_tz_format_datetime(&utc);
}

bool firebase_tz_zone_exists(const char* name) {
    if (!name || !*name) {
        return false;
    }
    if (strstr(name, "..")) {
        return false;
    }
    for (const char* p = name; *p; p++) {
        unsigned char ch = (unsigned char)*p;
        if (isalnum(ch) || ch == '/' || ch == '_' || ch == '+' || ch == '-') {
            continue;
        }
        return false;
    }
    if (name[0] == '/' || name[0] == '+' || name[0] == '-') {
        return false;
    }

    char path[512];
    int written = snprintf(path, sizeof(path), "%s/%s", FIREBASE_ZONEINFO_DIR, name);
    if (written < 0 || (size_t)written >= sizeof(path)) {
        return false;
    }

    struct stat st;
    if (stat(path, &st) != 0) {
        return false;
    }
    if (S_ISDIR(st.st_mode)) {
        return false;
    }
    return true;
}

bool firebase_tz_parse_datetime(const char* dt, struct tm* out) {
    if (!dt || !out) {
        return false;
    }
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;
    int consumed = 0;
    if (sscanf(dt, "%d-%d-%d %d:%d:%d%n", &year, &month, &day, &hour, &minute, &second, &consumed) != 6) {
        return false;
    }
    if (dt[consumed] != '\0' && dt[consumed] != '.') {
        return false;
    }
    if (year < 1970 || month < 1 || month > 12 || day < 1 || day > 31) {
        return false;
    }
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 60) {
        return false;
    }
    memset(out, 0, sizeof(*out));
    out->tm_year = year - 1900;
    out->tm_mon = month - 1;
    out->tm_mday = day;
    out->tm_hour = hour;
    out->tm_min = minute;
    out->tm_sec = second;
    out->tm_isdst = -1;
    return true;
}

char* firebase_tz_format_datetime(const struct tm* value) {
    if (!value) {
        return NULL;
    }
    char* out = malloc(20);
    if (!out) {
        return NULL;
    }
    if (strftime(out, 20, "%Y-%m-%d %H:%M:%S", value) == 0) {
        free(out);
        return NULL;
    }
    return out;
}

char* firebase_convert_tz(const char* dt, const char* from_tz, const char* to_tz) {
    if (!dt || !from_tz || !to_tz) {
        return NULL;
    }
    if (!firebase_tz_zone_exists(from_tz) || !firebase_tz_zone_exists(to_tz)) {
        return NULL;
    }

    struct tm parsed;
    if (!firebase_tz_parse_datetime(dt, &parsed)) {
        return NULL;
    }
    if (strcmp(from_tz, to_tz) == 0) {
        return firebase_tz_format_datetime(&parsed);
    }

    pthread_mutex_lock(&g_tz_lock);

    const char* old_tz = getenv("TZ");
    char* saved = old_tz ? strdup(old_tz) : NULL;
    bool ok = true;
    time_t epoch = (time_t)-1;
    struct tm out_tm;
    memset(&out_tm, 0, sizeof(out_tm));

    if (setenv("TZ", from_tz, 1) != 0) {
        ok = false;
    } else {
        tzset();
        parsed.tm_isdst = -1;
        epoch = mktime(&parsed);
        if (epoch == (time_t)-1) {
            ok = false;
        }
    }

    if (ok) {
        if (setenv("TZ", to_tz, 1) != 0) {
            ok = false;
        } else {
            tzset();
            if (!localtime_r(&epoch, &out_tm)) {
                ok = false;
            }
        }
    }

    if (saved) {
        setenv("TZ", saved, 1);
        free(saved);
    } else {
        unsetenv("TZ");
    }
    tzset();
    pthread_mutex_unlock(&g_tz_lock);

    if (!ok) {
        return NULL;
    }
    return firebase_tz_format_datetime(&out_tm);
}
