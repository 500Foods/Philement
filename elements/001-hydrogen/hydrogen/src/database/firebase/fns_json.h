/*
 * Firebase in-process JSON ingest and path extract.
 */

#ifndef DATABASE_ENGINE_FIREBASE_FNS_JSON_H
#define DATABASE_ENGINE_FIREBASE_FNS_JSON_H

#include <jansson.h>
#include <stdbool.h>

bool firebase_json_is_valid(const char* text);
char* firebase_json_fixup_controls(const char* text);
char* firebase_json_ingest(const char* text);
char* firebase_json_value_as_text(const json_t* value);
char* firebase_json_value(const char* json_text, const char* path);

#endif /* DATABASE_ENGINE_FIREBASE_FNS_JSON_H */
