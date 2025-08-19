#ifndef MDNS_KEYS_H
#define MDNS_KEYS_H

#include "../constants.h"
#include <openssl/rand.h>

char *generate_secret_mdns_key(void);

#endif // MDNS_KEYS_H
