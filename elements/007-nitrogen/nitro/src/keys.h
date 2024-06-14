#ifndef NITRO_KEYS_H
#define NITRO_KEYS_H

#include <openssl/rand.h>

#define SECRET_KEY_LENGTH 32

char *generate_secret_key();

#endif // NITRO_KEYS_H
