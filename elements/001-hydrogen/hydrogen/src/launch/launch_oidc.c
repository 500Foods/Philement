/*
 * Launch OIDC Subsystem
 * 
 * This module handles the initialization of the OIDC subsystem.
 * It provides functions for checking readiness and launching OIDC services.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

#include "launch.h"
#include "launch_oidc.h"
#include "../utils/utils_logging.h"
#include "../threads/threads.h"
#include "../config/config.h"
#include "../registry/registry_integration.h"

// External declarations
extern AppConfig* app_config;

// Helper function for URL validation
static bool validate_url(const char* url, const char* field_name, int* msg_count, const char** messages) {
    if (!url || strlen(url) == 0) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   OIDC %s is required", field_name);
        messages[(*msg_count)++] = strdup(msg);
        return false;
    }
    
    // Basic URL validation - must start with http:// or https://
    if (strncmp(url, "http://", 7) != 0 && strncmp(url, "https://", 8) != 0) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   Invalid URL format for %s", field_name);
        messages[(*msg_count)++] = strdup(msg);
        return false;
    }
    return true;
}

// Forward declarations for validation helpers
static bool validate_core_settings(const OIDCConfig* config, int* msg_count, const char** messages);
static bool validate_token_settings(const OIDCTokensConfig* config, int* msg_count, const char** messages);
static bool validate_key_settings(const OIDCKeysConfig* config, int* msg_count, const char** messages);

// Check if the OIDC subsystem is ready to launch
LaunchReadiness check_oidc_launch_readiness(void) {
    LaunchReadiness readiness = {0};
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(20 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    int msg_count = 0;
    
    // Add the subsystem name as the first message
    readiness.messages[msg_count++] = strdup("OIDC");
    
    // Check configuration
    if (!app_config) {
        readiness.messages[msg_count++] = strdup("  No-Go:   Configuration not loaded");
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }
    readiness.messages[msg_count++] = strdup("  Go:      Configuration loaded");

    // Skip validation if OIDC is disabled
    if (!app_config->oidc.enabled) {
        readiness.messages[msg_count++] = strdup("  Go:      OIDC disabled, skipping validation");
        readiness.messages[msg_count++] = strdup("  Decide:  Go For Launch of OIDC Subsystem");
        readiness.messages[msg_count] = NULL;
        readiness.ready = true;
        return readiness;
    }

    // Validate core settings
    if (!validate_core_settings(&app_config->oidc, &msg_count, readiness.messages)) {
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }

    // Validate token settings
    if (!validate_token_settings(&app_config->oidc.tokens, &msg_count, readiness.messages)) {
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }

    // Validate key settings
    if (!validate_key_settings(&app_config->oidc.keys, &msg_count, readiness.messages)) {
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }
    
    // All checks passed
    readiness.messages[msg_count++] = strdup("  Decide:  Go For Launch of OIDC Subsystem");
    readiness.messages[msg_count] = NULL;
    readiness.ready = true;
    
    return readiness;
}

static bool validate_core_settings(const OIDCConfig* config, int* msg_count, const char** messages) {
    // Validate required fields
    if (!validate_url(config->issuer, "issuer", msg_count, messages)) {
        return false;
    }

    if (!config->client_id || strlen(config->client_id) == 0) {
        messages[(*msg_count)++] = strdup("  No-Go:   OIDC client_id is required");
        return false;
    }

    if (!config->client_secret || strlen(config->client_secret) == 0) {
        messages[(*msg_count)++] = strdup("  No-Go:   OIDC client_secret is required");
        return false;
    }

    if (config->redirect_uri && !validate_url(config->redirect_uri, "redirect_uri", msg_count, messages)) {
        return false;
    }

    // Validate port
    if (config->port < MIN_OIDC_PORT || config->port > MAX_OIDC_PORT) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   Invalid OIDC port %d (must be between %d and %d)",
                config->port, MIN_OIDC_PORT, MAX_OIDC_PORT);
        messages[(*msg_count)++] = strdup(msg);
        return false;
    }

    messages[(*msg_count)++] = strdup("  Go:      Core settings valid");
    return true;
}

static bool validate_token_settings(const OIDCTokensConfig* config, int* msg_count, const char** messages) {
    if (config->access_token_lifetime < MIN_TOKEN_LIFETIME || 
        config->access_token_lifetime > MAX_TOKEN_LIFETIME) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   Invalid access token lifetime %d (must be between %d and %d)",
                config->access_token_lifetime, MIN_TOKEN_LIFETIME, MAX_TOKEN_LIFETIME);
        messages[(*msg_count)++] = strdup(msg);
        return false;
    }

    if (config->refresh_token_lifetime < MIN_REFRESH_LIFETIME || 
        config->refresh_token_lifetime > MAX_REFRESH_LIFETIME) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   Invalid refresh token lifetime %d (must be between %d and %d)",
                config->refresh_token_lifetime, MIN_REFRESH_LIFETIME, MAX_REFRESH_LIFETIME);
        messages[(*msg_count)++] = strdup(msg);
        return false;
    }

    if (config->id_token_lifetime < MIN_TOKEN_LIFETIME || 
        config->id_token_lifetime > MAX_TOKEN_LIFETIME) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   Invalid ID token lifetime %d (must be between %d and %d)",
                config->id_token_lifetime, MIN_TOKEN_LIFETIME, MAX_TOKEN_LIFETIME);
        messages[(*msg_count)++] = strdup(msg);
        return false;
    }

    messages[(*msg_count)++] = strdup("  Go:      Token settings valid");
    return true;
}

static bool validate_key_settings(const OIDCKeysConfig* config, int* msg_count, const char** messages) {
    if (config->encryption_enabled) {
        if (!config->encryption_key || strlen(config->encryption_key) == 0) {
            messages[(*msg_count)++] = strdup("  No-Go:   Encryption key required when encryption is enabled");
            return false;
        }
    }

    if (config->rotation_interval_days < MIN_KEY_ROTATION_DAYS || 
        config->rotation_interval_days > MAX_KEY_ROTATION_DAYS) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   Invalid key rotation interval %d days (must be between %d and %d)",
                config->rotation_interval_days, MIN_KEY_ROTATION_DAYS, MAX_KEY_ROTATION_DAYS);
        messages[(*msg_count)++] = strdup(msg);
        return false;
    }

    messages[(*msg_count)++] = strdup("  Go:      Key settings valid");
    return true;
}

// Launch the OIDC subsystem
int launch_oidc_subsystem(void) {
    // Skip initialization if OIDC is disabled
    if (!app_config->oidc.enabled) {
        log_this("OIDC", "OIDC subsystem disabled", LOG_LEVEL_STATE);
        return 1;
    }
    
    // Additional initialization as needed
    return 1;
}
