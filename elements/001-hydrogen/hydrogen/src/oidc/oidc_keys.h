/*
 * OIDC Key Management
 *
 * Implements cryptographic key operations for the OIDC service:
 * - RSA key pair generation for JWT signing
 * - Key rotation and versioning
 * - JWKS (JSON Web Key Set) handling
 * - Secure key storage
 */

#ifndef OIDC_KEYS_H
#define OIDC_KEYS_H

#include <src/globals.h>

// Standard Libraries
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

// OpenSSL Libraries
#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/pem.h>

/*
 * Key Usage
 * Defines how a key can be used within the OIDC service
 */
typedef enum {
    KEY_USAGE_SIGNING,         // Key used for signing tokens
    KEY_USAGE_ENCRYPTION       // Key used for encryption
} OIDCKeyUsage;

/*
 * Key Algorithm
 * Defines the cryptographic algorithm used for the key
 */
typedef enum {
    KEY_ALG_RS256,             // RSA with SHA-256
    KEY_ALG_RS384,             // RSA with SHA-384
    KEY_ALG_RS512,             // RSA with SHA-512
    KEY_ALG_ES256,             // ECDSA with P-256 and SHA-256
    KEY_ALG_ES384,             // ECDSA with P-384 and SHA-384
    KEY_ALG_ES512              // ECDSA with P-521 and SHA-512
} OIDCKeyAlgorithm;

/*
 * Key Status
 * Defines the current status of a key in its lifecycle
 */
typedef enum {
    KEY_STATUS_ACTIVE,         // Currently used for signing
    KEY_STATUS_ROTATING,       // Being phased out, still valid for verification
    KEY_STATUS_ARCHIVED        // No longer used, kept for token verification
} OIDCKeyStatus;

/*
 * OIDC Key
 * Represents a cryptographic key used by the OIDC service
 */
typedef struct {
    char kid[OIDC_KEY_ID_LENGTH + 1];  // Key ID (used in JWT header)
    OIDCKeyUsage usage;                // How the key is used
    OIDCKeyAlgorithm algorithm;        // Cryptographic algorithm
    OIDCKeyStatus status;              // Current key status
    time_t created_at;                 // Creation timestamp
    time_t expires_at;                 // Expiration timestamp
    void *key_data;                    // Key data (RSA or EC key)
} OIDCKey;

/*
 * OIDC Key Context
 * Manages the set of keys used by the OIDC service
 */
typedef struct {
    OIDCKey **keys;            // Array of keys
    size_t key_count;          // Number of keys
    char *storage_path;        // Path for persisting keys
    bool encryption_enabled;   // Whether keys are encrypted on disk
    int rotation_interval;     // Days between key rotations
    time_t next_rotation;      // Next scheduled rotation time
} OIDCKeyContext;

/*
 * Initialize key management system
 * 
 * @param storage_path Path to store keys
 * @param encryption_enabled Whether to encrypt stored keys
 * @param rotation_interval_days Number of days between key rotations
 * @return Initialized key context or NULL on failure
 */
OIDCKeyContext* init_oidc_key_management(const char *storage_path, 
                                        bool encryption_enabled,
                                        int rotation_interval_days);

/*
 * Cleanup key management resources
 * 
 * @param context Key context to clean up
 */
void cleanup_oidc_key_management(OIDCKeyContext *context);

/*
 * Generate a new signing key
 * 
 * @param context Key context
 * @param algorithm Cryptographic algorithm to use
 * @return Generated key or NULL on failure
 */
OIDCKey* oidc_generate_signing_key(OIDCKeyContext *context, OIDCKeyAlgorithm algorithm);

/*
 * Rotate keys based on rotation policy
 * 
 * @param context Key context
 * @return true if rotation successful, false otherwise
 */
bool oidc_rotate_keys(OIDCKeyContext *context);

/*
 * Get the current active signing key
 * 
 * @param context Key context
 * @return Active signing key or NULL if none available
 */
OIDCKey* oidc_get_active_signing_key(OIDCKeyContext *context);

/*
 * Find a key by its ID
 * 
 * @param context Key context
 * @param kid Key ID to find
 * @return Key with matching ID or NULL if not found
 */
OIDCKey* oidc_find_key_by_id(OIDCKeyContext *context, const char *kid);

/*
 * Generate JWKS (JSON Web Key Set) document
 *
 * @param context Key context
 * @return JWKS document as a JSON string (caller must free)
 */
char* oidc_generate_jwks(const OIDCKeyContext *context);

/*
 * Sign data using a specified key
 * 
 * @param key Key to use for signing
 * @param data Data to sign
 * @param data_len Length of data
 * @param signature Output buffer for signature
 * @param signature_len Output parameter for signature length
 * @return true if signing successful, false otherwise
 */
bool oidc_sign_data(OIDCKey *key, const unsigned char *data, size_t data_len,
                   unsigned char **signature, size_t *signature_len);

/*
 * Verify signature using a specified key
 * 
 * @param key Key to use for verification
 * @param data Data that was signed
 * @param data_len Length of data
 * @param signature Signature to verify
 * @param signature_len Length of signature
 * @return true if signature is valid, false otherwise
 */
bool oidc_verify_signature(OIDCKey *key, const unsigned char *data, size_t data_len,
                          const unsigned char *signature, size_t signature_len);

/*
 * Export a public key in PEM format
 * 
 * @param key Key to export
 * @return Public key in PEM format (caller must free)
 */
char* oidc_export_public_key_pem(OIDCKey *key);

/*
 * Load keys from storage
 * 
 * @param context Key context
 * @return true if keys loaded successfully, false otherwise
 */
bool oidc_load_keys_from_storage(OIDCKeyContext *context);

/*
 * Save keys to storage
 * 
 * @param context Key context
 * @return true if keys saved successfully, false otherwise
 */
bool oidc_save_keys_to_storage(OIDCKeyContext *context);

/*
 * Convert algorithm enum to string representation
 * 
 * @param algorithm Algorithm enum value
 * @return String representation (e.g., "RS256")
 */
const char* oidc_algorithm_to_string(OIDCKeyAlgorithm algorithm);

/*
 * Parse algorithm string to enum value
 * 
 * @param algorithm_str Algorithm string (e.g., "RS256")
 * @return Corresponding enum value
 */
OIDCKeyAlgorithm oidc_algorithm_from_string(const char *algorithm_str);

#endif // OIDC_KEYS_H
