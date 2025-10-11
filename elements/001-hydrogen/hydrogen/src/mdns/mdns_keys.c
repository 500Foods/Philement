/*
 * 
 */

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "mdns_keys.h"

// Third-party libraries
#include <openssl/rand.h>

char *generate_secret_mdns_key(void) {
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
