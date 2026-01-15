/*
 * Auth Service JWT Functions
 *
 * This file contains all JWT-related operations including:
 * - Base64url encoding/decoding
 * - JWT generation and validation
 * - Token hashing
 * - JWT configuration management
 */

// Global includes
#include <src/hydrogen.h>
#include <string.h>
#include <ctype.h>
#include <jansson.h>
#include <src/logging/logging.h>

// OpenSSL includes for JWT
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <openssl/rand.h>

// Local includes
#include "auth_service.h"
#include "auth_service_jwt.h"
#include "auth_service_database.h"
#include <src/utils/utils_crypto.h>

// JWT Constants
#define JWT_LIFETIME 3600  // 1 hour in seconds
#define JWT_ALGORITHM "HS256"
#define JWT_TYPE "JWT"

/**
 * Generate a unique JWT ID (JTI)
 */
char* generate_jti(void) {
    unsigned char random_bytes[16];
    if (!utils_random_bytes(random_bytes, sizeof(random_bytes))) {
        log_this(SR_AUTH, "Failed to generate random bytes for JTI", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    return utils_base64url_encode(random_bytes, sizeof(random_bytes));
}

/**
 * Compute SHA256 hash of token for storage/lookup
 */
char* compute_token_hash(const char* token) {
    if (!token) return NULL;

    return utils_sha256_hash((const unsigned char*)token, strlen(token));
}

/**
 * Compute password hash using account_id + password
 */
char* compute_password_hash(const char* password, int account_id) {
    return utils_password_hash(password, account_id);
}


/**
 * Get JWT configuration from system config
 */
jwt_config_t* get_jwt_config(void) {
    jwt_config_t* config = calloc(1, sizeof(jwt_config_t));
    if (!config) return NULL;

    // Load JWT secret from application configuration
    if (app_config && app_config->api.jwt_secret) {
        config->hmac_secret = strdup(app_config->api.jwt_secret);
        log_this(SR_AUTH, "Using JWT secret from configuration (length: %zu)",
                 LOG_LEVEL_DEBUG, 1, strlen(config->hmac_secret));
    } else {
        // Fallback to default if not configured
        config->hmac_secret = strdup("default-jwt-secret-change-me-in-production");
        log_this(SR_AUTH, "Using default JWT secret - configure API.JWTSecret in production!",
                 LOG_LEVEL_ALERT, 0);
    }
    
    config->use_rsa = false;
    config->rotation_interval_days = 90;

    return config;
}

/**
 * Generate a JWT token
 */
char* generate_jwt(account_info_t* account, system_info_t* system,
                   const char* client_ip, const char* tz, const char* database, time_t issued_at) {
    if (!account || !system || !client_ip || !tz || !database) {
        log_this(SR_AUTH, "Invalid parameters for JWT generation", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    jwt_config_t* config = get_jwt_config();
    if (!config) {
        log_this("AUTH", "Failed to get JWT configuration", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    if (!config->hmac_secret) {
        log_this("AUTH", "JWT configuration missing secret", LOG_LEVEL_ERROR, 0);
        free_jwt_config(config);
        return NULL;
    }

    // Create JWT header
    char* header_json = NULL;
    int ret = asprintf(&header_json, "{\"alg\":\"%s\",\"typ\":\"%s\"}",
                       config->use_rsa ? "RS256" : "HS256", JWT_TYPE);
    if (ret == -1) {
        log_this("AUTH", "Failed to create JWT header", LOG_LEVEL_ERROR, 0);
        free_jwt_config(config);
        return NULL;
    }

    // Create JWT payload (claims)
    time_t now = issued_at;
    time_t exp = now + JWT_LIFETIME;
    char* jti = generate_jti();
    if (!jti) {
        log_this("AUTH", "Failed to generate JTI", LOG_LEVEL_ERROR, 0);
        free(header_json);
        free_jwt_config(config);
        return NULL;
    }

    // Calculate timezone offset
    int tzoffset = calculate_timezone_offset(tz);
    
    char* payload_json = NULL;
    ret = asprintf(&payload_json,
                   "{\"iss\":\"hydrogen-auth\",\"sub\":\"%d\",\"aud\":\"%d\",\"exp\":%ld,\"iat\":%ld,\"nbf\":%ld,\"jti\":\"%s\",\"user_id\":%d,\"system_id\":%d,\"app_id\":%d,\"username\":\"%s\",\"email\":\"%s\",\"roles\":\"%s\",\"ip\":\"%s\",\"tz\":\"%s\",\"tzoffset\":%d,\"database\":\"%s\"}",
                   account->id, system->app_id, exp, now, now, jti,
                   account->id, system->system_id, system->app_id,
                   account->username ? account->username : "",
                   account->email ? account->email : "",
                   account->roles ? account->roles : "",
                   client_ip, tz, tzoffset, database);

    free(jti);

    if (ret == -1) {
        log_this("AUTH", "Failed to create JWT payload", LOG_LEVEL_ERROR, 0);
        free(header_json);
        free_jwt_config(config);
        return NULL;
    }

    // Base64url encode header and payload
    char* header_b64 = utils_base64url_encode((const unsigned char*)header_json, strlen(header_json));
    char* payload_b64 = utils_base64url_encode((const unsigned char*)payload_json, strlen(payload_json));

    free(header_json);
    free(payload_json);

    if (!header_b64 || !payload_b64) {
        log_this("AUTH", "Failed to encode JWT parts", LOG_LEVEL_ERROR, 0);
        free(header_b64);
        free(payload_b64);
        free_jwt_config(config);
        return NULL;
    }

    // Create signing input
    char* signing_input = NULL;
    ret = asprintf(&signing_input, "%s.%s", header_b64, payload_b64);
    if (ret == -1) {
        log_this("AUTH", "Failed to create signing input", LOG_LEVEL_ERROR, 0);
        free(header_b64);
        free(payload_b64);
        free_jwt_config(config);
        return NULL;
    }

    // Create signature
    unsigned char signature[SHA256_DIGEST_LENGTH];
    unsigned int signature_len = SHA256_DIGEST_LENGTH;

    if (HMAC(EVP_sha256(), config->hmac_secret, (int)strlen(config->hmac_secret),
             (const unsigned char*)signing_input, strlen(signing_input),
             signature, &signature_len) == NULL) {
        log_this("AUTH", "Failed to create HMAC signature", LOG_LEVEL_ERROR, 0);
        free(header_b64);
        free(payload_b64);
        free(signing_input);
        free_jwt_config(config);
        return NULL;
    }

    char* signature_b64 = utils_base64url_encode(signature, signature_len);
    if (!signature_b64) {
        log_this("AUTH", "Failed to encode signature", LOG_LEVEL_ERROR, 0);
        free(header_b64);
        free(payload_b64);
        free(signing_input);
        free_jwt_config(config);
        return NULL;
    }

    // Create final JWT
    char* jwt = NULL;
    ret = asprintf(&jwt, "%s.%s.%s", header_b64, payload_b64, signature_b64);

    free(header_b64);
    free(payload_b64);
    free(signature_b64);
    free(signing_input);
    free_jwt_config(config);

    if (ret == -1) {
        log_this("AUTH", "Failed to create final JWT", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    log_this("AUTH", "Generated JWT for user %s", LOG_LEVEL_DEBUG, 1, account->username);
    return jwt;
}

/**
 * Validate a JWT token
 */
jwt_validation_result_t validate_jwt(const char* token, const char* database) {
    jwt_validation_result_t result = {0};
    result.valid = false;

    if (!token) {
        result.error = JWT_ERROR_INVALID_FORMAT;
        return result;
    }

    // Split token into parts
    char* token_copy = strdup(token);
    if (!token_copy) {
        result.error = JWT_ERROR_INVALID_FORMAT;
        return result;
    }

    char* header_b64 = strtok(token_copy, ".");
    char* payload_b64 = strtok(NULL, ".");
    const char* signature_b64 = strtok(NULL, ".");

    if (!header_b64 || !payload_b64 || !signature_b64) {
        free(token_copy);
        result.error = JWT_ERROR_INVALID_FORMAT;
        return result;
    }

    // Decode payload to get claims
    size_t payload_len;
    unsigned char* payload_decoded = utils_base64url_decode(payload_b64, &payload_len);
    if (!payload_decoded) {
        free(token_copy);
        result.error = JWT_ERROR_INVALID_FORMAT;
        return result;
    }

    // Parse JSON claims using jansson
    char* payload_str = strndup((char*)payload_decoded, payload_len);
    free(payload_decoded);
    payload_decoded = NULL; // Mark as freed
    
    if (!payload_str) {
        free(token_copy);
        result.error = JWT_ERROR_INVALID_FORMAT;
        return result;
    }

    json_error_t json_err;
    json_t* payload_json = json_loads(payload_str, 0, &json_err);
    free(payload_str);
    
    if (!payload_json) {
        log_this(SR_AUTH, "Failed to parse JWT payload: %s", LOG_LEVEL_ERROR, 1, json_err.text);
        free(token_copy);
        result.error = JWT_ERROR_INVALID_FORMAT;
        return result;
    }

    // Extract expiration time
    json_t* exp_json = json_object_get(payload_json, "exp");
    if (!exp_json || !json_is_integer(exp_json)) {
        json_decref(payload_json);
        free(token_copy);
        result.error = JWT_ERROR_INVALID_FORMAT;
        return result;
    }

    time_t exp_time = (time_t)json_integer_value(exp_json);
    time_t now = time(NULL);

    if (exp_time < now) {
        json_decref(payload_json);
        free(token_copy);
        result.error = JWT_ERROR_EXPIRED;
        return result;
    }

    // If database not provided, extract it from JWT claims first
    const char* db_to_use = database;
    if (!db_to_use) {
        json_t* database_json = json_object_get(payload_json, "database");
        if (database_json && json_is_string(database_json)) {
            db_to_use = json_string_value(database_json);
        }
    }

    // Check if token is revoked (requires database)
    if (db_to_use) {
        char* token_hash = compute_token_hash(token);
        if (is_token_revoked(token_hash, db_to_use)) {
            free(token_hash);
            json_decref(payload_json);
            free(token_copy);
            result.error = JWT_ERROR_REVOKED;
            return result;
        }
        free(token_hash);
    }

    // Verify signature
    jwt_config_t* config = get_jwt_config();
    if (!config) {
        free(payload_decoded);
        free(token_copy);
        result.error = JWT_ERROR_INVALID_SIGNATURE;
        return result;
    }

    char* signing_input = NULL;
    int ret = asprintf(&signing_input, "%s.%s", header_b64, payload_b64);
    if (ret == -1) {
        free_jwt_config(config);
        free(payload_decoded);
        free(token_copy);
        result.error = JWT_ERROR_INVALID_SIGNATURE;
        return result;
    }

    unsigned char expected_signature[SHA256_DIGEST_LENGTH];
    unsigned int expected_len = SHA256_DIGEST_LENGTH;

    if (HMAC(EVP_sha256(), config->hmac_secret, (int)strlen(config->hmac_secret),
             (const unsigned char*)signing_input, strlen(signing_input),
             expected_signature, &expected_len) == NULL) {
        free(signing_input);
        free_jwt_config(config);
        free(payload_decoded);
        free(token_copy);
        result.error = JWT_ERROR_INVALID_SIGNATURE;
        return result;
    }

    size_t signature_len;
    unsigned char* signature_decoded = utils_base64url_decode(signature_b64, &signature_len);
    if (!signature_decoded || signature_len != expected_len ||
        memcmp(signature_decoded, expected_signature, expected_len) != 0) {
        free(signature_decoded);
        free(signing_input);
        free_jwt_config(config);
        free(payload_decoded);
        free(token_copy);
        result.error = JWT_ERROR_INVALID_SIGNATURE;
        return result;
    }

    free(signature_decoded);
    free(signing_input);
    free_jwt_config(config);

    // Token is valid - create claims structure with all fields
    result.valid = true;
    result.error = JWT_ERROR_NONE;
    result.claims = calloc(1, sizeof(jwt_claims_t));
    if (!result.claims) {
        json_decref(payload_json);
        free(token_copy);
        result.valid = false;
        result.error = JWT_ERROR_INVALID_FORMAT;
        return result;
    }

    // Extract all claims from JSON
    result.claims->exp = exp_time;
    
    json_t* iat_json = json_object_get(payload_json, "iat");
    result.claims->iat = (iat_json && json_is_integer(iat_json)) ?
                         (time_t)json_integer_value(iat_json) : now;
    
    json_t* nbf_json = json_object_get(payload_json, "nbf");
    result.claims->nbf = (nbf_json && json_is_integer(nbf_json)) ?
                         (time_t)json_integer_value(nbf_json) : now;
    
    json_t* user_id_json = json_object_get(payload_json, "user_id");
    result.claims->user_id = (user_id_json && json_is_integer(user_id_json)) ?
                             (int)json_integer_value(user_id_json) : 0;
    
    json_t* system_id_json = json_object_get(payload_json, "system_id");
    result.claims->system_id = (system_id_json && json_is_integer(system_id_json)) ?
                               (int)json_integer_value(system_id_json) : 0;
    
    json_t* app_id_json = json_object_get(payload_json, "app_id");
    result.claims->app_id = (app_id_json && json_is_integer(app_id_json)) ?
                            (int)json_integer_value(app_id_json) : 0;
    
    // Extract string claims
    json_t* iss_json = json_object_get(payload_json, "iss");
    if (iss_json && json_is_string(iss_json)) {
        result.claims->iss = strdup(json_string_value(iss_json));
    }
    
    json_t* sub_json = json_object_get(payload_json, "sub");
    if (sub_json && json_is_string(sub_json)) {
        result.claims->sub = strdup(json_string_value(sub_json));
    }
    
    json_t* aud_json = json_object_get(payload_json, "aud");
    if (aud_json && json_is_string(aud_json)) {
        result.claims->aud = strdup(json_string_value(aud_json));
    }
    
    json_t* jti_json = json_object_get(payload_json, "jti");
    if (jti_json && json_is_string(jti_json)) {
        result.claims->jti = strdup(json_string_value(jti_json));
    }
    
    json_t* username_json = json_object_get(payload_json, "username");
    if (username_json && json_is_string(username_json)) {
        result.claims->username = strdup(json_string_value(username_json));
    }
    
    json_t* email_json = json_object_get(payload_json, "email");
    if (email_json && json_is_string(email_json)) {
        result.claims->email = strdup(json_string_value(email_json));
    }
    
    json_t* roles_json = json_object_get(payload_json, "roles");
    if (roles_json && json_is_string(roles_json)) {
        result.claims->roles = strdup(json_string_value(roles_json));
    }
    
    json_t* ip_json = json_object_get(payload_json, "ip");
    if (ip_json && json_is_string(ip_json)) {
        result.claims->ip = strdup(json_string_value(ip_json));
    }
    
    json_t* tz_json = json_object_get(payload_json, "tz");
    if (tz_json && json_is_string(tz_json)) {
        result.claims->tz = strdup(json_string_value(tz_json));
    }
    
    // Extract tzoffset field
    json_t* tzoffset_json = json_object_get(payload_json, "tzoffset");
    result.claims->tzoffset = (tzoffset_json && json_is_integer(tzoffset_json)) ?
                              (int)json_integer_value(tzoffset_json) : 0;
    
    // Extract database field
    json_t* database_json = json_object_get(payload_json, "database");
    if (database_json && json_is_string(database_json)) {
        result.claims->database = strdup(json_string_value(database_json));
    }

    json_decref(payload_json);
    free(token_copy);

    log_this(SR_AUTH, "Successfully validated JWT for user %s (database: %s)",
             LOG_LEVEL_DEBUG, 2,
             result.claims->username ? result.claims->username : "unknown",
             result.claims->database ? result.claims->database : "none");

    return result;
}

/**
 * Generate a new JWT from old claims (token renewal)
 * Creates a new JWT with updated timestamps while preserving user/system info
 */
char* generate_new_jwt(jwt_claims_t* old_claims) {
    if (!old_claims) return NULL;

    jwt_config_t* config = get_jwt_config();
    if (!config) {
        log_this("AUTH", "Failed to get JWT configuration for renewal", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    if (!config->hmac_secret) {
        log_this("AUTH", "JWT configuration missing secret for renewal", LOG_LEVEL_ERROR, 0);
        free_jwt_config(config);
        return NULL;
    }

    // Create JWT header
    char* header_json = NULL;
    int ret = asprintf(&header_json, "{\"alg\":\"%s\",\"typ\":\"%s\"}",
                       config->use_rsa ? "RS256" : "HS256", JWT_TYPE);
    if (ret == -1) {
        log_this("AUTH", "Failed to create JWT header for renewal", LOG_LEVEL_ERROR, 0);
        free_jwt_config(config);
        return NULL;
    }

    // Generate new timestamps and JTI
    time_t now = time(NULL);
    time_t exp = now + JWT_LIFETIME;
    char* jti = generate_jti();
    if (!jti) {
        log_this("AUTH", "Failed to generate JTI for renewal", LOG_LEVEL_ERROR, 0);
        free(header_json);
        free_jwt_config(config);
        return NULL;
    }

    // Create JWT payload preserving original claims but with new timestamps
    char* payload_json = NULL;
    ret = asprintf(&payload_json,
                   "{\"iss\":\"hydrogen-auth\",\"sub\":\"%d\",\"aud\":\"%d\",\"exp\":%ld,\"iat\":%ld,\"nbf\":%ld,\"jti\":\"%s\",\"user_id\":%d,\"system_id\":%d,\"app_id\":%d,\"username\":\"%s\",\"email\":\"%s\",\"roles\":\"%s\",\"ip\":\"%s\",\"tz\":\"%s\",\"tzoffset\":%d,\"database\":\"%s\"}",
                   old_claims->user_id, old_claims->app_id, exp, now, now, jti,
                   old_claims->user_id, old_claims->system_id, old_claims->app_id,
                   old_claims->username ? old_claims->username : "",
                   old_claims->email ? old_claims->email : "",
                   old_claims->roles ? old_claims->roles : "",
                   old_claims->ip ? old_claims->ip : "",
                   old_claims->tz ? old_claims->tz : "",
                   old_claims->tzoffset,
                   old_claims->database ? old_claims->database : "");

    free(jti);

    if (ret == -1) {
        log_this("AUTH", "Failed to create JWT payload for renewal", LOG_LEVEL_ERROR, 0);
        free(header_json);
        free_jwt_config(config);
        return NULL;
    }

    // Base64url encode header and payload
    char* header_b64 = utils_base64url_encode((const unsigned char*)header_json, strlen(header_json));
    char* payload_b64 = utils_base64url_encode((const unsigned char*)payload_json, strlen(payload_json));

    free(header_json);
    free(payload_json);

    if (!header_b64 || !payload_b64) {
        log_this("AUTH", "Failed to encode JWT parts for renewal", LOG_LEVEL_ERROR, 0);
        free(header_b64);
        free(payload_b64);
        free_jwt_config(config);
        return NULL;
    }

    // Create signing input
    char* signing_input = NULL;
    ret = asprintf(&signing_input, "%s.%s", header_b64, payload_b64);
    if (ret == -1) {
        log_this("AUTH", "Failed to create signing input for renewal", LOG_LEVEL_ERROR, 0);
        free(header_b64);
        free(payload_b64);
        free_jwt_config(config);
        return NULL;
    }

    // Create signature
    unsigned char signature[SHA256_DIGEST_LENGTH];
    unsigned int signature_len = SHA256_DIGEST_LENGTH;

    if (HMAC(EVP_sha256(), config->hmac_secret, (int)strlen(config->hmac_secret),
             (const unsigned char*)signing_input, strlen(signing_input),
             signature, &signature_len) == NULL) {
        log_this("AUTH", "Failed to create HMAC signature for renewal", LOG_LEVEL_ERROR, 0);
        free(header_b64);
        free(payload_b64);
        free(signing_input);
        free_jwt_config(config);
        return NULL;
    }

    char* signature_b64 = utils_base64url_encode(signature, signature_len);
    if (!signature_b64) {
        log_this("AUTH", "Failed to encode signature for renewal", LOG_LEVEL_ERROR, 0);
        free(header_b64);
        free(payload_b64);
        free(signing_input);
        free_jwt_config(config);
        return NULL;
    }

    // Create final JWT
    char* jwt = NULL;
    ret = asprintf(&jwt, "%s.%s.%s", header_b64, payload_b64, signature_b64);

    free(header_b64);
    free(payload_b64);
    free(signature_b64);
    free(signing_input);
    free_jwt_config(config);

    if (ret == -1) {
        log_this("AUTH", "Failed to create final JWT for renewal", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    log_this("AUTH", "Generated renewed JWT for user_id=%d", LOG_LEVEL_DEBUG, 1, old_claims->user_id);
    return jwt;
}


/**
 * Validate JWT token (wrapper)
 */
jwt_validation_result_t validate_jwt_token(const char* token, const char* database) {
    return validate_jwt(token, database);
}

/**
 * Validate JWT for logout (allows expired tokens)
 */
jwt_validation_result_t validate_jwt_for_logout(const char* token, const char* database) {
    jwt_validation_result_t result = validate_jwt(token, database);

    // For logout, we accept expired tokens but still validate signature
    if (result.error == JWT_ERROR_EXPIRED) {
        result.valid = true;
        result.error = JWT_ERROR_NONE;
    }

    return result;
}

/**
 * Free JWT configuration
 */
void free_jwt_config(jwt_config_t* config) {
    if (config) {
        free(config->hmac_secret);
        free(config->rsa_private_key);
        free(config->rsa_public_key);
        free(config);
    }
}

/**
 * Free JWT claims
 */
void free_jwt_claims(jwt_claims_t* claims) {
    if (claims) {
        free(claims->iss);
        free(claims->sub);
        free(claims->aud);
        free(claims->jti);
        free(claims->username);
        free(claims->email);
        free(claims->roles);
        free(claims->ip);
        free(claims->tz);
        free(claims->database);
        free(claims);
    }
}

/**
 * Free JWT validation result contents (not the result struct itself)
 * The result struct may be stack-allocated, so we only free the heap-allocated claims
 */
void free_jwt_validation_result(jwt_validation_result_t* result) {
    if (result) {
        if (result->claims) {
            free_jwt_claims(result->claims);
            result->claims = NULL;  // Prevent double-free
        }
        // Note: Do NOT free(result) - it may be stack-allocated
    }
}
