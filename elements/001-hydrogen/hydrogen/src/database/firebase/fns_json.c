/*
 * Firebase JSON ingest matches plpgsql/MySQL/DB2: fast-path if already
 * valid, else escape raw \n/\t/\r only inside strings, parse again.
 * Store what ingest returned (SQLite-style), not jsonb dumps.
 * $ref/$id/$schema are ordinary keys. Missing JSON_VALUE path is NULL.
 */

#include <src/hydrogen.h>
#include <src/database/database.h>
#include "types.h"
#include "fns_json.h"

bool firebase_json_is_valid(const char* text) {
    if (!text) {
        return false;
    }
    json_error_t err;
    json_t* root = json_loads(text, 0, &err);
    if (!root) {
        return false;
    }
    json_decref(root);
    return true;
}

char* firebase_json_fixup_controls(const char* text) {
    if (!text) {
        return NULL;
    }
    size_t len = strlen(text);
    char* out = malloc(len * 2 + 1);
    if (!out) {
        return NULL;
    }

    size_t j = 0;
    bool in_str = false;
    bool esc = false;
    for (size_t i = 0; i < len; i++) {
        unsigned char ch = (unsigned char)text[i];
        if (esc) {
            out[j++] = (char)ch;
            esc = false;
            continue;
        }
        if (ch == '\\') {
            out[j++] = (char)ch;
            esc = true;
            continue;
        }
        if (ch == '"') {
            out[j++] = (char)ch;
            in_str = !in_str;
            continue;
        }
        if (in_str && ch == '\n') {
            out[j++] = '\\';
            out[j++] = 'n';
            continue;
        }
        if (in_str && ch == '\r') {
            out[j++] = '\\';
            out[j++] = 'r';
            continue;
        }
        if (in_str && ch == '\t') {
            out[j++] = '\\';
            out[j++] = 't';
            continue;
        }
        out[j++] = (char)ch;
    }
    out[j] = '\0';
    return out;
}

char* firebase_json_ingest(const char* text) {
    if (!text) {
        return NULL;
    }
    if (strlen(text) > FIREBASE_MAX_FIELD_BYTES) {
        return NULL;
    }
    if (firebase_json_is_valid(text)) {
        return strdup(text);
    }
    char* fixed = firebase_json_fixup_controls(text);
    if (!fixed) {
        return NULL;
    }
    if (firebase_json_is_valid(fixed)) {
        return fixed;
    }
    free(fixed);
    return NULL;
}

char* firebase_json_value_as_text(const json_t* value) {
    if (!value || json_is_null(value)) {
        return NULL;
    }
    if (json_is_string(value)) {
        const char* s = json_string_value(value);
        return s ? strdup(s) : NULL;
    }
    if (json_is_true(value)) {
        return strdup("true");
    }
    if (json_is_false(value)) {
        return strdup("false");
    }
    return json_dumps(value, JSON_COMPACT | JSON_ENCODE_ANY);
}

char* firebase_json_value(const char* json_text, const char* path) {
    if (!json_text || !path || path[0] != '$') {
        return NULL;
    }

    json_error_t err;
    json_t* root = json_loads(json_text, 0, &err);
    if (!root) {
        return NULL;
    }

    json_t* cur = root;
    const char* p = path + 1;
    bool failed = false;

    while (*p && !failed) {
        if (*p != '.') {
            failed = true;
            break;
        }
        p++;
        if (*p == '\0' || *p == '.' || *p == '[') {
            failed = true;
            break;
        }
        const char* start = p;
        while (*p && *p != '.' && *p != '[') {
            p++;
        }
        size_t klen = (size_t)(p - start);
        char* key = malloc(klen + 1);
        if (!key) {
            failed = true;
            break;
        }
        memcpy(key, start, klen);
        key[klen] = '\0';
        if (!json_is_object(cur)) {
            free(key);
            failed = true;
            break;
        }
        cur = json_object_get(cur, key);
        free(key);
        if (!cur) {
            failed = true;
            break;
        }
        while (*p == '[') {
            p++;
            char* endp = NULL;
            long idx = strtol(p, &endp, 10);
            if (!endp || *endp != ']' || idx < 0) {
                failed = true;
                break;
            }
            p = endp + 1;
            if (!json_is_array(cur)) {
                failed = true;
                break;
            }
            cur = json_array_get(cur, (size_t)idx);
            if (!cur) {
                failed = true;
                break;
            }
        }
    }

    char* text = NULL;
    if (!failed && *p == '\0') {
        text = firebase_json_value_as_text(cur);
    }
    json_decref(root);
    return text;
}
