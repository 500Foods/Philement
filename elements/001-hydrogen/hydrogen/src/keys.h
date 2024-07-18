#ifndef KEYS_H
#define KEYS_H

#include <openssl/rand.h>

#define SECRET_KEY_LENGTH 32

char *generate_secret_key();

#endif // KEYS_H
