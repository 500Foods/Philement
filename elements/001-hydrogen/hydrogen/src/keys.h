/*
 * Cryptographic key generation interface for the Hydrogen server.
 * 
 * Provides secure random key generation using OpenSSL's RAND_bytes
 * for authentication and encryption purposes throughout the application.
 */

#ifndef KEYS_H
#define KEYS_H

#include <openssl/rand.h>

#define SECRET_KEY_LENGTH 32

char *generate_secret_key();

#endif // KEYS_H
