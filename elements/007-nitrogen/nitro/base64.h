#ifndef NITRO_BASE64_H
#define NITRO_BASE64_H

#include <stddef.h>

char *base64_encode(const unsigned char *data, size_t input_length);
unsigned char *base64_decode(const char *data, size_t *output_length);

#endif // NITRO_BASE64_H
