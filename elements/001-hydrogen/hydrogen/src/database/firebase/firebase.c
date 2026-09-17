/*
 * Firebase engine - version and description helpers.
 */

#include <src/hydrogen.h>
#include <src/database/database.h>
#include "firebase.h"
#include "fns_base64.h"
#include "fns_brotli.h"
#include "fns_json.h"
#include "fns_sha256.h"
#include "fns_tz.h"

const char* firebase_engine_get_version(void) {
    return "Firebase Engine v1.0.0";
}

bool firebase_engine_is_available(void) {
    return true;
}

const char* firebase_engine_get_description(void) {
    return "Firebase / Cloud Firestore Supported";
}

void firebase_engine_test_functions(void) {
    const char* version = firebase_engine_get_version();
    bool available = firebase_engine_is_available();
    const char* description = firebase_engine_get_description();
    size_t unused_len = 0;
    (void)version;
    (void)available;
    (void)description;
    free(firebase_base64_encode(NULL, 0));
    free(firebase_base64_decode(NULL, &unused_len));
    free(firebase_brotli_decompress(NULL, 0, NULL));
    free(firebase_sha256_b64(NULL, NULL));
    free(firebase_json_ingest(NULL));
    free(firebase_json_value(NULL, NULL));
    firebase_now_test_set_unix(0);
    free(firebase_now());
    firebase_now_test_clear();
    free(firebase_convert_tz(NULL, NULL, NULL));
}
