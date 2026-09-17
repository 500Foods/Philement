/*
 * Firebase in-process NOW and CONVERT_TZ (IANA via OS zoneinfo).
 */

#ifndef DATABASE_ENGINE_FIREBASE_FNS_TZ_H
#define DATABASE_ENGINE_FIREBASE_FNS_TZ_H

#include <stdbool.h>
#include <time.h>

void firebase_now_test_set_unix(time_t unix_seconds);
void firebase_now_test_clear(void);
time_t firebase_now_unix(void);
char* firebase_now(void);

bool firebase_tz_zone_exists(const char* name);
bool firebase_tz_parse_datetime(const char* dt, struct tm* out);
char* firebase_tz_format_datetime(const struct tm* value);
char* firebase_convert_tz(const char* dt, const char* from_tz, const char* to_tz);

#endif /* DATABASE_ENGINE_FIREBASE_FNS_TZ_H */
