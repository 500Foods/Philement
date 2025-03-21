/*
 * Security System for 3D Printer Control
 * 
 * Why Strong Security Matters:
 * 1. Machine Safety
 *    - Prevent unauthorized commands
 *    - Protect against malicious G-code
 *    - Control access to heating elements
 *    - Safeguard motion systems
 * 
 * 2. Network Security
 *    Why Cryptographic Quality?
 *    - Remote access protection
 *    - Command authentication
 *    - Session management
 *    - API security
 * 
 * 3. Key Management
 *    Why These Practices?
 *    - Secure key generation
 *    - Safe key storage
 *    - Key rotation policies
 *    - Access control
 * 
 * 4. Integration Points
 *    Why Comprehensive Security?
 *    - WebSocket authentication
 *    - API authorization
 *    - Configuration protection
 *    - Audit logging
 * 
 * Implementation Features:
 * - OpenSSL CSPRNG for randomness
 * - Secure memory handling
 * - Error detection
 * - Safe key encoding
 */

// Feature test macros
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Standard C headers
#include <stdio.h>
#include <stdlib.h>

// Third-party libraries
#include <openssl/rand.h>

// Project headers
#include "keys.h"

// Generate cryptographically secure random key for authentication
//
// Key generation strategy:
// 1. Security
//    - Uses OpenSSL's CSPRNG for cryptographic quality
//    - Key length chosen for attack resistance
//    - Avoids modulo bias in random distribution
//    - Hexadecimal encoding for safe transport
//
// 2. Memory Safety
//    - Bounded buffer sizes
//    - NULL termination guarantee
//    - Cleanup on error paths
//    - No sensitive data leaks
//
// 3. Error Handling
//    - CSPRNG failure detection
//    - Allocation failure recovery
//    - Clean error propagation
//    - Resource cleanup
char *generate_secret_key(void) {
    unsigned char random_bytes[SECRET_KEY_LENGTH];
    char *secret_key = malloc(SECRET_KEY_LENGTH * 2 + 1);

    if (!secret_key) {
        return NULL;
    }

    if (RAND_bytes(random_bytes, SECRET_KEY_LENGTH) != 1) {
        free(secret_key);
        return NULL;
    }

    for (int i = 0; i < SECRET_KEY_LENGTH; i++) {
        sprintf(secret_key + (i * 2), "%02x", random_bytes[i]);
    }
    secret_key[SECRET_KEY_LENGTH * 2] = '\0';

    return secret_key;
}