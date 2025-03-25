/*
 * API Configuration Implementation
 */

#include <stdlib.h>
#include <string.h>
#include "config_api.h"
#include "../types/config_string.h"

// Validation limits
#define MIN_JWT_SECRET_LENGTH 32

int config_api_init(APIConfig* config) {
    if (!config) {
        return -1;
    }

    config->jwt_secret = strdup(DEFAULT_JWT_SECRET);
    if (!config->jwt_secret) {
        return -1;
    }

    return 0;
}

void config_api_cleanup(APIConfig* config) {
    if (!config) {
        return;
    }

    free(config->jwt_secret);
    memset(config, 0, sizeof(APIConfig));
}

int config_api_validate(const APIConfig* config) {
    if (!config) {
        return -1;
    }

    // Verify jwt_secret is set and meets minimum length
    if (!config->jwt_secret || strlen(config->jwt_secret) < MIN_JWT_SECRET_LENGTH) {
        return -1;
    }

    return 0;
}