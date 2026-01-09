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

    // TODO: Load from actual configuration
    // For now, use defaults
    config->hmac_secret = strdup("default-jwt-secret-change-me-in-production");
    config->use_rsa = false;
    config->rotation_interval_days = 90;

    return config;
}

/**
 * Generate a JWT token
 */
char* generate_jwt(account_info_t* account, system_info_t* system,
                   const char* client_ip, time_t issued_at) {
    if (!account || !system || !client_ip) {
        log_this(SR_AUTH, "Invalid parameters for JWT generation", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    jwt_config_t* config = get_jwt_config();
    if (!config) {
        log_this("AUTH", "Failed to get JWT configuration", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // Create JWT header
    char* header_json = NULL;
    asprintf(&header_json, "{\"alg\":\"%s\",\"typ\":\"%s\"}",
             config->use_rsa ? "RS256" : "HS256", JWT_TYPE);
    if (!header_json) {
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

    char* payload_json = NULL;
    asprintf(&payload_json,
             "{\"iss\":\"hydrogen-auth\",\"sub\":\"%d\",\"aud\":\"%d\",\"exp\":%ld,\"iat\":%ld,\"nbf\":%ld,\"jti\":\"%s\",\"user_id\":%d,\"system_id\":%d,\"app_id\":%d,\"username\":\"%s\",\"email\":\"%s\",\"roles\":\"%s\",\"ip\":\"%s\",\"tz\":\"%s\"}",
             account->id, system->app_id, exp, now, now, jti,
             account->id, system->system_id, system->app_id,
             account->username ? account->username : "",
             account->email ? account->email : "",
             account->roles ? account->roles : "",
             client_ip, "UTC"); // TODO: Pass timezone

    free(jti);

    if (!payload_json) {
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
    asprintf(&signing_input, "%s.%s", header_b64, payload_b64);
    if (!signing_input) {
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
    asprintf(&jwt, "%s.%s.%s", header_b64, payload_b64, signature_b64);

    free(header_b64);
    free(payload_b64);
    free(signature_b64);
    free(signing_input);
    free_jwt_config(config);

    if (!jwt) {
        log_this("AUTH", "Failed to create final JWT", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    log_this("AUTH", "Generated JWT for user %s", LOG_LEVEL_DEBUG, 1, account->username);
    return jwt;
}

/**
 * Validate a JWT token
 */
jwt_validation_result_t validate_jwt(const char* token) {
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

    // Parse JSON claims (simplified - in real implementation use proper JSON parser)
    char* payload_str = (char*)payload_decoded;
    payload_str[payload_len] = '\0';

    // Extract expiration time (very basic parsing)
    const char* exp_str = strstr(payload_str, "\"exp\":");
    if (!exp_str) {
        free(payload_decoded);
        free(token_copy);
        result.error = JWT_ERROR_INVALID_FORMAT;
        return result;
    }

    time_t exp_time = atol(exp_str + 6);
    time_t now = time(NULL);

    if (exp_time < now) {
        free(payload_decoded);
        free(token_copy);
        result.error = JWT_ERROR_EXPIRED;
        return result;
    }

    // Check if token is revoked
    char* token_hash = compute_token_hash(token);
    if (is_token_revoked(token_hash)) {
        free(token_hash);
        free(payload_decoded);
        free(token_copy);
        result.error = JWT_ERROR_REVOKED;
        return result;
    }
    free(token_hash);

    // Verify signature
    jwt_config_t* config = get_jwt_config();
    if (!config) {
        free(payload_decoded);
        free(token_copy);
        result.error = JWT_ERROR_INVALID_SIGNATURE;
        return result;
    }

    char* signing_input = NULL;
    asprintf(&signing_input, "%s.%s", header_b64, payload_b64);
    if (!signing_input) {
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

    // Token is valid - create claims structure
    result.valid = true;
    result.error = JWT_ERROR_NONE;
    result.claims = calloc(1, sizeof(jwt_claims_t));
    if (result.claims) {
        // Parse basic claims from payload (simplified)
        result.claims->exp = exp_time;
        result.claims->iat = now;
        result.claims->nbf = now;
        // TODO: Parse other claims from JSON
    }

    free(payload_decoded);
    free(token_copy);

    return result;
}

/**
 * Generate a new JWT from old claims (token renewal)
 */
char* generate_new_jwt(jwt_claims_t* old_claims) {
    if (!old_claims) return NULL;

    // TODO: Implement token renewal logic
    // For now, return NULL
    (void)old_claims; // Suppress unused parameter warning
    return NULL;
}


/**
 * Validate JWT token (wrapper)
 */
jwt_validation_result_t validate_jwt_token(const char* token) {
    return validate_jwt(token);
}

/**
 * Validate JWT for logout (allows expired tokens)
 */
jwt_validation_result_t validate_jwt_for_logout(const char* token) {
    jwt_validation_result_t result = validate_jwt(token);

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
        free(claims);
    }
}

/**
 * Free JWT validation result
 */
void free_jwt_validation_result(jwt_validation_result_t* result) {
    if (result) {
        if (result->claims) {
            free_jwt_claims(result->claims);
        }
        free(result);
    }
}
