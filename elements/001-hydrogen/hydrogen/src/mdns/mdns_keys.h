#ifndef MDNS_KEYS_H
#define MDNS_KEYS_H

#include <src/globals.h>
#include <openssl/rand.h>

char *generate_secret_mdns_key(void);

#endif // MDNS_KEYS_H
