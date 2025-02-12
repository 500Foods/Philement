/*
 * Cryptographic key generation implementation for the Hydrogen server.
 * 
 * Generates secure random keys using OpenSSL's RAND_bytes function,
 * converting the random bytes to a hexadecimal string for use in
 * authentication and encryption throughout the application.
 */

// Feature test macros must come first
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Standard C headers
#include <stdio.h>
#include <stdlib.h>

// Third-party libraries
#include <openssl/rand.h>

// Project headers
#include "keys.h"

char *generate_secret_key() {
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
