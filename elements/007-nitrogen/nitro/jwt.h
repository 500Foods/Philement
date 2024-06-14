#ifndef NITRO_JWT_H
#define NITRO_JWT_H

#include <stdbool.h>

#define JWT_ALG_HS256 "HS256"
#define JWT_TYPE "JWT"

char *generate_jwt(const char *device_id, const char *secret_key);
bool verify_jwt(const char *jwt, const char *secret_key);

#endif // NITRO_JWT_H
