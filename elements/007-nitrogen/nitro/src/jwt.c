#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
#include "jwt.h"
#include "base64.h"
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>


char *generate_jwt(const char *device_id, const char *secret_key) {
    time_t now = time(NULL);
    char header[] = "{\"alg\":\"HS256\",\"typ\":\"JWT\"}";
    char payload[256];
    snprintf(payload, sizeof(payload), "{\"device_id\":\"%s\",\"exp\":%ld}", device_id, now + 3600);  // 1 hour expiration

    // Base64 encode header and payload
    char *base64_header = base64_encode((const unsigned char *)header, strlen(header));
    char *base64_payload = base64_encode((const unsigned char *)payload, strlen(payload));

    // Create signature
    char signature_input[512];
    snprintf(signature_input, sizeof(signature_input), "%s.%s", base64_header, base64_payload);
    
    unsigned char hmac[EVP_MAX_MD_SIZE];
    unsigned int hmac_len;
    HMAC(EVP_sha256(), secret_key, strlen(secret_key), (unsigned char*)signature_input, strlen(signature_input), hmac, &hmac_len);

    char *base64_signature = base64_encode(hmac, hmac_len);

    // Combine to form JWT
    char *jwt = malloc(strlen(base64_header) + strlen(base64_payload) + strlen(base64_signature) + 3);
    sprintf(jwt, "%s.%s.%s", base64_header, base64_payload, base64_signature);

    free(base64_header);
    free(base64_payload);
    free(base64_signature);

    return jwt;
}

bool verify_jwt(const char *jwt, const char *secret_key) {
    // Split JWT into parts
    char *header_payload = strdup(jwt);
    if (!header_payload) {
        return false;
    }
    char *signature = strchr(header_payload, '.');
    if (!signature) {
        free(header_payload);
        return false;
    }
    *signature = '\0';
    signature++;
    char *payload = strchr(signature, '.');
    if (!payload) {
        free(header_payload);
        return false;
    }
    *payload = '\0';
    payload++;

    // Verify signature
    unsigned char hmac[EVP_MAX_MD_SIZE];
    unsigned int hmac_len;
    HMAC(EVP_sha256(), secret_key, strlen(secret_key), (unsigned char*)header_payload, strlen(header_payload), hmac, &hmac_len);
    char *base64_signature = base64_encode(hmac, hmac_len);

    bool sig_valid = strcmp(base64_signature, signature) == 0;
    free(base64_signature);

    if (!sig_valid) {
        return false;
    }

    // Check expiration
    char *base64_payload = base64_encode((const unsigned char *)payload, strlen(payload));
    size_t json_payload_len;
    unsigned char *json_payload = base64_decode(base64_payload, &json_payload_len);
    free(base64_payload);

    time_t exp = 0;
    sscanf((const char *)json_payload, "{\"device_id\":\"%*[^\"]\",\"exp\":%ld}", &exp);
    free(json_payload);


    free(header_payload);

    time_t now = time(NULL);
    return now < exp;
}
