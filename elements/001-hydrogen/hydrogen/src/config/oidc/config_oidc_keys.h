/*
 * OpenID Connect Keys Configuration
 *
 * Defines the configuration structure and defaults for OIDC key management.
 * This includes settings for key rotation, storage, and encryption.
 */

#ifndef HYDROGEN_CONFIG_OIDC_KEYS_H
#define HYDROGEN_CONFIG_OIDC_KEYS_H

// Project headers
#include "../config_forward.h"

// Default values for key management
#define DEFAULT_KEY_ROTATION_DAYS 30
#define DEFAULT_KEY_STORAGE_PATH "/var/lib/hydrogen/oidc/keys"
#define DEFAULT_KEY_ENCRYPTION_ENABLED 1

// OIDC keys configuration structure
struct OIDCKeysConfig {
    int rotation_interval_days;  // How often keys should be rotated
    char* storage_path;         // Where to store key material
    int encryption_enabled;     // Whether to encrypt stored keys
};

/*
 * Initialize OIDC keys configuration with default values
 *
 * This function initializes a new OIDCKeysConfig structure with secure defaults
 * for key management, including rotation schedule and storage settings.
 *
 * @param config Pointer to OIDCKeysConfig structure to initialize
 * @return 0 on success, -1 on failure (memory allocation or null pointer)
 *
 * Error conditions:
 * - If config is NULL
 * - If memory allocation fails for storage path
 */
int config_oidc_keys_init(OIDCKeysConfig* config);

/*
 * Free resources allocated for OIDC keys configuration
 *
 * This function frees all resources allocated by config_oidc_keys_init.
 * It safely handles NULL pointers and partial initialization.
 * After cleanup, the structure is zeroed to prevent use-after-free.
 *
 * @param config Pointer to OIDCKeysConfig structure to cleanup
 */
void config_oidc_keys_cleanup(OIDCKeysConfig* config);

/*
 * Validate OIDC keys configuration values
 *
 * This function performs comprehensive validation of the key settings:
 * - Verifies rotation interval is within acceptable range
 * - Validates storage path exists and is writable
 * - Checks encryption settings are valid
 *
 * @param config Pointer to OIDCKeysConfig structure to validate
 * @return 0 if valid, -1 if invalid
 *
 * Error conditions:
 * - If config is NULL
 * - If rotation interval is invalid (<=0 or too large)
 * - If storage path is missing or invalid
 */
int config_oidc_keys_validate(const OIDCKeysConfig* config);

#endif /* HYDROGEN_CONFIG_OIDC_KEYS_H */