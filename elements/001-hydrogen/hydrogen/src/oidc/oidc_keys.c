/*
 * OpenID Connect (OIDC) Key Management Implementation
 *
 * Manages RSA signing keys, PEM persistence, and JWKS export.
 */

#include <src/hydrogen.h>

#include "oidc_keys.h"

#include <src/utils/utils_crypto.h>
#include <src/logging/logging.h>

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

#include <openssl/evp.h>

#define OIDC_DEFAULT_RSA_BITS 2048
#define OIDC_MAX_KEYS 16
#define OIDC_ACTIVE_KEY_FILENAME "signing-active.pem"
#define OIDC_ACTIVE_KID_FILENAME "signing-active.kid"

OIDCKeyContext* init_oidc_key_management(const char *storage_path,
                                        bool encryption_enabled,
                                        int rotation_interval_days);
void cleanup_oidc_key_management(OIDCKeyContext *context);
OIDCKey* oidc_generate_signing_key(OIDCKeyContext *context, OIDCKeyAlgorithm algorithm);
bool oidc_rotate_keys(OIDCKeyContext *context);
OIDCKey* oidc_get_active_signing_key(OIDCKeyContext *context);
OIDCKey* oidc_find_key_by_id(OIDCKeyContext *context, const char *kid);
char* oidc_generate_jwks(const OIDCKeyContext *context);
bool oidc_sign_data(OIDCKey *key, const unsigned char *data, size_t data_len,
                   unsigned char **signature, size_t *signature_len);
bool oidc_verify_signature(OIDCKey *key, const unsigned char *data, size_t data_len,
                          const unsigned char *signature, size_t signature_len);
char* oidc_export_public_key_pem(OIDCKey *key);
bool oidc_load_keys_from_storage(OIDCKeyContext *context);
bool oidc_save_keys_to_storage(OIDCKeyContext *context);
const char* oidc_algorithm_to_string(OIDCKeyAlgorithm algorithm);
OIDCKeyAlgorithm oidc_algorithm_from_string(const char *algorithm_str);

void oidc_free_key(OIDCKey *key);
bool oidc_context_add_key(OIDCKeyContext *context, OIDCKey *key);
bool oidc_ensure_storage_dir(const char *path);
char* oidc_build_storage_path(const char *dir, const char *filename);
bool oidc_write_text_file(const char *path, const char *contents);
char* oidc_read_text_file(const char *path);
void oidc_generate_kid(char *kid_out, size_t kid_out_len);

void oidc_free_key(OIDCKey *key) {
    if (!key) {
        return;
    }
    if (key->key_data) {
        EVP_PKEY_free((EVP_PKEY*)key->key_data);
        key->key_data = NULL;
    }
    free(key);
}

void oidc_generate_kid(char *kid_out, size_t kid_out_len) {
    unsigned char raw[16];
    if (!kid_out || kid_out_len < 9U) {
        return;
    }
    if (!utils_random_bytes(raw, sizeof(raw))) {
        snprintf(kid_out, kid_out_len, "oidc-default");
        return;
    }
    char* encoded = utils_base64url_encode(raw, sizeof(raw));
    if (!encoded) {
        snprintf(kid_out, kid_out_len, "oidc-default");
        return;
    }
    snprintf(kid_out, kid_out_len, "%s", encoded);
    free(encoded);
}

bool oidc_ensure_storage_dir(const char *path) {
    if (!path || path[0] == '\0') {
        return false;
    }
    struct stat st;
    if (stat(path, &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    if (mkdir(path, 0700) != 0 && errno != EEXIST) {
        log_this(SR_OIDC, "Failed to create key storage directory", LOG_LEVEL_ERROR, 0);
        return false;
    }
    return true;
}

char* oidc_build_storage_path(const char *dir, const char *filename) {
    if (!dir || !filename) {
        return NULL;
    }
    size_t need = strlen(dir) + 1U + strlen(filename) + 1U;
    char* out = (char*)malloc(need);
    if (!out) {
        return NULL;
    }
    snprintf(out, need, "%s/%s", dir, filename);
    return out;
}

bool oidc_write_text_file(const char *path, const char *contents) {
    if (!path || !contents) {
        return false;
    }
    FILE* fp = fopen(path, "w");
    if (!fp) {
        log_this(SR_OIDC, "Failed to open key file for write", LOG_LEVEL_ERROR, 0);
        return false;
    }
    size_t len = strlen(contents);
    size_t written = fwrite(contents, 1, len, fp);
    if (fclose(fp) != 0 || written != len) {
        log_this(SR_OIDC, "Failed to write key file", LOG_LEVEL_ERROR, 0);
        return false;
    }
    return true;
}

char* oidc_read_text_file(const char *path) {
    if (!path) {
        return NULL;
    }
    FILE* fp = fopen(path, "r");
    if (!fp) {
        return NULL;
    }
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return NULL;
    }
    long sz = ftell(fp);
    if (sz < 0) {
        fclose(fp);
        return NULL;
    }
    if (fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return NULL;
    }
    char* buf = (char*)malloc((size_t)sz + 1U);
    if (!buf) {
        fclose(fp);
        return NULL;
    }
    size_t n = fread(buf, 1, (size_t)sz, fp);
    fclose(fp);
    buf[n] = '\0';
    return buf;
}

bool oidc_context_add_key(OIDCKeyContext *context, OIDCKey *key) {
    if (!context || !key) {
        return false;
    }
    if (context->key_count >= OIDC_MAX_KEYS) {
        log_this(SR_OIDC, "Key context full", LOG_LEVEL_ERROR, 0);
        return false;
    }
    OIDCKey **grown = (OIDCKey**)realloc(context->keys,
                                         (context->key_count + 1U) * sizeof(OIDCKey*));
    if (!grown) {
        return false;
    }
    context->keys = grown;
    context->keys[context->key_count] = key;
    context->key_count++;
    return true;
}

const char* oidc_algorithm_to_string(OIDCKeyAlgorithm algorithm) {
    switch (algorithm) {
        case KEY_ALG_RS256: return "RS256";
        case KEY_ALG_RS384: return "RS384";
        case KEY_ALG_RS512: return "RS512";
        case KEY_ALG_ES256: return "ES256";
        case KEY_ALG_ES384: return "ES384";
        case KEY_ALG_ES512: return "ES512";
        default: return "RS256";
    }
}

OIDCKeyAlgorithm oidc_algorithm_from_string(const char *algorithm_str) {
    if (!algorithm_str) {
        return KEY_ALG_RS256;
    }
    if (strcmp(algorithm_str, "RS384") == 0) {
        return KEY_ALG_RS384;
    }
    if (strcmp(algorithm_str, "RS512") == 0) {
        return KEY_ALG_RS512;
    }
    if (strcmp(algorithm_str, "ES256") == 0) {
        return KEY_ALG_ES256;
    }
    if (strcmp(algorithm_str, "ES384") == 0) {
        return KEY_ALG_ES384;
    }
    if (strcmp(algorithm_str, "ES512") == 0) {
        return KEY_ALG_ES512;
    }
    return KEY_ALG_RS256;
}

OIDCKeyContext* init_oidc_key_management(const char *storage_path,
                                        bool encryption_enabled,
                                        int rotation_interval_days) {
    log_this(SR_OIDC, "Initializing key management", LOG_LEVEL_STATE, 0);

    OIDCKeyContext *context = (OIDCKeyContext*)calloc(1, sizeof(OIDCKeyContext));
    if (!context) {
        log_this(SR_OIDC, "Failed to allocate memory for key context", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    if (storage_path && storage_path[0] != '\0') {
        context->storage_path = strdup(storage_path);
        if (!context->storage_path) {
            free(context);
            return NULL;
        }
        if (!oidc_ensure_storage_dir(context->storage_path)) {
            free(context->storage_path);
            free(context);
            return NULL;
        }
    }

    context->encryption_enabled = encryption_enabled;
    context->rotation_interval = rotation_interval_days;
    context->next_rotation = time(NULL) + ((time_t)rotation_interval_days * 86400);

    if (context->storage_path) {
        if (!oidc_load_keys_from_storage(context)) {
            log_this(SR_OIDC, "No existing keys loaded; generating new signing key", LOG_LEVEL_STATE, 0);
            OIDCKey *key = oidc_generate_signing_key(context, KEY_ALG_RS256);
            if (!key) {
                cleanup_oidc_key_management(context);
                return NULL;
            }
            if (!oidc_save_keys_to_storage(context)) {
                log_this(SR_OIDC, "Failed to persist newly generated keys", LOG_LEVEL_ERROR, 0);
                cleanup_oidc_key_management(context);
                return NULL;
            }
        }
    } else {
        OIDCKey *key = oidc_generate_signing_key(context, KEY_ALG_RS256);
        if (!key) {
            cleanup_oidc_key_management(context);
            return NULL;
        }
    }

    log_this(SR_OIDC, "Key management initialized successfully", LOG_LEVEL_STATE, 0);
    return context;
}

void cleanup_oidc_key_management(OIDCKeyContext *context) {
    if (!context) {
        return;
    }

    log_this(SR_OIDC, "Cleaning up key management", LOG_LEVEL_STATE, 0);

    if (context->keys) {
        for (size_t i = 0; i < context->key_count; i++) {
            oidc_free_key(context->keys[i]);
            context->keys[i] = NULL;
        }
        free(context->keys);
        context->keys = NULL;
    }
    context->key_count = 0;

    free(context->storage_path);
    context->storage_path = NULL;
    free(context);
    log_this(SR_OIDC, "Key management cleanup completed", LOG_LEVEL_STATE, 0);
}

OIDCKey* oidc_generate_signing_key(OIDCKeyContext *context, OIDCKeyAlgorithm algorithm) {
    if (!context) {
        return NULL;
    }
    if (algorithm != KEY_ALG_RS256) {
        log_this(SR_OIDC, "Only RS256 signing keys are supported", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    EVP_PKEY* pkey = utils_rsa_generate_keypair(OIDC_DEFAULT_RSA_BITS);
    if (!pkey) {
        return NULL;
    }

    OIDCKey *key = (OIDCKey*)calloc(1, sizeof(OIDCKey));
    if (!key) {
        EVP_PKEY_free(pkey);
        return NULL;
    }

    oidc_generate_kid(key->kid, sizeof(key->kid));
    key->usage = KEY_USAGE_SIGNING;
    key->algorithm = KEY_ALG_RS256;
    key->status = KEY_STATUS_ACTIVE;
    key->created_at = time(NULL);
    key->expires_at = 0;
    key->key_data = pkey;

    /* Demote any previous ACTIVE signing keys to ROTATING. */
    for (size_t i = 0; i < context->key_count; i++) {
        OIDCKey *existing = context->keys[i];
        if (existing && existing->usage == KEY_USAGE_SIGNING &&
            existing->status == KEY_STATUS_ACTIVE) {
            existing->status = KEY_STATUS_ROTATING;
        }
    }

    if (!oidc_context_add_key(context, key)) {
        oidc_free_key(key);
        return NULL;
    }
    return key;
}

bool oidc_rotate_keys(OIDCKeyContext *context) {
    if (!context) {
        return false;
    }
    /* MVP: generate a new ACTIVE RS256 key; prior ACTIVE becomes ROTATING. */
    OIDCKey *key = oidc_generate_signing_key(context, KEY_ALG_RS256);
    if (!key) {
        return false;
    }
    context->next_rotation = time(NULL) + ((time_t)context->rotation_interval * 86400);
    if (context->storage_path) {
        return oidc_save_keys_to_storage(context);
    }
    return true;
}

OIDCKey* oidc_get_active_signing_key(OIDCKeyContext *context) {
    if (!context || !context->keys) {
        return NULL;
    }
    for (size_t i = 0; i < context->key_count; i++) {
        OIDCKey *key = context->keys[i];
        if (key && key->usage == KEY_USAGE_SIGNING && key->status == KEY_STATUS_ACTIVE) {
            return key;
        }
    }
    return NULL;
}

OIDCKey* oidc_find_key_by_id(OIDCKeyContext *context, const char *kid) {
    if (!context || !kid || !context->keys) {
        return NULL;
    }
    for (size_t i = 0; i < context->key_count; i++) {
        OIDCKey *key = context->keys[i];
        if (key && strcmp(key->kid, kid) == 0) {
            return key;
        }
    }
    return NULL;
}

char* oidc_generate_jwks(const OIDCKeyContext *context) {
    if (!context) {
        log_this(SR_OIDC, "Cannot generate JWKS: Invalid context", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    json_t *root = json_object();
    json_t *keys = json_array();
    if (!root || !keys) {
        if (root) {
            json_decref(root);
        }
        if (keys) {
            json_decref(keys);
        }
        return NULL;
    }

    for (size_t i = 0; i < context->key_count; i++) {
        OIDCKey *key = context->keys ? context->keys[i] : NULL;
        if (!key || !key->key_data) {
            continue;
        }
        if (key->status != KEY_STATUS_ACTIVE && key->status != KEY_STATUS_ROTATING) {
            continue;
        }
        char *jwk_str = utils_rsa_public_to_jwk((EVP_PKEY*)key->key_data, key->kid);
        if (!jwk_str) {
            continue;
        }
        json_error_t err;
        json_t *jwk = json_loads(jwk_str, 0, &err);
        free(jwk_str);
        if (jwk) {
            json_array_append_new(keys, jwk);
        }
    }

    json_object_set_new(root, "keys", keys);
    char *dumped = json_dumps(root, JSON_COMPACT);
    json_decref(root);
    if (!dumped) {
        log_this(SR_OIDC, "Failed to allocate memory for JWKS", LOG_LEVEL_ERROR, 0);
        return NULL;
    }
    return dumped;
}

bool oidc_sign_data(OIDCKey *key, const unsigned char *data, size_t data_len,
                   unsigned char **signature, size_t *signature_len) {
    if (!key || !key->key_data || !data || data_len == 0 || !signature || !signature_len) {
        return false;
    }
    return utils_rs256_sign((EVP_PKEY*)key->key_data, data, data_len, signature, signature_len);
}

bool oidc_verify_signature(OIDCKey *key, const unsigned char *data, size_t data_len,
                          const unsigned char *signature, size_t signature_len) {
    if (!key || !key->key_data || !data || data_len == 0 || !signature || signature_len == 0) {
        return false;
    }
    return utils_rs256_verify((EVP_PKEY*)key->key_data, data, data_len, signature, signature_len);
}

char* oidc_export_public_key_pem(OIDCKey *key) {
    if (!key || !key->key_data) {
        return NULL;
    }
    return utils_rsa_public_to_pem((EVP_PKEY*)key->key_data);
}

bool oidc_load_keys_from_storage(OIDCKeyContext *context) {
    if (!context || !context->storage_path) {
        return false;
    }

    char *pem_path = oidc_build_storage_path(context->storage_path, OIDC_ACTIVE_KEY_FILENAME);
    char *kid_path = oidc_build_storage_path(context->storage_path, OIDC_ACTIVE_KID_FILENAME);
    if (!pem_path || !kid_path) {
        free(pem_path);
        free(kid_path);
        return false;
    }

    char *pem = oidc_read_text_file(pem_path);
    char *kid = oidc_read_text_file(kid_path);
    free(pem_path);
    free(kid_path);

    if (!pem) {
        free(kid);
        return false;
    }

    /* Trim trailing newline from kid file. */
    if (kid) {
        size_t klen = strlen(kid);
        while (klen > 0U && (kid[klen - 1U] == '\n' || kid[klen - 1U] == '\r')) {
            kid[klen - 1U] = '\0';
            klen--;
        }
    }

    EVP_PKEY *pkey = utils_rsa_private_from_pem(pem);
    free(pem);
    if (!pkey) {
        free(kid);
        return false;
    }

    OIDCKey *key = (OIDCKey*)calloc(1, sizeof(OIDCKey));
    if (!key) {
        EVP_PKEY_free(pkey);
        free(kid);
        return false;
    }

    if (kid && kid[0] != '\0') {
        snprintf(key->kid, sizeof(key->kid), "%s", kid);
    } else {
        oidc_generate_kid(key->kid, sizeof(key->kid));
    }
    free(kid);

    key->usage = KEY_USAGE_SIGNING;
    key->algorithm = KEY_ALG_RS256;
    key->status = KEY_STATUS_ACTIVE;
    key->created_at = time(NULL);
    key->expires_at = 0;
    key->key_data = pkey;

    if (!oidc_context_add_key(context, key)) {
        oidc_free_key(key);
        return false;
    }
    return true;
}

bool oidc_save_keys_to_storage(OIDCKeyContext *context) {
    if (!context || !context->storage_path) {
        return false;
    }
    if (!oidc_ensure_storage_dir(context->storage_path)) {
        return false;
    }

    OIDCKey *active = oidc_get_active_signing_key(context);
    if (!active || !active->key_data) {
        return false;
    }

    char *pem = utils_rsa_private_to_pem((EVP_PKEY*)active->key_data);
    if (!pem) {
        return false;
    }

    char *pem_path = oidc_build_storage_path(context->storage_path, OIDC_ACTIVE_KEY_FILENAME);
    char *kid_path = oidc_build_storage_path(context->storage_path, OIDC_ACTIVE_KID_FILENAME);
    if (!pem_path || !kid_path) {
        free(pem);
        free(pem_path);
        free(kid_path);
        return false;
    }

    bool ok = oidc_write_text_file(pem_path, pem);
    if (ok) {
        ok = oidc_write_text_file(kid_path, active->kid);
    }

    free(pem);
    free(pem_path);
    free(kid_path);
    return ok;
}
